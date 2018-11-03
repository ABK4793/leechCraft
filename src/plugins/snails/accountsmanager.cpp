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

#include "accountsmanager.h"
#include <QStandardItemModel>
#include <QSettings>
#include <QCoreApplication>
#include <QtDebug>
#include <util/sll/prelude.h>
#include "accountfoldermanager.h"
#include "accountconfigdialog.h"
#include "accountaddwizard.h"

namespace LeechCraft
{
namespace Snails
{
	AccountsManager::AccountsManager (ProgressManager *pm, Storage *st, QObject *parent)
	: QObject { parent }
	, AccountsModel_ { new QStandardItemModel { this } }
	, ProgressMgr_ { pm }
	, Storage_ { st }
	{
		AccountsModel_->setHorizontalHeaderLabels ({ tr ("Name"), tr ("Server") });
	}

	QAbstractItemModel* AccountsManager::GetAccountsModel () const
	{
		return AccountsModel_;
	}

	QList<Account_ptr> AccountsManager::GetAccounts () const
	{
		return Accounts_;
	}

	Account_ptr AccountsManager::GetAccount (const QModelIndex& index) const
	{
		if (!index.isValid ())
			return Account_ptr ();

		return Accounts_ [index.row ()];
	}

	void AccountsManager::InitiateAccountAddition ()
	{
		auto wiz = new AccountAddWizard;
		wiz->setAttribute (Qt::WA_DeleteOnClose);
		wiz->show ();

		connect (wiz,
				&QDialog::accepted,
				this,
				[this, wiz]
				{
					auto dia = new AccountConfigDialog;
					dia->setAttribute (Qt::WA_DeleteOnClose);
					dia->SetConfig (wiz->GetConfig ());
					dia->show ();

					connect (dia,
							&QDialog::accepted,
							this,
							[this, dia]
							{
								AddAccountImpl (std::make_shared<Account> (dia->GetConfig (),
										Account::Dependencies { Storage_, ProgressMgr_ }));
								SaveAccounts ();
							});
				});
	}

	void AccountsManager::InitWithPlugins ()
	{
		LoadAccounts ();
	}

	void AccountsManager::AddAccountImpl (Account_ptr account)
	{
		Accounts_ << account;

		const auto& config = account->GetConfig ();
		const QList<QStandardItem*> row
		{
			new QStandardItem { config.AccName_ },
			new QStandardItem { config.InHost_ + ':' + QString::number (config.InPort_) }
		};
		AccountsModel_->appendRow (row);

		connect (account.get (),
				&Account::accountChanged,
				this,
				&AccountsManager::SaveAccounts);
		connect (account->GetFolderManager (),
				&AccountFolderManager::foldersUpdated,
				this,
				&AccountsManager::SaveAccounts);
	}

	void AccountsManager::LoadAccounts ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Snails_Accounts");
		for (const auto& var : settings.value ("Accounts").toList ())
		{
			try
			{
				const auto acc = Account::Deserialize (var.toByteArray (), { Storage_, ProgressMgr_ });
				AddAccountImpl (acc);
			}
			catch (const std::exception& e)
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to deserialize account, sorry :("
						<< e.what ();
			}
		}
	}

	void AccountsManager::SaveAccounts () const
	{
		const auto& serialized = Util::Map (Accounts_,
				[] (const Account_ptr& acc) -> QVariant { return acc->Serialize (); });

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Snails_Accounts");
		settings.setValue ("Accounts", serialized);
	}
}
}
