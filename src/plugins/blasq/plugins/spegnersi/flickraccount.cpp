/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "flickraccount.h"
#include <QTimer>
#include <QSettings>
#include <QCoreApplication>
#include <QUuid>
#include <QtDebug>
#include <interfaces/core/ientitymanager.h>
#include <util/util.h>
#include "flickrservice.h"

namespace LeechCraft
{
namespace Blasq
{
namespace Spegnersi
{
	const QString RequestTokenURL = "http://www.flickr.com/services/oauth/request_token";
	const QString UserAuthURL = "http://www.flickr.com/services/oauth/authorize";
	const QString AccessTokenURL = "http://www.flickr.com/services/oauth/access_token";

	FlickrAccount::FlickrAccount (const QString& name,
			FlickrService *service, ICoreProxy_ptr proxy, const QByteArray& id)
	: QObject (service)
	, Name_ (name)
	, ID_ (id.isEmpty () ? QUuid::createUuid ().toByteArray () : id)
	, Service_ (service)
	, Proxy_ (proxy)
	, Req_ (new KQOAuthRequest (this))
	, AuthMgr_ (new KQOAuthManager (this))
	{
		AuthMgr_->setNetworkManager (proxy->GetNetworkAccessManager ());
		AuthMgr_->setHandleUserAuthorization (true);

		connect (AuthMgr_,
				SIGNAL (temporaryTokenReceived (QString, QString)),
				this,
				SLOT (handleTempToken (QString, QString)));
		connect (AuthMgr_,
				SIGNAL (authorizationReceived (QString, QString)),
				this,
				SLOT (handleAuthorization (QString, QString)));
		connect (AuthMgr_,
				SIGNAL (accessTokenReceived (QString, QString)),
				this,
				SLOT (handleAccessToken (QString, QString)));

		QTimer::singleShot (0,
				this,
				SLOT (checkAuthTokens ()));
	}

	QByteArray FlickrAccount::Serialize () const
	{
		QByteArray result;
		{
			QDataStream ostr (&result, QIODevice::WriteOnly);
			ostr << static_cast<quint8> (1)
					<< ID_
					<< Name_
					<< AuthToken_
					<< AuthSecret_;
		}
		return result;
	}

	FlickrAccount* FlickrAccount::Deserialize (const QByteArray& ba, FlickrService *service, ICoreProxy_ptr proxy)
	{
		QDataStream istr (ba);

		quint8 version = 0;
		istr >> version;
		if (version != 1)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown version"
					<< version;
			return nullptr;
		}

		QByteArray id;
		QString name;
		istr >> id
				>> name;

		auto acc = new FlickrAccount (name, service, proxy, id);
		istr >> acc->AuthToken_
				>> acc->AuthSecret_;
		return acc;
	}

	QObject* FlickrAccount::GetQObject ()
	{
		return this;
	}

	IService* FlickrAccount::GetService () const
	{
		return Service_;
	}

	QString FlickrAccount::GetName () const
	{
		return Name_;
	}

	QByteArray FlickrAccount::GetID () const
	{
		return ID_;
	}

	void FlickrAccount::UpdateCollections ()
	{
	}

	KQOAuthRequest* FlickrAccount::MakeRequest (const QUrl& url, KQOAuthRequest::RequestType type)
	{
		Req_->clearRequest ();
		Req_->initRequest (type, url);
		Req_->setConsumerKey ("08efe88f972b2b89bd35e42bb26f970e");
		Req_->setConsumerSecretKey ("f70ac4b1ab7c499b");
		Req_->setSignatureMethod (KQOAuthRequest::HMAC_SHA1);

		if (!AuthToken_.isEmpty () && !AuthSecret_.isEmpty ())
		{
			Req_->setToken (AuthToken_);
			Req_->setTokenSecret (AuthSecret_);
		}

		return Req_;
	}

	void FlickrAccount::checkAuthTokens ()
	{
		if (AuthToken_.isEmpty () || AuthSecret_.isEmpty ())
			requestTempToken ();
	}

	void FlickrAccount::requestTempToken ()
	{
		auto req = MakeRequest (RequestTokenURL, KQOAuthRequest::TemporaryCredentials);
		AuthMgr_->executeRequest (req);
	}

	void FlickrAccount::handleTempToken (const QString&, const QString&)
	{
		if (AuthMgr_->lastError () != KQOAuthManager::NoError)
		{
			qWarning () << Q_FUNC_INFO
					<< AuthMgr_->lastError ();
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Blasq Spegnersi",
						tr ("Unable to get temporary auth token."),
						PCritical_));

			QTimer::singleShot (10 * 60 * 1000,
					this,
					SLOT (requestTempToken ()));
			return;
		}

		AuthMgr_->getUserAuthorization (UserAuthURL);
	}

	void FlickrAccount::handleAuthorization (const QString&, const QString&)
	{
		switch (AuthMgr_->lastError ())
		{
		case KQOAuthManager::NoError:
			break;
		case KQOAuthManager::RequestUnauthorized:
			return;
		default:
			qWarning () << Q_FUNC_INFO
					<< AuthMgr_->lastError ();
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Blasq Spegnersi",
						tr ("Unable to get user authorization."),
						PCritical_));

			QTimer::singleShot (10 * 60 * 1000,
					this,
					SLOT (requestTempToken ()));
			return;
		}

		AuthMgr_->getUserAccessTokens (AccessTokenURL);
	}

	void FlickrAccount::handleAccessToken (const QString& token, const QString& secret)
	{
		qDebug () << Q_FUNC_INFO
				<< "access token received";
		AuthToken_ = token;
		AuthSecret_ = secret;
		emit accountChanged (this);
	}
}
}
}
