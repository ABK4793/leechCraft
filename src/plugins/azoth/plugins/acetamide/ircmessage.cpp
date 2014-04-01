/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
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

#include "ircmessage.h"
#include <QtDebug>
#include <QTextDocument>
#include "clientconnection.h"
#include "core.h"
#include "ircserverhandler.h"
#include "channelsmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Acetamide
{
	IrcMessage::IrcMessage (IMessage::MessageType type,
			IMessage::Direction dir,
			const QString& id,
			const QString& nickname,
			ClientConnection *conn)
	: Type_ (type)
	, SubType_ (MSTOther)
	, Direction_ (dir)
	, ID_ (id)
	, NickName_ (nickname)
	, Connection_ (conn)
	, OtherPart_ (0)
	{
		Message_.Stamp_ = QDateTime::currentDateTime ();
		Message_.Nickname_ = NickName_;
	}

	IrcMessage::IrcMessage (const Message& msg,
			const QString& id, ClientConnection* conn)
	: Type_ (MTMUCMessage)
	, SubType_ (MSTOther)
	, Direction_ (DIn)
	, ID_ (id)
	, Message_ (msg)
	, Connection_ (conn)
	, OtherPart_ (0)
	{
		if (!Message_.Stamp_.isValid ())
			Message_.Stamp_ = QDateTime::currentDateTime ();
	}

	QObject* IrcMessage::GetQObject ()
	{
		return this;
	}

	void IrcMessage::Send ()
	{
		if (Direction_ == DIn)
		{
			qWarning () << Q_FUNC_INFO
					<< "tried to send incoming message";
			return;
		}

		switch (Type_)
		{
		case MTChatMessage:
		case MTMUCMessage:
			Connection_->GetIrcServerHandler (ID_)->SendPrivateMessage (this);
			Connection_->GetIrcServerHandler (ID_)->GetChannelManager ()->SetPrivateChat (GetOtherVariant ());
			return;
		case MTStatusMessage:
		case MTEventMessage:
		case MTServiceMessage:
			qWarning () << Q_FUNC_INFO
					<< this
					<< "cannot send a service message";
			break;
		}
	}

	void IrcMessage::Store ()
	{
		ServerParticipantEntry_ptr entry =
				Connection_->GetIrcServerHandler (ID_)->
						GetParticipantEntry (GetOtherVariant ());
		entry->HandleMessage (this);
	}

	IMessage::Direction IrcMessage::GetDirection () const
	{
		return Direction_;
	}

	IMessage::MessageType IrcMessage::GetMessageType () const
	{
		return Type_;
	}

	IMessage::MessageSubType IrcMessage::GetMessageSubType () const
	{
		return SubType_;
	}

	void IrcMessage::SetMessageSubType (IMessage::MessageSubType subtype)
	{
		SubType_ = subtype;
	}

	QObject* IrcMessage::OtherPart () const
	{
		return OtherPart_;
	}

	void IrcMessage::SetOtherPart (QObject *entry)
	{
		OtherPart_ = entry;
	}

	QString IrcMessage::GetID () const
	{
		return ID_;
	}

	QString IrcMessage::GetOtherVariant () const
	{
		return Message_.Nickname_;
	}

	void IrcMessage::SetOtherVariant (const QString& nick)
	{
		Message_.Nickname_ = nick;
	}

	QString IrcMessage::GetBody () const
	{
		return Message_.Body_;
	}

	void IrcMessage::SetBody (const QString& body)
	{
		Message_.Body_ = body;
	}

	QDateTime IrcMessage::GetDateTime () const
	{
		return Message_.Stamp_;
	}

	void IrcMessage::SetDateTime (const QDateTime& dateTime)
	{
		Message_.Stamp_ = dateTime;
	}
};
};
};
