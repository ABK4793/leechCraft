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

#ifndef INTERFACES_IMWPROXY_H
#define INTERFACES_IMWPROXY_H
#include <Qt>

class QDockWidget;
class QToolBar;
class QWidget;
class QKeySequence;
class QMenu;

/** @brief This interface is used for manipulating the main window.
 *
 * All the interaction with LeechCraft main window should be done
 * through this interface.
 */
class Q_DECL_EXPORT IMWProxy
{
public:
	enum WidgetArea
	{
		WALeft,
		WARight,
		WABottom
	};

	virtual ~IMWProxy () {}

	struct DockWidgetParams
	{
		Qt::DockWidgetArea Area_ = Qt::NoDockWidgetArea;
	};

	/** @brief Adds the given dock widget to the main window
	 *
	 * The action for toggling the visibility of this dock widget is
	 * also added to the corresponding menus by default. The
	 * ToggleViewActionVisiblity() method could be used to change that.
	 *
	 * @param[in] widget The dock widget to add.
	 * @param[in] params The parameters of the newly added dock widget.
	 *
	 * @sa AssociateDockWidget(), ToggleViewActionVisiblity()
	 */
	virtual void AddDockWidget (QDockWidget *widget, const DockWidgetParams& params) = 0;

	/** @brief Connects the given dock widget with the given tab.
	 *
	 * This function associates the given dock widget with the given tab
	 * widget so that the dock widget is only visible when the tab is
	 * current tab.
	 *
	 * A dock widget may be associated with only one tab widget. Calling
	 * this function repeatedly will override older associations.
	 *
	 * @param[in] dock The dock widget to associate.
	 * @param[in] tab The tab widget for which the dock widget should be
	 * active.
	 *
	 * @sa AddDockWidget()
	 */
	virtual void AssociateDockWidget (QDockWidget *dock, QWidget *tab) = 0;

	/** @brief Sets the visibility of the previously added dock widget.
	 *
	 * This function sets the visibility of the given \em dock (which
	 * should be a dock widget previously added via AddDockWidget()).
	 * If \em dock has been associated with a tab via
	 * AssociateDockWidget(), calling this function makes sure that the
	 * visibility of the dock \em dock will be equal to \em visible.
	 *
	 * @note This function should be called after AssociateDockWidget()
	 * if the later is called at all.
	 *
	 * @param[in] dock The dock widget previously added via
	 * AddDockWidget().
	 * @param[in] visible The visibility of the dock widget.
	 *
	 * @sa AddDockWidget(), AssociateDockWidget()
	 */
	virtual void SetDockWidgetVisibility (QDockWidget *dock, bool visible) = 0;

	/** @brief Toggles the visibility of the toggle view action.
	 *
	 * By default all newly added dock widgets have their toggle view
	 * actions shown.
	 *
	 * @param[in] widget The widget for which to update the toggle
	 * action visibility.
	 * @param[in] visible Whether the corresponding action should be
	 * visible.
	 */
	virtual void ToggleViewActionVisiblity (QDockWidget *widget, bool visible) = 0;

	/** @brief Sets the visibility action shortcut of the given widget.
	 *
	 * @param[in] widget The widget for which the visibility action
	 * shortcut.
	 * @param[in] seq The widget's visibility action shortcut sequence.
	 */
	virtual void SetViewActionShortcut (QDockWidget *widget, const QKeySequence& seq) = 0;

	/** @brief Toggles the visibility of the main window.
	 */
	virtual void ToggleVisibility () = 0;

	/** @brief Show/raise main window
	 */
	virtual void ShowMain () = 0;

	/** @brief Returns the main LeechCraft menu.
	 *
	 * @return The main LeechCraft menu.
	 *
	 * @sa HideMainMenu()
	 */
	virtual QMenu* GetMainMenu () = 0;

	/** @brief Hides the main LeechCraft menu.
	 *
	 * Calling this function hides the main LeechCraft menu in the
	 * tabbar. There is no way of showing it back again after that. The
	 * menu is still accessable via GetMainMenu() and can be shown via
	 * other means.
	 *
	 * @sa GetMainMenu().
	 */
	virtual void HideMainMenu () = 0;
};

Q_DECLARE_INTERFACE (IMWProxy, "org.Deviant.LeechCraft.IMWProxy/1.0")

#endif
