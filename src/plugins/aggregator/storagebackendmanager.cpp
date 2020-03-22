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

#include "storagebackendmanager.h"
#include <QCoreApplication>
#include <QThread>
#include <util/sll/either.h>
#include "dumbstorage.h"
#include "xmlsettingsmanager.h"

namespace LC
{
namespace Aggregator
{
	StorageBackendManager& StorageBackendManager::Instance ()
	{
		static StorageBackendManager sbm;
		return sbm;
	}

	void StorageBackendManager::Release ()
	{
		if (auto cnt = PrimaryStorageBackend_.use_count (); cnt > 1)
			qWarning () << Q_FUNC_INFO
					<< "primary storage use count is"
					<< cnt;

		PrimaryStorageBackend_.reset ();
	}

	StorageBackendManager::StorageCreationResult_t StorageBackendManager::CreatePrimaryStorage ()
	{
		const auto& strType = XmlSettingsManager::Instance ()->property ("StorageType").toByteArray ();
		try
		{
			PrimaryStorageBackend_ = StorageBackend::Create (strType);
		}
		catch (const std::runtime_error& s)
		{
			PrimaryStorageBackend_ = std::make_shared<DumbStorage> ();
			return StorageCreationResult_t::Left ({ s.what () });
		}

		const int feedsTable = 2;
		const int channelsTable = 2;
		const int itemsTable = 6;

		auto runUpdate = [this, &strType] (auto updater, const char *suffix, int targetVersion)
		{
			const auto curVersion = XmlSettingsManager::Instance ()->Property (strType + suffix, targetVersion).toInt ();
			if (curVersion == targetVersion)
				return true;

			if (!std::invoke (updater, PrimaryStorageBackend_.get (), curVersion))
				return false;

			XmlSettingsManager::Instance ()->setProperty (strType + suffix, targetVersion);
			return true;
		};

		if (!runUpdate (&StorageBackend::UpdateFeedsStorage, "FeedsTableVersion", feedsTable) ||
			!runUpdate (&StorageBackend::UpdateChannelsStorage, "ChannelsTableVersion", channelsTable) ||
			!runUpdate (&StorageBackend::UpdateItemsStorage, "ItemsTableVersion", itemsTable))
			return StorageCreationResult_t::Left ({ "Unable to update tables" });

		PrimaryStorageBackend_->Prepare ();

		emit storageCreated ();

		return StorageCreationResult_t::Right (PrimaryStorageBackend_);
	}

	bool StorageBackendManager::IsPrimaryStorageCreated () const
	{
		return static_cast<bool> (PrimaryStorageBackend_);
	}

	StorageBackend_ptr StorageBackendManager::MakeStorageBackendForThread () const
	{
		if (QThread::currentThread () == qApp->thread ())
			return PrimaryStorageBackend_;

		const auto& strType = XmlSettingsManager::Instance ()->property ("StorageType").toString ();
		try
		{
			auto mgr = StorageBackend::Create (strType, "_AuxThread");
			mgr->Prepare ();
			return mgr;
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot create storage for auxiliary thread";
			return {};
		}
	}

	void StorageBackendManager::Register (const StorageBackend_ptr& backend)
	{
		auto backendPtr = backend.get ();
		connect (backendPtr,
				&StorageBackend::channelAdded,
				this,
				&StorageBackendManager::channelAdded);
		connect (backendPtr,
				&StorageBackend::channelUnreadCountUpdated,
				this,
				&StorageBackendManager::channelUnreadCountUpdated);
		connect (backendPtr,
				&StorageBackend::itemReadStatusUpdated,
				this,
				&StorageBackendManager::itemReadStatusUpdated);
		connect (backendPtr,
				&StorageBackend::itemDataUpdated,
				this,
				&StorageBackendManager::itemDataUpdated);
		connect (backendPtr,
				&StorageBackend::itemsRemoved,
				this,
				&StorageBackendManager::itemsRemoved);
		connect (backendPtr,
				&StorageBackend::channelRemoved,
				this,
				&StorageBackendManager::channelRemoved);
		connect (backendPtr,
				&StorageBackend::feedRemoved,
				this,
				&StorageBackendManager::feedRemoved);
		connect (backendPtr,
				&StorageBackend::hookItemLoad,
				this,
				&StorageBackendManager::hookItemLoad);
		connect (backendPtr,
				&StorageBackend::hookItemAdded,
				this,
				&StorageBackendManager::hookItemAdded);
	}
}
}
