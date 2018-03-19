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

#include "volumenotifycontroller.h"
#include <QTimer>
#include <util/xpc/util.h>
#include <interfaces/core/ientitymanager.h>
#include "engine/output.h"
#include "core.h"

namespace LeechCraft
{
namespace LMP
{
	VolumeNotifyController::VolumeNotifyController (Output *out, QObject *parent)
	: QObject (parent)
	, Output_ (out)
	, NotifyTimer_ (new QTimer (this))
	{
		NotifyTimer_->setSingleShot (true);
		NotifyTimer_->setInterval (200);
		connect (NotifyTimer_,
				SIGNAL (timeout ()),
				this,
				SLOT (notify ()));
	}

	void VolumeNotifyController::volumeUp ()
	{
		Output_->setVolume (Output_->GetVolume () + 0.05);

		NotifyTimer_->start ();
	}

	void VolumeNotifyController::volumeDown ()
	{
		const auto val = std::max (Output_->GetVolume () - 0.05, 0.);
		Output_->setVolume (val);

		NotifyTimer_->start ();
	}

	void VolumeNotifyController::notify ()
	{
		auto e = Util::MakeNotification ("LMP",
				tr ("LMP volume has been changed to %1%.")
					.arg (static_cast<int> (Output_->GetVolume () * 100)),
				Priority::Info);
		e.Additional_ ["org.LC.AdvNotifications.SenderID"] = "org.LeechCraft.LMP";
		e.Additional_ ["org.LC.AdvNotifications.EventID"] = "VolumeChange";
		Core::Instance ().GetProxy ()->GetEntityManager ()->HandleEntity (e);
	}
}
}
