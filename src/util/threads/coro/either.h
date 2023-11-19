/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <util/sll/either.h>
#include "task.h"

namespace LC::Util
{
	template<typename L, std::default_initializable R>
	struct IgnoreLeft
	{
		Either<L, R> Result_;

		bool await_ready () const noexcept
		{
			return true;
		}

		void await_suspend (auto) const noexcept
		{
		}

		R await_resume () const noexcept
		{
			return RightOr (Result_, R {});
		}
	};

	template<typename L, typename R>
	IgnoreLeft (Either<L, R>) -> IgnoreLeft<L, R>;

	namespace detail
	{
		template<typename ErrorHandler>
		struct EitherAwaiterErrorHandler
		{
			ErrorHandler Handler_;

			template<typename L>
			void HandleError (L&& left)
			{
				Handler_ (std::forward<L> (left));
			}
		};

		template<>
		struct EitherAwaiterErrorHandler<void>
		{
			void HandleError (auto&&)
			{
			}
		};

		template<typename L, typename R, typename ErrorHandler = void>
		struct EitherAwaiter
		{
			Either<L, R> Either_;
			EitherAwaiterErrorHandler<ErrorHandler> Handler_ {};

			bool await_ready () const noexcept
			{
				return Either_.IsRight ();
			}

			template<typename Promise>
			void await_suspend (std::coroutine_handle<Promise> handle)
			{
				Handler_.HandleError (Either_.GetLeft ());

				[] (auto handle, auto either) -> Task<void>
				{
					auto& promise = handle->promise ();
					if constexpr (Promise::IsVoid)
						promise.return_void ();
					else
						promise.return_value (Promise::ReturnType_t::Left (either->GetLeft ()));

					co_await promise.final_suspend ();
					handle->destroy ();
				} (&handle, &Either_);
			}

			R await_resume () const noexcept
			{
				return Either_.GetRight ();
			}
		};
	}

	template<typename L, typename R, typename F>
		requires std::invocable<F, const L&>
	Util::detail::EitherAwaiter<L, R, F> WithHandler (const Util::Either<L, R>& either, F&& errorHandler)
	{
		return { either, { std::forward<F> (errorHandler) } };
	}
}

namespace LC
{
	template<typename L, typename R>
	Util::detail::EitherAwaiter<L, R> operator co_await (const Util::Either<L, R>& either)
	{
		return { either };
	}
}
