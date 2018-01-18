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

#include "readitemsfilter.h"
#include <Wt/WTimer.h>
#include "aggregatorapp.h"
#include "wf.h"

namespace LeechCraft
{
namespace Aggregator
{
namespace WebAccess
{
	ReadItemsFilter::ReadItemsFilter ()
	{
		setDynamicSortFilter (true);
	}

	void ReadItemsFilter::SetHideRead (bool hide)
	{
		HideRead_ = hide;
		Invalidate ();
	}

	void ReadItemsFilter::SetCurrentItem (IDType_t id)
	{
		if (id == CurrentId_)
			return;

		if (Prevs_.isEmpty ())
			Wt::WTimer::singleShot (500, WF ([this] { PullOnePrev (); }));

		Prevs_ << CurrentId_;
		CurrentId_ = id;

		Invalidate ();
	}

	void ReadItemsFilter::ClearCurrentItem ()
	{
		SetCurrentItem (static_cast<IDType_t> (-1));
	}

	bool ReadItemsFilter::filterAcceptRow (int row, const Wt::WModelIndex& parent) const
	{
		if (HideRead_)
		{
			auto idx = sourceModel ()->index (row, 0, parent);
			if (idx.isValid ())
			{
				try
				{
					const auto idAny = idx.data (AggregatorApp::ItemRole::IID);
					const auto id = boost::any_cast<IDType_t> (idAny);
					if (id != CurrentId_ && !Prevs_.contains (id))
					{
						const auto data = idx.data (AggregatorApp::ItemRole::IsRead);
						if (boost::any_cast<bool> (data))
							return false;
					}
				}
				catch (const std::exception& e)
				{
					qWarning () << Q_FUNC_INFO
							<< "cannot get read status"
							<< e.what ();
					return true;
				}
			}
		}

		return Wt::WSortFilterProxyModel::filterAcceptRow (row, parent);
	}

	void ReadItemsFilter::PullOnePrev ()
	{
		if (Prevs_.isEmpty ())
			return;

		Prevs_.removeFirst ();
		Invalidate ();

		if (!Prevs_.isEmpty ())
			Wt::WTimer::singleShot (500, WF ([this] { PullOnePrev (); }));
	}

	void ReadItemsFilter::Invalidate ()
	{
		setFilterRegExp (".*");
	}
}
}
}
