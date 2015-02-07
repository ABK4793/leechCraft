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

#include "seekthru.h"
#include <interfaces/entitytesthandleresult.h>
#include <util/util.h>
#include <util/xpc/util.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "core.h"
#include "xmlsettingsmanager.h"
#include "searcherslist.h"
#include "wizardgenerator.h"

namespace LeechCraft
{
namespace SeekThru
{
	void SeekThru::Init (ICoreProxy_ptr proxy)
	{
		Translator_.reset (Util::InstallTranslator ("seekthru"));

		Core::Instance ().SetProxy (proxy);

		connect (&Core::Instance (),
				SIGNAL (delegateEntity (const LeechCraft::Entity&,
						int*, QObject**)),
				this,
				SIGNAL (delegateEntity (const LeechCraft::Entity&,
						int*, QObject**)));
		connect (&Core::Instance (),
				SIGNAL (gotEntity (const LeechCraft::Entity&)),
				this,
				SIGNAL (gotEntity (const LeechCraft::Entity&)));
		connect (&Core::Instance (),
				SIGNAL (error (const QString&)),
				this,
				SLOT (handleError (const QString&)),
				Qt::QueuedConnection);
		connect (&Core::Instance (),
				SIGNAL (warning (const QString&)),
				this,
				SLOT (handleWarning (const QString&)),
				Qt::QueuedConnection);
		connect (&Core::Instance (),
				SIGNAL (categoriesChanged (const QStringList&, const QStringList&)),
				this,
				SIGNAL (categoriesChanged (const QStringList&, const QStringList&)));

		Core::Instance ().DoDelayedInit ();

		XmlSettingsDialog_.reset (new Util::XmlSettingsDialog ());
		XmlSettingsDialog_->RegisterObject (&XmlSettingsManager::Instance (),
				"seekthrusettings.xml");

		auto searchersList = new SearchersList;
		connect (searchersList,
				SIGNAL (gotEntity (LeechCraft::Entity)),
				this,
				SIGNAL (gotEntity (LeechCraft::Entity)));
		XmlSettingsDialog_->SetCustomWidget ("SearchersList", searchersList);
	}

	void SeekThru::SecondInit ()
	{
	}

	void SeekThru::Release ()
	{
		XmlSettingsDialog_.reset ();
	}

	QByteArray SeekThru::GetUniqueID () const
	{
		return "org.LeechCraft.SeekThru";
	}

	QString SeekThru::GetName () const
	{
		return "SeekThru";
	}

	QString SeekThru::GetInfo () const
	{
		return tr ("Search via OpenSearch-aware search providers.");
	}

	QIcon SeekThru::GetIcon () const
	{
		static QIcon icon ("lcicons:/resources/images/seekthru.svg");
		return icon;
	}

	QStringList SeekThru::Provides () const
	{
		return QStringList ("search");
	}

	QStringList SeekThru::Needs () const
	{
		return QStringList ("http");
	}

	QStringList SeekThru::Uses () const
	{
		return QStringList ("webbrowser");
	}

	void SeekThru::SetProvider (QObject *object, const QString& feature)
	{
		Core::Instance ().SetProvider (object, feature);
	}

	QStringList SeekThru::GetCategories () const
	{
		return Core::Instance ().GetCategories ();
	}

	QList<IFindProxy_ptr> SeekThru::GetProxy (const LeechCraft::Request& r)
	{
		QList<IFindProxy_ptr> result;
		result << Core::Instance ().GetProxy (r);
		return result;
	}

	std::shared_ptr<LeechCraft::Util::XmlSettingsDialog> SeekThru::GetSettingsDialog () const
	{
		return XmlSettingsDialog_;
	}

	EntityTestHandleResult SeekThru::CouldHandle (const Entity& e) const
	{
		return Core::Instance ().CouldHandle (e) ?
				EntityTestHandleResult (EntityTestHandleResult::PIdeal) :
				EntityTestHandleResult ();
	}

	void SeekThru::Handle (Entity e)
	{
		Core::Instance ().Handle (e);
	}

	QString SeekThru::GetFilterVerb () const
	{
		return tr ("Search in OpenSearch engines");
	}

	QList<SeekThru::FilterVariant> SeekThru::GetFilterVariants () const
	{
		QList<FilterVariant> result;
		for (const auto& cat : Core::Instance ().GetCategories ())
			result << FilterVariant
				{
					cat.toUtf8 (),
					cat,
					tr ("Search this term in OpenSearch engines in category %1.").arg (cat),
					{}
				};
		return result;
	}

	QList<QWizardPage*> SeekThru::GetWizardPages () const
	{
		return WizardGenerator {}.GetPages ();
	}

	ISyncProxy* SeekThru::SeekThru::GetSyncProxy ()
	{
		// TODO ISyncProxy
		return nullptr;
	}

	void SeekThru::handleError (const QString& error)
	{
		emit gotEntity (Util::MakeNotification ("SeekThru", error, PCritical_));
	}

	void SeekThru::handleWarning (const QString& error)
	{
		emit gotEntity (Util::MakeNotification ("SeekThru", error, PWarning_));
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_seekthru, LeechCraft::SeekThru::SeekThru);
