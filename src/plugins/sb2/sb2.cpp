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

#include "sb2.h"
#include <QIcon>
#include <QMainWindow>
#include <QStatusBar>
#include <QGraphicsEffect>
#include <QtDebug>

#if USE_QT5
#include <QtQuick>
#else
#include <QtDeclarative>
#endif

#include <util/shortcuts/shortcutmanager.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/irootwindowsmanager.h>
#include <interfaces/imwproxy.h>
#include "viewmanager.h"
#include "sbview.h"
#include "launchercomponent.h"
#include "traycomponent.h"
#include "lcmenucomponent.h"
#include "desaturateeffect.h"
#include "dockactioncomponent.h"

Q_DECLARE_METATYPE (QSet<QByteArray>);

namespace LeechCraft
{
namespace SB2
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;

		ShortcutMgr_ = new Util::ShortcutManager (proxy, this);
		ShortcutMgr_->SetObject (this);

		qmlRegisterType<QGraphicsBlurEffect> ("Effects", 1, 0, "Blur");
		qmlRegisterType<QGraphicsColorizeEffect> ("Effects", 1, 0, "Colorize");
		qmlRegisterType<QGraphicsDropShadowEffect> ("Effects", 1, 0, "DropShadow");
		qmlRegisterType<QGraphicsOpacityEffect> ("Effects", 1, 0, "OpacityEffect");
		qmlRegisterType<DesaturateEffect> ("Effects", 1, 0, "Desaturate");

		qRegisterMetaType<QSet<QByteArray>> ("QSet<QByteArray>");
		qRegisterMetaTypeStreamOperators<QSet<QByteArray>> ();

		auto rootWM = proxy->GetRootWindowsManager ();
		for (int i = 0; i < rootWM->GetWindowsCount (); ++i)
			handleWindow (i, true);

		connect (rootWM->GetQObject (),
				SIGNAL (windowAdded (int)),
				this,
				SLOT (handleWindow (int)));
		connect (rootWM->GetQObject (),
				SIGNAL (windowRemoved (int)),
				this,
				SLOT (handleWindowRemoved (int)));
	}

	void Plugin::SecondInit ()
	{
		emit pluginsAvailable ();

		for (const auto& info : Managers_)
			info.Mgr_->SecondInit ();
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.SB2";
	}

	void Plugin::Release ()
	{
	}

	QString Plugin::GetName () const
	{
		return "SB2";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Next-generation fluid sidebar.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/resources/images/sb2.svg");
		return icon;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Core.Plugins/1.0";
		return result;
	}

	QMap<QString, ActionInfo> Plugin::GetActionInfo () const
	{
		return ShortcutMgr_->GetActionInfo ();
	}

	void Plugin::SetShortcut (const QString& id, const QKeySequences_t& seqs)
	{
		ShortcutMgr_->SetShortcut (id, seqs);
	}

	void Plugin::hookDockWidgetActionVisToggled (IHookProxy_ptr proxy,
			QMainWindow*, QDockWidget*, bool)
	{
		proxy->CancelDefault ();
	}

	void Plugin::hookAddingDockAction (IHookProxy_ptr,
			QMainWindow *win, QAction *act, Qt::DockWidgetArea)
	{
		auto rootWM = Proxy_->GetRootWindowsManager ();
		const int idx = rootWM->GetWindowIndex (win);

		Managers_ [idx].Dock_->AddActions ({ act }, TrayComponent::ActionPos::Beginning);
	}

	void Plugin::hookRemovingDockAction (IHookProxy_ptr,
			QMainWindow *win, QAction *act, Qt::DockWidgetArea)
	{
		auto rootWM = Proxy_->GetRootWindowsManager ();
		const int idx = rootWM->GetWindowIndex (win);

		Managers_ [idx].Dock_->RemoveAction (act);
	}

	void Plugin::hookDockBarWillBeShown (IHookProxy_ptr proxy,
			QMainWindow*, QToolBar*, Qt::DockWidgetArea)
	{
		proxy->CancelDefault ();
	}

	void Plugin::handleWindow (int index, bool init)
	{
		auto rootWM = Proxy_->GetRootWindowsManager ();
		auto win = rootWM->GetMainWindow (index);

		auto mgr = new ViewManager (Proxy_, ShortcutMgr_, win, this);
		auto view = mgr->GetView ();

		auto mwProxy = rootWM->GetMWProxy (index);
		auto ictw = rootWM->GetTabWidget (index);

		win->statusBar ()->hide ();

		mgr->RegisterInternalComponent ((new LCMenuComponent (mwProxy))->GetComponent ());

		auto launcher = new LauncherComponent (ictw, Proxy_, mgr);
		mgr->RegisterInternalComponent (launcher->GetComponent ());
		if (init)
			connect (this,
					SIGNAL (pluginsAvailable ()),
					launcher,
					SLOT (handlePluginsAvailable ()));
		else
			launcher->handlePluginsAvailable ();

		auto tray = new TrayComponent (Proxy_, view);
		mgr->RegisterInternalComponent (tray->GetComponent ());
		if (init)
			connect (this,
					SIGNAL (pluginsAvailable ()),
					tray,
					SLOT (handlePluginsAvailable ()));
		else
			tray->handlePluginsAvailable ();

		auto dock = new DockActionComponent (Proxy_, view);
		mgr->RegisterInternalComponent (dock->GetComponent ());

		if (!init)
			mgr->SecondInit ();

		Managers_.push_back ({ mgr, tray, dock });
	}

	void Plugin::handleWindowRemoved (int index)
	{
		const auto& info = Managers_.takeAt (index);
		delete info.Mgr_;
		delete info.Tray_;
		delete info.Dock_;
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_sb2, LeechCraft::SB2::Plugin);

