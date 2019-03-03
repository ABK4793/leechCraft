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

#include "executecommanddialog.h"
#include <QVBoxLayout>
#include <QAbstractButton>
#include <QLabel>
#include "glooxaccount.h"
#include "clientconnection.h"
#include "clientconnectionextensionsmanager.h"
#include "formbuilder.h"
#include "ui_commandslistpage.h"
#include "ui_commandresultpage.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	class WaitPage : public QWizardPage
	{
		bool DataFetched_;
	public:
		WaitPage (const QString& text, QWidget *parent = 0)
		: QWizardPage { parent }
		, DataFetched_ { false }
		{
			setTitle (tr ("Fetching data..."));
			setCommitPage (true);

			setLayout (new QVBoxLayout);
			layout ()->addWidget (new QLabel { text });
		}

		bool isComplete () const
		{
			return DataFetched_;
		}

		void SetDataFetched ()
		{
			DataFetched_ = true;
			emit completeChanged ();
		}
	};

	class CommandsListPage : public QWizardPage
	{
		Ui::CommandsListPage Ui_;
		QList<AdHocCommand> Commands_;
	public:
		CommandsListPage (const QList<AdHocCommand>& commands, QWidget *parent = 0)
		: QWizardPage { parent }
		, Commands_ { commands }
		{
			Ui_.setupUi (this);
			setCommitPage (true);

			for (const auto& cmd : commands)
				Ui_.CommandsBox_->addItem (cmd.GetName ());
		}

		AdHocCommand GetSelectedCommand () const
		{
			const int idx = Ui_.CommandsBox_->currentIndex ();
			return idx >= 0 ?
					Commands_.at (idx) :
					AdHocCommand { {}, {} };
		}
	};

	namespace
	{
		QString GetSeverityText (AdHocNote::Severity severity)
		{
			switch (severity)
			{
			case AdHocNote::Severity::Info:
				return QObject::tr ("Info:") + " ";
			case AdHocNote::Severity::Warn:
				return QObject::tr ("Warning:") + " ";
			case AdHocNote::Severity::Error:
				return QObject::tr ("Error:") + " ";
			}

			qWarning () << Q_FUNC_INFO
					<< "unknown severity level"
					<< static_cast<int> (severity);
			return {};
		}
	}

	class CommandResultPage : public QWizardPage
	{
		Ui::CommandResultPage Ui_;

		AdHocResult Result_;
		mutable FormBuilder FB_;
	public:
		CommandResultPage (const AdHocResult& result, GlooxAccount *acc, QWidget *parent = 0)
		: QWizardPage { parent }
		, Result_ { result }
		, FB_ { {}, acc->GetClientConnection ()->GetBobManager () }
		{
			Ui_.setupUi (this);
			setCommitPage (true);

			Ui_.Actions_->addItems (result.GetActions ());

			const auto& form = result.GetDataForm ();
			if (!form.isNull ())
				Ui_.FormArea_->setWidget (FB_.CreateForm (form));
			else
				Ui_.FormArea_->hide ();

			const auto& notes = result.GetNotes ();
			if (notes.isEmpty ())
				Ui_.NotesLabel_->hide ();
			else
			{
				QStringList strs;
				for (const auto& note : notes)
				{
					auto str = GetSeverityText (note.GetSeverity ()) + note.GetText ();
					str.replace ('\n', "<br/>");
					strs << str;
				}
				Ui_.NotesLabel_->setText (strs.join ("<br/><br/>"));
			}
		}

		QString GetSelectedAction () const
		{
			return Ui_.Actions_->currentText ();
		}

		QXmppDataForm GetForm () const
		{
			return FB_.GetForm ();
		}

		AdHocResult GetResult () const
		{
			return Result_;
		}
	};

	ExecuteCommandDialog::ExecuteCommandDialog (const QString& jid,
			GlooxAccount *account, QWidget *parent, Tag)
	: QWizard { parent }
	, Account_ { account }
	, Manager_ { account->GetClientConnection ()->GetExtensionsManager ().Get<AdHocCommandManager> () }
	, JID_ { jid }
	{
		Ui_.setupUi (this);
		setAttribute (Qt::WA_DeleteOnClose);
		connect (this,
				SIGNAL (currentIdChanged (int)),
				this,
				SLOT (handleCurrentChanged (int)));

		connect (&Manager_,
				SIGNAL (gotError (QString, QString)),
				this,
				SLOT (handleError (QString)));
	}

	ExecuteCommandDialog::ExecuteCommandDialog (const QString& jid,
			GlooxAccount *account, QWidget *parent)
	: ExecuteCommandDialog { jid, account, parent, Tag {} }
	{
		RequestCommands ();

		setButtonText (QWizard::CustomButton1, tr ("Execute another command"));
		setOption (QWizard::HaveCustomButton1);

		connect (button (QWizard::CustomButton1),
				SIGNAL (released ()),
				this,
				SLOT (recreate ()));
	}

	ExecuteCommandDialog::ExecuteCommandDialog (const QString& jid,
			const QString& command,
			GlooxAccount *account, QWidget *parent)
	: ExecuteCommandDialog { jid, account, parent, Tag {} }
	{
		const int idx = addPage (new WaitPage { tr ("Please wait while "
				"the selected command is executed.") });
		if (currentId () != idx)
			next ();

		ExecuteCommand ({ {}, command });
	}

	void ExecuteCommandDialog::RequestCommands ()
	{
		const int idx = addPage (new WaitPage (tr ("Please wait while "
				"the list of commands is fetched.")));
		if (currentId () != idx)
			next ();

		connect (&Manager_,
				SIGNAL (gotCommands (QString, QList<AdHocCommand>)),
				this,
				SLOT (handleGotCommands (QString, QList<AdHocCommand>)),
				Qt::UniqueConnection);
		Manager_.QueryCommands (JID_);
	}

	void ExecuteCommandDialog::ExecuteCommand (const AdHocCommand& command)
	{
		connect (&Manager_,
				SIGNAL (gotResult (QString, AdHocResult)),
				this,
				SLOT (handleGotResult (QString, AdHocResult)),
				Qt::UniqueConnection);
		Manager_.ExecuteCommand (JID_, command);
	}

	void ExecuteCommandDialog::ProceedExecuting (const AdHocResult& result, const QString& action)
	{
		connect (&Manager_,
				SIGNAL (gotResult (QString, AdHocResult)),
				this,
				SLOT (handleGotResult (QString, AdHocResult)),
				Qt::UniqueConnection);
		Manager_.ProceedExecuting (JID_, result, action);
	}

	void ExecuteCommandDialog::handleCurrentChanged (int id)
	{
		if (!dynamic_cast<WaitPage*> (currentPage ()))
			return;

		const auto& ids = pageIds ();

		const int pos = ids.indexOf (id);
		if (pos <= 0)
			return;

		const auto prevPage = page (ids.at (pos - 1));
		if (const auto listPage = dynamic_cast<CommandsListPage*> (prevPage))
		{
			const AdHocCommand& cmd = listPage->GetSelectedCommand ();
			if (cmd.GetName ().isEmpty ())
				deleteLater ();
			else
				ExecuteCommand (cmd);
		}
		else if (const auto crp = dynamic_cast<CommandResultPage*> (prevPage))
		{
			const auto& action = crp->GetSelectedAction ();
			if (action.isEmpty ())
				return;

			auto result = crp->GetResult ();
			result.SetDataForm (crp->GetForm ());
			ProceedExecuting (result, action);
		}
	}

	void ExecuteCommandDialog::handleGotCommands (const QString& jid, const QList<AdHocCommand>& commands)
	{
		if (jid != JID_)
			return;

		disconnect (&Manager_,
				SIGNAL (gotCommands (QString, QList<AdHocCommand>)),
				this,
				SLOT (handleGotCommands (QString, QList<AdHocCommand>)));

		addPage (new CommandsListPage { commands });
		addPage (new WaitPage { tr ("Please wait while command result "
					"is fetched.") });
		next ();
	}

	void ExecuteCommandDialog::handleGotResult (const QString& jid, const AdHocResult& result)
	{
		if (jid != JID_)
			return;

		disconnect (&Manager_,
				SIGNAL (gotResult (QString, AdHocResult)),
				this,
				SLOT (handleGotResult (QString, AdHocResult)));

		addPage (new CommandResultPage { result, Account_ });
		if (!result.GetActions ().isEmpty ())
			addPage (new WaitPage { tr ("Please wait while action "
						"is performed") });
		next ();
	}

	void ExecuteCommandDialog::handleError (const QString& errStr)
	{
		AdHocResult result;
		result.AddNote ({ AdHocNote::Severity::Error, errStr });
		addPage (new CommandResultPage { result, Account_ });
		next ();
	}

	void ExecuteCommandDialog::recreate ()
	{
		deleteLater ();

		const auto dia = new ExecuteCommandDialog { JID_, Account_, parentWidget () };
		dia->show ();
	}
}
}
}
