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

#include "sqlstoragebackend.h"
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QtDebug>
#include <util/db/dblock.h>
#include <util/db/util.h>
#include <util/db/oral/oral.h>
#include <util/sys/paths.h>

BOOST_FUSION_ADAPT_STRUCT (LeechCraft::NamAuth::SQLStorageBackend::AuthRecord,
		RealmName_,
		Context_,
		Login_,
		Password_)

namespace LeechCraft
{
namespace NamAuth
{
	SQLStorageBackend::SQLStorageBackend ()
	: DB_ (std::make_shared<QSqlDatabase> (QSqlDatabase::addDatabase ("QSQLITE",
			Util::GenConnectionName ("NamAuth.Connection"))))
	{
		DB_->setDatabaseName (GetDBPath ());
		if (!DB_->open ())
		{
			Util::DBLock::DumpError (DB_->lastError ());
			return;
		}

		AdaptedRecord_ = Util::oral::AdaptPtr<AuthRecord> (*DB_);
	}

	QString SQLStorageBackend::GetDBPath ()
	{
		return Util::CreateIfNotExists ("core").filePath ("core.db");
	}

	namespace sph = Util::oral::sph;

	boost::optional<SQLStorageBackend::AuthRecord> SQLStorageBackend::GetAuth (const QString& realm, const QString& context)
	{
		return AdaptedRecord_->SelectOne (sph::_0 == realm && sph::_1 == context);
	}

	void SQLStorageBackend::SetAuth (const AuthRecord& record)
	{
		AdaptedRecord_->Insert (record, Util::oral::InsertAction::Replace);
	}
}
}
