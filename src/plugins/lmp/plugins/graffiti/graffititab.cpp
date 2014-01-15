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

#include "graffititab.h"
#include <functional>
#include <QFileSystemModel>
#include <QToolBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QtConcurrentRun>
#include <QFutureWatcher>
#include <QtDebug>
#include <QSettings>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <util/tags/tagscompletionmodel.h>
#include <util/tags/tagscompleter.h>
#include <util/gui/clearlineeditaddon.h>
#include <util/util.h>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/media/itagsfetcher.h>
#include <interfaces/lmp/ilmpproxy.h>
#include <interfaces/lmp/itagresolver.h>
#include <interfaces/lmp/mediainfo.h>
#include "filesmodel.h"
#include "renamedialog.h"
#include "genres.h"
#include "fileswatcher.h"
#include "cuesplitter.h"
#include "tagsfetchmanager.h"

namespace LeechCraft
{
namespace LMP
{
namespace Graffiti
{
	const int MaxHistoryCount = 25;

	GraffitiTab::GraffitiTab (ICoreProxy_ptr coreProxy, ILMPProxy_ptr proxy, const TabClassInfo& tc, QObject *plugin)
	: CoreProxy_ (coreProxy)
	, LMPProxy_ (proxy)
	, TC_ (tc)
	, Plugin_ (plugin)
	, FSModel_ (new QFileSystemModel (this))
	, FilesModel_ (new FilesModel (this))
	, FilesWatcher_ (new FilesWatcher (this))
	, Toolbar_ (new QToolBar ("Graffiti"))
	, IsChangingCurrent_ (false)
	{
		Ui_.setupUi (this);
		new Util::ClearLineEditAddon (coreProxy, Ui_.Album_);
		new Util::ClearLineEditAddon (coreProxy, Ui_.Artist_);
		new Util::ClearLineEditAddon (coreProxy, Ui_.Title_);
		new Util::ClearLineEditAddon (coreProxy, Ui_.Genre_);

		FSModel_->setRootPath (QDir::homePath ());
		FSModel_->setFilter (QDir::Dirs | QDir::NoDotAndDotDot);
		FSModel_->setReadOnly (true);
		Ui_.DirectoryTree_->setModel (FSModel_);

		auto idx = FSModel_->index (QDir::homePath ());
		while (idx.isValid ())
		{
			Ui_.DirectoryTree_->expand (idx);
			idx = idx.parent ();
		}

		Ui_.FilesList_->setModel (FilesModel_);

		connect (Ui_.FilesList_->selectionModel (),
				SIGNAL (currentRowChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (currentFileChanged (QModelIndex)));

		Save_ = Toolbar_->addAction (tr ("Save"),
				this, SLOT (save ()));
		Save_->setProperty ("ActionIcon", "document-save");
		Save_->setShortcut (QString ("Ctrl+S"));

		Revert_ = Toolbar_->addAction (tr ("Revert"),
				this, SLOT (revert ()));
		Revert_->setProperty ("ActionIcon", "document-revert");

		Toolbar_->addSeparator ();

		RenameFiles_ = Toolbar_->addAction (tr ("Rename files"),
				this, SLOT (renameFiles ()));
		RenameFiles_->setProperty ("ActionIcon", "edit-rename");

		Toolbar_->addSeparator ();

		GetTags_ = Toolbar_->addAction (tr ("Fetch tags"),
				this, SLOT (fetchTags ()));
		GetTags_->setProperty ("ActionIcon", "download");

		SplitCue_ = Toolbar_->addAction (tr ("Split CUE..."),
				this, SLOT (splitCue ()));
		SplitCue_->setProperty ("ActionIcon", "split");
		SplitCue_->setEnabled (false);

		Ui_.Genre_->SetSeparator (" / ");

		auto model = new Util::TagsCompletionModel (this);
		model->UpdateTags (Genres);
		auto completer = new Util::TagsCompleter (Ui_.Genre_, this);
		completer->OverrideModel (model);

		Ui_.Genre_->AddSelector ();

		connect (FilesWatcher_,
				SIGNAL (rereadFiles ()),
				this,
				SLOT (handleRereadFiles ()));

		RestorePathHistory ();
	}

	TabClassInfo GraffitiTab::GetTabClassInfo () const
	{
		return TC_;
	}

	QObject* GraffitiTab::ParentMultiTabs ()
	{
		return Plugin_;
	}

	void GraffitiTab::Remove ()
	{
		emit removeTab (this);
		deleteLater ();
	}

	QToolBar* GraffitiTab::GetToolBar () const
	{
		return Toolbar_.get ();
	}

	template<typename T, typename F>
	void GraffitiTab::UpdateData (const T& newData, F getter)
	{
		if (IsChangingCurrent_)
			return;

		static_assert (std::is_lvalue_reference<typename std::result_of<F (MediaInfo&)>::type>::value,
				"functor doesn't return an lvalue reference");

		const auto& selected = Ui_.FilesList_->selectionModel ()->selectedRows ();
		for (const auto& index : selected)
		{
			const auto& infoData = index.data (FilesModel::Roles::MediaInfoRole);
			auto info = infoData.template value<MediaInfo> ();
			getter (info) = newData;
			FilesModel_->UpdateInfo (index, info);
		}

		if (!selected.isEmpty ())
		{
			Save_->setEnabled (true);
			Revert_->setEnabled (true);
		}
	}

	void GraffitiTab::SetPath (const QString& path)
	{
		AddToPathHistory (path);

		setEnabled (false);
		FilesModel_->Clear ();
		FilesWatcher_->Clear ();

		auto watcher = new QFutureWatcher<QList<QFileInfo>> ();
		connect (watcher,
				SIGNAL (finished ()),
				this,
				SLOT (handleIterateFinished ()));

		watcher->setFuture (QtConcurrent::run ([this, path]
					{ return LMPProxy_->RecIterateInfo (path, true); }));

		SplitCue_->setEnabled (!QDir (path).entryList ({ "*.cue" }).isEmpty ());
	}

	void GraffitiTab::RestorePathHistory ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_LMP_Graffiti");
		settings.beginGroup ("PathHistory");
		const auto& paths = settings.value ("HistList").toStringList ();
		settings.endGroup ();

		Ui_.PathLine_->blockSignals (true);
		for (const auto& item : paths)
			if (QFile::exists (item))
				Ui_.PathLine_->addItem (item);

		Ui_.PathLine_->setCurrentIndex (-1);
		Ui_.PathLine_->blockSignals (false);
	}

	void GraffitiTab::AddToPathHistory (const QString& path)
	{
		const auto sameIdx = Ui_.PathLine_->findText (path);
		if (!sameIdx)
			return;

		Ui_.PathLine_->blockSignals (true);
		if (sameIdx > 0)
			Ui_.PathLine_->removeItem (sameIdx);

		Ui_.PathLine_->insertItem (0, path);

		while (Ui_.PathLine_->count () > MaxHistoryCount)
			Ui_.PathLine_->removeItem (Ui_.PathLine_->count () - 1);

		Ui_.PathLine_->setCurrentIndex (0);
		Ui_.PathLine_->blockSignals (false);

		QStringList paths;
		for (int i = 0; i < Ui_.PathLine_->count (); ++i)
			paths << Ui_.PathLine_->itemText (i);

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_LMP_Graffiti");
		settings.beginGroup ("PathHistory");
		settings.setValue ("HistList", paths);
		settings.endGroup ();
	}

	void GraffitiTab::on_Artist__textChanged (const QString& artist)
	{
		UpdateData (artist, [] (MediaInfo& info) -> QString& { return info.Artist_; });
	}

	void GraffitiTab::on_Album__textChanged (const QString& album)
	{
		UpdateData (album, [] (MediaInfo& info) -> QString& { return info.Album_; });
	}

	void GraffitiTab::on_Title__textChanged (const QString& title)
	{
		UpdateData (title, [] (MediaInfo& info) -> QString& { return info.Title_; });
	}

	void GraffitiTab::on_Genre__textChanged (const QString& genreString)
	{
		auto genres = genreString.split ('/', QString::SkipEmptyParts);
		for (auto& genre : genres)
			genre = genre.trimmed ();

		UpdateData (genres, [] (MediaInfo& info) -> QStringList& { return info.Genres_; });
	}

	void GraffitiTab::on_Year__valueChanged (int year)
	{
		UpdateData (year, [] (MediaInfo& info) -> int& { return info.Year_; });
	}

	void GraffitiTab::save ()
	{
		const auto& modified = FilesModel_->GetModified ();
		if (modified.isEmpty ())
			return;

		if (QMessageBox::question (this,
				"LMP Graffiti",
				tr ("Do you really want to accept changes to %n file(s)?", 0, modified.size ()),
				QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;

		ITagResolver *resolver = LMPProxy_->GetTagResolver ();

		auto toTLStr = [] (const QString& str)
		{
			return TagLib::String (str.toUtf8 ().constData (), TagLib::String::UTF8);
		};

		for (const auto& pair : modified)
		{
			const auto& newInfo = pair.first;

			QMutexLocker locker (&resolver->GetMutex ());
			auto file = resolver->GetFileRef (newInfo.LocalPath_);
			auto tag = file.tag ();

			tag->setArtist (toTLStr (newInfo.Artist_));
			tag->setAlbum (toTLStr (newInfo.Album_));
			tag->setTitle (toTLStr (newInfo.Title_));
			tag->setYear (newInfo.Year_);
			tag->setGenre (toTLStr (newInfo.Genres_.join (" / ")));

			if (!file.save ())
				qWarning () << Q_FUNC_INFO
						<< "unable to save file"
						<< newInfo.LocalPath_;
		}

		handleRereadFiles ();
	}

	void GraffitiTab::revert ()
	{
		const auto& modified = FilesModel_->GetModified ();
		if (modified.isEmpty ())
			return;

		if (QMessageBox::question (this,
				"LMP Graffiti",
				tr ("Do you really want to revert changes to %n file(s)?", 0, modified.size ()),
				QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;

		QList<MediaInfo> origs;
		for (const auto& pair : modified)
			origs << pair.second;
		FilesModel_->SetInfos (origs);

		Save_->setEnabled (false);
		Revert_->setEnabled (false);

		currentFileChanged (Ui_.FilesList_->currentIndex ());
	}

	void GraffitiTab::renameFiles ()
	{
		if (!FilesModel_->GetModified ().isEmpty ())
		{
			auto res = QMessageBox::question (this,
					"LMP Graffiti",
					tr ("You have unsaved files with changed tags. Do you want to save or discard those changes?"),
					QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
			if (res == QMessageBox::Save)
				save ();
			else if (res == QMessageBox::Discard)
				revert ();
			else
				return;
		}

		QList<MediaInfo> infos;
		for (const auto& index : Ui_.FilesList_->selectionModel ()->selectedRows ())
			infos << index.data (FilesModel::Roles::MediaInfoRole).value<MediaInfo> ();
		if (infos.isEmpty ())
			return;

		auto dia = new RenameDialog (LMPProxy_, this);
		dia->SetInfos (infos);

		dia->setAttribute (Qt::WA_DeleteOnClose);
		dia->show ();
	}

	void GraffitiTab::fetchTags ()
	{
		auto provs = CoreProxy_->GetPluginsManager ()->GetAllCastableTo<Media::ITagsFetcher*> ();
		if (provs.isEmpty ())
			return;

		const auto& rows = Ui_.FilesList_->selectionModel ()->selectedRows ();
		QStringList paths;
		for (const auto& index : rows)
		{
			const auto& info = index.data (FilesModel::Roles::MediaInfoRole).value<MediaInfo> ();
			paths << info.LocalPath_;
		}

		if (paths.isEmpty ())
			return;

		GetTags_->setEnabled (false);

		auto fetcher = new TagsFetchManager (paths, provs.first (), FilesModel_, this);
		connect (fetcher,
				SIGNAL (tagsFetchProgress (int, int, QObject*)),
				this,
				SIGNAL (tagsFetchProgress (int, int, QObject*)));
		connect (fetcher,
				SIGNAL (tagsFetched (QString)),
				this,
				SLOT (handleTagsFetched (QString)));
		connect (fetcher,
				SIGNAL (finished (bool)),
				GetTags_,
				SLOT (setEnabled (bool)));
	}

	void GraffitiTab::splitCue ()
	{
		const auto& curDirIdx = Ui_.DirectoryTree_->currentIndex ();
		if (!curDirIdx.isValid ())
			return;

		const auto& path = FSModel_->filePath (curDirIdx);
		const QDir dir (path);

		const auto& cues = dir.entryList ({ "*.cue" });
		if (cues.isEmpty ())
		{
			QMessageBox::critical (this,
					"LMP Graffiti",
					tr ("No cue sheets are available in this directory."));
			return;
		}

		QString cue;
		if (cues.size () >= 2)
		{
			cue = QInputDialog::getItem (this,
					"Select cue sheet",
					tr ("Select cue sheet to use for splitting:"),
					cues,
					0,
					false);
			if (cue.isEmpty ())
				return;
		}
		else
			cue = cues.first ();

		auto splitter = new CueSplitter (cue, path);
		connect (splitter,
				SIGNAL (error (QString)),
				this,
				SLOT (handleCueSplitError (QString)));
		connect (splitter,
				SIGNAL (finished (CueSplitter*)),
				this,
				SLOT (handleCueSplitFinished ()));
		emit cueSplitStarted (splitter);
	}

	void GraffitiTab::handleTagsFetched (const QString& filename)
	{
		const auto& curIdx = Ui_.FilesList_->selectionModel ()->currentIndex ();
		const auto& curInfo = curIdx.data (FilesModel::Roles::MediaInfoRole).value<MediaInfo> ();
		if (curInfo.LocalPath_ == filename)
			currentFileChanged (curIdx);
	}

	void GraffitiTab::on_DirectoryTree__activated (const QModelIndex& index)
	{
		const auto& path = FSModel_->filePath (index);
		Ui_.PathLine_->blockSignals (true);
		Ui_.PathLine_->setEditText (path);
		Ui_.PathLine_->blockSignals (false);

		SetPath (path);
	}

	void GraffitiTab::on_PathLine__editTextChanged (QString path)
	{
		if (path.startsWith ('~'))
		{
			path.replace (0, 1, QDir::homePath ());
			Ui_.PathLine_->setEditText (path);
		}

		Ui_.DirectoryTree_->setCurrentIndex (FSModel_->index (path));

		SetPath (path);
	}

	void GraffitiTab::currentFileChanged (const QModelIndex& index)
	{
		const auto& infoData = FilesModel_->data (index, FilesModel::Roles::MediaInfoRole);
		const auto& info = infoData.value<MediaInfo> ();

		IsChangingCurrent_ = true;

		Ui_.Album_->setText (info.Album_);
		Ui_.Artist_->setText (info.Artist_);
		Ui_.Title_->setText (info.Title_);
		Ui_.Genre_->setText (info.Genres_.join (" / "));

		Ui_.Year_->setValue (info.Year_);

		IsChangingCurrent_ = false;
	}

	void GraffitiTab::handleRereadFiles ()
	{
		const auto& current = Ui_.DirectoryTree_->currentIndex ();
		on_DirectoryTree__activated (current);
	}

	void GraffitiTab::handleIterateFinished ()
	{
		auto watcher = dynamic_cast<QFutureWatcher<QList<QFileInfo>>*> (sender ());
		watcher->deleteLater ();

		const auto& files = watcher->result ();

		FilesWatcher_->AddFiles (files);
		FilesModel_->AddFiles (files);

		auto resolver = LMPProxy_->GetTagResolver ();
		auto worker = [resolver, files] () -> QList<MediaInfo>
		{
			QList<MediaInfo> infos;
			for (const auto& file : files)
				try
				{
					infos << resolver->ResolveInfo (file.absoluteFilePath ());
				}
				catch (const std::exception& e)
				{
					qWarning () << Q_FUNC_INFO
							<< e.what ();
				}
			return infos;
		};

		auto scanWatcher = new QFutureWatcher<QList<MediaInfo>> ();
		connect (scanWatcher,
				SIGNAL (finished ()),
				this,
				SLOT (handleScanFinished ()));
		scanWatcher->setFuture (QtConcurrent::run (std::function<QList<MediaInfo> ()> (worker)));
	}

	void GraffitiTab::handleScanFinished ()
	{
		auto watcher = dynamic_cast<QFutureWatcher<QList<MediaInfo>>*> (sender ());
		watcher->deleteLater ();

		FilesModel_->SetInfos (watcher->result ());
		setEnabled (true);
	}

	void GraffitiTab::handleCueSplitError (const QString& error)
	{
		const auto& e = Util::MakeNotification ("LMP Graffiti", error, PCritical_);
		CoreProxy_->GetEntityManager ()->HandleEntity (e);
	}

	void GraffitiTab::handleCueSplitFinished ()
	{
		const auto& e = Util::MakeNotification ("LMP Graffiti",
				tr ("Finished splitting CUE file"),
				PInfo_);
		CoreProxy_->GetEntityManager ()->HandleEntity (e);
	}
}
}
}
