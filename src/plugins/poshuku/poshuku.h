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

#pragma once

#include <memory>
#include <QAction>
#include <QTranslator>
#include <QWidget>
#include <interfaces/iinfo.h>
#include <interfaces/ihavetabs.h>
#include <interfaces/iwebbrowser.h>
#include <interfaces/ipluginready.h>
#include <interfaces/iactionsexporter.h>
#include <interfaces/ihavesettings.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/ihaveshortcuts.h>
#include <interfaces/ihaverecoverabletabs.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "browserwidget.h"

namespace LeechCraft
{
namespace Util
{
	class WkFontsWidget;
	class ShortcutManager;
}

namespace Poshuku
{
	class Poshuku : public QObject
					, public IInfo
					, public IHaveTabs
					, public IPluginReady
					, public IHaveSettings
					, public IEntityHandler
					, public IHaveShortcuts
					, public IWebBrowser
					, public IActionsExporter
					, public IHaveRecoverableTabs
	{
		Q_OBJECT
		Q_INTERFACES (IInfo
				IHaveTabs
				IHaveSettings
				IEntityHandler
				IPluginReady
				IWebBrowser
				IHaveShortcuts
				IActionsExporter
				IHaveRecoverableTabs)

		LC_PLUGIN_METADATA ("org.LeechCraft.Poshuku")

		QMenu *ToolMenu_;
		QAction *ImportXbel_;
		QAction *ExportXbel_;
		QAction *CheckFavorites_;
		QAction *ReloadAll_;

		Util::XmlSettingsDialog_ptr XmlSettingsDialog_;

		Util::ShortcutManager *ShortcutMgr_;

		Util::WkFontsWidget *FontsWidget_;
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		void Release ();
		QByteArray GetUniqueID () const;
		QString GetName () const;
		QString GetInfo () const;
		QStringList Provides () const;
		QIcon GetIcon () const;

		TabClasses_t GetTabClasses () const;
		void TabOpenRequested (const QByteArray&);

		QSet<QByteArray> GetExpectedPluginClasses () const;
		void AddPlugin (QObject*);

		std::shared_ptr<LeechCraft::Util::XmlSettingsDialog> GetSettingsDialog () const;

		EntityTestHandleResult CouldHandle (const LeechCraft::Entity&) const;
		void Handle (LeechCraft::Entity);

		void Open (const QString&);
		IWebWidget* GetWidget () const;

		void SetShortcut (const QString&, const QKeySequences_t&);
		QMap<QString, ActionInfo> GetActionInfo () const;

		QList<QAction*> GetActions (ActionsEmbedPlace) const;

		void RecoverTabs (const QList<TabRecoverInfo>&);
		bool HasSimilarTab (const QByteArray&, const QList<QByteArray>&) const;
	private:
		void InitConnections ();
		void PrepopulateShortcuts ();
	private slots:
		void createTabFirstTime ();
		void handleError (const QString&);
		void handleSettingsClicked (const QString&);
		void handleCheckFavorites ();
		void handleReloadAll ();
		void handleBrowserWidgetCreated (BrowserWidget*);
	signals:
		void addNewTab (const QString&, QWidget*);
		void removeTab (QWidget*);
		void changeTabName (QWidget*, const QString&);
		void changeTabIcon (QWidget*, const QIcon&);
		void changeTooltip (QWidget*, QWidget*);
		void statusBarChanged (QWidget*, const QString&);
		void raiseTab (QWidget*);

		void gotActions (QList<QAction*>, LeechCraft::ActionsEmbedPlace);

		void tabRecovered (const QByteArray&, QWidget*);
	};
}
}
