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

#include "accountlogger.h"
#include <QFile>
#include <QDateTime>
#include <QtDebug>
#include <QDir>
#include <util/sys/paths.h>
#include "account.h"

namespace LeechCraft
{
namespace Snails
{
	AccountLogger::AccountLogger (const QString& accName, QObject *parent)
	: QObject { parent }
	, AccName_ { accName }
	{
	}

	void AccountLogger::SetEnabled (bool enabled)
	{
		Enabled_ = enabled;
	}

	void AccountLogger::Log (const QString& context, int connId, const QString& msg)
	{
		const auto& now = QDateTime::currentDateTime ();
		const auto& str = QString { "[%1] [%2] [%3]: %4" }
				.arg (now.toString ("dd.MM.yyyy HH:mm:ss.zzz"))
				.arg (context)
				.arg (connId)
				.arg (msg);

		QMetaObject::invokeMethod (this,
				"writeLog",
				Qt::QueuedConnection,
				Q_ARG (QString, str));

		emit gotLog (now, context, connId, msg);
	}

	void AccountLogger::writeLog (const QString& log)
	{
		if (!Enabled_)
			return;

		if (!File_)
		{
			const auto& path = Util::CreateIfNotExists ("snails/logs").filePath (AccName_ + ".log");
			File_ = std::make_shared<QFile> (path);
			if (!File_->open (QIODevice::WriteOnly))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to open"
						<< path
						<< "for writing, error:"
						<< File_->errorString ();
				return;
			}
		}

		if (File_->isOpen ())
		{
			File_->write (log.toUtf8 () + "\n");
			File_->flush ();
		}
	}
}
}
