/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "flattofoldersproxymodel.h"
#include <QSet>
#include <QMimeData>
#include <QItemSelectionRange>
#include <util/sll/containerconversions.h>
#include <util/sll/prelude.h>
#include <interfaces/iinfo.h>
#include <interfaces/core/itagsmanager.h>

namespace LC::Util
{
	struct FlatTreeItem
	{
		QList<FlatTreeItem_ptr> C_;
		FlatTreeItem_ptr Parent_;

		enum class Type
		{
			Root,
			Folder,
			Item
		};

		Type Type_ = Type::Root;

		QPersistentModelIndex Index_;
		QString Tag_;

		int Row () const
		{
			if (Parent_)
			{
				const auto& c = Parent_->C_;
				for (int i = 0, size = c.size (); i < size; ++i)
					if (c.at (i).get () == this)
						return i;
			}
			return 0;
		}
	};

	FlatTreeItem* ToFlat (const QModelIndex& idx)
	{
		return static_cast<FlatTreeItem*> (idx.internalPointer ());
	}

	FlatToFoldersProxyModel::FlatToFoldersProxyModel (const ITagsManager *itm, QObject *parent)
	: QAbstractItemModel { parent }
	, TM_ { itm }
	, Root_ { std::make_shared<FlatTreeItem> () }
	{
	}

	int FlatToFoldersProxyModel::columnCount (const QModelIndex&) const
	{
		return SourceModel_ ?
			SourceModel_->columnCount ({}) :
			0;
	}

	QVariant FlatToFoldersProxyModel::data (const QModelIndex& index, int role) const
	{
		const auto fti = ToFlat (index);

		if (fti->Type_ == FlatTreeItem::Type::Item)
		{
			const auto& source = fti->Index_;
			return source.sibling (source.row (), index.column ()).data (role);
		}

		if (fti->Type_ == FlatTreeItem::Type::Folder && index.column () == 0)
		{
			switch (role)
			{
			case Qt::DisplayRole:
			{
				if (fti->Tag_.isEmpty ())
					return tr ("untagged");

				const auto& ut = TM_->GetTag (fti->Tag_);
				return ut.isEmpty () ? tr ("<unknown tag>") : ut;
			}
			case RoleTags:
				return fti->Tag_;
			default:
				return {};
			}
		}

		return {};
	}

	QVariant FlatToFoldersProxyModel::headerData (int section, Qt::Orientation orient, int role) const
	{
		if (!SourceModel_)
			return {};

		return SourceModel_->headerData (section, orient, role);
	}

	Qt::ItemFlags FlatToFoldersProxyModel::flags (const QModelIndex& index) const
	{
		if (const auto fti = ToFlat (index);
			fti && fti->Type_ == FlatTreeItem::Type::Item)
			return fti->Index_.flags ();

		return Qt::ItemIsSelectable |
				Qt::ItemIsEnabled |
				Qt::ItemIsDragEnabled |
				Qt::ItemIsDropEnabled;
	}

	QModelIndex FlatToFoldersProxyModel::index (int row, int column, const QModelIndex& parent) const
	{
		if (!hasIndex (row, column, parent))
			return {};

		const auto& fti = ToFlatOrRoot (parent);
		return fti.Type_ == FlatTreeItem::Type::Item ?
				QModelIndex {} :
				createIndex (row, column, fti.C_.at (row).get ());
	}

	QModelIndex FlatToFoldersProxyModel::parent (const QModelIndex& index) const
	{
		const auto& parent = ToFlatOrRoot (index).Parent_;
		if (!parent || parent->Type_ == FlatTreeItem::Type::Root)
			return {};

		return createIndex (parent->Row (), 0, parent.get ());
	}

	int FlatToFoldersProxyModel::rowCount (const QModelIndex& index) const
	{
		return ToFlatOrRoot (index).C_.size ();
	}

	Qt::DropActions FlatToFoldersProxyModel::supportedDropActions() const
	{
		return SourceModel_ ?
				SourceModel_->supportedDropActions () :
				QAbstractItemModel::supportedDropActions ();
	}

	QStringList FlatToFoldersProxyModel::mimeTypes() const
	{
		return SourceModel_ ?
				SourceModel_->mimeTypes () :
				QAbstractItemModel::mimeTypes ();
	}

	QMimeData* FlatToFoldersProxyModel::mimeData (const QModelIndexList& indexes) const
	{
		if (!SourceModel_)
			return QAbstractItemModel::mimeData (indexes);

		QModelIndexList sourceIdxs;
		for (const auto& index : indexes)
		{
			auto item = static_cast<FlatTreeItem*> (index.internalPointer ());
			switch (item->Type_)
			{
			case FlatTreeItem::Type::Item:
				sourceIdxs << MapToSource (index);
				break;
			case FlatTreeItem::Type::Folder:
				for (const auto& subItem : item->C_)
					sourceIdxs << subItem->Index_;
				break;
			default:
				break;
			}
		}

		return SourceModel_->mimeData (sourceIdxs);
	}

	bool FlatToFoldersProxyModel::dropMimeData (const QMimeData* data, Qt::DropAction action, int, int, const QModelIndex& parent)
	{
		if (!SourceModel_)
			return false;

		QMimeData modified;
		for (const auto& format : data->formats ())
			modified.setData (format, data->data (format));

		switch (const auto ptr = static_cast<FlatTreeItem*> (parent.internalPointer ());
				ptr->Type_)
		{
		case FlatTreeItem::Type::Folder:
		case FlatTreeItem::Type::Item:
			modified.setData (QStringLiteral ("x-leechcraft/tag"), ptr->Tag_.toLatin1 ());
			break;
		default:
			break;
		}

		return SourceModel_->dropMimeData (&modified, action, -1, -1, QModelIndex ());
	}

	void FlatToFoldersProxyModel::SetSourceModel (QAbstractItemModel *model)
	{
		if (SourceModel_)
			disconnect (SourceModel_,
					nullptr,
					this,
					nullptr);

		SourceModel_ = model;

		if (model)
		{
			// We don't support changing columns (yet) so don't connect
			// to columns* signals.
			connect (model,
					&QAbstractItemModel::headerDataChanged,
					this,
					&QAbstractItemModel::headerDataChanged);
			connect (model,
					&QAbstractItemModel::dataChanged,
					this,
					&FlatToFoldersProxyModel::HandleDataChanged);
			connect (model,
					&QAbstractItemModel::layoutAboutToBeChanged,
					this,
					&QAbstractItemModel::layoutAboutToBeChanged);
			connect (model,
					&QAbstractItemModel::layoutChanged,
					this,
					&QAbstractItemModel::layoutChanged);
			connect (model,
					&QAbstractItemModel::modelReset,
					this,
					&FlatToFoldersProxyModel::HandleModelReset);
			connect (model,
					&QAbstractItemModel::rowsInserted,
					this,
					&FlatToFoldersProxyModel::HandleRowsInserted);
			connect (model,
					&QAbstractItemModel::rowsAboutToBeRemoved,
					this,
					&FlatToFoldersProxyModel::HandleRowsAboutToBeRemoved);
		}

		HandleModelReset ();
	}

	QAbstractItemModel* FlatToFoldersProxyModel::GetSourceModel () const
	{
		return SourceModel_;
	}

	QModelIndex FlatToFoldersProxyModel::MapToSource (const QModelIndex& proxy) const
	{
		if (!GetSourceModel ())
			return {};

		if (!proxy.isValid ())
			return {};

		const auto item = ToFlat (proxy);

		if (item->Type_ != FlatTreeItem::Type::Item)
			return {};

		return item->Index_;
	}

	QList<QModelIndex> FlatToFoldersProxyModel::MapFromSource (const QModelIndex& source) const
	{
		auto tags = source.data (RoleTags).toStringList ();
		if (tags.isEmpty ())
			tags << QString ();

		QList<QModelIndex> result;
		for (const auto& tag : tags)
		{
			const auto& folder = FindFolder (tag);
			if (!folder)
			{
				qWarning () << Q_FUNC_INFO
						<< "could not find folder for tag"
						<< tag
						<< GetSourceModel ();
				continue;
			}

			const auto& folderIdx = index (folder->Row (), 0, {});

			for (int i = 0; i < folder->C_.size (); ++i)
			{
				const auto& child = folder->C_.at (i);
				if (child->Index_ != source)
					continue;

				result << index (i, 0, folderIdx);
				break;
			}
		}
		return result;
	}

	bool FlatToFoldersProxyModel::IsFolder (const QModelIndex& index) const
	{
		const auto item = ToFlat (index);
		return item && item->Type_ == FlatTreeItem::Type::Folder;
	}

	QList<QModelIndex> FlatToFoldersProxyModel::GetChildren (const QModelIndex& index) const
	{
		if (!IsFolder (index))
			return {};

		const auto& children = ToFlat (index)->C_;
		QList<QModelIndex> result;
		result.reserve (children.size ());
		for (const auto& child : children)
			result << child->Index_;
		return result;
	}

	QList<QVariant> FlatToFoldersProxyModel::GetChildrenData (const QModelIndex& index, int role) const
	{
		const auto& children = GetChildren (index);

		QList<QVariant> result;
		result.reserve (children.size ());
		for (const auto& child : children)
			result.push_back (child.data (role));
		return result;
	}

	FlatTreeItem_ptr FlatToFoldersProxyModel::FindFolder (const QString& tag) const
	{
		for (const auto& item : Root_->C_)
			if (item->Tag_ == tag)
				return item;

		return {};
	}

	FlatTreeItem_ptr FlatToFoldersProxyModel::GetFolder (const QString& tag)
	{
		auto& c = Root_->C_;
		for (const auto& item : c)
			if (item->Tag_ == tag)
				return item;

		const auto& item = std::make_shared<FlatTreeItem> ();
		item->Type_ = FlatTreeItem::Type::Folder;
		item->Tag_ = tag;
		item->Parent_ = Root_;

		int size = c.size ();
		beginInsertRows (QModelIndex (), size, size);
		c.append (item);
		endInsertRows ();

		return item;
	}

	const FlatTreeItem& FlatToFoldersProxyModel::ToFlatOrRoot (const QModelIndex& idx) const
	{
		return idx.isValid () ? *ToFlat (idx) : *Root_;
	}

	void FlatToFoldersProxyModel::HandleRowInserted (int i)
	{
		const auto& idx = SourceModel_->index (i, 0);

		auto tags = idx.data (RoleTags).toStringList ();

		if (tags.isEmpty ())
			tags << QString ();

		QPersistentModelIndex pidx (idx);

		for (const auto& tag : std::as_const (tags))
			AddForTag (tag, pidx);
	}

	void FlatToFoldersProxyModel::HandleRowRemoved (int i)
	{
		QAbstractItemModel *model = SourceModel_;
		QModelIndex idx = model->index (i, 0);

		QStringList tags = idx.data (RoleTags).toStringList ();

		if (tags.isEmpty ())
			tags << QString ();

		QPersistentModelIndex pidx (idx);

		for (const auto& tag : tags)
			RemoveFromTag (tag, pidx);
	}

	void FlatToFoldersProxyModel::AddForTag (const QString& tag,
			const QPersistentModelIndex& pidx)
	{
		FlatTreeItem_ptr folder = GetFolder (tag);

		const auto& item = std::make_shared<FlatTreeItem> ();
		item->Type_ = FlatTreeItem::Type::Item;
		item->Index_ = pidx;
		item->Parent_ = folder;
		item->Tag_ = tag;

		int size = folder->C_.size ();
		QModelIndex iidx = index (Root_->C_.indexOf (folder), 0);
		beginInsertRows (iidx, size, size);
		folder->C_.append (item);
		Items_.insert (pidx, item);
		endInsertRows ();
	}

	void FlatToFoldersProxyModel::RemoveFromTag (const QString& tag,
			const QPersistentModelIndex& pidx)
	{
		const auto& folder = GetFolder (tag);
		auto& c = folder->C_;
		int findex = Root_->C_.indexOf (folder);
		for (int i = 0, size = c.size ();
				i < size; ++i)
		{
			if (c.at (i)->Index_ != pidx)
				continue;

			beginRemoveRows (index (findex, 0), i, i);
			Items_.remove (pidx, c.at (i));
			c.removeAt (i);
			endRemoveRows ();
			break;
		}

		if (c.isEmpty ())
		{
			beginRemoveRows (QModelIndex (), findex, findex);
			Root_->C_.removeAt (findex);
			endRemoveRows ();
		}
	}

	void FlatToFoldersProxyModel::HandleChanged (const QModelIndex& idx)
	{
		auto newTags = Util::AsSet (idx.data (RoleTags).toStringList ());
		if (newTags.isEmpty ())
			newTags << QString {};

		QPersistentModelIndex pidx (idx);

		const auto& oldTags = Util::MapAs<QSet> (Items_.values (pidx), [] (const auto& item) { return item->Tag_; });

		const auto added = QSet<QString> (newTags).subtract (oldTags);
		const auto removed = QSet<QString> (oldTags).subtract (newTags);
		const auto changed = QSet<QString> (newTags).intersect (oldTags);

		for (const auto& ch : changed)
		{
			FlatTreeItem_ptr folder = GetFolder (ch);

			QList<FlatTreeItem_ptr>& c = folder->C_;
			int findex = Root_->C_.indexOf (folder);
			QModelIndex fmi = index (findex, 0);
			for (int i = 0, size = c.size ();
					i < size; ++i)
			{
				if (c.at (i)->Index_ != pidx)
					continue;

				emit dataChanged (index (i, 0, fmi),
						index (i, columnCount () - 1, fmi));
				break;
			}
		}

		for (const auto& rem : removed)
			RemoveFromTag (rem, pidx);

		for (const auto& add : added)
			AddForTag (add, pidx);
	}

	void FlatToFoldersProxyModel::HandleDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
	{
		const QItemSelectionRange range
		{
			topLeft.sibling (topLeft.row (), 0),
			bottomRight.sibling (bottomRight.row (), 0)
		};
		for (const auto& index : range.indexes ())
			HandleChanged (index);
	}

	void FlatToFoldersProxyModel::HandleModelReset ()
	{
		if (const int size = Root_->C_.size ())
		{
			beginRemoveRows (QModelIndex (), 0, size - 1);
			Root_->C_.clear ();
			Items_.clear ();
			endRemoveRows ();
		}

		if (SourceModel_)
			for (int i = 0, size = SourceModel_->rowCount (); i < size; ++i)
				HandleRowInserted (i);
	}

	void FlatToFoldersProxyModel::HandleRowsInserted (const QModelIndex&, int start, int end)
	{
		for (int i = start; i <= end; ++i)
			HandleRowInserted (i);
	}

	void FlatToFoldersProxyModel::HandleRowsAboutToBeRemoved (const QModelIndex&, int start, int end)
	{
		for (int i = start; i <= end; ++i)
			HandleRowRemoved (i);
	}
}
