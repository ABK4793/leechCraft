/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "utiltest.h"
#include <QtTest>
#include "util.h"

QTEST_APPLESS_MAIN (LC::Util::UtilTest)

namespace LC::Util
{
	void UtilTest::testRgbOperator ()
	{
		QCOMPARE ("#abcdef"_rgb, QColor { "#abcdef" });
	}

	void UtilTest::testRgbOperatorConstexpr ()
	{
		constexpr QColor parsed = "#abcdef"_rgb;
		QCOMPARE (parsed, QColor { "#abcdef" });
	}
}
