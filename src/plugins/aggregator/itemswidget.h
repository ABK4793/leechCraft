/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <QWidget>
#include "common.h"

class QModelIndex;
class QToolBar;
class IWebBrowser;
class QAbstractItemModel;

namespace LC
{
namespace Util
{
	class ShortcutManager;
}

namespace Aggregator
{
	struct ItemsWidget_Impl;
	class ChannelActions;
	class AppWideActions;
	class Aggregator;
	class ItemsFilterModel;
	class UpdatesManager;
	class ItemActions;
	class ItemSelectionTracker;
	enum class ChannelDirection;

	struct ItemsWidgetDependencies
	{
		Util::ShortcutManager& ShortcutsMgr_;
		const AppWideActions& AppWideActions_;
		const ChannelActions& ChannelActions_;
		UpdatesManager& UpdatesManager_;
		std::function<bool (ChannelDirection)> ChannelNavigator_;
	};

	class ItemsWidget : public QWidget
	{
		Q_OBJECT

		friend class Aggregator;

		std::unique_ptr<ItemsWidget_Impl> Impl_;
		std::unique_ptr<ItemActions> Actions_;
		std::unique_ptr<ItemSelectionTracker> SelectionTracker_;
	public:
		using Dependencies = ItemsWidgetDependencies;

		explicit ItemsWidget (const Dependencies&, QWidget* = nullptr);
		~ItemsWidget () override;

		void ConstructBrowser ();

		QToolBar* GetToolBar () const;

		void SetChannels (const QList<IDType_t>&);
	private:
		void SetTapeMode (bool);

		void RenderSelectedItems ();
	private slots:
		void on_CaseSensitiveSearch__stateChanged (int);

		void makeCurrentItemVisible ();
		void updateItemsFilter ();
	};
}
}
