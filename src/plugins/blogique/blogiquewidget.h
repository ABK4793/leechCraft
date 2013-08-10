/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2012  Oleg Linkin
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

#include <QWidget>
#include <interfaces/ihavetabs.h>
#include <interfaces/ihaverecoverabletabs.h>
#include "interfaces/blogique/iaccount.h"
#include "ui_blogiquewidget.h"

class QStandardItem;
class QStandardItemModel;
class IEditorWidget;
class QToolBar;
class QComboBox;
class QProgressBar;

namespace LeechCraft
{
namespace Blogique
{
	class IBlogiqueSideWidget;
	class IAccount;
	class DraftEntriesWidget;
	class BlogEntriesWidget;
	class TagsProxyModel;

	class BlogiqueWidget : public QWidget
						, public ITabWidget
						, public IRecoverableTab
	{
		Q_OBJECT
		Q_INTERFACES (ITabWidget IRecoverableTab)

		enum BlogiqueSideWidgets
		{
			PostOptionsWidget = 2
		};

		static QObject *S_ParentMultiTabs_;

		Ui::BlogiqueWidget Ui_;

		IEditorWidget *PostEdit_;
		QWidget *PostEditWidget_;
		QToolBar *ToolBar_;
		QToolBar *ProgressToolBar_;
		QAction *AccountsBoxAction_;
		QComboBox *AccountsBox_;
		QComboBox *PostTargetBox_;
		QAction *PostTargetAction_;
		QAction *ProgressBarLabelAction_;
		QLabel *ProgressBarLabel_;
		QAction *ProgressBarAction_;
		QList<QAction*> InlineTagInserters_;

		DraftEntriesWidget *DraftEntriesWidget_;
		BlogEntriesWidget *BlogEntriesWidget_;
		QHash<int, IAccount*> Id2Account_;
		int PrevAccountId_;
		QList<QWidget*> SidePluginsWidgets_;

		EntryType EntryType_;
		qint64 EntryId_;
		QUrl EntryUrl_;

		bool EntryChanged_;

		TagsProxyModel *TagsProxyModel_;
		QStandardItemModel *TagsModel_;

	public:
		enum TagRoles
		{
			TagFrequency = Qt::UserRole + 1
		};

		BlogiqueWidget (QWidget *parent = 0);

		QObject* ParentMultiTabs ();
		TabClassInfo GetTabClassInfo () const;
		QToolBar* GetToolBar () const;
		void Remove ();

		static void SetParentMultiTabs (QObject *tab);

		QByteArray GetTabRecoverData () const;
		QString GetTabRecoverName () const;
		QIcon GetTabRecoverIcon () const;
		void FillWidget (const Entry& e, const QByteArray& accId = QByteArray ());
	private:
		void SetTextEditor ();
		void SetToolBarActions ();
		void SetDefaultSideWidgets ();
		void PrepareQmlWidgets ();
		void RemovePostingTargetsWidget ();

		void SetPostDate (const QDateTime& dt);
		QDateTime GetPostDate () const;

		void SetPostTags (const QStringList& tags);
		QStringList GetPostTags () const;

		void ClearEntry ();

		Entry GetCurrentEntry (bool interactive = false) const;

		void ShowProgress (const QString& labelText = QString ());

	public slots:
		void handleAutoSave ();
		void handleEntryPosted ();
		void handleEntryRemoved ();
		void handleRequestEntriesBegin ();
		void handleRequestEntriesEnd ();
		void handleTagsUpdated (const QHash<QString, int>& tags);
		void handleInsertTag (const QString& tag);
		void handleGotError (int errorCode, const QString& errorString,
				const QString& localizedErrorString);
		void handleAccountAdded (QObject *acc);
		void handleAccountRemoved (QObject *acc);

	private slots:
		void handleCurrentAccountChanged (int id);
		void fillCurrentTabWithEntry (const Entry& entry);
		void fillNewTabWithEntry (const Entry& entry, const QByteArray& accountId);

		void handleEntryChanged (const QString& str = QString ());
		void handleRemovingEntryBegin ();

		void newEntry ();
		void saveEntry (const Entry& e = Entry ());
		void saveNewEntry (const Entry& e = Entry ());
		void submit (const Entry& e = Entry ());
		void submitTo (const Entry& e = Entry ());
		void on_SideWidget__dockLocationChanged (Qt::DockWidgetArea area);
		void on_UpdateProfile__triggered ();
		void on_CurrentTime__released ();
		void on_SelectTags__toggled (bool checked);
		void handleTagTextChanged (const QString& text);
		void handleTagRemoved (const QString& tag);
		void handleTagAdded (const QString& tag);
		void on_OpenInBrowser__triggered ();
		void on_PreviewPost__triggered ();

	signals:
		void removeTab (QWidget *tab);
		void addNewTab (const QString& name, QWidget *tab);
		void changeTabName (QWidget *content, const QString& name);

		void tabRecoverDataChanged ();

		void tagSelected (const QString& tag);
	};
}
}
