/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "handlerchoicedialog.h"
#include <QRadioButton>
#include <QFileInfo>
#include <QSettings>
#include <QDir>
#include <QFileDialog>
#include "interfaces/iinfo.h"
#include "interfaces/idownload.h"
#include "interfaces/ientityhandler.h"
#include "core.h"

namespace LeechCraft
{
	HandlerChoiceDialog::HandlerChoiceDialog (const QString& entity, QWidget *parent)
	: QDialog (parent)
	, Buttons_ (new QButtonGroup)
	{
		Ui_.setupUi (this);
		Ui_.EntityLabel_->setText (entity);
		Ui_.DownloadersLabel_->hide ();
		Ui_.HandlersLabel_->hide ();

		connect (Buttons_.get (),
				SIGNAL (buttonReleased (int)),
				this,
				SLOT (populateLocationsBox ()));
	}

	void HandlerChoiceDialog::SetFilenameSuggestion (const QString& location)
	{
		Suggestion_ = location;
	}

	void HandlerChoiceDialog::Add (QObject *obj)
	{
		auto ii = qobject_cast<IInfo*> (obj);
		if (auto idl = qobject_cast<IDownload*> (obj))
			Add (ii, idl);
		if (auto ieh = qobject_cast<IEntityHandler*> (obj))
			Add (ii, ieh);
	}

	QRadioButton* HandlerChoiceDialog::AddCommon (const IInfo *ii, const QString& addedAs)
	{
		QString name;
		QString tooltip;
		QIcon icon;
		try
		{
			name = ii->GetName ();
			tooltip = ii->GetInfo ();
			icon = ii->GetIcon ();
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
				<< "could not query"
				<< e.what ()
				<< ii;
			return 0;
		}
		catch (...)
		{
			qWarning () << Q_FUNC_INFO
				<< "could not query"
				<< ii;
			return 0;
		}

		auto but = new QRadioButton (name, this);
		but->setToolTip (tooltip);
		but->setIconSize (QSize (32, 32));
		but->setIcon (icon);
		but->setProperty ("AddedAs", addedAs);
		but->setProperty ("PluginID", ii->GetUniqueID ());

		if (Buttons_->buttons ().isEmpty ())
			but->setChecked (true);

		Buttons_->addButton (but);

		Infos_ [name] = ii;

		if (Downloaders_.size () + Handlers_.size () == 1)
			populateLocationsBox ();

		return but;
	}

	bool HandlerChoiceDialog::Add (const IInfo *ii, IDownload *id)
	{
		auto button = AddCommon (ii, "IDownload");
		if (!button)
			return false;

		Ui_.DownloadersLayout_->addWidget (button);
		Ui_.DownloadersLabel_->show ();
		Downloaders_ [ii->GetName ()] = id;
		return true;
	}

	bool HandlerChoiceDialog::Add (const IInfo *ii, IEntityHandler *ih)
	{
		auto button = AddCommon (ii, "IEntityHandler");
		if (!button)
			return false;

		Ui_.HandlersLayout_->addWidget (button);
		Ui_.HandlersLabel_->show ();
		Handlers_ [ii->GetName ()] = ih;
		return true;
	}

	QObject* HandlerChoiceDialog::GetSelected () const
	{
		auto checked = Buttons_->checkedButton ();
		if (!checked)
			return 0;

		const auto& id = checked->property ("PluginID").toByteArray ();
		return Core::Instance ().GetPluginManager ()->GetPluginByID (id);
	}

	IDownload* HandlerChoiceDialog::GetDownload ()
	{
		if (!Buttons_->checkedButton () ||
				Buttons_->checkedButton ()->
					property ("AddedAs").toString () != "IDownload")
			return 0;

		return Downloaders_.value (Buttons_->
				checkedButton ()->text (), 0);
	}

	IDownload* HandlerChoiceDialog::GetFirstDownload ()
	{
		return Downloaders_.empty () ? 0 : Downloaders_.begin ().value ();
	}

	IEntityHandler* HandlerChoiceDialog::GetEntityHandler ()
	{
		return Handlers_.value (Buttons_->
				checkedButton ()->text (), 0);
	}

	IEntityHandler* HandlerChoiceDialog::GetFirstEntityHandler ()
	{
		return Handlers_.empty () ? 0 : Handlers_.begin ().value ();
	}

	QList<IEntityHandler*> HandlerChoiceDialog::GetAllEntityHandlers ()
	{
		return Handlers_.values ();
	}

	QString HandlerChoiceDialog::GetFilename ()
	{
		const QString& name = Buttons_->checkedButton ()->
			property ("PluginID").toString ();

		QString result;
		if (Ui_.LocationsBox_->currentIndex () == 0 &&
				Ui_.LocationsBox_->currentText ().isEmpty ())
			on_BrowseButton__released ();

		result = Ui_.LocationsBox_->currentText ();
		if (result.isEmpty ())
			return QString ();

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName ());
		settings.setValue ("PreviousEntitySavePath", result);

		settings.beginGroup ("SavePaths");
		QStringList pluginTexts = settings.value (name).toStringList ();
		pluginTexts.removeAll (result);
		pluginTexts.prepend (result);
		pluginTexts = pluginTexts.mid (0, 20);
		settings.setValue (name, pluginTexts);
		settings.endGroup ();

		return result;
	}

	int HandlerChoiceDialog::NumChoices () const
	{
		return Buttons_->buttons ().size ();
	}

	QStringList HandlerChoiceDialog::GetPluginSavePaths (const QString& plugin) const
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName ());
		settings.beginGroup ("SavePaths");

		const QStringList& l = settings.value (plugin).toStringList ();
		settings.endGroup ();
		return l;
	}

	void HandlerChoiceDialog::populateLocationsBox ()
	{
		while (Ui_.LocationsBox_->count () > 1)
			Ui_.LocationsBox_->removeItem (1);

		QAbstractButton *checked = Buttons_->checkedButton ();
		if (!checked)
			return;

		if (checked->property ("AddedAs").toString () == "IEntityHandler")
		{
			Ui_.LocationsBox_->setEnabled (false);
			Ui_.BrowseButton_->setEnabled (false);
			return;
		}
		Ui_.LocationsBox_->setEnabled (true);
		Ui_.BrowseButton_->setEnabled (true);

		Ui_.LocationsBox_->insertSeparator (1);

		if (Suggestion_.size ())
			Ui_.LocationsBox_->addItem (Suggestion_);

		const QString& plugin = checked->property ("PluginID").toString ();
		const QStringList& pluginTexts = GetPluginSavePaths (plugin).mid (0, 7);

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName ());
		settings.beginGroup ("SavePaths");
		QStringList otherPlugins = settings.childKeys ();
		settings.endGroup ();

		otherPlugins.removeAll (plugin);
		QList<QStringList> otherTextsList;
		Q_FOREACH (const QString& otherPlugin, otherPlugins)
			otherTextsList.append (GetPluginSavePaths (otherPlugin));

		for (QList<QStringList>::iterator it = otherTextsList.begin (), end = otherTextsList.end ();
			 it != end; ++it)
			Q_FOREACH (const QString& ptext, pluginTexts)
				it->removeAll (ptext);

		QStringList otherTexts;
		while (otherTexts.size () < 16)
		{
			bool added = false;
			for (QList<QStringList>::iterator it = otherTextsList.begin (), end = otherTextsList.end ();
				 it != end; ++it)
			{
				if (!it->isEmpty ())
				{
					otherTexts += it->takeFirst ();
					added = true;
				}
			}
			if (!added)
				break;
		}

		if (!pluginTexts.isEmpty ())
		{
			Ui_.LocationsBox_->addItems (pluginTexts);
			if (!otherTexts.isEmpty ())
				Ui_.LocationsBox_->insertSeparator (pluginTexts.size () + 2);
		}
		Ui_.LocationsBox_->addItems (otherTexts);

		if (!Suggestion_.isEmpty ())
			Ui_.LocationsBox_->setCurrentIndex (1);
		else
		{
			const QString& prev = settings.value ("PreviousEntitySavePath").toString ();
			if (!prev.isEmpty () &&
					pluginTexts.contains (prev))
			{
				const int pos = Ui_.LocationsBox_->findText (prev);
				if (pos != -1)
					Ui_.LocationsBox_->setCurrentIndex (pos);
			}
			else if (!pluginTexts.isEmpty ())
				Ui_.LocationsBox_->setCurrentIndex (2);
		}
	}

	void HandlerChoiceDialog::on_BrowseButton__released ()
	{
		const QString& name = Buttons_->checkedButton ()->
			property ("PluginID").toString ();

		if (Suggestion_.isEmpty ())
			Suggestion_ = GetPluginSavePaths (name).value (0, QDir::homePath ());

		const QString& result = QFileDialog::getExistingDirectory (0,
				tr ("Select save location"),
				Suggestion_);
		if (result.isEmpty ())
			return;

		Ui_.LocationsBox_->setCurrentIndex (0);
		Ui_.LocationsBox_->setItemText (0, result);
	}
}

