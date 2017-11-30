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

#include "hili.h"
#include <QCoreApplication>
#include <QIcon>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTranslator>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include <util/util.h>
#include <interfaces/azoth/imessage.h>
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace HiLi
{
	void Plugin::Init (ICoreProxy_ptr)
	{
		Translator_.reset (Util::InstallTranslator ("azoth_hili"));

		XmlSettingsDialog_.reset (new Util::XmlSettingsDialog ());
		XmlSettingsDialog_->RegisterObject (&XmlSettingsManager::Instance (),
				"azothhilisettings.xml");

		XmlSettingsManager::Instance ().RegisterObject ("HighlightRegexps",
				this, "handleRegexpsChanged");
		handleRegexpsChanged ();
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Azoth.HiLi";
	}

	void Plugin::Release ()
	{
	}

	QString Plugin::GetName () const
	{
		return "Azoth HiLi";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Azoth Hili allows one to customize the settings of highlights in conferences.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/plugins/azoth/plugins/hili/resources/images/hili.svg");
		return icon;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Plugins.Azoth.Plugins.IGeneralPlugin";
		return result;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XmlSettingsDialog_;
	}

	void Plugin::hookIsHighlightMessage (IHookProxy_ptr proxy, QObject *msgObj)
	{
		if (RegexpsCache_.isEmpty ())
			return;

		IMessage *msg = qobject_cast<IMessage*> (msgObj);
		if (msg->GetMessageType () != IMessage::Type::MUCMessage)
			return;

		const auto& body = msg->GetBody ();
		if (body.size () > 1024)
		{
			qWarning () << Q_FUNC_INFO
					<< "too big string to handle:"
					<< body.size ();
			return;
		}

		if (std::any_of (RegexpsCache_.begin (), RegexpsCache_.end (),
				[&body] (const QRegExp& rx) { return body.contains (rx); }))
		{
			proxy->CancelDefault ();
			proxy->SetReturnValue (true);
		}
	}

	void Plugin::handleRegexpsChanged ()
	{
		RegexpsCache_.clear ();
		const auto& strings = XmlSettingsManager::Instance ().property ("HighlightRegexps").toStringList ();
		for (auto string : strings)
		{
			string = string.trimmed ();
			if (string.isEmpty ())
				continue;

			string.prepend (".*");
			string.append (".*");
			RegexpsCache_ << QRegExp (string, Qt::CaseInsensitive, QRegExp::RegExp2);
		}
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_azoth_hili, LeechCraft::Azoth::HiLi::Plugin);
