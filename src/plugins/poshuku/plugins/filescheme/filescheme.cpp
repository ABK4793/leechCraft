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

#include "filescheme.h"
#include <typeinfo>
#include <QIcon>
#include <util/util.h>
#include "schemereply.h"

Q_DECLARE_METATYPE (QNetworkReply*);

namespace LeechCraft
{
namespace Poshuku
{
namespace FileScheme
{
	void FileScheme::Init (ICoreProxy_ptr)
	{
		Util::InstallTranslator ("poshuku_filescheme");
	}

	void FileScheme::SecondInit ()
	{
	}

	void FileScheme::Release ()
	{
	}

	QByteArray FileScheme::GetUniqueID () const
	{
		return "org.LeechCraft.Poshuku.FileScheme";
	}

	QString FileScheme::GetName () const
	{
		return "Poshuku FileScheme";
	}

	QString FileScheme::GetInfo () const
	{
		return tr ("Provides support for file:// scheme.");
	}

	QIcon FileScheme::GetIcon () const
	{
		static QIcon icon ("lcicons:/plugins/poshuku/plugins/filescheme/resources/images/poshuku_filescheme.svg");
		return icon;
	}

	QSet<QByteArray> FileScheme::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Poshuku.Plugins/1.0";
		result << "org.LeechCraft.Core.Plugins/1.0";
		return result;
	}

	void FileScheme::hookNAMCreateRequest (IHookProxy_ptr proxy,
			QNetworkAccessManager*,
			QNetworkAccessManager::Operation *op,
			QIODevice**)
	{
		if (*op != QNetworkAccessManager::GetOperation)
			return;

		const QNetworkRequest& req = proxy->GetValue ("request").value<QNetworkRequest> ();
		const QUrl& url = req.url ();
		if (url.scheme () != "file" ||
				!QFileInfo (url.toLocalFile ()).isDir ())
			return;

		proxy->CancelDefault ();
		proxy->SetReturnValue (QVariant::fromValue<QNetworkReply*> (new SchemeReply (req, this)));
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_poshuku_filescheme,
		LeechCraft::Poshuku::FileScheme::FileScheme);

