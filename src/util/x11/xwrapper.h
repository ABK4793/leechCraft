/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <string>
#include <QList>
#include <QString>
#include <QHash>
#include <QObject>
#include <QAbstractNativeEventFilter>
#include <X11/Xdefs.h>
#include "x11config.h"
#include "winflags.h"

class QIcon;
class QWidget;
class QRect;
class QScreen;

using Display = struct _XDisplay;
using Window = unsigned long;

using XEvent = union _XEvent;

struct xcb_connection_t;

namespace LC::Util
{
	class UTIL_X11_API XWrapper : public QObject
								, public QAbstractNativeEventFilter
	{
		Q_OBJECT

		xcb_connection_t *Conn_ = nullptr;
		Display *Display_ = nullptr;
		Window AppWin_ {};

		QHash<QByteArray, Atom> Atoms_;

		XWrapper ();
	public:
		enum class Layer
		{
			Top,
			Bottom,
			Normal
		};

		static XWrapper& Instance ();

		xcb_connection_t* GetConnection () const;
		Display* GetDisplay () const;
		Window GetRootWindow () const;

		bool nativeEventFilter (const QByteArray& eventType, void *message, qintptr *result) override;

		void Sync ();

		QList<Window> GetWindows ();
		QString GetWindowTitle (Window);
		QIcon GetWindowIcon (Window);
		WinStateFlags GetWindowState (Window);
		AllowedActionFlags GetWindowActions (Window);

		Window GetActiveApp ();

		bool IsLCWindow (Window);
		bool ShouldShow (Window);

		void Subscribe (Window);

		void SetStrut (QWidget*, Qt::ToolBarArea);
		void ClearStrut (QWidget*);
		void SetStrut (Window wid,
				ulong left, ulong right, ulong top, ulong bottom,
				ulong leftStartY, ulong leftEndY,
				ulong rightStartY, ulong rightEndY,
				ulong topStartX, ulong topEndX,
				ulong bottomStartX, ulong bottomEndX);

		void RaiseWindow (Window);
		void MinimizeWindow (Window);
		void MaximizeWindow (Window);
		void UnmaximizeWindow (Window);
		void ShadeWindow (Window);
		void UnshadeWindow (Window);
		void MoveWindowTo (Window, Layer);
		void CloseWindow (Window);

		void ResizeWindow (Window, int, int);

		int GetDesktopCount ();
		int GetCurrentDesktop ();
		void SetCurrentDesktop (int);
		QStringList GetDesktopNames ();
		QString GetDesktopName (int, const QString& = QString ());
		int GetWindowDesktop (Window);
		void MoveWindowToDesktop (Window, int);

		QRect GetAvailableGeometry (QScreen&);
		QRect GetAvailableGeometry (QWidget*);

		/** @brief Returns the atom denoting the given string.
		 *
		 * @param[in] string A view to a (null-terminated) string with the name of the atom.
		 * @return An X11 Atom.
		 *
		 * @note The string pointed by str should be null-terminated — that is, <code>std::strlen(str.data())</code>
		 * should be equal to <code>str.size()</code>.
		 */
		Atom GetAtom (std::string_view str);
	private:
		template<typename T>
		void HandlePropNotify (T);

		template<typename Flag>
		QFlags<Flag> GetFlagsList (Window wid, Atom property, const QHash<Atom, Flag>& atom2flag) const;

		Window GetActiveWindow ();

		bool GetWinProp (Window, Atom, ulong*, uchar**, Atom = static_cast<Atom> (0)) const;
		bool GetRootWinProp (Atom, ulong*, uchar**, Atom = static_cast<Atom> (0)) const;
		QList<Atom> GetWindowType (Window);

		bool SendMessage (Window, Atom, ulong, ulong = 0, ulong = 0, ulong = 0, ulong = 0);
	private slots:
		void initialize ();
	signals:
		void windowListChanged ();
		void activeWindowChanged ();
		void desktopChanged ();

		void windowNameChanged (ulong);
		void windowIconChanged (ulong);
		void windowDesktopChanged (ulong);
		void windowStateChanged (ulong);
		void windowActionsChanged (ulong);
	};
}
