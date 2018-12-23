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

#include "repoinfofetcher.h"
#include <QTimer>
#include <util/sll/either.h>
#include <util/sll/visitor.h>
#include <util/sys/paths.h>
#include <util/threads/futures.h>
#include <util/xpc/util.h>
#include <interfaces/core/ientitymanager.h>
#include "core.h"
#include "xmlparsers.h"
#include "lackmanutil.h"

namespace LeechCraft
{
namespace LackMan
{
	RepoInfoFetcher::RepoInfoFetcher (const ICoreProxy_ptr& proxy, QObject *parent)
	: QObject { parent }
	, Proxy_ { proxy }
	{
	}

	namespace
	{
		template<typename SuccessF>
		void FetchImpl (const QUrl& url, const ICoreProxy_ptr& proxy, QObject *object,
				const QString& failureHeading, SuccessF&& successFun)
		{
			const auto& location = Util::GetTemporaryName ("lackman_XXXXXX.gz");

			const auto iem = proxy->GetEntityManager ();

			const auto& e = Util::MakeEntity (url,
					location,
					Internal |
						DoNotNotifyUser |
						DoNotSaveInHistory |
						NotPersistent |
						DoNotAnnounceEntity);
			const auto& result = iem->DelegateEntity (e);
			if (!result)
			{
				iem->HandleEntity (Util::MakeNotification (RepoInfoFetcher::tr ("Error fetching repository"),
						RepoInfoFetcher::tr ("Could not find any plugins to fetch %1.")
							.arg ("<em>" + url.toString () + "</em>"),
						Priority::Critical));
				return;
			}

			Util::Sequence (object, boost::any_cast<QFuture<IDownload::Result>> (result.ExtendedResult_)) >>
					Util::Visitor
					{
						[successFun, location] (IDownload::Success) { successFun (location); },
						[proxy, url, failureHeading, location] (const IDownload::Error&)
						{
							proxy->GetEntityManager ()->HandleEntity (Util::MakeNotification (failureHeading,
									RepoInfoFetcher::tr ("Error downloading file from %1.")
											.arg (url.toString ()),
									Priority::Critical));
							QFile::remove (location);
						}
					};
		}
	}

	void RepoInfoFetcher::FetchFor (QUrl url)
	{
		QString path = url.path ();
		if (!path.endsWith ("/Repo.xml.gz"))
		{
			path.append ("/Repo.xml.gz");
			url.setPath (path);
		}

		QUrl baseUrl = url;
		baseUrl.setPath (baseUrl.path ().remove ("/Repo.xml.gz"));

		FetchImpl (url, Proxy_, this, tr ("Error fetching repository"),
				[this, baseUrl] (const QString& location) { HandleRIFinished (location, baseUrl); });
	}

	void RepoInfoFetcher::FetchComponent (QUrl url, int repoId, const QString& component)
	{
		if (!url.path ().endsWith ("/Packages.xml.gz"))
			url.setPath (url.path () + "/Packages.xml.gz");

		FetchImpl (url, Proxy_, this, tr ("Error fetching component"),
				[=] (const QString& location) { HandleComponentFinished (url, location, component, repoId); });
	}

	void RepoInfoFetcher::ScheduleFetchPackageInfo (const QUrl& url,
			const QString& name,
			const QList<QString>& newVers,
			int componentId)
	{
		ScheduledPackageFetch f =
		{
			url,
			name,
			newVers,
			componentId
		};

		if (ScheduledPackages_.isEmpty ())
			QTimer::singleShot (0,
					this,
					SLOT (rotatePackageFetchQueue ()));

		ScheduledPackages_ << f;
	}

	void RepoInfoFetcher::FetchPackageInfo (const QUrl& baseUrl,
			const QString& packageName,
			const QList<QString>& newVersions,
			int componentId)
	{
		auto packageUrl = baseUrl;
		packageUrl.setPath (packageUrl.path () +
				LackManUtil::NormalizePackageName (packageName) + ".xml.gz");

		FetchImpl (packageUrl, Proxy_, this, tr ("Error fetching package info"),
				[=] (const QString& location)
				{
					HandlePackageFinished ({ packageUrl, baseUrl, location, packageName, newVersions, componentId });
				});
	}

	void RepoInfoFetcher::rotatePackageFetchQueue ()
	{
		if (ScheduledPackages_.isEmpty ())
			return;

		const auto& f = ScheduledPackages_.takeFirst ();
		FetchPackageInfo (f.BaseUrl_, f.PackageName_, f.NewVersions_, f.ComponentId_);

		if (!ScheduledPackages_.isEmpty ())
			QTimer::singleShot (50, this, SLOT (rotatePackageFetchQueue ()));
	}

	void RepoInfoFetcher::HandleRIFinished (const QString& location, const QUrl& url)
	{
		QProcess *unarch = new QProcess (this);
		unarch->setProperty ("URL", url);
		unarch->setProperty ("Filename", location);
		connect (unarch,
				SIGNAL (finished (int, QProcess::ExitStatus)),
				this,
				SLOT (handleRepoUnarchFinished (int, QProcess::ExitStatus)));
		connect (unarch,
				&QProcess::errorOccurred,
				[=] { HandleUnarchError (url, location, unarch); });
#ifdef Q_OS_WIN32
		unarch->start ("7za", { "e", "-so", location });
#else
		unarch->start ("gunzip", { "-c", location });
#endif
	}

	void RepoInfoFetcher::HandleComponentFinished (const QUrl& url,
			const QString& location, const QString& component, int repoId)
	{
		QProcess *unarch = new QProcess (this);
		unarch->setProperty ("Component", component);
		unarch->setProperty ("Filename", location);
		unarch->setProperty ("URL", url);
		unarch->setProperty ("RepoID", repoId);
		connect (unarch,
				SIGNAL (finished (int, QProcess::ExitStatus)),
				this,
				SLOT (handleComponentUnarchFinished (int, QProcess::ExitStatus)));
		connect (unarch,
				&QProcess::errorOccurred,
				[=] { HandleUnarchError (url, location, unarch); });
#ifdef Q_OS_WIN32
		unarch->start ("7za", { "e", "-so", location });
#else
		unarch->start ("gunzip", { "-c", location });
#endif
	}

	void RepoInfoFetcher::HandlePackageFinished (const PendingPackage& pp)
	{
		QProcess *unarch = new QProcess (this);
		unarch->setProperty ("Filename", pp.Location_);
		unarch->setProperty ("URL", pp.URL_);
		unarch->setProperty ("PP", QVariant::fromValue (pp));
		connect (unarch,
				SIGNAL (finished (int, QProcess::ExitStatus)),
				this,
				SLOT (handlePackageUnarchFinished (int, QProcess::ExitStatus)));
		connect (unarch,
				&QProcess::errorOccurred,
				[=] { HandleUnarchError (pp.URL_, pp.Location_, unarch); });
#ifdef Q_OS_WIN32
		unarch->start ("7za", { "e", "-so", pp.Location_ });
#else
		unarch->start ("gunzip", { "-c", pp.Location_ });
#endif
	}

	void RepoInfoFetcher::handleRepoUnarchFinished (int exitCode,
			QProcess::ExitStatus)
	{
		sender ()->deleteLater ();

		if (exitCode)
		{
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification (tr ("Repository unpack error"),
					tr ("Unable to unpack the repository file. gunzip error: %1. "
						"Problematic file is at %2.")
						.arg (exitCode)
						.arg (sender ()->property ("Filename").toString ()),
					Priority::Critical));
			return;
		}

		QByteArray data = qobject_cast<QProcess*> (sender ())->readAllStandardOutput ();
		QFile::remove (sender ()->property ("Filename").toString ());

		RepoInfo info;
		try
		{
			info = ParseRepoInfo (sender ()->property ("URL").toUrl (), QString (data));
		}
		catch (const QString& error)
		{
			qWarning () << Q_FUNC_INFO
					<< error;
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification (tr ("Repository parse error"),
					tr ("Unable to parse repository description: %1.")
						.arg (error),
					Priority::Critical));
			return;
		}

		emit infoFetched (info);
	}

	void RepoInfoFetcher::handleComponentUnarchFinished (int exitCode,
			QProcess::ExitStatus)
	{
		sender ()->deleteLater ();

		if (exitCode)
		{
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification (tr ("Component unpack error"),
					tr ("Unable to unpack the component file. gunzip error: %1. "
						"Problematic file is at %2.")
						.arg (exitCode)
						.arg (sender ()->property ("Filename").toString ()),
					Priority::Critical));
			return;
		}

		QByteArray data = qobject_cast<QProcess*> (sender ())->readAllStandardOutput ();
		QFile::remove (sender ()->property ("Filename").toString ());

		PackageShortInfoList infos;
		try
		{
			infos = ParseComponent (data);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< e.what ();
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification (tr ("Component parse error"),
					tr ("Unable to parse component %1 description file. "
						"More information is available in logs.")
						.arg (sender ()->property ("Component").toString ()),
					Priority::Critical));
			return;
		}

		emit componentFetched (infos,
				sender ()->property ("Component").toString (),
				sender ()->property ("RepoID").toInt ());
	}

	void RepoInfoFetcher::handlePackageUnarchFinished (int exitCode,
			QProcess::ExitStatus)
	{
		sender ()->deleteLater ();

		auto pp = sender ()->property ("PP").value<PendingPackage> ();

		if (exitCode)
		{
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification (tr ("Component unpack error"),
					tr ("Unable to unpack the component file. gunzip error: %1. "
						"Problematic file is at %2.")
						.arg (exitCode)
						.arg (sender ()->property ("Filename").toString ()),
					Priority::Critical));
			return;
		}

		QByteArray data = qobject_cast<QProcess*> (sender ())->readAllStandardOutput ();
		QFile::remove (sender ()->property ("Filename").toString ());

		PackageInfo packageInfo;
		try
		{
			packageInfo = ParsePackage (data, pp.BaseURL_, pp.PackageName_, pp.NewVersions_);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< e.what ();
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification (tr ("Package parse error"),
					tr ("Unable to parse package description file. "
						"More information is available in logs."),
					Priority::Critical));
			return;
		}

		emit packageFetched (packageInfo, pp.ComponentId_);
	}

	void RepoInfoFetcher::HandleUnarchError (const QUrl& url, const QString& filename, QProcess *process)
	{
		process->deleteLater ();

		auto error = process->error ();

		qWarning () << Q_FUNC_INFO
				<< "unable to unpack for"
				<< url
				<< filename
				<< "with"
				<< error
				<< process->readAllStandardError ();
		Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification (tr ("Component unpack error"),
					tr ("Unable to unpack file. Exit code: %1. "
						"Problematic file is at %2.")
						.arg (error)
						.arg (sender ()->property ("Filename").toString ()),
					Priority::Critical));
	}
}
}
