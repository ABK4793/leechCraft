/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <functional>
#include <type_traits>
#include <iterator>
#include <QPair>
#include <QStringList>

namespace LC
{
namespace Util
{
	template<typename T>
	struct WrapType
	{
		using type = T;
	};

	template<typename T>
	using WrapType_t = typename WrapType<T>::type;

	template<>
	struct WrapType<QList<QString>>
	{
		using type = QStringList;
	};

	template<template<typename U> class Container, typename T1, typename T2, typename F>
	auto ZipWith (const Container<T1>& c1, const Container<T2>& c2, F f)
	{
		WrapType_t<Container<std::decay_t<std::invoke_result_t<F, T1, T2>>>> result;

		using std::begin;
		using std::end;

		auto i1 = begin (c1), e1 = end (c1);
		auto i2 = begin (c2), e2 = end (c2);
		for ( ; i1 != e1 && i2 != e2; ++i1, ++i2)
			result.push_back (f (*i1, *i2));
		return result;
	}

	template<typename T1, typename T2,
		template<typename U> class Container,
		template<typename U1, typename U2> class Pair = QPair>
	auto Zip (const Container<T1>& c1, const Container<T2>& c2) -> Container<Pair<T1, T2>>
	{
		return ZipWith (c1, c2,
				[] (const T1& t1, const T2& t2) -> Pair<T1, T2>
					{ return { t1, t2}; });
	}

	namespace detail
	{
		template<typename Res, typename T>
		void Append (Res& result, T&& val) noexcept
		{
			if constexpr (requires { result.push_back (std::forward<T> (val)); })
				result.push_back (std::forward<T> (val));
			else
				result.insert (std::forward<T> (val));
		}

		template<typename Container, typename T>
		struct Replace
		{
			using Type = QList<T>;
		};

		template<template<typename...> class Container, typename U, typename T>
		struct Replace<Container<U>, T>
		{
			using Type = Container<T>;
		};

		template<typename ResultContainer, typename Container, typename F>
		auto MapImpl (Container&& c, F f)
		{
			WrapType_t<ResultContainer> cont;
			for (auto&& t : c)
				Append (cont, std::invoke (f, t));
			return cont;
		}
	}

	// NOTE
	// The noexcept specifier here is somewhat misleading:
	// this function indeed _might_ throw if the underlying container throws
	// when appending an element, for any reason,
	// (be it a copy/move ctor throwing or failure to allocate memory).
	// Due to how LC is written and intended to be used,
	// such exceptions are not and should not be handled,
	// so they are fatal anyway.
	// Thus we're totally fine with std::unexpected() and the likes.
	template<typename Container, typename F>
	auto Map (Container&& c, F&& f) noexcept (noexcept (std::is_nothrow_invocable_v<F, decltype (*c.begin ())>))
	{
		using FRet_t = std::decay_t<decltype (std::invoke (f, *c.begin ()))>;
		return detail::MapImpl<typename detail::Replace<std::decay_t<Container>, FRet_t>::Type> (std::forward<Container> (c), std::forward<F> (f));
	}

	template<template<typename...> class Fallback, typename Container, typename F>
	auto MapAs (Container&& c, F&& f) noexcept (noexcept (std::is_nothrow_invocable_v<F, decltype (*c.begin ())>))
	{
		using FRet_t = std::decay_t<decltype (std::invoke (f, *c.begin ()))>;
		return detail::MapImpl<Fallback<FRet_t>> (std::forward<Container> (c), std::forward<F> (f));
	}

	template<typename T, template<typename U> class Container, typename F>
	Container<T> Filter (const Container<T>& c, F f)
	{
		Container<T> result;
		for (const auto& item : c)
			if (std::invoke (f, item))
				detail::Append (result, item);
		return result;
	}

	template<template<typename> class Container, typename T>
	Container<T> Concat (const Container<Container<T>>& containers)
	{
		Container<T> result;

		decltype (result.size ()) size {};
		for (const auto& cont : containers)
			size += cont.size ();
		result.reserve (size);

		for (const auto& cont : containers)
			std::copy (cont.begin (), cont.end (), std::back_inserter (result));
		return result;
	}

	template<template<typename> class Container, typename T>
	Container<T> Concat (Container<Container<T>>&& containers)
	{
		Container<T> result;

		decltype (result.size ()) size {};
		for (const auto& cont : containers)
			size += cont.size ();
		result.reserve (size);

		for (auto&& cont : containers)
			std::move (cont.begin (), cont.end (), std::back_inserter (result));
		return result;
	}

	template<template<typename...> class Container, typename... ContArgs>
	auto Concat (const Container<ContArgs...>& containers) -> std::decay_t<decltype (*containers.begin ())>
	{
		std::decay_t<decltype (*containers.begin ())> result;
		for (const auto& cont : containers)
			for (const auto& item : cont)
				detail::Append (result, item);
		return result;
	}

	template<typename Cont, typename F>
	auto ConcatMap (Cont&& c, F&& f)
	{
		return Concat (Map (std::forward<Cont> (c), std::forward<F> (f)));
	}

	template<template<typename> class Container, typename T>
	Container<Container<T>> SplitInto (size_t numChunks, const Container<T>& container)
	{
		Container<Container<T>> result;

		const size_t chunkSize = container.size () / numChunks;
		for (size_t i = 0; i < numChunks; ++i)
		{
			Container<T> subcont;
			const auto start = container.begin () + chunkSize * i;
			const auto end = start + chunkSize;
			std::copy (start, end, std::back_inserter (subcont));
			result.push_back (subcont);
		}

		const auto lastStart = container.begin () + chunkSize * numChunks;
		const auto lastEnd = container.end ();
		std::copy (lastStart, lastEnd, std::back_inserter (result.front ()));

		return result;
	}

	template<typename Cont>
	decltype (auto) Sorted (Cont&& cont)
	{
		std::sort (cont.begin (), cont.end ());
		return std::forward<Cont> (cont);
	}

	constexpr auto Id = [] (auto&& t) -> decltype (auto) { return std::forward<decltype (t)> (t); };

	template<typename R>
	auto ComparingBy (R r)
	{
		return [r] (const auto& left, const auto& right) { return std::invoke (r, left) < std::invoke (r, right); };
	}

	template<typename R>
	auto EqualityBy (R r)
	{
		return [r] (const auto& left, const auto& right) { return std::invoke (r, left) == std::invoke (r, right); };
	}

	constexpr auto Apply = [] (const auto& t) { return t (); };

	constexpr auto Fst = [] (const auto& pair) { return pair.first; };

	constexpr auto Snd = [] (const auto& pair) { return pair.second; };

	template<typename F>
	auto First (F&& f)
	{
		return [f = std::forward<F> (f)] (const auto& pair) { return std::invoke (f, pair.first); };
	}

	template<typename F>
	auto Second (F&& f)
	{
		return [f = std::forward<F> (f)] (const auto& pair) { return std::invoke (f, pair.second); };
	}
}
}
