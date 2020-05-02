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

#include "seekslider.h"
#include <cmath>
#include <util/util.h>
#include "engine/sourceobject.h"

namespace LC
{
namespace LMP
{
	SeekSlider::SeekSlider (SourceObject *source, QWidget *parent)
	: QWidget (parent)
	, Source_ (source)
	{
		Ui_.setupUi (this);

		connect (source,
				&SourceObject::currentSourceChanged,
				this,
				&SeekSlider::UpdateRanges);
		connect (source,
				&SourceObject::totalTimeChanged,
				this,
				&SeekSlider::UpdateRanges);
		connect (source,
				&SourceObject::tick,
				this,
				&SeekSlider::HandleCurrentPlayTime);
		connect (source,
				&SourceObject::stateChanged,
				this,
				&SeekSlider::HandleStateChanged);

		connect (Ui_.Slider_,
				&QSlider::sliderPressed,
				[this] { IsPressed_ = true; });
		connect (Ui_.Slider_,
				&QSlider::sliderReleased,
				[this] { IsPressed_ = false; });
	}

	void SeekSlider::HandleCurrentPlayTime (qint64 time)
	{
		auto niceTime = [] (qint64 time)
		{
			if (!time)
				return QString {};

			auto played = Util::MakeTimeFromLong (time / 1000);
			if (played.startsWith ("00:"))
				played = played.mid (3);
			return played;
		};
		Ui_.Played_->setText (niceTime (time));

		const auto remaining = Source_->GetRemainingTime ();
		Ui_.Remaining_->setText (remaining < 0 ? QString () : niceTime (remaining));

		if (!IsPressed_)
			Ui_.Slider_->setValue (time / 1000);
	}

	void SeekSlider::UpdateRanges ()
	{
		const auto newMax = Source_->GetTotalTime () / 1000;
		if (newMax <= Ui_.Slider_->value ())
			IgnoreNextValChange_ = true;

		Ui_.Slider_->setMaximum (newMax);
	}

	void SeekSlider::HandleStateChanged ()
	{
		const auto state = Source_->GetState ();
		switch (state)
		{
		case SourceState::Buffering:
		case SourceState::Playing:
		case SourceState::Paused:
			UpdateRanges ();
			HandleCurrentPlayTime (Source_->GetCurrentTime ());
			Ui_.Slider_->setEnabled (true);
			break;
		default:
			Ui_.Slider_->setRange (0, 0);
			Ui_.Slider_->setEnabled (false);
			Ui_.Played_->setText ({});
			Ui_.Remaining_->setText ({});
			break;
		}
	}

	void SeekSlider::on_Slider__valueChanged (int value)
	{
		value *= 1000;
		const double diff = value - Source_->GetCurrentTime ();
		if (std::abs (diff) < 1500 || IgnoreNextValChange_)
		{
			IgnoreNextValChange_ = false;
			return;
		}

		Source_->Seek (value);
	}
}
}
