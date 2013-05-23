/**********************************************************************
 *  LeechCraft - modular cross-platform feature rich internet client.
 *  Copyright (C) 2010-2012  Oleg Linkin
 *
 *  Boost Software License - Version 1.0 - August 17th, 2003
 *
 *  Permission is hereby granted, free of charge, to any person or organization
 *  obtaining a copy of the software and accompanying documentation covered by
 *  this license (the "Software") to use, reproduce, display, distribute,
 *  execute, and transmit the Software, and to prepare derivative works of the
 *  Software, and to permit third-parties to whom the Software is furnished to
 *  do so, all subject to the following:
 *
 *  The copyright notices in the Software and this entire statement, including
 *  the above license grant, this restriction and the following disclaimer,
 *  must be included in all copies of the Software, in whole or in part, and
 *  all derivative works of the Software, unless such copies or derivative
 *  works are solely in the form of machine-executable object code generated by
 *  a source language processor.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 *  SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 *  FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 *  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 **********************************************************************/

#include "remotedirectoryselectdialog.h"
#include <QMessageBox>
#include <QStandardItemModel>
#include <QtDebug>
#include "interfaces/netstoremanager/istorageaccount.h"
#include "accountsmanager.h"
#include "filesproxymodel.h"
#include "utils.h"

namespace LeechCraft
{
namespace NetStoreManager
{
	RemoteDirectorySelectDialog::RemoteDirectorySelectDialog (const QByteArray& accId,
			AccountsManager *am, QWidget *parent)
	: QDialog (parent)
	, AccountId_ (accId)
	, Model_ (new QStandardItemModel (this))
	, ProxyModel_ (new FilesProxyModel (this))
	, AM_ (am)
	{
		Ui_.setupUi (this);

		Model_->setHorizontalHeaderLabels ({ tr ("Directory") });
		ProxyModel_->setSourceModel (Model_);
		Ui_.DirectoriesView_->setModel (ProxyModel_);
		auto account = am->GetAccountFromUniqueID (accId);
		if (account)
			if (auto isfl = qobject_cast<ISupportFileListings*> (account->GetQObject ()))
			{
				connect (account->GetQObject (),
						SIGNAL (gotListing (QList<StorageItem>)),
						this,
						SLOT (handleGotListing (QList<StorageItem>)));
				isfl->RefreshListing ();
			}
	}

	QString RemoteDirectorySelectDialog::GetDirectoryPath () const
	{
		return QString ();
	}

	void RemoteDirectorySelectDialog::handleGotListing (const QList<StorageItem>& items)
	{
		disconnect (sender (),
				SIGNAL (gotListing (QList<StorageItem>)),
				0,
				0);

		QHash<QByteArray, StorageItem> id2Item;
		QHash<QByteArray, QStandardItem*> id2StandardItem;
		for (const auto& item : items)
		{
			if (!item.IsDirectory_ ||
					item.IsTrashed_)
				continue;

			id2Item [item.ID_] = item;
			QStandardItem *dir = new QStandardItem (AM_->GetProxy ()->
					GetIcon ("inode-directory"), item.Name_);
			dir->setEditable (false);
			id2StandardItem [item.ID_] = dir;
		}

		for (const auto& key : id2StandardItem.keys ())
		{
			if (!id2Item.contains (id2Item [key].ParentID_))
				Model_->appendRow (id2StandardItem [key]);
			else
				id2StandardItem [id2Item [key].ParentID_]->appendRow (id2StandardItem [key]);
		}
	}

}
}
