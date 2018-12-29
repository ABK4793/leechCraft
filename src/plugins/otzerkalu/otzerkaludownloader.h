/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2011  Minh Ngo
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

#ifndef PLUGINS_OTZERKALU_OTZERKALUDOWNLOADER_H
#define PLUGINS_OTZERKALU_OTZERKALUDOWNLOADER_H
#include <QObject>
#include <QUrl>
#include <QWebFrame>
#include <QWebElementCollection>
#include <interfaces/structures.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/core/icoreproxyfwd.h>

namespace LeechCraft
{
namespace Otzerkalu
{
	struct DownloadParams
	{
		QUrl DownloadUrl_;
		QString DestDir_;
		int RecLevel_ = 0;
		bool FromOtherSite_ = false;
	};

	struct FileData
	{
		QUrl Url_;
		QString Filename_;
		int RecLevel_ = 0;
	};

	class OtzerkaluDownloader : public QObject
	{
		Q_OBJECT

		const DownloadParams Param_;
		const ICoreProxy_ptr Proxy_;
		QSet<QString> DownloadedFiles_;
		int UrlCount_ = 0;
	public:
		OtzerkaluDownloader (const DownloadParams& param, const ICoreProxy_ptr&, QObject *parent = 0);
		void Begin ();
	private:
		QString Download (const QUrl&, int);
		QList<QUrl> CSSParser (const QString&) const;
		QString CSSUrlReplace (QString, const FileData&);
		bool HTMLReplace (QWebElement element, const FileData& data);
		bool WriteData (const QString& filename, const QString& data);
		void HandleJobFinished (const FileData& data);
	signals:
		void delegateEntity (const LeechCraft::Entity&, int*, QObject**);
		void gotEntity (const LeechCraft::Entity&);
		void fileDownloaded (int count);
		void mirroringFinished ();
	};
};
};

#endif // PLUGINS_OTZERKALU_OTZERKALUDOWNLOADER_H
