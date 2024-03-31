/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "linkitem.h"
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QPen>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/iiconthememanager.h>
#include "linkactionexecutor.h"

namespace LC
{
namespace Monocle
{
	LinkItem::LinkItem (const ILink_ptr& link, QGraphicsItem *parent, DocumentTab& tab)
	: QGraphicsRectItem { parent }
	, DocTab_ { tab }
	, Link_ { link }
	{
		setCursor (Qt::PointingHandCursor);
		setPen (Qt::NoPen);
		setFlag (QGraphicsItem::ItemHasNoContents);
		setToolTip (link->GetToolTip ());
	}

	void LinkItem::contextMenuEvent (QGraphicsSceneContextMenuEvent *event)
	{
		QMenu menu;
		AddLinkMenuActions (Link_->GetLinkAction (), menu, DocTab_);
		GetProxyHolder ()->GetIconThemeManager ()->ManageWidget (&menu);
		menu.exec (event->screenPos ());
	}

	void LinkItem::mousePressEvent (QGraphicsSceneMouseEvent *event)
	{
		PressedPos_ = event->pos ();
	}

	void LinkItem::mouseReleaseEvent (QGraphicsSceneMouseEvent *event)
	{
		if ((event->pos () - PressedPos_).manhattanLength () < 4)
			ExecuteLinkAction (Link_->GetLinkAction (), DocTab_);
	}
}
}
