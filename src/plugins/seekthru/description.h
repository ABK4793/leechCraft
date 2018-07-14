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

#pragma once

#include <QStringList>
#include <QString>
#include <QVariantMap>
#include <QUrl>
#include <QMetaType>

namespace LeechCraft
{
namespace SeekThru
{
	struct UrlDescription
	{
		QString Template_;
		QString Type_;
		qint32 IndexOffset_;
		qint32 PageOffset_;

		QUrl MakeUrl (const QString&, const QHash<QString, QVariant>&) const;
	};

	QDataStream& operator<< (QDataStream&, const UrlDescription&);
	QDataStream& operator>> (QDataStream&, UrlDescription&);

	struct QueryDescription
	{
		enum Role
		{
			RoleRequest,
			RoleExample,
			RoleRelated,
			RoleCorrection,
			RoleSubset,
			RoleSuperset
		};

		Role Role_;
		QString Title_;
		qint32 TotalResults_;
		QString SearchTerms_;
		qint32 Count_;
		qint32 StartIndex_;
		qint32 StartPage_;
		QString Language_;
		QString InputEncoding_;
		QString OutputEncoding_;
	};

	QDataStream& operator<< (QDataStream&, const QueryDescription&);
	QDataStream& operator>> (QDataStream&, QueryDescription&);

	struct Description
	{
		enum class SyndicationRight
		{
			Open,
			Limited,
			Private,
			Closed
		};

		QString ShortName_;
		QString Description_;
		QList<UrlDescription> URLs_;
		QString Contact_;
		QStringList Tags_;
		QString LongName_;
		QList<QueryDescription> Queries_;
		QString Developer_;
		QString Attribution_;
		SyndicationRight Right_;
		bool Adult_;
		QStringList Languages_;
		QStringList InputEncodings_;
		QStringList OutputEncodings_;
	};

	QDataStream& operator<< (QDataStream&, const Description&);
	QDataStream& operator>> (QDataStream&, Description&);
}
}

Q_DECLARE_METATYPE (LeechCraft::SeekThru::Description)
