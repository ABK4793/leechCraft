/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "fspathsmanager.h"
#include <QStandardItemModel>
#include <QSettings>
#include <QCoreApplication>
#include <QtDebug>
#include <HUpnpAv/HFileSystemDataSource>
#include <HUpnpAv/HRootDir>
#include <xmlsettingsdialog/datasourceroles.h>

namespace LeechCraft
{
namespace DLNiwe
{
	namespace HAV = Herqq::Upnp::Av;

	FSPathsManager::FSPathsManager (HAV::HFileSystemDataSource *source, QObject *parent)
	: QObject (parent)
	, Model_ (new QStandardItemModel (this))
	, FSSource_ (source)
	{
		Model_->setHorizontalHeaderLabels ({ tr ("Path") });
		auto headerItem = Model_->horizontalHeaderItem (0);
		headerItem->setData (DataSources::DataFieldType::LocalPath,
				DataSources::DataSourceRole::FieldType);

		LoadSettings ();
	}

	QAbstractItemModel* FSPathsManager::GetModel () const
	{
		return Model_;
	}

	void FSPathsManager::RefillSource ()
	{
		FSSource_->clear ();

		for (auto i = 0; i < Model_->rowCount (); ++i)
			FSSource_->add ({
					Model_->index (i, 0).data ().toString (),
					HAV::HRootDir::RecursiveScan,
					HAV::HRootDir::WatchForChanges
				});
	}

	void FSPathsManager::AppendItem (const QString& path)
	{
		auto item = new QStandardItem (path);
		item->setEditable (false);
		Model_->appendRow (item);

		FSSource_->add ({
				path,
				HAV::HRootDir::RecursiveScan,
				HAV::HRootDir::WatchForChanges
			});
	}

	void FSPathsManager::LoadSettings ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_DLNiwe");
		const auto& paths = settings.value ("RootPaths").toStringList ();

		for (const auto& path : paths)
			AppendItem (path);
	}

	void FSPathsManager::SaveSettings ()
	{
		QStringList paths;
		for (auto i = 0; i < Model_->rowCount (); ++i)
			paths << Model_->index (i, 0).data ().toString ();

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_DLNiwe");
		settings.setValue ("RootPaths", paths);
	}

	void FSPathsManager::addRequested (const QString&, const QVariantList& rowData)
	{
		const auto& path = rowData.value (0).toString ();
		if (path.isEmpty ())
			return;

		AppendItem (path);

		SaveSettings ();
	}

	void FSPathsManager::removeRequested (const QString&, QModelIndexList rows)
	{
		if (rows.size () > 1)
			std::reverse (rows.begin (), rows.end ());

		for (const auto& row : rows)
			Model_->removeRow (row.row ());

		RefillSource ();

		SaveSettings ();
	}
}
}
