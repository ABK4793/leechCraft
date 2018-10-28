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

#include "accountconfigdialog.h"
#include <numeric>
#include <QMenu>

namespace LeechCraft
{
namespace Snails
{
	AccountConfigDialog::AccountConfigDialog (QWidget *parent)
	: QDialog (parent)
	{
		Ui_.setupUi (this);
		Ui_.BrowseToSync_->setMenu (new QMenu (tr ("Folders to sync")));
		Ui_.OutgoingFolder_->addItem (QString ());

		connect (Ui_.InSecurityType_,
				SIGNAL (currentIndexChanged (int)),
				this,
				SLOT (resetInPort ()));
	}

	AccountConfig AccountConfigDialog::GetConfig () const
	{
		return
		{
			GetName (),
			GetUserName (),
			GetUserEmail (),
			GetLogin (),

			GetUseSASL (),
			GetSASLRequired (),
			GetInSecurity (),
			GetInSecurityRequired (),
			GetOutSecurity (),
			GetOutSecurityRequired (),

			GetSMTPAuth (),
			GetInHost (),
			GetInPort (),
			GetOutHost (),
			GetOutPort (),
			GetOutLogin (),
			GetOutType (),

			GetKeepAliveInterval (),
			GetLogConnectionsToFile (),

			GetDeleteBehaviour ()
		};
	}

	void AccountConfigDialog::SetConfig (const AccountConfig& cfg)
	{
		SetName (cfg.AccName_);
		SetUserName (cfg.UserName_);
		SetUserEmail (cfg.UserEmail_);
		SetLogin (cfg.Login_);

		SetUseSASL (cfg.UseSASL_);
		SetSASLRequired (cfg.SASLRequired_);
		SetInSecurity (cfg.InSecurity_);
		SetInSecurityRequired (cfg.InSecurityRequired_);
		SetOutSecurity (cfg.OutSecurity_);
		SetOutSecurityRequired (cfg.OutSecurityRequired_);

		SetSMTPAuth (cfg.SMTPNeedsAuth_);
		SetInHost (cfg.InHost_);
		SetInPort (cfg.InPort_);
		SetOutHost (cfg.OutHost_);
		SetOutPort (cfg.OutPort_);
		SetOutLogin (cfg.OutLogin_);
		SetOutType (cfg.OutType_);

		SetKeepAliveInterval (cfg.KeepAliveInterval_);
		SetLogConnectionsToFile (cfg.LogToFile_);

		SetDeleteBehaviour (cfg.DeleteBehaviour_);
	}

	QString AccountConfigDialog::GetName () const
	{
		return Ui_.AccName_->text ();
	}

	void AccountConfigDialog::SetName (const QString& name)
	{
		Ui_.AccName_->setText (name);
	}

	QString AccountConfigDialog::GetUserName () const
	{
		return Ui_.UserName_->text ();
	}

	void AccountConfigDialog::SetUserName (const QString& name)
	{
		Ui_.UserName_->setText (name);
	}

	QString AccountConfigDialog::GetUserEmail () const
	{
		return Ui_.UserEmail_->text ();
	}

	void AccountConfigDialog::SetUserEmail (const QString& email)
	{
		Ui_.UserEmail_->setText (email);
	}

	QString AccountConfigDialog::GetLogin () const
	{
		return Ui_.InLogin_->text ();
	}

	void AccountConfigDialog::SetLogin (const QString& login)
	{
		Ui_.InLogin_->setText (login);
	}

	QString AccountConfigDialog::GetInHost () const
	{
		return Ui_.InHost_->text ();
	}

	void AccountConfigDialog::SetInHost (const QString& host)
	{
		Ui_.InHost_->setText (host);
	}

	int AccountConfigDialog::GetInPort () const
	{
		return Ui_.InPort_->value ();
	}

	void AccountConfigDialog::SetInPort (int port)
	{
		Ui_.InPort_->setValue (port);
	}

	AccountConfig::OutType AccountConfigDialog::GetOutType () const
	{
		return static_cast<AccountConfig::OutType> (Ui_.OutType_->currentIndex ());
	}

	void AccountConfigDialog::SetOutType (AccountConfig::OutType type)
	{
		Ui_.OutType_->setCurrentIndex (static_cast<int> (type));
	}

	QString AccountConfigDialog::GetOutHost () const
	{
		return Ui_.OutAddress_->text ();
	}

	void AccountConfigDialog::SetOutHost (const QString& host)
	{
		Ui_.OutAddress_->setText (host);
	}

	int AccountConfigDialog::GetOutPort () const
	{
		return Ui_.OutPort_->value ();
	}

	void AccountConfigDialog::SetOutPort (int port)
	{
		Ui_.OutPort_->setValue (port);
	}

	QString AccountConfigDialog::GetOutLogin () const
	{
		return Ui_.CustomOut_->isChecked () ?
				Ui_.OutLogin_->text () :
				QString ();
	}

	void AccountConfigDialog::SetOutLogin (const QString& login)
	{
		Ui_.CustomOut_->setChecked (!login.isEmpty ());
		Ui_.OutLogin_->setText (login);
	}

	bool AccountConfigDialog::GetUseSASL () const
	{
		return Ui_.UseSASL_->checkState () == Qt::Checked;
	}

	void AccountConfigDialog::SetUseSASL (bool use)
	{
		Ui_.UseSASL_->setCheckState (use ? Qt::Checked : Qt::Unchecked);
	}

	bool AccountConfigDialog::GetSASLRequired () const
	{
		return Ui_.SASLRequired_->checkState () == Qt::Checked;
	}

	void AccountConfigDialog::SetSASLRequired (bool req)
	{
		Ui_.SASLRequired_->setCheckState (req ? Qt::Checked : Qt::Unchecked);
	}

	AccountConfig::SecurityType AccountConfigDialog::GetInSecurity () const
	{
		return static_cast<AccountConfig::SecurityType> (Ui_.InSecurityType_->currentIndex ());
	}

	void AccountConfigDialog::SetInSecurity (AccountConfig::SecurityType type)
	{
		Ui_.InSecurityType_->setCurrentIndex (static_cast<int> (type));
	}

	bool AccountConfigDialog::GetInSecurityRequired () const
	{
		return Ui_.InSecurityRequired_->checkState () == Qt::Checked;
	}

	void AccountConfigDialog::SetInSecurityRequired (bool req)
	{
		Ui_.InSecurityRequired_->setCheckState (req ? Qt::Checked : Qt::Unchecked);
	}

	AccountConfig::SecurityType AccountConfigDialog::GetOutSecurity () const
	{
		return static_cast<AccountConfig::SecurityType> (Ui_.OutSecurityType_->currentIndex ());
	}

	void AccountConfigDialog::SetOutSecurity (AccountConfig::SecurityType type)
	{
		Ui_.OutSecurityType_->setCurrentIndex (static_cast<int> (type));
	}

	bool AccountConfigDialog::GetOutSecurityRequired () const
	{
		return Ui_.OutSecurityRequired_->checkState () == Qt::Checked;
	}

	void AccountConfigDialog::SetOutSecurityRequired (bool req)
	{
		Ui_.OutSecurityRequired_->setCheckState (req ? Qt::Checked : Qt::Unchecked);
	}

	bool AccountConfigDialog::GetSMTPAuth () const
	{
		return Ui_.SMTPAuthRequired_->checkState () == Qt::Checked;
	}

	void AccountConfigDialog::SetSMTPAuth (bool smtp)
	{
		Ui_.SMTPAuthRequired_->setCheckState (smtp ? Qt::Checked : Qt::Unchecked);
	}

	void AccountConfigDialog::SetAllFolders (const QList<QStringList>& folders)
	{
		for (const auto& f : folders)
		{
			const auto& name = f.join ("/");
			Ui_.OutgoingFolder_->addItem (name, f);

			auto act = Ui_.BrowseToSync_->menu ()->addAction (name);
			act->setCheckable (true);
			act->setData (f);

			connect (act,
					SIGNAL (toggled (bool)),
					this,
					SLOT (rebuildFoldersToSyncLine ()));
		}
	}

	QList<QStringList> AccountConfigDialog::GetFoldersToSync () const
	{
		QList<QStringList> result;
		for (const auto& action : Ui_.BrowseToSync_->menu ()->actions ())
			if (action->isChecked ())
				result << action->data ().toStringList ();

		return result;
	}

	void AccountConfigDialog::SetFoldersToSync (const QList<QStringList>& folders)
	{
		for (const auto& action : Ui_.BrowseToSync_->menu ()->actions ())
		{
			const auto& folder = action->data ().toStringList ();
			action->setChecked (folders.contains (folder));
		}

		rebuildFoldersToSyncLine ();
	}

	QStringList AccountConfigDialog::GetOutFolder () const
	{
		return Ui_.OutgoingFolder_->itemData (Ui_.OutgoingFolder_->currentIndex ()).toStringList ();
	}

	void AccountConfigDialog::SetOutFolder (const QStringList& folder)
	{
		const int idx = Ui_.OutgoingFolder_->findData (folder);
		if (idx == -1)
			return;

		Ui_.OutgoingFolder_->setCurrentIndex (-1);
	}

	int AccountConfigDialog::GetKeepAliveInterval () const
	{
		return Ui_.KeepAliveInterval_->value () * 1000;
	}

	void AccountConfigDialog::SetKeepAliveInterval (int interval)
	{
		Ui_.KeepAliveInterval_->setValue (interval / 1000);
	}

	bool AccountConfigDialog::GetLogConnectionsToFile () const
	{
		return Ui_.LogConnectionsToFile_->checkState () == Qt::Checked;
	}

	void AccountConfigDialog::SetLogConnectionsToFile (bool log)
	{
		Ui_.LogConnectionsToFile_->setCheckState (log ? Qt::Checked : Qt::Unchecked);
	}

	void AccountConfigDialog::resetInPort ()
	{
		const QList<int> values { 465, 993, 143 };

		const int pos = Ui_.InSecurityType_->currentIndex ();
		Ui_.InPort_->setValue (values.value (pos));
	}

	void AccountConfigDialog::rebuildFoldersToSyncLine ()
	{
		const auto& sync = GetFoldersToSync ();
		const auto& folders = std::accumulate (sync.begin (), sync.end (), QStringList (),
				[] (QStringList fs, const QStringList& f) { return fs << f.join ("/"); });
		Ui_.FoldersToSync_->setText (folders.join ("; "));
	}

	AccountConfig::DeleteBehaviour AccountConfigDialog::GetDeleteBehaviour () const
	{
		return static_cast<AccountConfig::DeleteBehaviour> (Ui_.DeletionBehaviour_->currentIndex ());
	}

	void AccountConfigDialog::SetDeleteBehaviour (AccountConfig::DeleteBehaviour behaviour)
	{
		Ui_.DeletionBehaviour_->setCurrentIndex (static_cast<int> (behaviour));
	}
}
}
