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

#include "windowsmodel.h"
#include <QtDebug>
#include <QIcon>
#include <QFrame>
#include <util/qml/widthiconprovider.h>
#include "xwrapper.h"

namespace LeechCraft
{
namespace Krigstask
{
	class TaskbarImageProvider : public Util::WidthIconProvider
	{
		QHash<QString, QIcon> Icons_;
	public:
		void SetIcon (const QString& wid, const QIcon& icon)
		{
			Icons_ [wid] = icon;
		}

		void RemoveIcon (const QString& wid)
		{
			Icons_.remove (wid);
		}

		QIcon GetIcon (const QStringList& path)
		{
			const auto& wid = path.value (0);
			return Icons_.value (wid);
		}
	};

	WindowsModel::WindowsModel (QObject *parent)
	: QAbstractItemModel (parent)
	, ImageProvider_ (new TaskbarImageProvider)
	{
		auto& w = XWrapper::Instance ();
		auto windows = w.GetWindows ();
		for (auto wid : windows)
			AddWindow (wid, w);

		connect (&XWrapper::Instance (),
				SIGNAL (windowListChanged ()),
				this,
				SLOT (updateWinList ()));
		connect (&XWrapper::Instance (),
				SIGNAL (activeWindowChanged ()),
				this,
				SLOT (updateActiveWindow ()));

		connect (&XWrapper::Instance (),
				SIGNAL (windowNameChanged (ulong)),
				this,
				SLOT (updateWindowName (ulong)));
		connect (&XWrapper::Instance (),
				SIGNAL (windowIconChanged (ulong)),
				this,
				SLOT (updateWindowIcon (ulong)));
		connect (&XWrapper::Instance (),
				SIGNAL (windowStateChanged (ulong)),
				this,
				SLOT (updateWindowState (ulong)));
		connect (&XWrapper::Instance (),
				SIGNAL (windowActionsChanged (ulong)),
				this,
				SLOT (updateWindowActions (ulong)));

		QHash<int, QByteArray> roleNames;
		roleNames [Role::WindowName] = "windowName";
		roleNames [Role::WindowID] = "windowID";
		roleNames [Role::IconGenID] = "iconGenID";
		roleNames [Role::IsActiveWindow] = "isActiveWindow";
		roleNames [Role::IsMinimizedWindow] = "isMinimizedWindow";
		setRoleNames (roleNames);
	}

	QDeclarativeImageProvider* WindowsModel::GetImageProvider () const
	{
		return ImageProvider_;
	}

	int WindowsModel::columnCount (const QModelIndex&) const
	{
		return 1;
	}

	int WindowsModel::rowCount (const QModelIndex& parent) const
	{
		return parent.isValid () ? 0 : Windows_.size ();
	}

	QModelIndex WindowsModel::index (int row, int column, const QModelIndex& parent) const
	{
		return hasIndex (row, column, parent) ?
				createIndex (row, column) :
				QModelIndex {};
	}

	QModelIndex WindowsModel::parent (const QModelIndex&) const
	{
		return {};
	}

	QVariant WindowsModel::data (const QModelIndex& index, int role) const
	{
		const auto row = index.row ();
		const auto& item = Windows_.at (row);

		switch (role)
		{
		case Qt::DisplayRole:
		case Role::WindowName:
			return item.Title_;
		case Qt::DecorationRole:
			return item.Icon_;
		case Role::WindowID:
			return QString::number (item.WID_);
		case Role::IconGenID:
			return QString::number (item.IconGenID_);
		case Role::IsActiveWindow:
			return item.IsActive_;
		case Role::IsMinimizedWindow:
			return item.State_.testFlag (WinStateFlag::Hidden);
		}

		return {};
	}

	void WindowsModel::AddWindow (ulong wid, XWrapper& w)
	{
		if (!w.ShouldShow (wid))
			return;

		const auto& icon = w.GetWindowIcon (wid);
		Windows_.append ({
					wid,
					w.GetWindowTitle (wid),
					icon,
					0,
					w.GetActiveApp () == wid,
					w.GetWindowState (wid),
					w.GetWindowActions (wid)
				});
		ImageProvider_->SetIcon (QString::number (wid), icon);

		w.Subscribe (wid);
	}

	QList<WindowsModel::WinInfo>::iterator WindowsModel::FindWinInfo (Window wid)
	{
		return std::find_if (Windows_.begin (), Windows_.end (),
				[&wid] (const WinInfo& info) { return info.WID_ == wid; });
	}

	void WindowsModel::UpdateWinInfo (Window w, std::function<void (WinInfo&)> updater)
	{
		auto pos = FindWinInfo (w);
		if (pos == Windows_.end ())
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown window"
					<< w;
			return;
		}

		updater (*pos);

		const auto& idx = createIndex (std::distance (Windows_.begin (), pos), 0);
		emit dataChanged (idx, idx);
	}

	void WindowsModel::updateWinList ()
	{
		auto& w = XWrapper::Instance ();

		QSet<Window> known;
		for (const auto& info : Windows_)
			known << info.WID_;

		auto current = w.GetWindows ();
		current.erase (std::remove_if (current.begin (), current.end (),
					[this, &w] (Window wid) { return !w.ShouldShow (wid); }), current.end ());

		for (auto i = current.begin (); i != current.end (); )
		{
			if (known.remove (*i))
				i = current.erase (i);
			else
				++i;
		}

		for (auto wid : known)
		{
			const auto pos = std::find_if (Windows_.begin (), Windows_.end (),
					[&wid] (const WinInfo& info) { return info.WID_ == wid; });
			const auto dist = std::distance (Windows_.begin (), pos);
			beginRemoveRows ({}, dist, dist);
			Windows_.erase (pos);
			endRemoveRows ();

			ImageProvider_->RemoveIcon (QString::number (wid));
		}

		if (!current.isEmpty ())
		{
			beginInsertRows ({}, Windows_.size (), Windows_.size () + current.size () - 1);
			for (auto wid : current)
				AddWindow (wid, w);
			endInsertRows ();
		}
	}

	void WindowsModel::updateActiveWindow ()
	{
		const auto active = XWrapper::Instance ().GetActiveApp ();
		if (!active || !XWrapper::Instance ().ShouldShow (active))
			return;

		for (auto i = 0; i < Windows_.size (); ++i)
		{
			auto& info = Windows_ [i];
			if ((info.WID_ == active) == info.IsActive_)
				continue;

			info.IsActive_ = info.WID_ == active;
			emit dataChanged (createIndex (i, 0), createIndex (i, 0));
		}
	}

	void WindowsModel::updateWindowName (ulong w)
	{
		UpdateWinInfo (w,
				[&w] (WinInfo& info) { info.Title_ = XWrapper::Instance ().GetWindowTitle (w); });
	}

	void WindowsModel::updateWindowIcon (ulong w)
	{
		UpdateWinInfo (w,
				[&w] (WinInfo& info) -> void
				{
					info.Icon_ = XWrapper::Instance ().GetWindowIcon (w);
					++info.IconGenID_;
				});
	}

	void WindowsModel::updateWindowState (ulong w)
	{
		UpdateWinInfo (w,
				[&w] (WinInfo& info) -> void
				{
					info.State_ = XWrapper::Instance ().GetWindowState (w);
					if (info.State_.testFlag (WinStateFlag::Hidden))
						info.IsActive_ = false;
				});
	}

	void WindowsModel::updateWindowActions (ulong w)
	{
		UpdateWinInfo (w,
				[&w] (WinInfo& info) { info.Actions_ = XWrapper::Instance ().GetWindowActions (w); });
	}
}
}
