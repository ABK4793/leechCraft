/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2013  Oleg Linkin <MaledictusDeMagog@gmail.com>
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

#include <functional>
#include <memory>
#include <QObject>
#include <QSet>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/blasq/iaccount.h>
#include <interfaces/blasq/isupportdeletes.h>
#include <interfaces/blasq/isupportuploads.h>
#include "picasamanager.h"

class QStandardItemModel;
class QStandardItem;

namespace LeechCraft
{
namespace Blasq
{
namespace Vangog
{
	class PicasaService;

	class PicasaAccount : public QObject
						, public IAccount
						, public ISupportDeletes
						, public ISupportUploads
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Blasq::IAccount LeechCraft::Blasq::ISupportDeletes
				LeechCraft::Blasq::ISupportUploads)

		QString Name_;
		PicasaService * const Service_;
		const ICoreProxy_ptr Proxy_;
		QByteArray ID_;
		QString Login_;
		QString AccessToken_;
		QDateTime AccessTokenExpireDate_;
		QString RefreshToken_;
		bool Ready_;

		PicasaManager *PicasaManager_;

		QStandardItemModel * const CollectionsModel_;
		QStandardItem *AllPhotosItem_;
		QHash<QByteArray, QStandardItem*> AlbumId2AlbumItem_;
		QHash<QByteArray, QSet<QByteArray>> AlbumID2PhotosSet_;
		QHash<QStandardItem*, QByteArray> Item2PhotoId_;
		QHash<QByteArray, QModelIndex> DeletedPhotoId2Index_;

	public:
		enum PicasaRole
		{
			AlbumId = CollectionRole::CollectionRoleMax + 1
		};

		PicasaAccount (const QString& name, PicasaService *service,
				ICoreProxy_ptr proxy, const QString& login,
				const QByteArray& id = QByteArray ());

		ICoreProxy_ptr GetProxy () const;

		void Release ();
		QByteArray Serialize () const;
		static PicasaAccount* Deserialize (const QByteArray& data,
				PicasaService *service, ICoreProxy_ptr proxy);

		QObject* GetQObject () override;
		IService* GetService () const override;
		QString GetName () const override;
		QByteArray GetID () const override;

		QString GetLogin () const;
		QString GetAccessToken () const;
		QDateTime GetAccessTokenExpireDate () const;
		void SetRefreshToken (const QString& token);
		QString GetRefreshToken () const;

		QAbstractItemModel* GetCollectionsModel () const override;

		void UpdateCollections () override;

		void Delete (const QModelIndex& index) override;

		void CreateCollection(const QModelIndex& parent) override;
		bool HasUploadFeature(Feature ) const override;
		void UploadImages(const QModelIndex& collection, const QList<UploadItem>& paths) override;
	private:
		bool TryToEnterLoginIfNoExists ();

	private slots:
		void handleGotAlbums (const QList<Album>& albums);
		void handleGotAlbum (const Album& album);
		void handleGotPhotos (const QList<Photo>& photos);
		void handleDeletedPhotos (const QByteArray& id);

	signals:
		void accountChanged (PicasaAccount *acc);
		void doneUpdating () override;
	};
}
}
}
