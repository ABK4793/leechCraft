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

#include "dockmanager.h"
#include <QDockWidget>
#include <QToolButton>
#include <QToolBar>
#include <QMenu>
#include <util/xpc/defaulthookproxy.h>
#include <util/sll/qtutil.h>
#include <util/sll/delayedexecutor.h>
#include <interfaces/ihavetabs.h>
#include "tabmanager.h"
#include "core.h"
#include "rootwindowsmanager.h"
#include "mainwindow.h"
#include "docktoolbarmanager.h"
#include "mainwindowmenumanager.h"

namespace LeechCraft
{
	DockManager::DockManager (RootWindowsManager *rootWM, QObject *parent)
	: QObject (parent)
	, RootWM_ (rootWM)
	{
		for (int i = 0; i < RootWM_->GetWindowsCount (); ++i)
			handleWindow (i);

		connect (RootWM_,
				SIGNAL (windowAdded (int)),
				this,
				SLOT (handleWindow (int)));
	}

	void DockManager::AddDockWidget (QDockWidget *dw, Qt::DockWidgetArea area)
	{
		auto win = static_cast<MainWindow*> (RootWM_->GetPreferredWindow ());
		win->addDockWidget (area, dw);
		win->resizeDocks ({ dw }, { 1 }, Qt::Horizontal);		// https://bugreports.qt.io/browse/QTBUG-65592
		Dock2Info_ [dw].Window_ = win;

		connect (dw,
				SIGNAL (destroyed (QObject*)),
				this,
				SLOT (handleDockDestroyed ()));

		Window2DockToolbarMgr_ [win]->AddDock (dw, area);

		dw->installEventFilter (this);

		auto toggleAct = dw->toggleViewAction ();
		ToggleAct2Dock_ [toggleAct] = dw;
		connect (toggleAct,
				SIGNAL (triggered (bool)),
				this,
				SLOT (handleDockToggled (bool)));
	}

	void DockManager::AssociateDockWidget (QDockWidget *dock, QWidget *tab)
	{
		Dock2Info_ [dock].Associated_ = tab;

		auto rootWM = Core::Instance ().GetRootWindowsManager ();
		const auto winIdx = rootWM->GetWindowForTab (qobject_cast<ITabWidget*> (tab));
		if (winIdx >= 0)
			handleTabChanged (rootWM->GetTabManager (winIdx)->GetCurrentWidget ());
		else
			dock->setVisible (false);
	}

	void DockManager::ToggleViewActionVisiblity (QDockWidget *widget, bool visible)
	{
		auto win = Dock2Info_ [widget].Window_;

		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		emit hookDockWidgetActionVisToggled (proxy, win, widget, visible);
		if (proxy->IsCancelled ())
			return;

		const auto toggleView = widget->toggleViewAction ();
		const auto menu = win->GetMenuManager ()->GetSubMenu (MainWindowMenuManager::Role::View);
		if (visible)
			menu->addAction (toggleView);
		else
			menu->removeAction (toggleView);
	}

	void DockManager::SetDockWidgetVisibility (QDockWidget *dw, bool visible)
	{
		dw->setVisible (visible);
		HandleDockToggled (dw, visible);
	}

	QSet<QDockWidget*> DockManager::GetWindowDocks (const MainWindow *window) const
	{
		QSet<QDockWidget*> result;
		for (auto i = Dock2Info_.begin (); i != Dock2Info_.end (); ++i)
			if (i->Window_ == window)
				result << i.key ();
		return result;
	}

	void DockManager::MoveDock (QDockWidget *dw, MainWindow *fromWin, MainWindow *toWin)
	{
		Dock2Info_ [dw].Window_ = toWin;

		const auto area = fromWin->dockWidgetArea (dw);

		fromWin->removeDockWidget (dw);
		Window2DockToolbarMgr_ [fromWin]->RemoveDock (dw);
		toWin->addDockWidget (area, dw);
		Window2DockToolbarMgr_ [toWin]->AddDock (dw, area);
	}

	QSet<QDockWidget*> DockManager::GetForcefullyClosed () const
	{
		return ForcefullyClosed_;
	}

	bool DockManager::eventFilter (QObject *obj, QEvent *event)
	{
		auto dock = qobject_cast<QDockWidget*> (obj);
		if (!dock)
			return false;

		switch (event->type ())
		{
		case QEvent::Close:
			ForcefullyClosed_ << dock;
			break;
		case QEvent::Hide:
			Dock2Info_ [dock].Width_ = dock->width ();
			break;
		case QEvent::Show:
		{
			const auto width = Dock2Info_ [dock].Width_;
			if (width > 0)
			{
				const auto prevMin = dock->minimumWidth ();
				const auto prevMax = dock->maximumWidth ();

				dock->setMinimumWidth (width);
				dock->setMaximumWidth (width);

				Util::ExecuteLater ([dock = QPointer<QDockWidget> { dock }, prevMin, prevMax]
						{
							if (!dock)
								return;

							dock->setMinimumWidth (prevMin);
							dock->setMaximumWidth (prevMax);
						});
			}
			break;
		}
		default:
			break;
		}

		return false;
	}

	void DockManager::HandleDockToggled (QDockWidget *dock, bool isVisible)
	{
		if (isVisible)
		{
			if (ForcefullyClosed_.remove (dock))
			{
				auto win = Dock2Info_ [dock].Window_;
				Window2DockToolbarMgr_ [win]->AddDock (dock, win->dockWidgetArea (dock));
			}
		}
		else
			ForcefullyClosed_ << dock;
	}

	void DockManager::handleTabMove (int from, int to, int tab)
	{
		auto rootWM = Core::Instance ().GetRootWindowsManager ();

		auto fromWin = rootWM->GetMainWindow (from);
		auto toWin = rootWM->GetMainWindow (to);
		auto widget = fromWin->GetTabWidget ()->Widget (tab);

		for (auto i = Dock2Info_.begin (), end = Dock2Info_.end (); i != end; ++i)
			if (i->Associated_ == widget)
				MoveDock (i.key (), fromWin, toWin);
	}

	void DockManager::handleDockDestroyed ()
	{
		auto dock = static_cast<QDockWidget*> (sender ());

		auto toggleAct = ToggleAct2Dock_.key (dock);
		Window2DockToolbarMgr_ [Dock2Info_ [dock].Window_]->HandleDockDestroyed (dock, toggleAct);

		Dock2Info_.remove (dock);
		ToggleAct2Dock_.remove (toggleAct);
		ForcefullyClosed_.remove (dock);
	}

	void DockManager::handleDockToggled (bool isVisible)
	{
		auto dock = ToggleAct2Dock_ [static_cast<QAction*> (sender ())];
		if (!dock)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown toggler"
					<< sender ();
			return;
		}

		HandleDockToggled (dock, isVisible);
	}

	void DockManager::handleTabChanged (QWidget *tabWidget)
	{
		auto thisWindowIdx = RootWM_->GetWindowForTab (qobject_cast<ITabWidget*> (tabWidget));
		auto thisWindow = RootWM_->GetMainWindow (thisWindowIdx);
		auto toolbarMgr = Window2DockToolbarMgr_ [thisWindow];

		QList<QDockWidget*> toShowAssoc;
		QList<QDockWidget*> toShowUnassoc;
		for (const auto& pair : Util::Stlize (Dock2Info_))
		{
			const auto dock = pair.first;
			const auto& info = pair.second;
			const auto otherWindow = RootWM_->GetWindowIndex (info.Window_);
			if (otherWindow != thisWindowIdx)
				continue;

			const auto otherWidget = info.Associated_;
			if (otherWidget && otherWidget != tabWidget)
			{
				dock->setVisible (false);
				toolbarMgr->RemoveDock (dock);
			}
			else if (!ForcefullyClosed_.contains (dock))
			{
				if (otherWidget)
					toShowAssoc << dock;
				else
					toShowUnassoc << dock;
			}
		}

		for (auto dock : toShowUnassoc + toShowAssoc)
		{
			dock->setVisible (true);
			if (!dock->isFloating ())
				toolbarMgr->AddDock (dock, thisWindow->dockWidgetArea (dock));
		}
	}

	void DockManager::handleWindow (int index)
	{
		auto win = RootWM_->GetMainWindow (index);
		Window2DockToolbarMgr_ [win] = new DockToolbarManager (win, this);
		connect (win,
				&QObject::destroyed,
				this,
				[this, win] { Window2DockToolbarMgr_.remove (win); });
	}
}
