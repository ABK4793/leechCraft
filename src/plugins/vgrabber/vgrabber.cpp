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

#include "vgrabber.h"
#include <QIcon>
#include <util/util.h>
#include <util/xpc/util.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/itagsmanager.h>
#include "audiofindproxy.h"
#include "videofindproxy.h"
#include "xmlsettingsmanager.h"
#include "categoriesselector.h"

namespace LeechCraft
{
namespace vGrabber
{
	void vGrabber::Init (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;

		Util::InstallTranslator ("vgrabber");

		SettingsDialog_.reset (new Util::XmlSettingsDialog ());
		SettingsDialog_->RegisterObject (XmlSettingsManager::Instance (),
				"vgrabbersettings.xml");

		Audio_ = new CategoriesSelector (FindProxy::FPTAudio,
				this);
		Video_ = new CategoriesSelector (FindProxy::FPTVideo,
				this);
		connect (Audio_,
				SIGNAL (goingToAccept (const QStringList&,
						const QStringList&)),
				this,
				SLOT (handleCategoriesGoingToChange (const QStringList&,
						const QStringList&)));
		connect (Video_,
				SIGNAL (goingToAccept (const QStringList&,
						const QStringList&)),
				this,
				SLOT (handleCategoriesGoingToChange (const QStringList&,
						const QStringList&)));

		SettingsDialog_->SetCustomWidget ("AudioCategoriesToUse", Audio_);
		SettingsDialog_->SetCustomWidget ("VideoCategoriesToUse", Video_);
	}

	void vGrabber::SecondInit ()
	{
	}

	void vGrabber::Release ()
	{
		delete Audio_;
		delete Video_;
	}

	QByteArray vGrabber::GetUniqueID () const
	{
		return "org.LeechCraft.vGrabber";
	}

	QString vGrabber::GetName () const
	{
		return "vGrabber";
	}

	QString vGrabber::GetInfo () const
	{
		return tr ("vkontakte.ru audio/video grabber.");
	}

	QIcon vGrabber::GetIcon () const
	{
		static QIcon icon ("lcicons:/resources/images/vgrabber.svg");
		return icon;
	}

	QStringList vGrabber::Needs () const
	{
		return QStringList ("http");
	}

	QStringList vGrabber::GetCategories () const
	{
		QStringList result;
		result += Audio_->GetHRCategories ();
		result += Video_->GetHRCategories ();
		return result;
	}

	QList<IFindProxy_ptr> vGrabber::GetProxy (const Request& req)
	{
		QList<FindProxy_ptr> preresult;
		if (Audio_->GetHRCategories ().contains (req.Category_))
			preresult << FindProxy_ptr (new AudioFindProxy (req, Audio_));

		if (Video_->GetHRCategories ().contains (req.Category_))
			preresult << FindProxy_ptr (new VideoFindProxy (req, Video_));

		QList<IFindProxy_ptr> result;
		Q_FOREACH (FindProxy_ptr fp, preresult)
		{
			connect (fp.get (),
					SIGNAL (delegateEntity (const LeechCraft::Entity&,
							int*, QObject**)),
					this,
					SIGNAL (delegateEntity (const LeechCraft::Entity&,
							int*, QObject**)));
			connect (fp.get (),
					SIGNAL (gotEntity (const LeechCraft::Entity&)),
					this,
					SIGNAL (gotEntity (const LeechCraft::Entity&)));
			connect (fp.get (),
					SIGNAL (error (const QString&)),
					this,
					SLOT (handleError (const QString&)));

			fp->Start ();

			result << IFindProxy_ptr (fp);
		}
		return result;
	}

	ICoreProxy_ptr vGrabber::GetProxy () const
	{
		return Proxy_;
	}

	std::shared_ptr<Util::XmlSettingsDialog> vGrabber::GetSettingsDialog () const
	{
		return SettingsDialog_;
	}

	void vGrabber::handleError (const QString& msg)
	{
		qWarning () << Q_FUNC_INFO << sender () << msg;
		emit gotEntity (Util::MakeNotification ("vGrabber", msg, Priority::Warning));
	}

	void vGrabber::handleCategoriesGoingToChange (const QStringList& added,
			const QStringList& removed)
	{
		QStringList hrAdded, hrRemoved;
		Q_FOREACH (QString a, added)
			hrAdded << Proxy_->GetTagsManager ()->GetTag (a);
		Q_FOREACH (QString r, removed)
			hrRemoved << Proxy_->GetTagsManager ()->GetTag (r);

		emit categoriesChanged (hrAdded, hrRemoved);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_vgrabber, LeechCraft::vGrabber::vGrabber);
