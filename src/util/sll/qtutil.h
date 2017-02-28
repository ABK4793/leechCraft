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

#include <tuple>
#include <boost/range.hpp>
#include "sllconfig.h"

namespace LeechCraft
{
namespace Util
{
	namespace detail
	{
		template<template<typename,
					template<typename, typename> class> class This,
				typename Iter, template<typename, typename> class PairType>
		using IteratorAdaptorBase = boost::iterator_adaptor<
				This<Iter, PairType>,
				Iter,
				PairType<decltype (Iter {}.key ()), decltype (Iter {}.value ())>,
				boost::use_default,
				PairType<decltype (Iter {}.key ()), decltype (Iter {}.value ())>
			>;

		template<typename Iter, template<typename, typename> class PairType>
		class StlAssocIteratorAdaptor : public IteratorAdaptorBase<StlAssocIteratorAdaptor, Iter, PairType>
		{
			friend class boost::iterator_core_access;

			using Super_t = IteratorAdaptorBase<detail::StlAssocIteratorAdaptor, Iter, PairType>;
		public:
			StlAssocIteratorAdaptor () = default;

			StlAssocIteratorAdaptor (Iter it)
			: Super_t { it }
			{
			}
		private:
			typename Super_t::reference dereference () const
			{
				return { this->base ().key (), this->base ().value () };
			}
		};

		template<typename Iter, typename Assoc, template<typename K, typename V> class PairType>
		struct StlAssocRange : private std::tuple<Assoc>
							 , public boost::iterator_range<StlAssocIteratorAdaptor<Iter, PairType>>
		{
		public:
			StlAssocRange (Assoc&& assoc)
			: std::tuple<Assoc> { std::move (assoc) }
			, boost::iterator_range<StlAssocIteratorAdaptor<Iter, PairType>> { std::get<0> (*this).begin (), std::get<0> (*this).end () }
			{
			}
		};

		template<typename Iter, typename Assoc, template<typename K, typename V> class PairType>
		struct StlAssocRange<Iter, Assoc&, PairType> : public boost::iterator_range<StlAssocIteratorAdaptor<Iter, PairType>>
		{
		public:
			StlAssocRange (Assoc& assoc)
			: boost::iterator_range<StlAssocIteratorAdaptor<Iter, PairType>> { assoc.begin (), assoc.end () }
			{
			}
		};
	}

	/** @brief Converts an Qt's associative sequence \em assoc to an
	 * STL-like iteratable range.
	 *
	 * This function takes an associative container \em assoc (one of
	 * Qt's containers like QHash and QMap) and returns a range with
	 * <code>value_type</code> equal to <code>PairType<K, V></code>.
	 *
	 * This way, both the key and the value of each pair in the \em assoc
	 * can be accessed in a range-for loop, for example.
	 *
	 * Example usage:
	 *	\code
		QMap<QString, int> someMap;
		for (const auto& pair : Util::Stlize (someMap))
			qDebug () << pair.first		// outputs a QString key
					<< pair.second;		// outputs an integer value corresponding to the key
		\endcode
	 *
	 * All kinds of accesses are supported: elements of a non-const
	 * container may be modified via the iterators in the returned range.
	 *
	 * @param[in] assoc The Qt's associative container to iterate over.
	 * @return A range with iterators providing access to both the key
	 * and the value via its <code>value_type</code>.
	 *
	 * @tparam PairType The type of the pairs that should be used in the
	 * resulting range's iterators' <code>value_type</code>.
	 * @tparam Assoc The type of the source Qt associative container.
	 */
	template<template<typename K, typename V> class PairType = std::pair, typename Assoc>
	auto Stlize (Assoc&& assoc) -> detail::StlAssocRange<decltype (assoc.begin ()), Assoc, PairType>
	{
		return { std::forward<Assoc> (assoc) };
	}

	UTIL_SLL_API QString Escape (const QString&);
}
}
