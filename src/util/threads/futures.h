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
#include <boost/optional.hpp>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <util/sll/oldcppkludges.h>
#include <util/sll/slotclosure.h>
#include "concurrentexception.h"

namespace LeechCraft
{
namespace Util
{
	template<typename R, typename F, typename... Args>
	EnableIf_t<!std::is_same<R, void>::value>
		ReportFutureResult (QFutureInterface<R>& iface, F&& f, Args&&... args)
	{
		try
		{
			const auto result = Invoke (std::forward<F> (f), std::forward<Args> (args)...);
			iface.reportFinished (&result);
		}
		catch (const QtException_t& e)
		{
			iface.reportException (e);
			iface.reportFinished ();
		}
		catch (const std::exception& e)
		{
			iface.reportException (ConcurrentStdException { e });
			iface.reportFinished ();
		}
	}

	template<typename F, typename... Args>
	void ReportFutureResult (QFutureInterface<void>& iface, F&& f, Args&&... args)
	{
		try
		{
			Invoke (std::forward<F> (f), std::forward<Args> (args)...);
		}
		catch (const QtException_t& e)
		{
			iface.reportException (e);
		}
		catch (const std::exception& e)
		{
			iface.reportException (ConcurrentStdException { e });
		}

		iface.reportFinished ();
	}

	namespace detail
	{
		template<typename T>
		struct UnwrapFutureTypeBase {};

		template<typename T>
		struct UnwrapFutureTypeBase<QFuture<T>>
		{
			using type = T;
		};

		template<typename T>
		struct UnwrapFutureType : UnwrapFutureTypeBase<Decay_t<T>>
		{
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

		template<typename ResultHandler, typename RetType, typename = ResultOf_t<ResultHandler (RetType)>>
		constexpr bool IsCompatibleImpl (int)
		{
			return true;
		}

		template<typename, typename>
		constexpr bool IsCompatibleImpl (float)
		{
			return false;
		}

		template<typename ResultHandler, typename = ResultOf_t<ResultHandler ()>>
		constexpr bool IsCompatibleImplVoid (int)
		{
			return true;
		}

		template<typename>
		constexpr bool IsCompatibleImplVoid (float)
		{
			return false;
		}

		template<typename ResultHandler, typename RetType>
		constexpr bool IsCompatible ()
		{
			return std::is_same<void, RetType>::value ?
					IsCompatibleImplVoid<ResultHandler> (0) :
					IsCompatibleImpl<ResultHandler, RetType> (0);
		}
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

		static_assert (detail::IsCompatible<ResultHandler, RetType_t> (),
				"Executor's watcher type and result handler argument type are not compatible.");

		const auto watcher = new QFutureWatcher<RetType_t> { parent };

		new SlotClosure<DeleteLaterPolicy>
		{
			[watcher, rh]
			{
				watcher->deleteLater ();
				detail::HandlerInvoker<RetType_t, ResultHandler> {} (rh, watcher);
			},
			watcher,
			SIGNAL (finished ()),
			watcher
		};

		watcher->setFuture (f (args...));
	}

	namespace detail
	{
		/** @brief Incapsulates the sequencing logic of asynchronous
		 * actions.
		 *
		 * The objects of this class are expected to be created on heap.
		 * They will delete themselves automatically after the chain is
		 * walked (or an exception is thrown).
		 *
		 * @tparam Executor The type of the initial functor in the async
		 * call chain.
		 * @tparam Args The types of the arguments that should be passed
		 * to the \em Executor.
		 */
		template<typename Future>
		class Sequencer : public QObject
		{
		public:
			/** @brief The type instantinating the QFuture returned by the
			 * \em Executor.
			 */
			using RetType_t = UnwrapFutureType_t<Future>;
		private:
			Future Future_;
			QFutureWatcher<RetType_t> BaseWatcher_;
			QObject *LastWatcher_ = &BaseWatcher_;
		public:
			/** @brief Constructs the sequencer.
			 *
			 * @param[in] f The first action in the chain.
			 * @param[in] args The arguments to the action.
			 * @param[in] parent The parent object for the sequencer.
			 */
			Sequencer (const Future& future, QObject *parent)
			: QObject { parent }
			, Future_ { future }
			, BaseWatcher_ { this }
			{
			}

			/** @brief Starts the first action in the chain.
			 *
			 * All the actions should be chained before calling this
			 * method to avoid a race condition.
			 */
			void Start ()
			{
				BaseWatcher_.setFuture (Future_);
			}

			/** @brief Chains the given asynchronous action.
			 *
			 * The \em action is a functor callable with a single
			 * parameter of type \em ArgT and returning a value of type
			 * <code>QFuture<RetT></code> for some \em RetT.
			 *
			 * The parameter type \em ArgT should match exactly the
			 * "unwrapped" \em RetT for the previous call of Then() (or
			 * RetType_t if this is the second action in the asynchronous
			 * chain). Otherwise, an exception will be thrown at runtime.
			 *
			 * @note The SequenceProxy class takes care of compile-time
			 * type-checking of arguments and return types.
			 *
			 * @param[in] action The action to add to the sequence chain.
			 * @tparam RetT The type instantiating the return type
			 * <code>QFuture<RetT></code> of the \em action.
			 * @tparam ArgT The type of the argument passed to the
			 * \em action.
			 */
			template<typename RetT, typename ArgT>
			void Then (const std::function<QFuture<RetT> (ArgT)>& action)
			{
				const auto last = dynamic_cast<QFutureWatcher<ArgT>*> (LastWatcher_);
				if (!last)
				{
					deleteLater ();
					throw std::runtime_error { std::string { "invalid type in " } + Q_FUNC_INFO };
				}

				const auto watcher = new QFutureWatcher<RetT> { this };
				LastWatcher_ = watcher;

				new SlotClosure<DeleteLaterPolicy>
				{
					[this, last, watcher, action]
					{
						if (static_cast<QObject*> (last) != &BaseWatcher_)
							last->deleteLater ();
						watcher->setFuture (action (last->result ()));
					},
					last,
					SIGNAL (finished ()),
					last
				};
			}

			/** @brief Chains the given asynchronous action and closes the
			 * chain.
			 *
			 * The \em action is a functor callable with a single
			 * parameter of type \em ArgT and returning <code>void</code>.
			 *
			 * No more functors may be chained after adding a
			 * <code>void</code>-returning functor.
			 *
			 * The parameter type \em ArgT should match exactly the
			 * "unwrapped" \em RetT for the previous call of Then() (or
			 * RetType_t if this is the second action in the asynchronous
			 * chain). Otherwise, an exception will be thrown at runtime.
			 *
			 * @note The SequenceProxy class takes care of compile-time
			 * type-checking of arguments and return types.
			 *
			 * @tparam ArgT The type of the argument passed to the
			 * \em action.
			 */
			template<typename ArgT>
			void Then (const std::function<void (ArgT)>& action)
			{
				const auto last = dynamic_cast<QFutureWatcher<ArgT>*> (LastWatcher_);
				if (!last)
				{
					deleteLater ();
					throw std::runtime_error { std::string { "invalid type in " } + Q_FUNC_INFO };
				}

				new SlotClosure<DeleteLaterPolicy>
				{
					[last, action, this]
					{
						action (last->result ());
						deleteLater ();
					},
					LastWatcher_,
					SIGNAL (finished ()),
					LastWatcher_
				};
			}

			void Then (const std::function<void ()>& action)
			{
				const auto last = dynamic_cast<QFutureWatcher<void>*> (LastWatcher_);
				if (!last)
				{
					deleteLater ();
					throw std::runtime_error { std::string { "invalid type in " } + Q_FUNC_INFO };
				}

				new SlotClosure<DeleteLaterPolicy>
				{
					[last, action, this]
					{
						action ();
						deleteLater ();
					},
					LastWatcher_,
					SIGNAL (finished ()),
					LastWatcher_
				};
			}
		};

		template<typename T>
		using SequencerRetType_t = typename Sequencer<T>::RetType_t;

		struct EmptyDestructionTag;

		template<typename T>
		using IsEmptyDestr_t = std::is_same<EmptyDestructionTag, T>;

		template<typename Ret, typename DestrType, typename = EnableIf_t<IsEmptyDestr_t<DestrType>::value>>
		void InvokeDestructionHandler (const std::function<DestrType ()>&, QFutureInterface<Ret>&, float)
		{
		}

		template<typename Ret, typename DestrType, typename = EnableIf_t<!IsEmptyDestr_t<DestrType>::value>>
		void InvokeDestructionHandler (const std::function<DestrType ()>& handler, QFutureInterface<Ret>& iface, int)
		{
			const auto res = handler ();
			iface.reportFinished (&res);
		}

		/** @brief A proxy object allowing type-checked sequencing of
		 * actions and responsible for starting the initial action.
		 *
		 * SequenceProxy manages a Sequencer object, which itself is
		 * directly responsible for walking the chain of sequenced
		 * actions.
		 *
		 * Internally, objects of this class are reference-counted. As
		 * soon as the last instance is destroyed, the initial action is
		 * started.
		 *
		 * @tparam Ret The type \em T that <code>QFuture<T></code>
		 * returned by the last chained executor is specialized with.
		 * @tparam E0 The type of the first executor.
		 * @tparam A0 The types of the arguments to the executor \em E0.
		 */
		template<typename Ret, typename Future, typename DestructionTag>
		class SequenceProxy
		{
			template<typename, typename, typename>
			friend class SequenceProxy;

			std::shared_ptr<void> ExecuteGuard_;
			Sequencer<Future> * const Seq_;

			boost::optional<QFuture<Ret>> ThisFuture_;

			std::function<DestructionTag ()> DestrHandler_;

			SequenceProxy (const std::shared_ptr<void>& guard, Sequencer<Future> *seq,
					const std::function<DestructionTag ()>& destrHandler)
			: ExecuteGuard_ { guard }
			, Seq_ { seq }
			, DestrHandler_ { destrHandler }
			{
			}
		public:
			using Ret_t = Ret;

			/** @brief Constructs a sequencer proxy managing the given
			 * \em sequencer.
			 *
			 * @param[in] sequencer The sequencer to manage.
			 */
			SequenceProxy (Sequencer<Future> *sequencer)
			: ExecuteGuard_ { nullptr, [sequencer] (void*) { sequencer->Start (); } }
			, Seq_ { sequencer }
			{
			}

			/** @brief Copy-constructs from \em proxy.
			 *
			 * @param[in] proxy The proxy object to share the managed
			 * sequencer with.
			 */
			SequenceProxy (const SequenceProxy& proxy) = delete;

			/** @brief Move-constructs from \em proxy.
			 *
			 * @param[in] proxy The proxy object from which the sequencer
			 * should be borrowed.
			 */
			SequenceProxy (SequenceProxy&& proxy) = default;

			/** @brief Adds the functor \em f to the chain of actions.
			 *
			 * The functor \em f should return <code>QFuture<T0></code>
			 * when called with a value of type \em Ret. That is, the
			 * expression <code>f (std::declval<Ret> ())</code> should be
			 * well-formed, and, moreover, its return type should be
			 * <code>QFuture<T0><code> for some T0.
			 *
			 * @param[in] f The functor to chain.
			 * @return An object of type
			 * <code>SequencerProxy<T0, E0, A0></code> ready for chaining
			 * new functions.
			 * @tparam F The type of the functor to chain.
			 */
			template<typename F>
			auto Then (F&& f) -> SequenceProxy<UnwrapFutureType_t<decltype (f (std::declval<Ret> ()))>, Future, DestructionTag>
			{
				if (ThisFuture_)
					throw std::runtime_error { "SequenceProxy::Then(): cannot chain more after being converted to a QFuture" };

				Seq_->template Then<UnwrapFutureType_t<decltype (f (std::declval<Ret> ()))>, Ret> (f);
				return { ExecuteGuard_, Seq_, DestrHandler_ };
			}

			/** @brief Adds the funtor \em f to the chain of actions and
			 * closes the chain.
			 *
			 * The function \em f should return <code>void</code> when
			 * called with a value of type \em Ret.
			 *
			 * No more functors may be chained after adding a
			 * <code>void</code>-returning functor.
			 *
			 * @param[in] f The functor to chain.
			 * @tparam F The type of the functor to chain.
			 */
			template<typename F>
			auto Then (F&& f) -> EnableIf_t<std::is_same<void, decltype (f (std::declval<Ret> ()))>::value>
			{
				if (ThisFuture_)
					throw std::runtime_error { "SequenceProxy::Then(): cannot chain more after being converted to a QFuture" };

				Seq_->template Then<Ret> (f);
			}

			template<typename F>
			auto Then (F&& f) -> EnableIf_t<std::is_same<void, Ret>::value && std::is_same<void, decltype (f ())>::value>
			{
				if (ThisFuture_)
					throw std::runtime_error { "SequenceProxy::Then(): cannot chain more after being converted to a QFuture" };

				Seq_->Then (std::function<void ()> { f });
			}

			template<typename F>
			auto operator>> (F&& f) -> decltype (this->Then (std::forward<F> (f)))
			{
				return Then (std::forward<F> (f));
			}

			template<typename F>
			SequenceProxy<Ret, Future, ResultOf_t<F ()>> DestructionValue (F&& f)
			{
				static_assert (std::is_same<DestructionTag, EmptyDestructionTag>::value,
						"Destruction handling function has been already set.");

				return { ExecuteGuard_, Seq_, std::forward<F> (f) };
			}

			operator QFuture<Ret> ()
			{
				constexpr bool isEmptyDestr = std::is_same<DestructionTag, EmptyDestructionTag>::value;
				static_assert (std::is_same<DestructionTag, Ret>::value || isEmptyDestr,
						"Destruction handler's return type doesn't match expected future type.");

				if (ThisFuture_)
					return *ThisFuture_;

				QFutureInterface<Ret> iface;
				iface.reportStarted ();

				SlotClosure<DeleteLaterPolicy> *deleteGuard = nullptr;
				if (!isEmptyDestr)
				{
					// TODO C++14 capture the copy directly
					const auto destrHandler = DestrHandler_;
					deleteGuard = new SlotClosure<DeleteLaterPolicy>
					{
						[destrHandler, iface] () mutable
						{
							if (iface.isFinished ())
								return;

							InvokeDestructionHandler<Ret, DestructionTag> (destrHandler, iface, 0);
						},
						Seq_->parent (),
						SIGNAL (destroyed ()),
						Seq_
					};
				}

				Then ([deleteGuard, iface] (const Ret& ret) mutable
						{
							iface.reportFinished (&ret);

							delete deleteGuard;
						});

				const auto& future = iface.future ();
				ThisFuture_ = future;
				return future;
			}
		};
	}

	/** @brief Creates a sequencer that allows chaining multiple futures.
	 *
	 * This function creates a sequencer object that calls the given
	 * executor \em f with the given \em args, which must return a
	 * <code>QFuture<T></code> (or throw an exception) or
	 * <code>void</code>. The concrete object will be unwrapped from the
	 * <code>QFuture<T></code> and passed to the chained function, if any,
	 * and so on. The functors may also return <code>QFuture<void></code>,
	 * in which case the next action is expected to be invokable without
	 * any arguments.
	 *
	 * If a functor returns <code>void</code>, no further chaining is
	 * possible.
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
	template<typename T>
	detail::SequenceProxy<
			detail::SequencerRetType_t<QFuture<T>>,
			QFuture<T>,
			detail::EmptyDestructionTag
		>
		Sequence (QObject *parent, const QFuture<T>& future)
	{
		return { new detail::Sequencer<QFuture<T>> { future, parent } };
	}

	/** @brief Creates a ready future holding the given value.
	 *
	 * This function creates a ready future containing the value \em t.
	 * That is, calling <code>QFuture<T>::get()</code> on the returned
	 * future will not block.
	 *
	 * @param[in] t The value to keep in the future.
	 * @return The ready future with the value \em t.
	 *
	 * @tparam T The type of the value in the future.
	 */
	template<typename T>
	QFuture<T> MakeReadyFuture (const T& t)
	{
		QFutureInterface<T> iface;
		iface.reportStarted ();
		iface.reportFinished (&t);
		return iface.future ();
	}
}
}
