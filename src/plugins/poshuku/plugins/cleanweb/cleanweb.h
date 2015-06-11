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
#include <QObject>
#include <QMap>
#include <QTranslator>
#include <QWebPage>
#include <QNetworkAccessManager>
#include <QMenu>
#include <interfaces/iinfo.h>
#include <interfaces/iplugin2.h>
#include <interfaces/ihavesettings.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/istartupwizard.h>
#include <interfaces/poshuku/iwebplugin.h>
#include <interfaces/poshuku/poshukutypes.h>
#include <interfaces/core/ihookproxy.h>
#include <interfaces/core/ipluginsmanager.h>

class QWebView;
class QContextMenuEvent;

namespace LeechCraft
{
namespace Poshuku
{
namespace CleanWeb
{
	class Core;
	class FlashOnClickPlugin;
	class FlashOnClickWhitelist;

	class CleanWeb : public QObject
					, public IInfo
					, public IHaveSettings
					, public IEntityHandler
					, public IStartupWizard
					, public IPlugin2
	{
		Q_OBJECT
		Q_INTERFACES (IInfo IHaveSettings IEntityHandler IStartupWizard IPlugin2)

		LC_PLUGIN_METADATA ("org.LeechCraft.Poshuku.CleanWeb")

		ICoreProxy_ptr Proxy_;

		std::shared_ptr<FlashOnClickPlugin> FlashOnClickPlugin_;
		FlashOnClickWhitelist *FlashOnClickWhitelist_;

		std::shared_ptr<Core> Core_;

		Util::XmlSettingsDialog_ptr SettingsDialog_;
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		void Release ();
		QByteArray GetUniqueID () const;
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;
		QStringList Provides () const;
		QStringList Needs () const;
		QStringList Uses () const;
		void SetProvider (QObject*, const QString&);

		Util::XmlSettingsDialog_ptr GetSettingsDialog () const;

		EntityTestHandleResult CouldHandle (const Entity&) const;
		void Handle (Entity);

		QList<QWizardPage*> GetWizardPages () const;

		QSet<QByteArray> GetPluginClasses () const;
	public slots:
		void hookExtension (LeechCraft::IHookProxy_ptr,
				QWebPage*,
				QWebPage::Extension,
				const QWebPage::ExtensionOption*,
				QWebPage::ExtensionReturn*);
		void hookInitialLayoutCompleted (LeechCraft::IHookProxy_ptr,
				QWebPage*,
				QWebFrame*);
		void hookNAMCreateRequest (LeechCraft::IHookProxy_ptr proxy,
				QNetworkAccessManager *manager,
				QNetworkAccessManager::Operation *op,
				QIODevice **dev);
		void hookWebPluginFactoryReload (LeechCraft::IHookProxy_ptr,
				QList<IWebPlugin*>&);
		void hookWebViewContextMenu (LeechCraft::IHookProxy_ptr,
				QWebView*,
				QContextMenuEvent*,
				const QWebHitTestResult&, QMenu*,
				WebViewCtxMenuStage);
	};
}
}
}
