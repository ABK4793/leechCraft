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

#include "contactdropfilter.h"
#include <QDropEvent>
#include <QImage>
#include <QMessageBox>
#include <QInputDialog>
#include <QUrl>
#include <QFileInfo>
#include <QBuffer>
#include <util/util.h>
#include <util/xpc/util.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/idatafilter.h>
#include <interfaces/ientityhandler.h>
#include "interfaces/azoth/irichtextmessage.h"
#include "interfaces/azoth/iaccount.h"
#include "core.h"
#include "chattab.h"
#include "transferjobmanager.h"
#include "dndutil.h"

namespace LeechCraft
{
namespace Azoth
{
	ContactDropFilter::ContactDropFilter (const QString& entryId, ChatTab *parent)
	: QObject { parent }
	, EntryId_ { entryId }
	, ChatTab_ { parent }
	{
	}

	bool ContactDropFilter::eventFilter (QObject*, QEvent *e)
	{
		if (e->type () != QEvent::Drop)
			return false;

		HandleDrop (static_cast<QDropEvent*> (e)->mimeData ());

		return true;
	}

	void ContactDropFilter::HandleDrop (const QMimeData *data)
	{
		const auto& imgData = data->imageData ();

		const auto& urls = data->urls ();
		if (data->hasImage () && urls.size () <= 1)
			HandleImageDropped (imgData.value<QImage> (), urls.value (0));
		else if (DndUtil::HasContacts (data))
			HandleContactsDropped (data);
		else if (!urls.isEmpty ())
			HandleFilesDropped (urls);
	}

	namespace
	{
		template<typename T>
		T* GetEntry (const QString& id)
		{
			QObject *obj = Core::Instance ().GetEntry (id);
			if (!obj)
			{
				qWarning () << Q_FUNC_INFO
						<< "no entry for"
						<< id;
				return 0;
			}

			T *entry = qobject_cast<T*> (obj);
			if (!entry)
				qWarning () << Q_FUNC_INFO
						<< "object"
						<< obj
						<< "doesn't implement the required interface";
			return entry;
		}

		void SendInChat (const QImage& image, const QString& entryId, ChatTab *chatTab)
		{
			auto entry = GetEntry<ICLEntry> (entryId);
			if (!entry)
				return;

			const auto msgType = entry->GetEntryType () == ICLEntry::EntryType::MUC ?
						IMessage::Type::MUCMessage :
						IMessage::Type::ChatMessage;
			auto msgObj = entry->CreateMessage (msgType,
					chatTab->GetSelectedVariant (),
					ContactDropFilter::tr ("This message contains inline image, enable XHTML-IM to view it."));
			auto msg = qobject_cast<IMessage*> (msgObj);

			if (IRichTextMessage *richMsg = qobject_cast<IRichTextMessage*> (msgObj))
			{
				QString asBase;
				if (entry->GetEntryType () == ICLEntry::EntryType::MUC)
				{
					QBuffer buf;
					buf.open (QIODevice::ReadWrite);
					image.save (&buf, "JPG", 60);
					asBase = QString ("data:image/png;base64,%1")
							.arg (QString (buf.buffer ().toBase64 ()));
				}
				else
					asBase = Util::GetAsBase64Src (image);
				const auto& body = "<img src='" + asBase + "'/>";
				richMsg->SetRichBody (body);
			}

			msg->Send ();
		}

		void SendLink (const QUrl& url, const QString& entryId, ChatTab *chatTab)
		{
			auto entry = GetEntry<ICLEntry> (entryId);
			if (!entry)
				return;

			const auto msgType = entry->GetEntryType () == ICLEntry::EntryType::MUC ?
						IMessage::Type::MUCMessage :
						IMessage::Type::ChatMessage;
			auto msgObj = entry->CreateMessage (msgType,
					chatTab->GetSelectedVariant (),
					url.toEncoded ());
			auto msg = qobject_cast<IMessage*> (msgObj);
			msg->Send ();
		}

		void CollectDataFilters (QStringList& choiceItems,
				QList<std::function<void ()>>& functions,
				const QImage& image, const QString& entryId, const QString& variant)
		{
			const auto& entity = Util::MakeEntity (image,
					{},
					TaskParameter::NoParameters,
					"x-leechcraft/data-filter-request");

			auto prevChoices = choiceItems;
			choiceItems.clear ();
			auto prevFunctions = functions;
			functions.clear ();

			auto em = Core::Instance ().GetProxy ()->GetEntityManager ();
			for (const auto obj : em->GetPossibleHandlers (entity))
			{
				const auto idf = qobject_cast<IDataFilter*> (obj);

				const auto& verb = idf->GetFilterVerb ();

				for (const auto& var : idf->GetFilterVariants ())
				{
					choiceItems << verb + ": " + var.Name_;

					auto thisEnt = entity;
					thisEnt.Additional_ ["DataFilter"] = verb;
					thisEnt.Additional_ ["DataFilterCallback"] = QVariant::fromValue (DataFilterCallback_f {
								[entryId, variant] (const QVariant& var) -> void
								{
									const auto& url = var.toUrl ();
									if (url.isEmpty ())
										return;

									auto entry = GetEntry<ICLEntry> (entryId);
									if (!entry)
										return;

									const auto msgType = entry->GetEntryType () == ICLEntry::EntryType::MUC ?
												IMessage::Type::MUCMessage :
												IMessage::Type::ChatMessage;
									auto msgObj = entry->CreateMessage (msgType,
											variant,
											url.toString ());
									auto msg = qobject_cast<IMessage*> (msgObj);
									msg->Send ();
								}
							});

					functions.append ([thisEnt, obj]
							{
								qobject_cast<IEntityHandler*> (obj)->Handle (thisEnt);
							});
				}
			}

			choiceItems << prevChoices;
			functions << prevFunctions;
		}
	}

	void ContactDropFilter::HandleImageDropped (const QImage& image, const QUrl& url)
	{
		QStringList choiceItems
		{
			tr ("Send directly in chat")
		};

		QList<std::function<void ()>> functions
		{
			[this, &image] { SendInChat (image, EntryId_, ChatTab_); }
		};

		if (url.scheme () != "file")
		{
			choiceItems << tr ("Send link");
			functions.append ([this, url] { SendLink (url, EntryId_, ChatTab_); });
		}
		else
		{
			choiceItems.prepend (tr ("Send as file"));
			functions.prepend ([this, &url]
				{
					Core::Instance ().GetTransferJobManager ()->
							OfferURLs (GetEntry<ICLEntry> (EntryId_), { url });
				});

			CollectDataFilters (choiceItems, functions, image,
					EntryId_, ChatTab_->GetSelectedVariant ());
		}

		PerformChoice (choiceItems, functions);
	}

	void ContactDropFilter::PerformChoice (const QStringList& choiceItems, const QList< std::function<void ()>>& functions)
	{
		bool ok = false;
		const auto& choice = QInputDialog::getItem (ChatTab_,
				tr ("Send image"),
				tr ("How exactly would you like to send the image?"),
				choiceItems,
				0,
				false,
				&ok);
		if (!ok)
			return;

		const auto funcIdx = choiceItems.indexOf (choice);
		const auto& func = functions.at (funcIdx);
		if (!func)
		{
			qWarning () << Q_FUNC_INFO
					<< "no function for choice"
					<< choice;
			return;
		}

		func ();
	}

	bool ContactDropFilter::CheckImage (const QList<QUrl>& urls)
	{
		if (urls.size () != 1)
			return false;

		const auto& local = urls.at (0).toLocalFile ();
		if (!QFile::exists (local))
			return false;

		const QImage img (local);
		if (img.isNull ())
			return false;

		HandleImageDropped (img, urls.at (0));
		return true;
	}

	namespace
	{
		bool CanEntryBeInvited (ICLEntry *thisEntry, ICLEntry *entry)
		{
			const bool isMuc = thisEntry->GetEntryType () == ICLEntry::EntryType::MUC;

			const auto entryAcc = qobject_cast<IAccount*> (entry->GetParentAccount ());
			const auto thisAcc = qobject_cast<IAccount*> (thisEntry->GetParentAccount ());
			if (thisAcc->GetParentProtocol () != entryAcc->GetParentProtocol ())
				return false;

			const bool isThatMuc = entry->GetEntryType () == ICLEntry::EntryType::MUC;
			return isThatMuc != isMuc;
		}
	}

	void ContactDropFilter::HandleContactsDropped (const QMimeData *data)
	{
		const auto thisEntry = GetEntry<ICLEntry> (EntryId_);
		const bool isMuc = thisEntry->GetEntryType () == ICLEntry::EntryType::MUC;

		auto entries = DndUtil::DecodeEntryObjs (data);
		entries.erase (std::remove_if (entries.begin (), entries.end (),
					[thisEntry] (QObject *entryObj)
					{
						return !CanEntryBeInvited (thisEntry,
								qobject_cast<ICLEntry*> (entryObj));
					}),
				entries.end ());

		if (entries.isEmpty ())
			return;

		QString text;
		if (entries.size () > 1)
			text = isMuc ?
					tr ("Enter reason to invite %n contact(s) to %1:", 0, entries.size ())
						.arg (thisEntry->GetEntryName ()) :
					tr ("Enter reason to invite %1 to %n conference(s):", 0, entries.size ())
						.arg (thisEntry->GetEntryName ());
		else
		{
			const auto muc = isMuc ?
					thisEntry :
					qobject_cast<ICLEntry*> (entries.first ());
			const auto entry = isMuc ?
					qobject_cast<ICLEntry*> (entries.first ()) :
					thisEntry;
			text = tr ("Enter reason to invite %1 to %2:")
					.arg (entry->GetEntryName ())
					.arg (muc->GetEntryName ());
		}

		bool ok = false;
		auto reason = QInputDialog::getText (nullptr,
				tr ("Invite to a MUC"),
				text,
				QLineEdit::Normal,
				{},
				&ok);
		if (!ok)
			return;

		if (isMuc)
		{
			const auto muc = qobject_cast<IMUCEntry*> (thisEntry->GetQObject ());

			for (const auto& entry : entries)
				muc->InviteToMUC (qobject_cast<ICLEntry*> (entry)->GetHumanReadableID (), reason);
		}
		else
		{
			const auto thisId = thisEntry->GetHumanReadableID ();

			for (const auto& mucEntryObj : entries)
			{
				const auto muc = qobject_cast<IMUCEntry*> (mucEntryObj);
				muc->InviteToMUC (thisId, reason);
			}
		}
	}

	void ContactDropFilter::HandleFilesDropped (const QList<QUrl>& urls)
	{
		if (CheckImage (urls))
			return;

		Core::Instance ().GetTransferJobManager ()->
				OfferURLs (GetEntry<ICLEntry> (EntryId_), urls);
	}
}
}
