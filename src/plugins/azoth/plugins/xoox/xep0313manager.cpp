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

#include "xep0313manager.h"
#include <cstdlib>
#include <QDomDocument>
#include <QXmppClient.h>
#include <QXmppMessage.h>
#include <QXmppResultSet.h>
#include "accountsettingsholder.h"
#include "clientconnection.h"
#include "glooxaccount.h"
#include "xep0313prefiq.h"
#include "xep0313reqiq.h"
#include "util.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	const QString NsMam = "urn:xmpp:mam:2";

	bool Xep0313Manager::Supports0313 (const QStringList& features)
	{
		return features.contains (NsMam);
	}

	QString Xep0313Manager::GetNsUri ()
	{
		return NsMam;
	}

	Xep0313Manager::Xep0313Manager (ClientConnection& conn)
	: Conn_ { conn }
	{
	}

	QStringList Xep0313Manager::discoveryFeatures () const
	{
		return { NsMam };
	}

	bool Xep0313Manager::handleStanza (const QDomElement& element)
	{
		const auto& tagName = element.tagName ();

		if (tagName == "iq")
		{
			if (element.firstChildElement ("prefs").namespaceURI () == NsMam)
			{
				HandlePrefs (element);
				return true;
			}
			else if (element.attribute ("type") == "result")
			{
				const auto& fin = element.firstChildElement ("fin");
				if (fin.namespaceURI () == NsMam)
				{
					HandleHistoryQueryFinished (fin);
					return true;
				}
			}
		}
		else if (tagName == "message")
		{
			const auto& res = element.firstChildElement ("result");
			if (res.namespaceURI () == NsMam)
			{
				HandleMessage (res);
				return true;
			}

			const auto& fin = element.firstChildElement ("fin");
			if (fin.namespaceURI () == NsMam)
			{
				HandleHistoryQueryFinished (fin);
				return true;
			}
		}

		return false;
	}

	void Xep0313Manager::RequestPrefs ()
	{
		client ()->sendPacket (Xep0313PrefIq {});
	}

	void Xep0313Manager::SetPrefs (const Xep0313PrefIq& iq)
	{
		auto updateIq = iq;
		updateIq.setType (QXmppIq::Set);
		client ()->sendPacket (updateIq);
	}

	void Xep0313Manager::RequestHistory (const QString& jid, QString baseId, int count)
	{
		if (baseId == "-1")
			baseId = "";
		qDebug () << Q_FUNC_INFO << jid << baseId << count;

		const auto& queryId = "xep0313_" + QString::number (++NextQueryNumber_);

		QueryId2Jid_ [queryId] = jid;

		Xep0313ReqIq iq
		{
			jid,
			baseId,
			std::abs (count),
			count > 0 ?
					Xep0313ReqIq::Direction::Before :
					Xep0313ReqIq::Direction::After,
			queryId
		};
		client ()->sendPacket (iq);
	}

	void Xep0313Manager::HandleMessage (const QXmppElement& resultExt)
	{
		const auto& id = resultExt.attribute ("id");

		const auto& message = XooxUtil::Forwarded2Message (resultExt);
		if (message.to ().isEmpty ())
			return;

		if (message.body ().isEmpty () && message.xhtml ().isEmpty ())
			return;

		const auto& ourJid = client ()->configuration ().jidBare ();

		IMessage::Direction dir;
		QString otherJid;
		if (message.to ().startsWith (ourJid))
		{
			dir = IMessage::Direction::In;
			otherJid = message.from ().section ('/', 0, 0);
		}
		else
		{
			dir = IMessage::Direction::Out;
			otherJid = message.to ().section ('/', 0, 0);
		}

		const SrvHistMessage msg { dir, id.toUtf8 (), {}, message.body (), message.stamp (), message.xhtml () };
		Messages_ [otherJid] << msg;
	}

	void Xep0313Manager::HandleHistoryQueryFinished (const QDomElement& finElem)
	{
		const auto& setElem = finElem.firstChildElement ("set");

		QXmppResultSetReply resultSet;
		resultSet.parse (setElem);

		const auto& jid = QueryId2Jid_.take (finElem.attribute ("queryid"));
		auto messages = Messages_.take (jid);

		qDebug () << Q_FUNC_INFO << resultSet.first () << resultSet.last () << messages.size ();

		if (!messages.isEmpty () &&
				messages.first ().TS_ > messages.last ().TS_)
			std::reverse (messages.begin (), messages.end ());

		const auto& ourNick = Conn_.GetAccount ()->GetSettings ()->GetNick ();
		const auto jidEntry = Conn_.GetCLEntry (jid);
		const auto& otherNick = jidEntry ?
				qobject_cast<ICLEntry*> (jidEntry)->GetHumanReadableID () :
				jid;
		for (auto& message : messages)
			message.Nick_ = message.Dir_ == IMessage::Direction::In ?
					otherNick :
					ourNick;

		emit serverHistoryFetched (jid, resultSet.last (), messages);
	}

	void Xep0313Manager::HandlePrefs (const QDomElement& element)
	{
		Xep0313PrefIq iq;
		iq.parse (element);
		emit gotPrefs (iq);
	}
}
}
}
