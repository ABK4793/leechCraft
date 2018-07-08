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

#include "storagebackend.h"
#include <QSqlDatabase>
#include <util/sll/util.h>
#include <util/db/oral/oralfwd.h>

namespace LeechCraft
{
namespace Poshuku
{
	class SQLStorageBackend : public StorageBackend
	{
		QSqlDatabase DB_;
		const Util::DefaultScopeGuard DBGuard_;
	public:
		struct History;
		struct Favorites;
		struct FormsNever;
	private:
		Util::oral::ObjectInfo_ptr<History> History_;
		Util::oral::ObjectInfo_ptr<Favorites> Favorites_;
		Util::oral::ObjectInfo_ptr<FormsNever> FormsNever_;
	public:
		SQLStorageBackend (Type);

		void LoadHistory (history_items_t&) const override;
		history_items_t LoadResemblingHistory (const QString&) const override;
		void AddToHistory (const HistoryItem&) override;
		void ClearOldHistory (int, int) override;
		void LoadFavorites (FavoritesModel::items_t&) const override;
		void AddToFavorites (const FavoritesModel::FavoritesItem&) override;
		void RemoveFromFavorites (const FavoritesModel::FavoritesItem&) override;
		void UpdateFavorites (const FavoritesModel::FavoritesItem&) override;
		void SetFormsIgnored (const QString&, bool) override;
		bool GetFormsIgnored (const QString&) const override;
	};
}
}
