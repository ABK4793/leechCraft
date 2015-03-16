/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2013  Oleg Linkin <MaledictusDeMagog@gmail.com>
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

#include "core.h"
#include <util/xpc/util.h>
#include <QFileInfo>
#include <QUrl>

#if QT_VERSION < 0x050000
#include <QDesktopServices>
#else
#include <QStandardPaths>
#endif

#include <util/util.h>
#include <interfaces/core/ientitymanager.h>

namespace LeechCraft
{
namespace NetStoreManager
{
namespace DBox
{
	Core::Core ()
	{
	}

	Core& Core::Instance ()
	{
		static Core c;
		return c;
	}

	void Core::SetProxy (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
	}

	ICoreProxy_ptr Core::GetProxy () const
	{
		return Proxy_;
	}

	void Core::SendEntity (const LeechCraft::Entity& e)
	{
		Proxy_->GetEntityManager ()->HandleEntity (e);
	}

	void Core::DelegateEntity (const LeechCraft::Entity& e,
			const QString& targetPath, bool openAfterDownload)
	{
		int id = -1;
		QObject *pr;
		emit delegateEntity (e, &id, &pr);
		if (id == -1)
		{
			Entity notif = Util::MakeNotification (tr ("Import error"),
					tr ("Could not find plugin to download %1.")
							.arg (e.Entity_.toString ()),
					PCritical_);
			notif.Additional_ ["UntilUserSees"] = true;
			emit gotEntity (notif);
			return;
		}
		Id2SavePath_ [id] = targetPath;
		Id2OpenAfterDownloadState_ [id] = openAfterDownload;
		HandleProvider (pr, id);
	}

	void Core::HandleProvider (QObject *provider, int id)
	{
		if (Downloaders_.contains (provider))
			return;

		Downloaders_ << provider;
		connect (provider,
				SIGNAL (jobFinished (int)),
				this,
				SLOT (handleJobFinished (int)));
		connect (provider,
				SIGNAL (jobRemoved (int)),
				this,
				SLOT (handleJobRemoved (int)));
		connect (provider,
				SIGNAL (jobError (int, IDownload::Error)),
				this,
				SLOT (handleJobError (int, IDownload::Error)));

		Id2Downloader_ [id] = provider;
	}

	void Core::handleJobFinished (int id)
	{
		QString path = Id2SavePath_.take (id);
		Id2Downloader_.remove (id);

		if (Id2OpenAfterDownloadState_.contains (id) &&
				Id2OpenAfterDownloadState_ [id])
		{
			emit gotEntity (Util::MakeEntity (QUrl::fromLocalFile (path),
					QString (), OnlyHandle | FromUserInitiated));
			Id2OpenAfterDownloadState_.remove (id);
		}
	}

	void Core::handleJobRemoved (int id)
	{
		Id2Downloader_.remove (id);
		Id2SavePath_.remove (id);
		Id2OpenAfterDownloadState_.remove (id);
	}

	void Core::handleJobError (int id, IDownload::Error)
	{
		Id2Downloader_.remove (id);
		Id2SavePath_.remove (id);
		Id2OpenAfterDownloadState_.remove (id);
	}

}
}
}
