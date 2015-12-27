/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "packageprocessor.h"
#include <stdexcept>
#include <QFile>
#include <QDirIterator>
#include <QFileInfo>
#include <util/util.h>
#include <util/sys/paths.h>
#include "core.h"
#include "externalresourcemanager.h"
#include "storage.h"

namespace LeechCraft
{
namespace LackMan
{
	PackageProcessor::PackageProcessor (QObject *parent)
	: QObject (parent)
	, DBDir_ (Util::CreateIfNotExists ("lackman/filesdb/"))
	{
	}

	void PackageProcessor::Remove (int packageId)
	{
		QString filename = QString::number (packageId);
		if (!DBDir_.exists (filename))
		{
			qWarning () << Q_FUNC_INFO
					<< "file for package"
					<< packageId
					<< "not found";
			QString str = tr ("Could not find database file for package %1.")
					.arg (packageId);
			throw std::runtime_error (str.toUtf8 ().constData ());
		}

		QFile file (DBDir_.filePath (filename));
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "could not open file"
					<< file.fileName ()
					<< file.errorString ();
			QString str = tr ("Could not open database file %1: %2.")
					.arg (file.fileName ())
					.arg (file.errorString ());
			throw std::runtime_error (str.toUtf8 ().constData ());
		}

		const QDir& packageDir = Core::Instance ().GetPackageDir (packageId);

		const QByteArray& fileData = file.readAll ();
		QStringList files = QString::fromUtf8 (fileData)
				.split ('\n', QString::SkipEmptyParts);
		files.sort ();
		std::reverse (files.begin (), files.end ());
		Q_FOREACH (const QString& packageFilename, files)
		{
			const QString& fullName = packageDir.filePath (packageFilename);
#ifndef QT_NO_DEBUG
			qDebug () << Q_FUNC_INFO
					<< "gonna remove"
					<< fullName;
#endif
			const QFileInfo fi (fullName);

			if (fi.isFile ())
			{
				QFile packageFile (fullName);
				if (!packageFile.remove ())
				{
					qWarning () << Q_FUNC_INFO
							<< "could not remove file"
							<< packageFile.fileName ()
							<< packageFile.errorString ();
					QString str = tr ("Could not remove file %1: %2.")
							.arg (packageFile.fileName ())
							.arg (packageFile.errorString ());
					throw std::runtime_error (str.toUtf8 ().constData ());
				}
			}
			else if (fi.isDir ())
			{
				const auto& entries = QDir (fi.absoluteFilePath ())
						.entryList (QDir::NoDotAndDotDot |
								QDir::AllEntries |
								QDir::Hidden |
								QDir::System);
				if (!entries.isEmpty ())
				{
#ifndef QT_NO_DEBUG
					qDebug () << Q_FUNC_INFO
							<< "non-empty directory"
							<< fi.absoluteFilePath ()
							<< entries;
#endif
					continue;
				}
				else if (!QDir (fi.path ()).rmdir (fi.fileName ()))
				{
					qWarning () << Q_FUNC_INFO
							<< "could not remove directory"
							<< fullName
							<< "→ parent:"
							<< fi.path ()
							<< "; child node:"
							<< fi.fileName ();
					const auto& str = tr ("Could not remove directory %1.")
							.arg (fi.fileName ());
					throw std::runtime_error (str.toUtf8 ().constData ());
				}
			}
		}

		file.close ();
		if (!file.remove ())
		{
			qWarning () << Q_FUNC_INFO
					<< "could not delete DB file"
					<< file.fileName ()
					<< file.errorString ();
			const QString& str = tr ("Could not remove database file %1: %2.")
					.arg (file.fileName ())
					.arg (file.errorString ());
			throw std::runtime_error (str.toUtf8 ().constData ());
		}
	}

	void PackageProcessor::Install (int packageId)
	{
		const QUrl& url = GetURLFor (packageId);

		ExternalResourceManager *erm = PrepareResourceManager ();

		URL2Id_ [url] = packageId;
		URL2Mode_ [url] = MInstall;
		erm->GetResourceData (url);
	}

	void PackageProcessor::Update (int toPackageId)
	{
		QUrl url = GetURLFor (toPackageId);

		ExternalResourceManager *erm = PrepareResourceManager ();

		URL2Id_ [url] = toPackageId;
		URL2Mode_ [url] = MUpdate;
		erm->GetResourceData (url);
	}

	void PackageProcessor::handleResourceFetched (const QUrl& url)
	{
		if (URL2Id_.contains (url))
			HandleFile (URL2Id_.take (url), url, URL2Mode_.take (url));
	}

	void PackageProcessor::handlePackageUnarchFinished (int ret, QProcess::ExitStatus)
	{
		sender ()->deleteLater ();

		QProcess *unarch = qobject_cast<QProcess*> (sender ());
		int packageId = unarch->property ("PackageID").toInt ();
		const auto& stagingDir = unarch->property ("StagingDirectory").toString ();
		Mode mode = static_cast<Mode> (unarch->property ("Mode").toInt ());

		auto cleanupGuard = std::shared_ptr<void> (nullptr,
				[&stagingDir, this] (void*) { CleanupDir (stagingDir); });

		if (ret)
		{
			QString errString = QString::fromUtf8 (unarch->readAllStandardError ());
			qWarning () << Q_FUNC_INFO
					<< "unpacker exited with"
					<< ret
					<< errString
					<< "for"
					<< packageId
					<< unarch->property ("Path").toString ();

			QString errorString = tr ("Unable to unpack package archive, unpacker exited with %1: %2.")
					.arg (ret)
					.arg (errString);
			emit packageInstallError (packageId, errorString);

			return;
		}

		int oldId = -1;
		if (mode == MUpdate)
		{
			oldId = Core::Instance ().GetStorage ()->FindInstalledPackage (packageId);
			if (!CleanupBeforeUpdate (oldId, packageId))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to cleanup";
				return;
			}
		}

		QDir packageDir;
		try
		{
			packageDir = Core::Instance ().GetPackageDir (packageId);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "while trying to get dir for package"
					<< packageId
					<< "got we exception"
					<< e.what ();
			QString errorString = tr ("Unable to get directory for the package: %1.")
					.arg (QString::fromUtf8 (e.what ()));
			emit packageInstallError (packageId, errorString);
			return;
		}

		QDirIterator dirIt (stagingDir,
				QDir::NoDotAndDotDot |
					QDir::Readable |
					QDir::NoSymLinks |
					QDir::Dirs |
					QDir::Files,
				QDirIterator::Subdirectories);
		while (dirIt.hasNext ())
		{
			dirIt.next ();
			QFileInfo fi = dirIt.fileInfo ();

			if (fi.isDir () ||
					fi.isFile ())
				if (!HandleEntry (packageId, fi, stagingDir, packageDir))
				{
					try
					{
						Remove (packageId);
					}
					catch (const std::exception& e)
					{
						qWarning () << Q_FUNC_INFO
								<< "while removing partially installed package"
								<< packageId
								<< "got:"
								<< e.what ();
					}

					QString errorString = tr ("Unable to copy "
							"files from staging area to "
							"destination directory.");
					emit packageInstallError (packageId, errorString);
					return;
				}
		}

		switch (mode)
		{
		case MInstall:
			emit packageInstalled (packageId);
			break;
		case MUpdate:
			emit packageUpdated (oldId, packageId);
			break;
		}
	}

	void PackageProcessor::handleUnarchError (QProcess::ProcessError error)
	{
		sender ()->deleteLater ();

		QByteArray errString = qobject_cast<QProcess*> (sender ())->readAllStandardError ();
		qWarning () << Q_FUNC_INFO
				<< "unable to unpack for"
				<< sender ()->property ("PackageID").toInt ()
				<< sender ()->property ("Path").toString ()
				<< "with"
				<< error
				<< errString;

		QString errorString = tr ("Unable to unpack package archive, unpacker died with %1: %2.")
				.arg (error)
				.arg (QString::fromUtf8 (errString));
		emit packageInstallError (sender ()->property ("PackageID").toInt (),
				errorString);

		CleanupDir (sender ()->property ("StagingDirectory").toString ());
	}

	void PackageProcessor::HandleFile (int packageId,
			const QUrl& url, PackageProcessor::Mode mode)
	{
		QString path = Core::Instance ().GetExtResourceManager ()->GetResourcePath (url);

		PackageShortInfo info;
		try
		{
			info = Core::Instance ().GetStorage ()->GetPackage (packageId);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to get package info for"
					<< packageId
					<< e.what ();
			return;
		}

		QProcess *unarch = new QProcess (this);
		connect (unarch,
				SIGNAL (finished (int, QProcess::ExitStatus)),
				this,
				SLOT (handlePackageUnarchFinished (int, QProcess::ExitStatus)));
		connect (unarch,
				SIGNAL (error (QProcess::ProcessError)),
				this,
				SLOT (handleUnarchError (QProcess::ProcessError)));

		QString dirname = Util::GetTemporaryName ("lackman_stagingarea");
		QStringList args;
#ifdef Q_OS_WIN32
		args << "x"
			<< "-ttar"
			<< "-y"
			<< "-si";

		QString outDirArg ("-o");
		outDirArg.append (dirname);
		args << outDirArg;

		QProcess *firstStep = new QProcess (unarch);
		firstStep->setStandardOutputProcess (unarch);
		QStringList firstStepArgs;
		firstStepArgs << "x"
			<< "-y"
			<< "-so"
			<< path;
#else
		const auto& archiver = info.VersionArchivers_.value (info.Versions_.value (0), "gz");
		if (archiver == "lzma")
			args << "--lzma";
		args << "-xf";
		args << path;
		args << "-C";
		args << dirname;
#endif
		unarch->setProperty ("PackageID", packageId);
		unarch->setProperty ("StagingDirectory", dirname);
		unarch->setProperty ("Path", path);
		unarch->setProperty ("Mode", mode);

		QFileInfo sdInfo (dirname);
		QDir stagingParentDir (sdInfo.path ());
		if (!stagingParentDir.exists (sdInfo.fileName ()) &&
				!stagingParentDir.mkdir (sdInfo.fileName ()))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to create staging directory"
					<< sdInfo.fileName ()
					<< "in"
					<< sdInfo.path ();

			QString errorString = tr ("Unable to create staging directory %1.")
					.arg (sdInfo.fileName ());
			emit packageInstallError (packageId, errorString);

			return;
		}

#ifdef Q_OS_WIN32
		QString command = "7za";
		firstStep->start (command, firstStepArgs);
#else
		QString command = "tar";
#endif
		unarch->start (command, args);
	}

	QUrl PackageProcessor::GetURLFor (int packageId) const
	{
		const auto& urls = Core::Instance ().GetPackageURLs (packageId);
		if (!urls.size ())
			throw std::runtime_error (tr ("No URLs for package %1.")
					.arg (packageId).toUtf8 ().constData ());

		QUrl url = urls.at (0);
		qDebug () << Q_FUNC_INFO
				<< "would fetch"
				<< packageId
				<< "from"
				<< url;
		return url;
	}

	ExternalResourceManager* PackageProcessor::PrepareResourceManager ()
	{
		ExternalResourceManager *erm = Core::Instance ().GetExtResourceManager ();
		connect (erm,
				SIGNAL (resourceFetched (const QUrl&)),
				this,
				SLOT (handleResourceFetched (const QUrl&)),
				Qt::UniqueConnection);
		return erm;
	}

	bool PackageProcessor::HandleEntry (int packageId, const QFileInfo& fi,
			const QString& stagingDir, QDir& packageDir)
	{
		QFile dbFile (DBDir_.filePath (QString::number (packageId)));
		if (!dbFile.open (QIODevice::WriteOnly | QIODevice::Append))
		{
			qWarning () << Q_FUNC_INFO
					<< "could not open DB file"
					<< dbFile.fileName ()
					<< "for write:"
					<< dbFile.errorString ();
			return false;
		}

		QString sourceName = fi.absoluteFilePath ();
		sourceName = sourceName.mid (stagingDir.length ());
		if (sourceName.at (0) == '/')
			sourceName = sourceName.mid (1);

		if (fi.isFile ())
		{
			QString destName = packageDir.filePath (sourceName);
#ifndef QT_NO_DEBUG
			qDebug () << Q_FUNC_INFO
					<< "gotta copy"
					<< fi.absoluteFilePath ()
					<< "to"
					<< destName;
#endif

			QFile file (fi.absoluteFilePath ());
			if (!file.copy (destName))
			{
				qWarning () << Q_FUNC_INFO
						<< "could not copy"
						<< fi.absoluteFilePath ()
						<< "to"
						<< destName
						<< "because of"
						<< file.errorString ();

				QString errorString = tr ("Could not copy file %1 because of %2.")
						.arg (sourceName)
						.arg (file.errorString ());
				emit packageInstallError (packageId, errorString);
				return false;
			}
		}
		else if (fi.isDir ())
		{
#ifndef QT_NO_DEBUG
			qDebug () << Q_FUNC_INFO
					<< "gotta create"
					<< sourceName
					<< "for"
					<< fi.absoluteFilePath ();
#endif

			if (!packageDir.mkpath (sourceName))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to mkdir"
						<< sourceName
						<< "in"
						<< packageDir.path ();

				QString errorString = tr ("Unable to create directory %1.")
						.arg (sourceName);
				emit packageInstallError (packageId, errorString);
				return false;
			}
		}

		dbFile.write (sourceName.toUtf8 ());
		dbFile.write ("\n");

		return true;
	}

	bool PackageProcessor::CleanupBeforeUpdate (int oldId, int newId)
	{
		try
		{
			Remove (oldId);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "while removing package"
					<< oldId
					<< "for update to"
					<< newId
					<< "got exception:"
					<< e.what ();
			const auto& str = tr ("Unable to update package: %1.")
					.arg (QString::fromUtf8 (e.what ()));
			emit packageInstallError (newId, str);
			return false;
		}
		return true;
	}

	bool PackageProcessor::CleanupDir (const QString& directory)
	{
#ifndef QT_NO_DEBUG
		qDebug () << Q_FUNC_INFO
				<< directory;
#endif

		QDir dir (directory);
		for (const auto& subdir : dir.entryList (QDir::Dirs | QDir::NoDotAndDotDot))
			if (!CleanupDir (dir.absoluteFilePath (subdir)))
			{
				qWarning () << Q_FUNC_INFO
						<< "failed to cleanup subdir"
						<< subdir
						<< "for"
						<< directory;
				return false;
			}

		for (const auto& entry : dir.entryList (QDir::Files | QDir::Hidden | QDir::System))
			if (!dir.remove (entry))
			{
				qWarning () << Q_FUNC_INFO
						<< "failed to remove file"
						<< entry
						<< "for dir"
						<< directory;
				return false;
			}

		const auto& dirName = dir.dirName ();
		if (!dir.cdUp ())
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot cd up from"
					<< directory;
			return false;
		}

		const auto res = dir.rmdir (dirName);
		if (!res)
			qWarning () << Q_FUNC_INFO
					<< "cannot remove directory"
					<< dirName
					<< "from parent"
					<< dir.absolutePath ();

		return res;
	}
}
}
