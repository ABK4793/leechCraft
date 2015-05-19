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

#include <type_traits>
#include <functional>
#include <memory>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include "slotclosure.h"

namespace LeechCraft
{
namespace Util
{
	namespace detail
	{
		template<typename T>
		struct UnwrapFutureType;

		template<typename T>
		struct UnwrapFutureType<QFuture<T>>
		{
			typedef T type;
		};

		template<typename T>
		using UnwrapFutureType_t = typename UnwrapFutureType<T>::type;

		template<typename T>
		struct IsFuture
		{
			constexpr static bool Result_ = false;
		};

		template<typename T>
		struct IsFuture<QFuture<T>>
		{
			constexpr static bool Result_ = true;
		};

		template<typename RetType, typename ResultHandler>
		struct HandlerInvoker
		{
			void operator() (const ResultHandler& rh, QFutureWatcher<RetType> *watcher) const
			{
				rh (watcher->result ());
			}
		};

		template<typename ResultHandler>
		struct HandlerInvoker<void, ResultHandler>
		{
			void operator() (const ResultHandler& rh, QFutureWatcher<void>*) const
			{
				rh ();
			}
		};
	}

	/** @brief Runs a QFuture-returning function and feeding the future
	 * to a handler when it is ready.
	 *
	 * This function creates a <code>QFutureWatcher</code> of a type
	 * compatible with the QFuture type returned from the \em f, makes
	 * sure that \em rh handler is invoked when the future finishes,
	 * and then invokes the \em f with the given list of \em args (that
	 * may be empty).
	 *
	 * \em rh should accept a single argument of the same type \em T
	 * that is wrapped in a <code>QFuture</code> returned by the \em f
	 * (that is, \em f should return <code>QFuture<T></code>).
	 *
	 * @param[in] f A callable that should be executed, taking the
	 * arguments \em args and returning a <code>QFuture<T></code> for
	 * some \em T.
	 * @param[in] rh A callable that will be invoked when the future
	 * finishes, that should be callable with a single argument of type
	 * \em T.
	 * @param[in] parent The parent object for all QObject-derived
	 * classes created in this function, may be a <code>nullptr</code>.
	 * @param[in] args The arguments to be passed to the callable \em f.
	 */
	template<typename Executor, typename ResultHandler, typename... Args>
	void ExecuteFuture (Executor f, ResultHandler rh, QObject *parent, Args... args)
	{
		static_assert (detail::IsFuture<decltype (f (args...))>::Result_,
				"The passed functor should return a QFuture.");

		// Don't replace result_of with decltype, this triggers a gcc bug leading to segfault:
		// http://leechcraft.org:8080/job/leechcraft/=debian_unstable/1998/console
		using RetType_t = detail::UnwrapFutureType_t<typename std::result_of<Executor (Args...)>::type>;
		const auto watcher = new QFutureWatcher<RetType_t> { parent };

		new SlotClosure<DeleteLaterPolicy>
		{
			[watcher, rh] { detail::HandlerInvoker<RetType_t, ResultHandler> {} (rh, watcher); },
			watcher,
			SIGNAL (finished ()),
			watcher
		};

		watcher->setFuture (f (args...));
	}

	namespace detail
	{
		template<typename Executor, typename... Args>
		class Sequencer : public QObject
		{
		public:
			using FutureType_t = typename std::result_of<Executor (Args...)>::type;
			using RetType_t = UnwrapFutureType_t<FutureType_t>;
		private:
			const std::function<FutureType_t ()> Functor_;
			QFutureWatcher<RetType_t> BaseWatcher_;
			QObject *LastWatcher_ = &BaseWatcher_;
		public:
			Sequencer (Executor f, Args... args, QObject *parent)
			: QObject { parent }
			, Functor_ { [f, args...] { return f (args...); } }
			, BaseWatcher_ { this }
			{
			}

			void Start ()
			{
				BaseWatcher_.setFuture (Functor_ ());
			}

			template<typename RetT, typename ArgT>
			void Then (const std::function<QFuture<RetT> (ArgT)>& cont)
			{
				const auto last = dynamic_cast<QFutureWatcher<ArgT>*> (LastWatcher_);
				if (!last)
					throw std::runtime_error { std::string { "invalid type in " } + Q_FUNC_INFO };

				const auto watcher = new QFutureWatcher<RetT> { this };
				LastWatcher_ = watcher;

				new SlotClosure<DeleteLaterPolicy>
				{
					[this, last, watcher, cont]
					{
						if (last != &BaseWatcher_)
							last->deleteLater ();
						watcher->setFuture (cont (last->result ()));
					},
					last,
					SIGNAL (finished ()),
					last
				};
			}

			template<typename T>
			void Then (const std::function<void (T)>& cont)
			{
				const auto last = dynamic_cast<QFutureWatcher<T>*> (LastWatcher_);
				if (!last)
					throw std::runtime_error { std::string { "invalid type in " } + Q_FUNC_INFO };

				new SlotClosure<DeleteLaterPolicy>
				{
					[last, cont, this]
					{
						cont (last->result ());
						deleteLater ();
					},
					LastWatcher_,
					SIGNAL (finished ()),
					LastWatcher_
				};
			}
		};

		/** @brief A proxy object allowing type-checked sequencing of actions.
		 */
		template<typename Ret, typename E0, typename... A0>
		class SequenceProxy
		{
			std::shared_ptr<void> ExecuteGuard_;
			Sequencer<E0, A0...> * const Seq_;

			SequenceProxy (const std::shared_ptr<void>& guard, Sequencer<E0, A0...> *seq)
			: ExecuteGuard_ { guard }
			, Seq_ { seq }
			{
			}
		public:
			using Ret_t = Ret;

			SequenceProxy (Sequencer<E0, A0...> *seq)
			: ExecuteGuard_ { nullptr, [seq] (void*) { seq->Start (); } }
			, Seq_ { seq }
			{
			}

			SequenceProxy (const SequenceProxy&) = default;
			SequenceProxy (SequenceProxy&&) = default;

			template<typename F>
			auto Then (const F& f) -> SequenceProxy<UnwrapFutureType_t<decltype (f (std::declval<Ret> ()))>, E0, A0...>
			{
				Seq_->template Then<UnwrapFutureType_t<decltype (f (std::declval<Ret> ()))>, Ret> (f);
				return { ExecuteGuard_, Seq_ };
			}

			template<typename F>
			auto Then (const F& f) -> typename std::enable_if<std::is_same<void, decltype (f (std::declval<Ret> ()))>::value>::type
			{
				Seq_->template Then<Ret> (f);
			}
		};
	}

	/** @brief Creates a sequencer that allows chaining multiple futures.
	 *
	 * This function creates a sequencer object that calls the given
	 * executor \em f with the given \em args, which must return a
	 * <code>QFuture<T></code> (or throw an exception) or void. The
	 * concrete object will be unwrapped <code>QFuture<T></code> and
	 * passed to the chained function, if any, and so on.
	 *
	 * The functions are chained via the detail::SequenceProxy::Then()
	 * method.
	 *
	 * The sequencer object is reference-counted internally, and it
	 * invokes the executor \em f after the last instance of this
	 * sequencer is destroyed.
	 *
	 * A \em parent QObject controls the lifetime of the sequencer: as
	 * soon as it is destroyed, the sequencer is destroyed as well, and
	 * all pending actions are cancelled (note, the currently executing
	 * action will still continue to execute). This parameter is optional
	 * and may be <code>nullptr</code>.
	 *
	 * A sample usage may look like:
	 * \code
		Util::Sequence (this,
					[this, &]
					{
						return QtConcurrent::run ([this, &]
								{
									const auto& contents = file->readAll ();
									file->close ();
									file->remove ();
									return DoSomethingWith (contents);
								});
					})
				.Then ([this, url, script] (const QString& contents)
					{
						const auto& result = Parse (contents);
						if (result.isEmpty ())
						{
							qWarning () << Q_FUNC_INFO
									<< "empty result for"
									<< url;
							return;
						}

						const auto id = DoSomethingSynchronouslyWith (result);
						emit gotResult (id);
					});
	   \endcode
	 *
	 * @param[in] parent The parent object of the sequencer (may be
	 * <code>nullptr</code>.
	 * @param[in] f The executor to run when chaining is finished.
	 * @param[in] args The arguments to pass to \em f.
	 * @return The sequencer object.
	 * @tparam Executor The type of the executor object.
	 * @tparam Args The types of the arguments for the \em Executor, if
	 * any.
	 *
	 * @sa detail::SequenceProxy
	 */
	template<typename Executor, typename... Args>
	detail::SequenceProxy<typename detail::Sequencer<Executor, Args...>::RetType_t, Executor, Args...> Sequence (QObject *parent, Executor f, Args... args)
	{
		return { new detail::Sequencer<Executor, Args...> { f, args..., parent } };
	}
}
}
