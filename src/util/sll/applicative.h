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

#include <boost/optional.hpp>
#include "oldcppkludges.h"

namespace LeechCraft
{
namespace Util
{
	template<typename T>
	struct InstanceApplicative;

	template<typename AF, typename AV>
	using GSLResult_t = typename InstanceApplicative<AF>::template GSLResult<AV>::Type_t;

	template<template<typename...> class Applicative, typename... Args, typename T>
	Applicative<Args..., T> Pure (const T& v)
	{
		return InstanceApplicative<Applicative<Args..., T>>::Pure (v);
	}

	template<typename AF, typename AV>
	GSLResult_t<AF, AV> GSL (const AF& af, const AV& av)
	{
		return InstanceApplicative<AF>::GSL (af, av);
	}

	template<typename AF, typename AV>
	auto operator* (const AF& af, const AV& av) -> decltype (GSL (af, av))
	{
		return GSL (af, av);
	}

	// Implementations
	template<typename T>
	struct InstanceApplicative<boost::optional<T>>
	{
		using Type_t = boost::optional<T>;

		template<typename>
		struct GSLResult;

		template<typename V>
		struct GSLResult<boost::optional<V>>
		{
			using Type_t = boost::optional<ResultOf_t<T (V)>>;
		};

		static Type_t Pure (const T& v)
		{
			return { v };
		}

		template<typename AV>
		static GSLResult_t<Type_t, AV> GSL (const Type_t& f, const AV& v)
		{
			if (!f || !v)
				return {};

			return { (*f) (*v) };
		}
	};
}
}
