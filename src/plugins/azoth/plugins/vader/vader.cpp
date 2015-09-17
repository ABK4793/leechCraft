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

#include "vader.h"
#include <QIcon>
#include <QAction>
#include <QUrl>
#include <util/util.h>
#include <util/xpc/util.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "core.h"
#include "mrimprotocol.h"
#include "mrimbuddy.h"
#include "vaderutil.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Vader
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
        Util::InstallTranslator ("azoth_vader");

		XSD_.reset (new Util::XmlSettingsDialog);
		XSD_->RegisterObject (&XmlSettingsManager::Instance (), "azothvadersettings.xml");

		Core::Instance ().SetCoreProxy (proxy);

		connect (&Core::Instance (),
				SIGNAL (gotEntity (LeechCraft::Entity)),
				this,
				SIGNAL (gotEntity (LeechCraft::Entity)));
	}

	void Plugin::SecondInit ()
	{
		Proto_ = std::make_shared<MRIMProtocol> ();
		Proto_->Init ();
	}

	void Plugin::Release ()
	{
		Proto_->Release ();
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Azoth.Vader";
	}

	QString Plugin::GetName () const
	{
		return "Azoth Vader";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Support for the Mail.ru Agent protocol.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/plugins/azoth/plugins/vader/resources/images/vader.svg");
		return icon;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> classes;
		classes << "org.LeechCraft.Plugins.Azoth.Plugins.IProtocolPlugin";
		return classes;
	}

	QObject* Plugin::GetQObject ()
	{
		return this;
	}

	QList<QObject*> Plugin::GetProtocols () const
	{
		return { Proto_.get () };
	}

	void Plugin::initPlugin (QObject *proxy)
	{
		Core::Instance ().SetProxy (proxy);
	}

	void Plugin::hookEntryActionAreasRequested (LeechCraft::IHookProxy_ptr,
			QObject*, QObject*)
	{
	}

	void Plugin::hookEntryActionsRequested (LeechCraft::IHookProxy_ptr proxy,
			QObject *entry)
	{
		if (!qobject_cast<MRIMBuddy*> (entry))
			return;

		if (!EntryServices_.contains (entry))
		{
			auto list = VaderUtil::GetBuddyServices (this,
					SLOT (entryServiceRequested ()));
			Q_FOREACH (QAction *act, list)
				act->setProperty ("Azoth/Vader/Entry", QVariant::fromValue<QObject*> (entry));
			EntryServices_ [entry] = list;
		}

		QList<QVariant> list = proxy->GetReturnValue ().toList ();
		Q_FOREACH (QAction *act, EntryServices_ [entry])
			list += QVariant::fromValue<QObject*> (act);
		proxy->SetReturnValue (list);
	}

	void Plugin::entryServiceRequested ()
	{
		const QString& url = sender ()->property ("URL").toString ();
		QObject *buddyObj = sender ()->property ("Azoth/Vader/Entry").value<QObject*> ();
		MRIMBuddy *buddy = qobject_cast<MRIMBuddy*> (buddyObj);
		const QString& subst = VaderUtil::SubstituteNameDomain (url,
				buddy->GetHumanReadableID ());
		const Entity& e = Util::MakeEntity (QUrl (subst),
				QString (),
				static_cast<LeechCraft::TaskParameters> (OnlyHandle | FromUserInitiated));
		emit gotEntity (e);
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_azoth_vader, LeechCraft::Azoth::Vader::Plugin);
