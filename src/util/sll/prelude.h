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
#include <type_traits>
#include <iterator>
#include <QPair>
#include <QStringList>
#include <boost/optional.hpp>

namespace boost
{
	template<typename>
	class iterator_range;
}

namespace LeechCraft
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
	auto ZipWith (const Container<T1>& c1, const Container<T2>& c2, F f) -> WrapType_t<Container<std::decay_t<std::result_of_t<F (T1, T2)>>>>
	{
		WrapType_t<Container<std::decay_t<std::result_of_t<F (T1, T2)>>>> result;

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
		void Append (Res& result, T&& val, decltype (result.push_back (std::forward<T> (val)))* = nullptr)
		{
			result.push_back (std::forward<T> (val));
		}

		template<typename Res, typename T>
		void Append (Res& result, T&& val, decltype (result.insert (std::forward<T> (val)))* = nullptr)
		{
			result.insert (std::forward<T> (val));
		}

		template<typename>
		struct CountArgs
		{
			static const size_t ArgsCount = 0;
		};

		template<template<typename...> class Container, typename... Args>
		struct CountArgs<Container<Args...>>
		{
			static const size_t ArgsCount = sizeof... (Args);
		};

		template<typename C>
		constexpr bool IsSimpleContainer ()
		{
			return CountArgs<std::decay_t<C>>::ArgsCount == 1;
		}

		template<typename Container, typename T>
		struct Replace
		{
			using Type = struct Fail;
		};

		template<template<typename> class Container, typename U, typename T>
		struct Replace<Container<U>, T>
		{
			using Type = Container<T>;
		};

		template<typename>
		struct IsNotBrokenSFINAE : std::false_type {};

		template<typename T>
		struct IsNotBrokenSFINAE<boost::iterator_range<T>> : std::true_type {};

		template<typename T>
		constexpr bool IsNotBrokenSFINAE_v = IsNotBrokenSFINAE<T> {};

		template<template<typename...> class Fallback, bool ForceFallback, typename Container, typename F>
		auto MapImpl (Container&& c, F f)
		{
			using FRet_t = std::decay_t<decltype (std::invoke (f, *c.begin ()))>;
			static_assert (!std::is_same<void, FRet_t> {}, "The function shall not return void.");

			using DecayContainer_t = std::decay_t<Container>;

			using ResultCont_t = std::conditional_t<
					!ForceFallback &&
						detail::IsSimpleContainer<DecayContainer_t> () &&
						!detail::IsNotBrokenSFINAE_v<DecayContainer_t>,
					typename detail::Replace<DecayContainer_t, FRet_t>::Type,
					Fallback<FRet_t>>;

			WrapType_t<ResultCont_t> cont;
			for (auto&& t : c)
				detail::Append (cont, std::invoke (f, t));
			return cont;
		}
	}

	template<typename Container, typename F>
	auto Map (Container&& c, F f)
	{
		return detail::MapImpl<QList, false> (std::forward<Container> (c), std::forward<F> (f));
	}

	template<template<typename...> class Fallback, typename Container, typename F>
	auto MapAs (Container&& c, F&& f)
	{
		return detail::MapImpl<Fallback, true> (std::forward<Container> (c), std::forward<F> (f));
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
		for (const auto& cont : containers)
			std::copy (cont.begin (), cont.end (), std::back_inserter (result));
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

	template<template<typename Pair, typename... Rest> class Cont, template<typename K, typename V> class Pair, typename K, typename V, typename KV, typename... Rest>
	boost::optional<V> Lookup (const KV& key, const Cont<Pair<K, V>, Rest...>& cont)
	{
		for (const auto& pair : cont)
			if (pair.first == key)
				return pair.second;

		return {};
	}

	template<typename Cont>
	Cont Sorted (Cont&& cont)
	{
		std::sort (cont.begin (), cont.end ());
		return std::move (cont);
	}

	const auto Id = [] (const auto& t) { return t; };

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

	const auto Apply = [] (const auto& t) { return t (); };

	const auto Fst = [] (const auto& pair) { return pair.first; };

	const auto Snd = [] (const auto& pair) { return pair.second; };

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

	template<typename F>
	auto Flip (F&& f)
	{
		return [f = std::move (f)] (auto&& left, auto&& right)
		{
			return f (std::forward<decltype (right)> (right),
					std::forward<decltype (left)> (left));
		};
	}
}
}
