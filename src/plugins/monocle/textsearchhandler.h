/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <QObject>
#include <QMap>
#include <util/gui/findnotification.h>
#include "interfaces/monocle/idocument.h"

class QGraphicsRectItem;

namespace LC::Monocle
{
	class PageGraphicsItem;

	struct TextSearchHandlerResults
	{
		QString Text_;
		Util::FindNotification::FindFlags FindFlags_;
		QMap<int, QList<QRectF>> Positions_;
	};

	class TextSearchHandler : public QObject
	{
		Q_OBJECT

		IDocument_ptr Doc_;
		QVector<PageGraphicsItem*> Pages_;

		QString CurrentSearchString_;

		QVector<QGraphicsRectItem*> CurrentHighlights_;
		int CurrentRectIndex_ = -1;
	public:
		using QObject::QObject;

		void HandleDoc (const IDocument_ptr&, const QVector<PageGraphicsItem*>&);

		bool Search (const QString&, Util::FindNotification::FindFlags);
		void SetPreparedResults (const TextSearchHandlerResults&, int selectedItem);
	private:
		bool RequestSearch (const QString&, Util::FindNotification::FindFlags);

		void BuildHighlights (const QMap<int, QList<QRectF>>&);
		void ClearHighlights ();

		void SelectItem (int);
	signals:
		void navigateRequested (const NavigationAction&);

		void gotSearchResults (const TextSearchHandlerResults&);
	};
}
