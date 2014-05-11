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

#include "addressesmodelmanager.h"
#include <QStandardItemModel>
#include <QNetworkInterface>
#include <xmlsettingsdialog/datasourceroles.h>
#include <xmlsettingsdialog/basesettingsmanager.h>

namespace LeechCraft
{
namespace Util
{
	AddressesModelManager::AddressesModelManager (BaseSettingsManager *bsm, int defaultPort, QObject *parent)
	: QObject { parent }
	, Model_ { new QStandardItemModel { this } }
	, BSM_ { bsm }
	{
		Model_->setHorizontalHeaderLabels ({ tr ("Host"), tr ("Port") });
		Model_->horizontalHeaderItem (0)->setData (DataSources::DataFieldType::Enum,
				DataSources::DataSourceRole::FieldType);
		Model_->horizontalHeaderItem (1)->setData (DataSources::DataFieldType::Integer,
				DataSources::DataSourceRole::FieldType);

		updateAvailInterfaces ();

		const auto& addrs = BSM_->Property ("ListenAddresses",
				QVariant::fromValue (GetLocalAddresses (defaultPort))).value<AddrList_t> ();
		qDebug () << Q_FUNC_INFO << addrs;
		for (const auto& addr : addrs)
			AppendRow (addr);
	}

	void AddressesModelManager::RegisterTypes ()
	{
		qRegisterMetaType<AddrList_t> ("LeechCraft::Util::AddrList_t");
		qRegisterMetaTypeStreamOperators<AddrList_t> ();
	}

	QAbstractItemModel* AddressesModelManager::GetModel () const
	{
		return Model_;
	}

	AddrList_t AddressesModelManager::GetAddresses () const
	{
		AddrList_t addresses;
		for (auto i = 0; i < Model_->rowCount (); ++i)
		{
			auto hostItem = Model_->item (i, 0);
			auto portItem = Model_->item (i, 1);
			addresses.push_back ({ hostItem->text (), portItem->text () });
		}
		return addresses;
	}

	void AddressesModelManager::SaveSettings () const
	{
		BSM_->setProperty ("ListenAddresses",
				QVariant::fromValue (GetAddresses ()));
	}

	void AddressesModelManager::AppendRow (const QPair<QString, QString>& pair)
	{
		QList<QStandardItem*> items
		{
			new QStandardItem { pair.first },
			new QStandardItem { pair.second }
		};
		for (const auto item : items)
			item->setEditable (false);
		Model_->appendRow (items);

		emit addressesChanged ();
	}

	void AddressesModelManager::updateAvailInterfaces ()
	{
		QVariantList hosts;
		for (const auto& addr : QNetworkInterface::allAddresses ())
		{
			if (!addr.scopeId ().isEmpty ())
				continue;

			QVariantMap map;
			map ["ID"] = map ["Name"] = addr.toString ();
			hosts << map;
		}
		Model_->horizontalHeaderItem (0)->setData (hosts,
				DataSources::DataSourceRole::FieldValues);
	}

	void AddressesModelManager::addRequested (const QString&, const QVariantList& data)
	{
		const auto port = data.value (1).toInt ();
		if (port < 1024 || port > 65535)
			return;

		AppendRow ({ data.value (0).toString (), QString::number (port) });
		SaveSettings ();
	}

	void AddressesModelManager::removeRequested (const QString&, const QModelIndexList& list)
	{
		for (const auto& item : list)
			Model_->removeRow (item.row ());

		SaveSettings ();
		emit addressesChanged ();
	}
}
}
