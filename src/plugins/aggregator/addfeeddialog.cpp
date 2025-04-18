/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "addfeeddialog.h"
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/iiconthememanager.h>
#include <interfaces/core/itagsmanager.h>
#include <util/tags/tagscompleter.h>
#include <util/sll/qtutil.h>

namespace LC::Aggregator
{
	AddFeedDialog::AddFeedDialog (const QString& url, QWidget *parent)
	: QDialog { parent }
	{
		Ui_.setupUi (this);
		setWindowIcon (GetProxyHolder ()->GetIconThemeManager ()->GetPluginIcon ());
		new Util::TagsCompleter (Ui_.Tags_);
		Ui_.Tags_->AddSelector ();

		Ui_.URL_->setText (url);
	}

	QString AddFeedDialog::GetURL () const
	{
		auto result = Ui_.URL_->text ().simplified ();
		if (result.startsWith ("itpc"_ql))
			result.replace (0, 4, "http"_ql);
		return result;
	}

	QStringList AddFeedDialog::GetTags () const
	{
		return GetProxyHolder ()->GetTagsManager ()->Split (Ui_.Tags_->text ());
	}
}
