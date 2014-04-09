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

#include "collectionsmanager.h"
#include <QStandardItemModel>
#include <QtDebug>
#include <util/models/mergemodel.h>
#include "interfaces/lmp/icollectionmodel.h"
#include "collectionsortermodel.h"
#include "player.h"

namespace LeechCraft
{
namespace LMP
{
	CollectionsManager::CollectionsManager (QObject *parent)
	: QObject { parent }
	, Model_ { new Util::MergeModel { { "Column" }, this } }
	, Sorter_ { new CollectionSorterModel { this } }
	{
		Sorter_->setSourceModel (Model_);
		Sorter_->setDynamicSortFilter (true);
		Sorter_->sort (0);
	}

	void CollectionsManager::Add (QAbstractItemModel *model)
	{
		Model_->AddModel (model);
	}

	QAbstractItemModel* CollectionsManager::GetModel () const
	{
		return Sorter_;
	}

	void CollectionsManager::Enqueue (const QList<QModelIndex>& indexes, Player *player)
	{
		QList<AudioSource> sources;
		for (const auto& idx : indexes)
		{
			const auto& srcIdx = Model_->mapToSource (Sorter_->mapToSource (idx));

			const auto& urls = dynamic_cast<const ICollectionModel*> (srcIdx.model ())->ToSourceUrls ({ srcIdx });
			for (const auto& url : urls)
				sources << url;
		}

		player->Enqueue (sources);
	}
}
}
