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

#include "gstutil.h"
#include <QMap>
#include <QString>
#include <QTextCodec>
#include <QtDebug>

#ifdef WITH_LIBGUESS
extern "C"
{
#include <libguess/libguess.h>
}
#endif

#include <gst/gst.h>
#include "../../xmlsettingsmanager.h"

namespace LeechCraft
{
namespace LMP
{
namespace GstUtil
{
	void AddGhostPad (GstElement *from, GstElement *to, const char *name)
	{
		auto pad = gst_element_get_static_pad (from, name);
		auto ghostPad = gst_ghost_pad_new (name, pad);
		gst_pad_set_active (ghostPad, TRUE);
		gst_element_add_pad (to, ghostPad);
		gst_object_unref (pad);
	}

	namespace
	{
		void FixEncoding (QString& out, const gchar *origStr, const QString& region)
		{
#ifdef WITH_LIBGUESS
			const auto& cp1252 = QTextCodec::codecForName ("CP-1252")->fromUnicode (origStr);
			if (cp1252.isEmpty ())
				return;

			const auto encoding = libguess_determine_encoding (cp1252.constData (),
					cp1252.size (), region.toUtf8 ().constData ());
			if (!encoding)
				return;

			auto codec = QTextCodec::codecForName (encoding);
			if (!codec)
			{
				qWarning () << Q_FUNC_INFO
						<< "no codec for encoding"
						<< encoding;
				return;
			}

			const auto& proper = codec->toUnicode (cp1252.constData ());
			if (proper.isEmpty ())
				return;

			int origQCount = 0;
			while (*origStr)
				if (*(origStr++) == '?')
					++origQCount;

			if (origQCount >= proper.count ('?'))
				out = proper;
#else
			Q_UNUSED (out);
			Q_UNUSED (origStr);
#endif
		}

		struct TagFunctionData
		{
			TagMap_t& Map_;
			QString Region_;
		};

		void TagFunction (const GstTagList *list, const gchar *tag, gpointer rawData)
		{
			const auto& data = static_cast<TagFunctionData*> (rawData);
			auto& map = data->Map_;

			const auto& tagName = QString::fromUtf8 (tag).toLower ();
			auto& valList = map [tagName];

			switch (gst_tag_get_type (tag))
			{
			case G_TYPE_STRING:
			{
				gchar *str = nullptr;
				gst_tag_list_get_string (list, tag, &str);
				valList = QString::fromUtf8 (str);

				const auto recodingEnabled = !data->Region_.isEmpty ();
				if (recodingEnabled &&
						(tagName == "title" || tagName == "album" || tagName == "artist"))
					FixEncoding (valList, str, data->Region_);

				g_free (str);
				break;
			}
			case G_TYPE_BOOLEAN:
			{
				int val = 0;
				gst_tag_list_get_boolean (list, tag, &val);
				valList = QString::number (val);
				break;
			}
			case G_TYPE_INT:
			{
				int val = 0;
				gst_tag_list_get_int (list, tag, &val);
				valList = QString::number (val);
				break;
			}
			case G_TYPE_UINT:
			{
				uint val = 0;
				gst_tag_list_get_uint (list, tag, &val);
				valList = QString::number (val);
				break;
			}
			case G_TYPE_FLOAT:
			{
				float val = 0;
				gst_tag_list_get_float (list, tag, &val);
				valList = QString::number (val);
				break;
			}
			case G_TYPE_DOUBLE:
			{
				double val = 0;
				gst_tag_list_get_double (list, tag, &val);
				valList = QString::number (val);
				break;
			}
			default:
				break;
			}
		}
	}

	bool ParseTagMessage (GstMessage *msg, TagMap_t& map, const QString& region)
	{
		GstTagList *tagList = nullptr;
		gst_message_parse_tag (msg, &tagList);
		if (!tagList)
			return false;

		TagFunctionData data { map, region };

		gst_tag_list_foreach (tagList,
				TagFunction,
				&data);
#if GST_VERSION_MAJOR < 1
		gst_tag_list_free (tagList);
#else
		gst_tag_list_unref (tagList);
#endif
		return true;
	}

	namespace
	{
		struct CallbackData
		{
			const std::function<void ()> Functor_;
			GstPad * const SinkPad_;
			guint ID_;
		};

#if GST_VERSION_MAJOR >= 1
		GstPadProbeReturn ProbeHandler (GstPad *pad, GstPadProbeInfo*, gpointer cbDataPtr)
		{
			const auto cbData = static_cast<CallbackData*> (cbDataPtr);
			cbData->Functor_ ();
			delete cbData;
			return GST_PAD_PROBE_REMOVE;
		}
#else
		gboolean EventProbeHandler (GstPad *pad, GstEvent *event, CallbackData *cbData)
		{
			if (GST_EVENT_TYPE (event) != GST_EVENT_EOS)
				return TRUE;
			qDebug () << Q_FUNC_INFO << "eos";
			gst_pad_remove_event_probe (pad, cbData->ID_);

			cbData->Functor_ ();
			delete cbData;

			return FALSE;
		}

		gboolean ProbeHandler (GstPad *pad, GstMiniObject*, CallbackData *cbData)
		{
			qDebug () << Q_FUNC_INFO;
			gst_pad_remove_data_probe (pad, cbData->ID_);

			cbData->ID_ = gst_pad_add_event_probe (pad, G_CALLBACK (EventProbeHandler), cbData);

			const auto sinkpad = cbData->SinkPad_;
			gst_pad_send_event (sinkpad, gst_event_new_eos ());
			gst_object_unref (sinkpad);

			return TRUE;
		}
#endif
	}

	void PerformWProbe (GstPad *srcpad, GstPad *sinkpad, const std::function<void ()>& functor)
	{
		auto data = new CallbackData { functor, sinkpad, 0 };
#if GST_VERSION_MAJOR < 1
		data->ID_ = gst_pad_add_data_probe (srcpad, G_CALLBACK (ProbeHandler), data);
#else
		data->ID_ = gst_pad_add_probe (srcpad, GST_PAD_PROBE_TYPE_IDLE,
				ProbeHandler, data, nullptr);
#endif
	}

	void DebugPrintState (GstElement *elem, GstClockTime time)
	{
		GstState state, pending;
		gst_element_get_state (elem, &state, &pending, time);
		qDebug () << state << pending;
	}
}
}
}
