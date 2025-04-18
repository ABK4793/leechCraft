/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "annitem.h"
#include <QBrush>
#include <QCursor>
#include <QTimer>
#include <QtDebug>
#include "components/layout/positions.h"

namespace LC::Monocle
{
	AnnBaseItem::AnnBaseItem (IAnnotation_ptr ann)
	: BaseAnn_ { std::move (ann) }
	{
		QTimer::singleShot (0, [this] { SetSelected (false); });
	}

	QGraphicsItem* AnnBaseItem::GetItem ()
	{
		return dynamic_cast<QGraphicsItem*> (this);
	}

	void AnnBaseItem::SetHandler (const Handler_f& handler)
	{
		Handler_ = handler;
	}

	bool AnnBaseItem::IsSelected () const
	{
		return IsSelected_;
	}

	void AnnBaseItem::SetSelected (bool selected)
	{
		IsSelected_ = selected;
	}

	QPen AnnBaseItem::GetPen (bool selected)
	{
		return selected ? QPen { QColor { 255, 93, 0 }, 2 } : Qt::NoPen;
	}

	QBrush AnnBaseItem::GetBrush (bool selected)
	{
		return QBrush { selected ? QColor { 255, 213, 0, 64 } : QColor { 255, 213, 0, 32 } };
	}

	AnnBaseItem* MakeItem (const IAnnotation_ptr& ann, QGraphicsItem *parent, LinkExecutionContext& ec)
	{
		switch (ann->GetAnnotationType ())
		{
		case AnnotationType::Text:
			return new TextAnnItem (std::dynamic_pointer_cast<ITextAnnotation> (ann), parent);
		case AnnotationType::Highlight:
			return new HighAnnItem (std::dynamic_pointer_cast<IHighlightAnnotation> (ann), parent);
		case AnnotationType::Link:
			return new LinkAnnItem (std::dynamic_pointer_cast<ILinkAnnotation> (ann), parent, ec);
		case AnnotationType::Caret:
			return new CaretAnnItem (std::dynamic_pointer_cast<ICaretAnnotation> (ann), parent);
		case AnnotationType::Other:
			qWarning () << "unknown annotation type with contents" << ann->GetText ();
			return nullptr;
		}

		qWarning () << "unhandled annotation type" << static_cast<int> (ann->GetAnnotationType ()) << ann->GetText ();
		return nullptr;
	}

	namespace
	{
		PageRelativeRect GetBoundingRect (const QList<QPolygonF>& polys)
		{
			PageRelativeRect::Type bounding {};
			for (const auto& poly : polys)
				bounding |= poly.boundingRect ();
			return PageRelativeRect { bounding };
		}
	}

	HighAnnItem::HighAnnItem (const IHighlightAnnotation_ptr& ann, QGraphicsItem *parent)
	: AnnBaseGraphicsItem { ann, parent }
	, Polys_ { ToPolyData (ann->GetPolygons ()) }
	, Bounding_ { GetBoundingRect (ann->GetPolygons ()) }
	{
		for (const auto& data : Polys_)
		{
			addToGroup (data.Item_);
			data.Item_->setPen (Qt::NoPen);
			data.Item_->setCursor (Qt::PointingHandCursor);
		}
	}

	void HighAnnItem::SetSelected (bool selected)
	{
		AnnBaseItem::SetSelected (selected);

		const auto& pen = GetPen (selected);
		const auto& brush = GetBrush (selected);
		for (const auto& data : Polys_)
		{
			data.Item_->setPen (pen);
			data.Item_->setBrush (brush);
		}
	}

	void HighAnnItem::UpdateRect (const PageAbsoluteRect& rect)
	{
		setPos (rect.ToRectF ().topLeft ());

		if (Bounding_.IsEmpty ())
			return;

		const auto xScale = rect.ToRectF ().width () / Bounding_.ToRectF ().width ();
		const auto yScale = rect.ToRectF ().height () / Bounding_.ToRectF ().height ();
		const auto xTran = - Bounding_.ToRectF ().x () * xScale;
		const auto yTran = - Bounding_.ToRectF ().y () * yScale;
		const QTransform transform { xScale, 0, 0, yScale, xTran, yTran };

		for (const auto& data : Polys_)
			data.Item_->setPolygon (data.Poly_ * transform);
	}

	QList<HighAnnItem::PolyData> HighAnnItem::ToPolyData (const QList<QPolygonF>& polys)
	{
		QList<PolyData> result;
		for (const auto& poly : polys)
			result.append ({ poly, new QGraphicsPolygonItem });
		return result;
	}

	LinkAnnItem::LinkAnnItem (const ILinkAnnotation_ptr& ann, QGraphicsItem *item, LinkExecutionContext& ec)
	: AnnRectGraphicsItem { ann, ann->GetLink (), item, ec }
	{
	}
}
