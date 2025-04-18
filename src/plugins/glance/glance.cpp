/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "glance.h"
#include <QAction>
#include <QTabWidget>
#include <QToolBar>
#include <QMainWindow>
#include <util/sll/qtutil.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/irootwindowsmanager.h>
#include <interfaces/core/iiconthememanager.h>
#include "glanceview.h"

namespace LC::Glance
{
	void Plugin::Init (ICoreProxy_ptr)
	{
		ActionGlance_ = new QAction (GetName (), this);
		ActionGlance_->setToolTip (tr ("Show the quick overview of tabs"));
		ActionGlance_->setShortcut ("Ctrl+Shift+G"_qs);
		ActionGlance_->setShortcutContext (Qt::ApplicationShortcut);
		ActionGlance_->setProperty ("ActionIcon", "view-list-icons");
		ActionGlance_->setProperty ("Action/ID", GetUniqueID () + "_glance");

		connect (ActionGlance_,
				&QAction::triggered,
				this,
				&Plugin::ShowGlance);
	}

	void Plugin::SecondInit ()
	{
	}

	void Plugin::Release ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Glance";
	}

	QString Plugin::GetName () const
	{
		return "Glance"_qs;
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Quick overview of tabs.");
	}

	QIcon Plugin::GetIcon () const
	{
		return GetProxyHolder ()->GetIconThemeManager ()->GetPluginIcon ();
	}

	void Plugin::ShowGlance ()
	{
		ActionGlance_->setEnabled (false);

		const auto rootWM = GetProxyHolder ()->GetRootWindowsManager ();

		const auto glance = new GlanceView { *rootWM->GetTabWidget (rootWM->GetPreferredWindowIndex ()) };
		connect (glance,
				&QObject::destroyed,
				ActionGlance_,
				[this] { ActionGlance_->setEnabled (true); });
	}

	QList<QAction*> Plugin::GetActions (ActionsEmbedPlace aep) const
	{
		QList<QAction*> result;
		if (aep == ActionsEmbedPlace::QuickLaunch)
			result << ActionGlance_;
		return result;
	}

	QMap<QByteArray, ActionInfo> Plugin::GetActionInfo () const
	{
		const auto& iconName = ActionGlance_->property ("ActionIcon").toString ();
		ActionInfo info
		{
			ActionGlance_->text (),
			ActionGlance_->shortcut (),
			GetProxyHolder ()->GetIconThemeManager ()->GetIcon (iconName)
		};
		return { { "ShowList", info } };
	}

	void Plugin::SetShortcut (const QByteArray&, const QKeySequences_t& seqs)
	{
		ActionGlance_->setShortcuts (seqs);
	}
}

LC_EXPORT_PLUGIN (leechcraft_glance, LC::Glance::Plugin);
