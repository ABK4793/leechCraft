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

#include "descparser.h"
#include <QFile>
#include <QDomDocument>
#include <QtDebug>
#include <util/util.h>
#include <util/sll/qtutil.h>
#include <interfaces/azoth/iprovidecommands.h>

namespace LeechCraft
{
namespace Azoth
{
namespace MuCommands
{
	namespace
	{
		class MDParser
		{
			enum class State
			{
				None,
				Em,
				Code
			} State_ = State::None;

			struct Pattern
			{
				QString Str_;
				State Expected_;

				bool Reversible_;

				bool operator< (const Pattern& other) const
				{
					if (Str_ != other.Str_)
						return Str_ < other.Str_;

					if (Expected_ != other.Expected_)
						return static_cast<int> (Expected_) < static_cast<int> (other.Expected_);

					return Reversible_ < other.Reversible_;
				}
			};

			struct Rep
			{
				QString TagBase_;
				State NextState_;
			};

			const QMap<Pattern, Rep> Repls_ = Util::MakeMap<Pattern, Rep> ({
						{ { "_", State::None, true }, { "em", State::Em } },
						{ { "@", State::None, true }, { "code", State::Code } }
					});

			QString Body_;
		public:
			MDParser (const QString& body);

			QString operator() ();
		private:
			int HandleSubstr (const QPair<Pattern, Rep>& rule, int pos);
		};

		MDParser::MDParser (const QString& body)
		: Body_ { body }
		{
			Body_.replace ("\n", "<br/>");

			const auto& stlized = Util::Stlize<QPair> (Repls_);
			for (int i = 0; i < Body_.size (); ++i)
				for (const auto& pair : stlized)
				{
					if (Body_.mid (i, pair.first.Str_.size ()) != pair.first.Str_)
						continue;

					const auto res = HandleSubstr (pair, i);
					if (res > 0)
					{
						i += res;
						break;
					}
				}
		}

		QString MDParser::operator() ()
		{
			return Body_;
		}

		int MDParser::HandleSubstr (const QPair<Pattern, Rep>& rule, int pos)
		{
			const auto& pat = rule.first;
			const auto& rep = rule.second;

			QString tag;
			if (State_ == pat.Expected_)
			{
				tag = "<" + rep.TagBase_ + ">";
				State_ = rep.NextState_;
			}
			else if (pat.Reversible_ && State_ == rep.NextState_)
			{
				tag = "</" + rep.TagBase_ + ">";
				State_ = pat.Expected_;
			}
			else
				return 0;

			Body_.replace (pos, pat.Str_.size (), tag);
			return tag.size () - pat.Str_.size ();
		}
	}

	DescParser::DescParser ()
	{
		QFile file { ":/azoth/mucommands/resources/data/descriptions.xml" };
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open descriptions file"
					<< file.errorString ();
			return;
		}

		QDomDocument doc;
		QString msg;
		int line = 0;
		int column = 0;
		if (!doc.setContent (&file, &msg, &line, &column))
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot parse descriptions file"
					<< msg
					<< line
					<< column;
			return;
		}

		auto cmdElem = doc.documentElement ().firstChildElement ("command");
		while (!cmdElem.isNull ())
		{
			const auto& name = cmdElem.attribute ("name");

			const auto& descr = cmdElem.firstChildElement ("desc").text ();
			const auto& help = MDParser { cmdElem.firstChildElement ("help").text () } ();

			Cmd2Desc_ [name] = Desc { descr, help };

			cmdElem = cmdElem.nextSiblingElement ("command");
		}
	}

	void DescParser::operator() (StaticCommand& cmd) const
	{
		const auto& desc = Cmd2Desc_.value (cmd.Names_.first ());
		cmd.Description_ = desc.Description_;
		cmd.Help_ = desc.Help_;
	}
}
}
}
