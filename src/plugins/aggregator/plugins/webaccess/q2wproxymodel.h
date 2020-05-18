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

#pragma once

#include <QObject>
#include <QMap>
#include <Wt/WAbstractItemModel.h>
#include <util/models/modelitem.h>
#include "serverupdater.h"

class QAbstractItemModel;
class QModelIndex;

namespace LC
{
namespace Aggregator
{
namespace WebAccess
{
	class Q2WProxyModel : public QObject
						, public Wt::WAbstractItemModel
	{
		Q_OBJECT

		QAbstractItemModel * const Src_;
		Util::ModelItem_ptr Root_;

		QMap<int, int> Mapping_;

		Wt::WApplication * const App_;
		ServerUpdater Update_;

		int LastModelResetRC_ = 0;
	public:
		using Morphism_t = std::function<Wt::cpp17::any (QModelIndex, Wt::ItemDataRole)>;
	private:
		QList<Morphism_t> Morphisms_;
	public:
		Q2WProxyModel (QAbstractItemModel*, Wt::WApplication*);

		void SetRoleMappings (const QMap<int, int>&);
		void AddDataMorphism (const Morphism_t&);

		QModelIndex MapToSource (const Wt::WModelIndex&) const;

		int columnCount (const Wt::WModelIndex& parent) const override;
		int rowCount (const Wt::WModelIndex& parent) const override;
		Wt::WModelIndex parent (const Wt::WModelIndex& index) const override;
		Wt::cpp17::any data (const Wt::WModelIndex& index, Wt::ItemDataRole role) const override;
		Wt::WModelIndex index (int row, int column, const Wt::WModelIndex& parent) const override;
		Wt::cpp17::any headerData (int section, Wt::Orientation orientation, Wt::ItemDataRole role) const override;

		void* toRawIndex (const Wt::WModelIndex& index) const override;
		Wt::WModelIndex fromRawIndex (void* rawIndex) const override;
	private:
		int WtRole2Qt (Wt::ItemDataRole) const;
		QModelIndex W2QIdx (const Wt::WModelIndex&) const;
		Wt::WModelIndex Q2WIdx (const QModelIndex&) const;
	private Q_SLOTS:
		void handleDataChanged (const QModelIndex&, const QModelIndex&);

		void handleRowsAboutToBeInserted (const QModelIndex&, int, int);
		void handleRowsInserted (const QModelIndex&, int, int);

		void handleRowsAboutToBeRemoved (const QModelIndex&, int, int);
		void handleRowsRemoved (const QModelIndex&, int, int);

		void handleModelAboutToBeReset ();
		void handleModelReset ();
	};
}
}
}
