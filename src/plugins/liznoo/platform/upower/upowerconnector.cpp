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

#include "upowerconnector.h"
#include <QTimer>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnectionInterface>
#include <QtDebug>
#include <util/xpc/util.h>

namespace LeechCraft
{
namespace Liznoo
{
namespace UPower
{
	UPowerConnector::UPowerConnector (QObject *parent)
	: ConnectorBase { "UPower", parent }
	{
		if (!TryAutostart ("org.freedesktop.UPower"))
			return;

		SB_.connect ("org.freedesktop.UPower",
				"/org/freedesktop/UPower",
				"org.freedesktop.UPower",
				"DeviceAdded",
				this,
				SLOT (requeryDevice (QString)));
		SB_.connect ("org.freedesktop.UPower",
				"/org/freedesktop/UPower",
				"org.freedesktop.UPower",
				"DeviceChanged",
				this,
				SLOT (requeryDevice (QString)));

		const auto& introspect = QDBusInterface
		{
			"org.freedesktop.UPower",
			"/org/freedesktop/UPower",
			"org.freedesktop.DBus.Introspectable",
			SB_
		}.call ("Introspect").arguments ().value (0).toString ();
		if (!introspect.contains ("\"Sleeping\"") || !introspect.contains ("\"Resuming\""))
		{
			qDebug () << Q_FUNC_INFO
					<< "no Sleeping() or Resuming() signals, we are probably on systemd";
			return;
		}

		const auto sleepConnected = SB_.connect ("org.freedesktop.UPower",
				"/org/freedesktop/UPower",
				"org.freedesktop.UPower",
				"Sleeping",
				this,
				SLOT (handleGonnaSleep ()));
		const auto resumeConnected = SB_.connect ("org.freedesktop.UPower",
				"/org/freedesktop/UPower",
				"org.freedesktop.UPower",
				"Resuming",
				this,
				SIGNAL (wokeUp ()));

		PowerEventsAvailable_ = sleepConnected && resumeConnected;
	}

	bool UPowerConnector::ArePowerEventsAvailable () const
	{
		return PowerEventsAvailable_;
	}

	void UPowerConnector::handleGonnaSleep ()
	{
		emit gonnaSleep (1000);
	}

	void UPowerConnector::enumerateDevices ()
	{
		QDBusInterface face ("org.freedesktop.UPower",
				"/org/freedesktop/UPower",
				"org.freedesktop.UPower",
				SB_);

		auto res = face.call ("EnumerateDevices");
		for (const auto& argument : res.arguments ())
		{
			auto arg = argument.value<QDBusArgument> ();
			QList<QDBusObjectPath> paths;
			arg >> paths;
			for (const auto& path : paths)
				requeryDevice (path.path ());
		}
	}

	namespace
	{
		QString TechIdToString (int id)
		{
			QMap<int, QString> id2str;
			id2str [1] = "Li-Ion";
			id2str [2] = "Li-Polymer";
			id2str [3] = "Li-Iron-Phosphate";
			id2str [4] = "Lead acid";
			id2str [5] = "NiCd";
			id2str [6] = "NiMh";

			return id2str.value (id, "<unknown>");
		}
	}

	void UPowerConnector::requeryDevice (const QString& id)
	{
		QDBusInterface face ("org.freedesktop.UPower",
				id,
				"org.freedesktop.UPower.Device",
				SB_);
		if (face.property ("Type").toInt () != 2)
			return;

		BatteryInfo info;
		info.ID_ = id;
		info.Percentage_ = face.property ("Percentage").toInt ();
		info.TimeToFull_ = face.property ("TimeToFull").toLongLong ();
		info.TimeToEmpty_ = face.property ("TimeToEmpty").toLongLong ();
		info.Voltage_ = face.property ("Voltage").toDouble ();
		info.Energy_ = face.property ("Energy").toDouble ();
		info.EnergyFull_ = face.property ("EnergyFull").toDouble ();
		info.DesignEnergyFull_ = face.property ("EnergyFullDesign").toDouble ();
		info.EnergyRate_ = face.property ("EnergyRate").toDouble ();
		info.Technology_ = TechIdToString (face.property ("Technology").toInt ());
		info.Temperature_ = 0;

		emit batteryInfoUpdated (info);
	}
}
}
}
