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

#include "basehookinterconnector.h"
#include <QMetaMethod>
#include <QtDebug>

namespace LeechCraft
{
namespace Util
{
	BaseHookInterconnector::BaseHookInterconnector (QObject *parent)
	: QObject (parent)
	{
	}

	BaseHookInterconnector::~BaseHookInterconnector ()
	{
	}

	namespace
	{
		bool IsHookMethod (const QMetaMethod& method)
		{
			return method.parameterTypes ().value (0) == "LeechCraft::IHookProxy_ptr";
		}

#if QT_VERSION >= 0x050000
		QHash<QByteArray, QMetaMethod> BuildHookSlots (const QObject *obj)
		{
			const auto objMo = obj->metaObject ();

			QHash<QByteArray, QMetaMethod> hookSlots;
			for (int i = 0, size = objMo->methodCount (); i < size; ++i)
			{
				const auto& method = objMo->method (i);

				if (!IsHookMethod (method))
					continue;

				const auto& name = method.name ();
				if (hookSlots.contains (name))
					qWarning () << Q_FUNC_INFO
							<< obj
							<< "has several hook methods with name"
							<< name;

				hookSlots [name] = method;
			}

			return hookSlots;
		}
#endif

		void CheckMatchingSigs (const QObject *snd, const QObject *rcv)
		{
#if QT_VERSION >= 0x050000
			if (!qEnvironmentVariableIsSet ("LC_VERBOSE_HOOK_CHECKS"))
				return;

			const auto& hookSlots = BuildHookSlots (snd);

			const auto rcvMo = rcv->metaObject ();

			for (int i = 0, size = rcvMo->methodCount (); i < size; ++i)
			{
				const auto& rcvMethod = rcvMo->method (i);
				if (!IsHookMethod (rcvMethod))
					continue;

				const auto& rcvName = rcvMethod.name ();
				if (!hookSlots.contains (rcvName))
				{
					qWarning () << Q_FUNC_INFO
							<< "no method matching method"
							<< rcvName
							<< "(receiver"
							<< rcv
							<< ") in sender object"
							<< snd;
					continue;
				}

				if (!QMetaObject::checkConnectArgs (hookSlots [rcvName], rcvMethod))
					qWarning () << Q_FUNC_INFO
							<< "incompatible signatures for hook"
							<< rcvName
							<< "in"
							<< snd
							<< "and"
							<< rcv;
			}
#else
			Q_UNUSED (sender)
			Q_UNUSED (receiver)
#endif
		}

#define LC_N(a) (QMetaObject::normalizedSignature(a))
#define LC_TOSLOT(a) ('1' + QByteArray(a))
#define LC_TOSIGNAL(a) ('2' + QByteArray(a))
		void ConnectHookSignals (QObject *sender, QObject *receiver, bool destSlot)
		{
			if (destSlot)
				CheckMatchingSigs (sender, receiver);

			const QMetaObject *mo = sender->metaObject ();
			for (int i = 0, size = mo->methodCount (); i < size; ++i)
			{
				QMetaMethod method = mo->method (i);
				if (method.methodType () != QMetaMethod::Signal)
					continue;

				if (!IsHookMethod (method))
					continue;

#if QT_VERSION >= 0x050000
				const auto& signature = method.methodSignature ();
#else
				const auto& signature = method.signature ();
#endif

				if (receiver->metaObject ()->indexOfMethod (LC_N (signature)) == -1)
				{
					if (!destSlot)
					{
						qWarning () << Q_FUNC_INFO
								<< "not found meta method for"
								<< signature
								<< "in receiver object"
								<< receiver;
					}
					continue;
				}

				if (!QObject::connect (sender,
						LC_TOSIGNAL (signature),
						receiver,
						destSlot ? LC_TOSLOT (signature) : LC_TOSIGNAL (signature),
						Qt::UniqueConnection))
				{
					qWarning () << Q_FUNC_INFO
							<< "connect for"
							<< sender
							<< "->"
							<< receiver
							<< ":"
							<< signature
							<< "failed";
				}
			}
		}
#undef LC_N
	};

	void BaseHookInterconnector::AddPlugin (QObject *plugin)
	{
		Plugins_.push_back (plugin);

		ConnectHookSignals (this, plugin, true);
	}

	void BaseHookInterconnector::RegisterHookable (QObject *object)
	{
		ConnectHookSignals (object, this, false);
	}
}
}
