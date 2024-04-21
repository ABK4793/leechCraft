/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

class QWidget;

namespace LC::Monocle
{
	class IDocument;

	void ExportToPdf (IDocument& doc, QWidget& parent);
	void Print (int curPage, IDocument& doc, QWidget& parent);
}
