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

#include "composemessagetab.h"
#include <QToolBar>
#include <QWebFrame>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileIconProvider>
#include <QInputDialog>
#include <QTextDocument>
#include <QToolButton>
#include <util/util.h>
#include <util/sys/mimedetector.h>
#include <util/sll/qtutil.h>
#include <util/sll/visitor.h>
#include <util/xpc/util.h>
#include <interfaces/itexteditor.h>
#include <interfaces/core/iiconthememanager.h>
#include <interfaces/core/ientitymanager.h>
#include "message.h"
#include "core.h"
#include "xmlsettingsmanager.h"
#include "accountsmanager.h"
#include "accountthread.h"
#include "msgtemplatesmanager.h"
#include "structures.h"

namespace LeechCraft
{
namespace Snails
{
	QObject *ComposeMessageTab::S_ParentPlugin_ = 0;
	TabClassInfo ComposeMessageTab::S_TabClassInfo_;

	void ComposeMessageTab::SetParentPlugin (QObject *obj)
	{
		S_ParentPlugin_ = obj;
	}

	void ComposeMessageTab::SetTabClassInfo (const TabClassInfo& info)
	{
		S_TabClassInfo_ = info;
	}

	ComposeMessageTab::ComposeMessageTab (const AccountsManager *accsMgr,
			const MsgTemplatesManager *templatesMgr, QWidget *parent)
	: QWidget (parent)
	, AccsMgr_ (accsMgr)
	, TemplatesMgr_ (templatesMgr)
	, Toolbar_ (new QToolBar (tr ("Compose tab bar")))
	{
		Ui_.setupUi (this);

		SetupToolbar ();
		SetupEditors ();
	}

	TabClassInfo ComposeMessageTab::GetTabClassInfo () const
	{
		return S_TabClassInfo_;
	}

	QObject* ComposeMessageTab::ParentMultiTabs ()
	{
		return S_ParentPlugin_;
	}

	void ComposeMessageTab::Remove ()
	{
		emit removeTab (this);
		deleteLater ();
	}

	QToolBar* ComposeMessageTab::GetToolBar () const
	{
		return Toolbar_;
	}

	void ComposeMessageTab::SelectAccount (const Account_ptr& account)
	{
		const auto& var = QVariant::fromValue<Account_ptr> (account);
		for (auto action : AccountsMenu_->actions ())
			if (action->property ("Account") == var)
			{
				action->setChecked (true);
				break;
			}
	}

	void ComposeMessageTab::PrepareReply (const Message_ptr& msg)
	{
		auto address = msg->GetAddress (Message::Address::ReplyTo);
		if (address.second.isEmpty ())
			address = msg->GetAddress (Message::Address::From);
		Ui_.To_->setText (GetNiceMail (address));

		auto subj = msg->GetSubject ();
		if (subj.left (3).toLower () != "re:")
			subj.prepend ("Re: ");
		Ui_.Subject_->setText (subj);

		PrepareReplyBody (msg);

		ReplyMessage_ = msg;
	}

	void ComposeMessageTab::PrepareReplyEditor (const Message_ptr& msg)
	{
		const auto& replyOpt = XmlSettingsManager::Instance ()
				.property ("ReplyMessageFormat").toString ();
		if (replyOpt == "Plain")
			Ui_.Editor_->SelectEditor (ContentType::PlainText);
		else if (replyOpt == "HTML")
			Ui_.Editor_->SelectEditor (ContentType::HTML);
		else if (replyOpt == "Orig")
		{
			if (msg->GetHTMLBody ().isEmpty ())
				Ui_.Editor_->SelectEditor (ContentType::PlainText);
			else
				Ui_.Editor_->SelectEditor (ContentType::HTML);
		}
		else
			qWarning () << Q_FUNC_INFO
					<< "unknown option"
					<< replyOpt;
	}

	namespace
	{
		template<typename Templater>
		void SetupReplyPlaintextContents (const Message_ptr& msg, IEditorWidget *editor, Templater&& tpl)
		{
			auto plainSplit = msg->GetBody ().split ('\n');
			for (auto& str : plainSplit)
			{
				str = str.trimmed ();
				if (str.at (0) != '>')
					str.prepend (' ');
				str.prepend ('>');
			}

			const auto& plainContent = plainSplit.join ("\n") + "\n\n";
			editor->SetContents (tpl (plainContent, msg.get ()), ContentType::PlainText);
		}

		QString WrapString (const QString& str, const QString& tagName)
		{
			const auto quoteStartMarker = "<" + tagName + " style='border-left: 2px solid #ccc; margin-left: 0; padding-left: 0.5em;'>";
			const auto quoteEndMarker = "</" + tagName + ">";

			return quoteStartMarker + str + quoteEndMarker;
		}

		static const QString BlockquoteBreakJS =
			R"delim(
				var input = document.querySelector('body');

				input.addEventListener('keydown', function(e, data) {
						if (e.keyCode === 13 && !e.altKey && !e.shiftKey && !e.ctrlKey && !e.metaKey) {
							if (split())
								e.preventDefault();
						}
					});

				function split() {
					var sel = window.getSelection(),
							range,
							textNode,
							elem;

					if (sel.rangeCount) {
						range = sel.getRangeAt(0);
						textNode = range.commonAncestorContainer;
						elem = textNode.parentNode;
						var topBlockquote = findTopBlockquote(elem);
						if (topBlockquote) {
							splitBlockquote(textNode, range.endOffset, topBlockquote);
							return true;
						}
					}

					return false;
				}

				function isBlockquote(elem) {
					return elem.tagName.toLowerCase() === 'blockquote';
				}

				function findTopBlockquote(elem) {
					var lastBlockquote = null;
					while (elem.parentNode && elem.parentNode.tagName) {
						elem = elem.parentNode;
						if (isBlockquote(elem))
							lastBlockquote = elem;
					}

					return lastBlockquote;
				}

				function splitBlockquote(textNode, pos, topBq) {
					var parent = topBq.parentNode,
						parentPos = getNodeIndex(parent, topBq),
						doc = textNode.ownerDocument,
						range = doc.createRange(),
						fragment,
						div = createExtra("div");

					range.setStart(parent, parentPos);
					range.setEnd(textNode, pos);
					fragment = range.extractContents();
					fragment.appendChild(div);

					parent.insertBefore(fragment, topBq);
					select(div);
				}

				function getNodeIndex(parent, node) {
					var index = parent.childNodes.length - 1;

					while (index > 0 && node !== parent.childNodes[index]) {
						index--;
					}

					return index;
				}

				function createExtra(tag) {
					var elem = document.createElement(tag);

					elem.innerHTML = "&#160;";

					return elem;
				}

				function select(elem) {
					var sel = window.getSelection();

					sel.removeAllRanges();
					sel.selectAllChildren(elem);
				}
			)delim";

		void SetReplyHTMLContents (const QString& contents, IEditorWidget *editor)
		{
			editor->SetContents (contents, ContentType::HTML);

			if (const auto iahe = dynamic_cast<IAdvancedHTMLEditor*> (editor))
				iahe->ExecJS (BlockquoteBreakJS);
		}

		template<typename Templater>
		void SetupReplyRichContents (const Message_ptr& msg, IEditorWidget *editor, Templater&&)
		{
			const auto& htmlBody = msg->GetHTMLBody ();
			if (!htmlBody.isEmpty ())
				SetReplyHTMLContents (WrapString (htmlBody, "blockquote"), editor);
			else
			{
				auto str = Util::Escape (msg->GetBody ());
				str.replace ("\r\n", "<br/>");
				str.replace ("\r", "<br/>");
				str.replace ("\n", "<br/>");

				SetReplyHTMLContents (WrapString (str, "blockquote"), editor);
			}
		}
	}

	void ComposeMessageTab::PrepareReplyBody (const Message_ptr& msg)
	{
		PrepareReplyEditor (msg);
		const auto editor = Ui_.Editor_->GetCurrentEditor ();

		const auto type = MsgType::Reply;

		SetupReplyPlaintextContents (msg, editor,
				[&] (auto... rest)
					{ return TemplatesMgr_->GetTemplatedText (ContentType::PlainText, type, rest...); });
		SetupReplyRichContents (msg, editor,
				[&] (auto... rest)
					{ return TemplatesMgr_->GetTemplatedText (ContentType::HTML, type, rest...); });
	}

	void ComposeMessageTab::SetupToolbar ()
	{
		QAction *send = new QAction (tr ("Send"), this);
		send->setProperty ("ActionIcon", "mail-send");
		connect (send,
				SIGNAL (triggered ()),
				this,
				SLOT (handleSend ()));
		Toolbar_->addAction (send);

		AccountsMenu_ = new QMenu (tr ("Accounts"));
		auto accsGroup = new QActionGroup (this);
		for (const auto& account : AccsMgr_->GetAccounts ())
		{
			QAction *act = new QAction (account->GetName (), this);
			accsGroup->addAction (act);
			act->setCheckable (true);
			act->setChecked (true);
			act->setProperty ("Account", QVariant::fromValue<Account_ptr> (account));

			AccountsMenu_->addAction (act);
		}

		auto accountsButton = new QToolButton (Toolbar_);
		accountsButton->setMenu (AccountsMenu_);
		accountsButton->setPopupMode (QToolButton::InstantPopup);
		accountsButton->setProperty ("ActionIcon", "system-users");
		Toolbar_->addWidget (accountsButton);

		AttachmentsMenu_ = new QMenu (tr ("Attachments"));
		AttachmentsMenu_->addSeparator ();
		QAction *add = AttachmentsMenu_->
				addAction (tr ("Add..."), this, SLOT (handleAddAttachment ()));
		add->setProperty ("ActionIcon", "list-add");

		auto attachmentsButton = new QToolButton (Toolbar_);
		attachmentsButton->setProperty ("ActionIcon", "mail-attachment");
		attachmentsButton->setMenu (AttachmentsMenu_);
		attachmentsButton->setPopupMode (QToolButton::InstantPopup);
		Toolbar_->addWidget (attachmentsButton);

		EditorsMenu_ = new QMenu (tr ("Editors"));

		auto editorsButton = new QToolButton (Toolbar_);
		editorsButton->setProperty ("ActionIcon", "story-editor");
		editorsButton->setMenu (EditorsMenu_);
		editorsButton->setPopupMode (QToolButton::InstantPopup);
		Toolbar_->addWidget (editorsButton);
	}

	void ComposeMessageTab::SetupEditors ()
	{
		Ui_.Editor_->SetupEditors ([this] (QAction *act) { EditorsMenu_->addAction (act); });

		connect (Ui_.Editor_,
				SIGNAL (editorChanged (IEditorWidget*, IEditorWidget*)),
				this,
				SLOT (handleEditorChanged (IEditorWidget*, IEditorWidget*)));
	}

	void ComposeMessageTab::SetMessageReferences (const Message_ptr& message) const
	{
		if (!ReplyMessage_)
			return;

		const auto& id = ReplyMessage_->GetMessageID ();
		if (id.isEmpty ())
			return;

		message->SetInReplyTo ({ id });

		auto references = ReplyMessage_->GetReferences ();
		while (references.size () > 20)
			references.removeAt (1);

		references << id;
		message->SetReferences (references);
	}

	namespace
	{
		Message::Addresses_t FromUserInput (const QString& text)
		{
			Message::Addresses_t result;

			for (auto address : text.split (',', QString::SkipEmptyParts))
			{
				address = address.trimmed ();

				QString name;

				const int idx = address.lastIndexOf (' ');
				if (idx > 0)
				{
					name = address.left (idx).trimmed ();
					address = address.mid (idx).simplified ();
				}

				if (address.startsWith ('<') &&
						address.endsWith ('>'))
				{
					address = address.mid (1);
					address.chop (1);
				}

				result.append ({ name, address });
			}

			return result;
		}
	}

	void ComposeMessageTab::handleSend ()
	{
		Account_ptr account;
		for (auto act : AccountsMenu_->actions ())
		{
			if (!act->isChecked ())
				continue;

			account = act->property ("Account").value<Account_ptr> ();
			break;
		}
		if (!account)
			return;

		const auto editor = Ui_.Editor_->GetCurrentEditor ();

		const auto& message = std::make_shared<Message> ();
		message->SetAddresses (Message::Address::To, FromUserInput (Ui_.To_->text ()));
		message->SetSubject (Ui_.Subject_->text ());
		message->SetBody (editor->GetContents (ContentType::PlainText));
		message->SetHTMLBody (editor->GetContents (ContentType::HTML));

		SetMessageReferences (message);

		Util::MimeDetector detector;

		for (auto act : AttachmentsMenu_->actions ())
		{
			const auto& path = act->property ("Snails/AttachmentPath").toString ();
			if (path.isEmpty ())
				continue;

			const auto& descr = act->property ("Snails/Description").toString ();

			const auto& split = detector (path).split ('/');
			const auto& type = split.value (0);
			const auto& subtype = split.value (1);

			message->AddAttachment ({ path, descr, type, subtype, QFileInfo (path).size () });
		}

		Util::Sequence (nullptr, account->SendMessage (message)) >>
				[safeThis = QPointer<ComposeMessageTab> { this }] (const auto& result)
				{
					Util::Visit (result.AsVariant (),
							[safeThis] (const boost::none_t&) { if (safeThis) safeThis->Remove (); },
							[safeThis] (const auto& err)
							{
								Util::Visit (err,
										[safeThis] (const vmime::exceptions::authentication_error& err)
										{
											QMessageBox::critical (safeThis, "LeechCraft",
													tr ("Unable to send the message: authorization failure. Server reports: %1.")
														.arg ("<br/><em>" + QString::fromStdString (err.response ()) + "</em>"));
										},
										[] (const vmime::exceptions::connection_error&)
										{
											const auto& notify = Util::MakeNotification ("Snails",
													tr ("Unable to send email: operation timed out.<br/><br/>"
														"Consider switching between SSL and TLS/STARTSSL or replacing port 465 with 587, or vice versa."
														"Port 465 is typically used with SSL, while port 587 is used with TLS."),
													PCritical_);
											Core::Instance ().GetProxy ()->GetEntityManager ()->HandleEntity (notify);
										},
										[] (const auto& err)
										{
											qWarning () << Q_FUNC_INFO << "caught exception:" << err.what ();

											const auto& notify = Util::MakeNotification ("Snails",
													tr ("Unable to send email: %1.")
															.arg (QString::fromUtf8 (err.what ())),
													PCritical_);
											Core::Instance ().GetProxy ()->GetEntityManager ()->HandleEntity (notify);
										});
							});
				};
	}

	void ComposeMessageTab::handleAddAttachment ()
	{
		const QString& path = QFileDialog::getOpenFileName (this,
				tr ("Select file to attach"),
				QDir::homePath ());
		if (path.isEmpty ())
			return;

		QFile file (path);
		if (!file.open (QIODevice::ReadOnly))
		{
			QMessageBox::critical (this,
					tr ("Error attaching file"),
					tr ("Error attaching file: %1 cannot be read")
						.arg (path));
			return;
		}

		const QString& descr = QInputDialog::getText (this,
				tr ("Attachment description"),
				tr ("Enter optional attachment description (you may leave it blank):"));

		const QFileInfo fi (path);

		const QString& size = Util::MakePrettySize (file.size ());
		QAction *attAct = new QAction (QString ("%1 (%2)").arg (fi.fileName (), size), this);
		attAct->setProperty ("Snails/AttachmentPath", path);
		attAct->setProperty ("Snails/Description", descr);
		attAct->setIcon (QFileIconProvider ().icon (fi));
		connect (attAct,
				SIGNAL (triggered ()),
				this,
				SLOT (handleRemoveAttachment ()));

		const auto& acts = AttachmentsMenu_->actions ();
		AttachmentsMenu_->insertAction (acts.at (acts.size () - 2), attAct);
	}

	void ComposeMessageTab::handleRemoveAttachment ()
	{
		QAction *act = qobject_cast<QAction*> (sender ());
		act->deleteLater ();
		AttachmentsMenu_->removeAction (act);
	}

	void ComposeMessageTab::handleEditorChanged (IEditorWidget *newEditor, IEditorWidget *previous)
	{
		const auto& currentHtml = previous->GetContents (ContentType::HTML);
		const auto& currentPlain = previous->GetContents (ContentType::PlainText);

		newEditor->SetContents (currentPlain, ContentType::PlainText);
		if (!currentHtml.isEmpty ())
			SetReplyHTMLContents (currentHtml, newEditor);
	}
}
}
