/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2012  Oleg Linkin
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

#include "utils.h"
#include <QUrl>
#include <QtDebug>
#include <util/sll/visitor.h>
#include <util/sll/either.h>
#include <util/xpc/util.h>
#include <interfaces/core/ientitymanager.h>

namespace LeechCraft
{
namespace NetStoreManager
{
namespace Utils
{
	QStringList ScanDir (QDir::Filters filter, const QString& path, bool recursive)
	{
		QDir baseDir (path);
		QStringList paths;
		for (const auto& entry : baseDir.entryInfoList (filter))
		{
			paths << entry.absoluteFilePath ();
			if (recursive &&
					entry.isDir ())
				paths << ScanDir (filter, entry.absoluteFilePath (), recursive);
		}
		return paths;
	}

	bool RemoveDirectoryContent (const QString& dirPath)
	{
		bool result = true;
		QDir dir (dirPath);

		if (dir.exists (dirPath))
		{
			for (const auto& info : dir.entryInfoList (QDir::NoDotAndDotDot | QDir::AllEntries))
			{
				if (info.isDir ())
					result = RemoveDirectoryContent (info.absoluteFilePath ());
				else
					result = QFile::remove (info.absoluteFilePath ());

				if (!result)
					return result;
			}

			result = dir.rmdir (dirPath);
		}

		return result;
	}

	std::function<void (ISupportFileListings::RequestUrlResult_t)> HandleRequestFileUrlResult (IEntityManager *entityMgr,
			const QString& errorText,
			const std::function<void (QUrl)>& urlHandler)
	{
		return Util::Visitor
		{
			std::move (urlHandler),
			Util::Visitor
			{
				[] (const ISupportFileListings::UserCancelled&) {},
				[] (const ISupportFileListings::InvalidItem&)
				{
					qWarning () << Q_FUNC_INFO
							<< "invalid item";
				},
				[=] (const QString& errStr)
				{
					const auto& e = Util::MakeNotification ("NetStoreManager",
							errorText + " " + errStr,
							Priority::Critical);
					entityMgr->HandleEntity (e);
				}
			}
		};
	}
}
}
}
