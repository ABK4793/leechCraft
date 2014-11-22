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

#include <functional>
#include <QObject>
#include "sllconfig.h"

namespace LeechCraft
{
namespace Util
{
	/** @brief Executes a given action after a given timeout.
	 *
	 * This class can be used to schedule execution of arbitrary
	 * functions after some arbitrary amount of time.
	 *
	 * The DelayedExecutor objects should be created via <code>new</code>
	 * on heap, and they will delete themselves after the corresponding
	 * action is executed.
	 *
	 * The typical usage is as follows:
	 *	\code
		new Util::DelayedExecutor
		{
			[] ()
			{
				// body of some lambda
			},
			interval
		};
		\endcode
	 * or
	 *	\code
	   new Util::DelayedExecutor
	   {
		   someCallable,
		   interval
	   };
	   \endcode
	 */
	class UTIL_SLL_API DelayedExecutor : QObject
	{
		Q_OBJECT
	public:
		typedef std::function<void ()> Actor_f;
	private:
		const Actor_f Actor_;
	public:
		/** @brief Constructs the delayed executor.
		 *
		 * Schedules the execution of \em action after a given \em
		 * timeout.
		 *
		 * If the \em timeout is 0, the \em action will be executed next
		 * time event loop is run.
		 *
		 * @param[in] action The action to execute.
		 * @param[in] timeout The timeout before executing the action.
		 */
		DelayedExecutor (Actor_f action, int timeout);
	private slots:
		void handleTimeout ();
	};

	inline void ExecuteLater (const DelayedExecutor::Actor_f& actor, int delay = 0)
	{
		new DelayedExecutor { actor, delay };
	}
}
}
