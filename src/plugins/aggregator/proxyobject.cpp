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

#include "proxyobject.h"
#include "core.h"
#include "channelsmodel.h"
#include "itemslistmodel.h"

namespace LeechCraft
{
namespace Aggregator
{
	ProxyObject::ProxyObject (QObject *parent)
	: QObject (parent)
	{
	}

	namespace
	{
		void FixItemID (Item_ptr item)
		{
			if (item->ItemID_)
				return;

			item->ItemID_ = Core::Instance ().GetPool (PTItem).GetID ();

			for (auto& enc : item->Enclosures_)
				enc.ItemID_ = item->ItemID_;
		}

		void FixChannelID (Channel_ptr channel)
		{
			if (channel->ChannelID_)
				return;

			channel->ChannelID_ = Core::Instance ().GetPool (PTChannel).GetID ();
			for (const auto& item : channel->Items_)
			{
				item->ChannelID_ = channel->ChannelID_;

				FixItemID (item);
			}
		}

		void FixFeedID (Feed_ptr feed)
		{
			if (feed->FeedID_)
				return;

			feed->FeedID_ = Core::Instance ().GetPool (PTFeed).GetID ();

			for (const auto& channel : feed->Channels_)
			{
				channel->FeedID_ = feed->FeedID_;

				FixChannelID (channel);
			}
		}
	}

	void ProxyObject::AddFeed (Feed_ptr feed)
	{
		FixFeedID (feed);

		Core::Instance ().GetStorageBackend ()->AddFeed (feed);
	}

	void ProxyObject::AddChannel (Channel_ptr channel)
	{
		FixChannelID (channel);

		Core::Instance ().GetStorageBackend ()->AddChannel (channel);
	}

	void ProxyObject::AddItem (Item_ptr item)
	{
		FixItemID (item);

		Core::Instance ().GetStorageBackend ()->AddItem (item);
	}

	QAbstractItemModel* ProxyObject::GetChannelsModel () const
	{
		return Core::Instance ().GetRawChannelsModel ();
	}

	QList<Channel_ptr> ProxyObject::GetAllChannels () const
	{
		QList<Channel_ptr> result;

		channels_shorts_t channels;
		Core::Instance ().GetChannels (channels);
		Q_FOREACH (ChannelShort cs, channels)
			result << Core::Instance ().GetStorageBackend ()->GetChannel (cs.ChannelID_, cs.FeedID_);

		return result;
	}

	int ProxyObject::CountUnreadItems (IDType_t channel) const
	{
		return Core::Instance ().GetStorageBackend ()->GetUnreadItems (channel);
	}

	QList<Item_ptr> ProxyObject::GetChannelItems (IDType_t channelId) const
	{
		// TODO rework when we change items_container_t
		items_container_t items;
		Core::Instance ().GetStorageBackend ()->GetItems (items, channelId);
		return QList<Item_ptr>::fromVector (QVector<Item_ptr>::fromStdVector (items));
	}

	QAbstractItemModel* ProxyObject::CreateItemsModel () const
	{
		return new ItemsListModel;
	}
}
}
