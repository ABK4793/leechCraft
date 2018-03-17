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

#include "imgaste.h"
#include <QIcon>
#include <QBuffer>
#include <QUrl>
#include <QStandardItemModel>
#include <QMessageBox>
#include <QImageReader>
#include <QClipboard>
#include <QApplication>
#include <util/sys/mimedetector.h>
#include <util/threads/futures.h>
#include <util/sll/visitor.h>
#include <util/sll/either.h>
#include <util/xpc/util.h>
#include <util/util.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/entitytesthandleresult.h>
#include "hostingservice.h"
#include "poster.h"

namespace LeechCraft
{
namespace Imgaste
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("imgaste");
		Proxy_ = proxy;

		ReprModel_ = new QStandardItemModel { this };
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Imgaste";
	}

	void Plugin::Release ()
	{
	}

	QString Plugin::GetName () const
	{
		return "Imgaste";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Simple image uploader to imagebin services like dump.bitcheese.net.");
	}

	QIcon Plugin::GetIcon () const
	{
		return QIcon ();
	}

	EntityTestHandleResult Plugin::CouldHandle (const Entity& e) const
	{
		if (e.Mime_ != "x-leechcraft/data-filter-request")
			return {};

		const auto& image = e.Entity_.value<QImage> ();
		if (!image.isNull ())
			return EntityTestHandleResult { EntityTestHandleResult::PIdeal };

		const auto& localFile = e.Entity_.toUrl ().toLocalFile ();
		if (!QFile::exists (localFile))
			return {};

		if (Util::DetectFileMime (localFile).startsWith ("image/"))
			return EntityTestHandleResult { EntityTestHandleResult::PHigh };

		return {};
	}

	void Plugin::Handle (Entity e)
	{
		const auto& img = e.Entity_.value<QImage> ();
		const auto& localFile = e.Entity_.toUrl ().toLocalFile ();
		if (!img.isNull ())
			UploadImage (img, e);
		else if (QFile::exists (localFile))
			UploadFile (localFile, e);
		else
			qWarning () << Q_FUNC_INFO
					<< "unhandled entity"
					<< e.Entity_;
	}

	QString Plugin::GetFilterVerb () const
	{
		return tr ("Upload image");
	}

	namespace
	{
		boost::optional<IDataFilter::FilterVariant> ToFilterVariant (HostingService s,
				const ImageInfo& imageInfo)
		{
			const auto& hostingInfo = ToInfo (s);
			if (!hostingInfo.Accepts_ (imageInfo))
				return {};

			const auto& str = hostingInfo.Name_;
			return { { str.toUtf8 (), str, {}, {} } };
		}

		boost::optional<ImageInfo> GetImageInfo (const QVariant& data)
		{
			const auto& file = data.toUrl ().toLocalFile ();
			const QFileInfo fileInfo { file };
			if (fileInfo.exists ())
			{
				const quint64 filesize = fileInfo.size ();
				return { { filesize, QImageReader { file }.size () } };
			}
			else if (data.canConvert<QImage> ())
				return { { 0, data.value<QImage> ().size () } };
			else
				return {};
		}
	}

	QList<IDataFilter::FilterVariant> Plugin::GetFilterVariants (const QVariant& data) const
	{
		const auto& maybeInfo = GetImageInfo (data);
		if (!maybeInfo)
			return {};

		const auto& info = *maybeInfo;

		QList<IDataFilter::FilterVariant> result;
		for (const auto& item : GetAllServices ())
			if (const auto res = ToFilterVariant (item, info))
				result << *res;
		return result;
	}

	QAbstractItemModel* Plugin::GetRepresentation () const
	{
		return ReprModel_;
	}

	void Plugin::UploadFile (const QString& name, const Entity& e)
	{
		QFile file { name };
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open file:"
					<< file.errorString ();
			return;
		}

		const auto& format = QString::fromLatin1 (Util::DetectFileMime (name)).section ('/', 1, 1);

		UploadImpl (file.readAll (), e, format);
	}

	void Plugin::UploadImage (const QImage& img, const Entity& e)
	{
		const auto& format = e.Additional_.value ("Format", "PNG").toString ();

		QByteArray bytes;
		QBuffer buf (&bytes);
		buf.open (QIODevice::ReadWrite);
		if (!img.save (&buf,
					qPrintable (format),
					e.Additional_ ["Quality"].toInt ()))
		{
			qWarning () << Q_FUNC_INFO
					<< "save failed";
			return;
		}

		UploadImpl (buf.data (), e, format);
	}

	void Plugin::UploadImpl (const QByteArray& data, const Entity& e, const QString& format)
	{
		const auto& dataFilter = e.Additional_ ["DataFilter"].toString ();
		const auto& type = FromString (dataFilter);
		if (!type)
		{
			QMessageBox::critical (nullptr,
					"LeechCraft",
					tr ("Unknown upload service: %1.")
						.arg (dataFilter));
			return;
		}

		const auto& callback = e.Additional_ ["DataFilterCallback"].value<DataFilterCallback_f> ();

		const auto em = Proxy_->GetEntityManager ();

		auto poster = new Poster (*type,
				data,
				format,
				Proxy_,
				ReprModel_);
		Util::Sequence (this, poster->GetFuture ()) >>
				Util::Visitor
				{
					[callback, em] (const QString& url)
					{
						if (!callback)
						{
							QApplication::clipboard ()->setText (url, QClipboard::Clipboard);

							auto text = tr ("Image pasted: %1, the URL was copied to the clipboard")
									.arg ("<em>" + url + "</em>");
							em->HandleEntity (Util::MakeNotification ("Imgaste", text, PInfo_));
						}
						else
							callback (url);
					},
					Util::Visitor
					{
						[em] (const Poster::NetworkRequestError& error)
						{
							const auto& text = tr ("Image upload failed: %1")
									.arg (error.ErrorString_);
							em->HandleEntity (Util::MakeNotification ("Imgaste", text, PCritical_));
						},
						[em] (const Poster::ServiceAPIError&)
						{
							const auto& text = tr ("Image upload failed: service error.");
							em->HandleEntity (Util::MakeNotification ("Imgaste", text, PCritical_));
						}
					}
				};
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_imgaste, LeechCraft::Imgaste::Plugin);
