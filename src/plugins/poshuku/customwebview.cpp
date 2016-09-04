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

#include "customwebview.h"
#include <cmath>
#include <limits>
#include <qwebframe.h>
#include <qwebinspector.h>
#include <QMenu>
#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QFile>
#include <QWebElement>
#include <QWebHistory>
#include <QTextCodec>
#include <QMouseEvent>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>

#if QT_VERSION < 0x050000
#include <QWindowsStyle>
#endif

#include <QFileDialog>
#include <QtDebug>
#include <util/xpc/util.h>
#include <util/xpc/defaulthookproxy.h>
#include <interfaces/core/icoreproxy.h>
#include "interfaces/poshuku/ibrowserwidget.h"
#include "interfaces/poshuku/poshukutypes.h"
#include "core.h"
#include "customwebpage.h"
#include "xmlsettingsmanager.h"
#include "webviewsmoothscroller.h"
#include "webviewrendersettingshandler.h"
#include "webviewsslwatcherhandler.h"

namespace LeechCraft
{
namespace Poshuku
{
	CustomWebView::CustomWebView (IEntityManager *iem, QWidget *parent)
	: QWebView { parent }
	, WebInspector_
	{
		new QWebInspector,
		[] (QWebInspector *insp)
		{
			insp->hide ();
			insp->deleteLater ();
		}
	}
	{
#if QT_VERSION < 0x050000
		QPalette p;
		if (p.color (QPalette::Window) != Qt::white)
			setPalette (QWindowsStyle ().standardPalette ());
#endif

		new WebViewSmoothScroller { this };
		new WebViewRenderSettingsHandler { this };

		const auto page = new CustomWebPage { iem, this };
		setPage (page);

		SslWatcherHandler_ = new WebViewSslWatcherHandler { this };

		WebInspector_->setPage (page);

		connect (this,
				SIGNAL (urlChanged (const QUrl&)),
				this,
				SLOT (remakeURL (const QUrl&)));
		connect (page,
				SIGNAL (loadingURL (const QUrl&)),
				this,
				SLOT (remakeURL (const QUrl&)));
		connect (page,
				SIGNAL (saveFrameStateRequested (QWebFrame*, QWebHistoryItem*)),
				this,
				SLOT (handleFrameState (QWebFrame*, QWebHistoryItem*)),
				Qt::QueuedConnection);

		connect (this,
				SIGNAL (loadFinished (bool)),
				this,
				SLOT (handleLoadFinished (bool)));

		connect (page,
				SIGNAL (printRequested (QWebFrame*)),
				this,
				SLOT (handlePrintRequested (QWebFrame*)));
		connect (page,
				SIGNAL (windowCloseRequested ()),
				this,
				SIGNAL (closeRequested ()));
		connect (page,
				SIGNAL (storeFormData (PageFormsData_t)),
				this,
				SIGNAL (storeFormData (PageFormsData_t)));

		connect (page,
				SIGNAL (linkHovered (QString, QString, QString)),
				this,
				SIGNAL (linkHovered (QString, QString, QString)));

		connect (page->mainFrame (),
				SIGNAL (initialLayoutCompleted ()),
				this,
				SIGNAL (earliestViewLayout ()));
	}

	void CustomWebView::SetBrowserWidget (IBrowserWidget *widget)
	{
		Browser_ = widget;
	}

	void CustomWebView::Load (const QUrl& url, const QString& title)
	{
		if (url.isEmpty () || !url.isValid ())
			return;

		if (url.scheme () == "javascript")
		{
			QVariant result = page ()->mainFrame ()->
				evaluateJavaScript (url.toString ().mid (11));
			if (result.canConvert (QVariant::String))
				setHtml (result.toString ());
			return;
		}

		emit navigateRequested (url);

		if (url.scheme () == "about")
		{
			if (url.path () == "plugins")
				NavigatePlugins ();
			else if (url.path () == "home")
				NavigateHome ();
			return;
		}

		remakeURL (url);
		if (title.isEmpty ())
			emit titleChanged (tr ("Loading..."));
		else
			emit titleChanged (title);
		load (url);
	}

	void CustomWebView::Load (const QNetworkRequest& req,
			QNetworkAccessManager::Operation op, const QByteArray& ba)
	{
		emit titleChanged (tr ("Loading..."));
		QWebView::load (req, op, ba);
	}

	QString CustomWebView::URLToProperString (const QUrl& url) const
	{
		QString string = url.toString ();
		QWebElement equivs = page ()->mainFrame ()->
				findFirstElement ("meta[http-equiv=\"Content-Type\"]");
		if (!equivs.isNull ())
		{
			QString content = equivs.attribute ("content", "text/html; charset=UTF-8");
			const QString charset = "charset=";
			int pos = content.indexOf (charset);
			if (pos >= 0)
				PreviousEncoding_ = content.mid (pos + charset.length ()).toLower ();
		}

		if (PreviousEncoding_ != "utf-8" &&
				PreviousEncoding_ != "utf8" &&
				!PreviousEncoding_.isEmpty ())
			string = url.toEncoded ();

		return string;
	}

	void CustomWebView::Print (bool preview)
	{
		PrintImpl (preview, page ()->mainFrame ());
	}

	QWidget* CustomWebView::GetQWidget ()
	{
		return this;
	}

	QList<QAction*> CustomWebView::GetActions (ActionArea area) const
	{
		switch (area)
		{
		case ActionArea::UrlBar:
			return { SslWatcherHandler_->GetStateAction () };
		}

		assert (false);
	}

	QAction* CustomWebView::GetPageAction (PageAction action) const
	{
#define ACT(x) \
		case PageAction::x: \
			return pageAction (QWebPage::x);

		switch (action)
		{
		ACT (Reload)
		ACT (Stop)
		ACT (Back)
		ACT (Forward)
		ACT (Cut)
		ACT (Copy)
		ACT (Paste)
		}

#undef ACT

		assert (false);
	}

	QString CustomWebView::GetTitle () const
	{
		return title ();
	}

	QUrl CustomWebView::GetUrl () const
	{
		return url ();
	}

	QString CustomWebView::GetHumanReadableUrl () const
	{
		return URLToProperString (url ());
	}

	void CustomWebView::SetContent (const QByteArray& data, const QByteArray& mime)
	{
		setContent (data, mime);
	}

	void CustomWebView::EvaluateJS (const QString& js, const std::function<void (QVariant)>& callback)
	{
		const auto& res = page ()->mainFrame ()->evaluateJavaScript (js);
		if (callback)
			callback (res);
	}

	void CustomWebView::AddJavaScriptObject (const QString& id, QObject *object)
	{
		page ()->mainFrame ()->addToJavaScriptWindowObject (id, object);
	}

	QPoint CustomWebView::GetScrollPosition () const
	{
		return page ()->mainFrame ()->scrollPosition ();
	}

	void CustomWebView::SetScrollPosition (const QPoint& point)
	{
		page ()->mainFrame ()->setScrollPosition (point);
	}

	double CustomWebView::GetZoomFactor () const
	{
		return zoomFactor ();
	}

	void CustomWebView::SetZoomFactor (double factor)
	{
		setZoomFactor (factor);
	}

	double CustomWebView::GetTextSizeMultiplier () const
	{
		return textSizeMultiplier ();
	}

	void CustomWebView::SetTextSizeMultiplier (double factor)
	{
		setTextSizeMultiplier (factor);
	}

	void CustomWebView::mousePressEvent (QMouseEvent *e)
	{
		qobject_cast<CustomWebPage*> (page ())->SetButtons (e->buttons ());
		qobject_cast<CustomWebPage*> (page ())->SetModifiers (e->modifiers ());

		const bool mBack = e->button () == Qt::XButton1;
		const bool mForward = e->button () == Qt::XButton2;
		if (mBack || mForward)
		{
			pageAction (mBack ? QWebPage::Back : QWebPage::Forward)->trigger ();
			e->accept ();
			return;
		}

		QWebView::mousePressEvent (e);
	}

	void CustomWebView::contextMenuEvent (QContextMenuEvent *e)
	{
		const auto& r = page ()->mainFrame ()->hitTestContent (e->pos ());
		emit contextMenuRequested (mapToGlobal (e->pos ()),
				{
					r.isContentEditable (),
					page ()->selectedText (),
					r.linkUrl (),
					r.linkText (),
					r.imageUrl (),
					r.pixmap ()
				});
	}

	void CustomWebView::keyReleaseEvent (QKeyEvent *event)
	{
		if (event->matches (QKeySequence::Copy))
			pageAction (QWebPage::Copy)->trigger ();
		else
			QWebView::keyReleaseEvent (event);
	}

	void CustomWebView::NavigatePlugins ()
	{
		QFile pef (":/resources/html/pluginsenum.html");
		pef.open (QIODevice::ReadOnly);
		QString contents = QString (pef.readAll ())
			.replace ("INSTALLEDPLUGINS", tr ("Installed plugins"))
			.replace ("NOPLUGINS", tr ("No plugins installed"))
			.replace ("FILENAME", tr ("File name"))
			.replace ("MIME", tr ("MIME type"))
			.replace ("DESCR", tr ("Description"))
			.replace ("SUFFIXES", tr ("Suffixes"))
			.replace ("ENABLED", tr ("Enabled"))
			.replace ("NO", tr ("No"))
			.replace ("YES", tr ("Yes"));
		setHtml (contents);
	}

	void CustomWebView::NavigateHome ()
	{
		QFile file (":/resources/html/home.html");
		file.open (QIODevice::ReadOnly);
		QString data = file.readAll ();
		data.replace ("{pagetitle}",
				tr ("Welcome to LeechCraft!"));
		data.replace ("{title}",
				tr ("Welcome to LeechCraft!"));
		data.replace ("{body}",
				tr ("Welcome to LeechCraft, the integrated internet-client.<br />"
					"More info is available on the <a href='http://leechcraft.org'>"
					"project's site</a>."));

		QBuffer iconBuffer;
		iconBuffer.open (QIODevice::ReadWrite);
		QPixmap pixmap ("lcicons:/resources/images/poshuku.svg");
		pixmap.save (&iconBuffer, "PNG");

		data.replace ("{img}",
				QByteArray ("data:image/png;base64,") + iconBuffer.buffer ().toBase64 ());

		setHtml (data);
	}

	void CustomWebView::PrintImpl (bool preview, QWebFrame *frame)
	{
		QPrinter printer;
		if (preview)
		{
			QPrintPreviewDialog prevDialog (&printer, this);
			connect (&prevDialog,
					SIGNAL (paintRequested (QPrinter*)),
					frame,
					SLOT (print (QPrinter*)));

			if (prevDialog.exec () != QDialog::Accepted)
				return;
		}
		else
		{
			QPrintDialog dialog (&printer, this);
			dialog.setWindowTitle (tr ("Print web page"));

			if (dialog.exec () != QDialog::Accepted)
				return;

			frame->print (&printer);
		}
	}

	void CustomWebView::remakeURL (const QUrl& url)
	{
		emit urlChanged (URLToProperString (url));
	}

	void CustomWebView::handleLoadFinished (bool ok)
	{
		if (ok)
			remakeURL (url ());
	}

	void CustomWebView::handleFrameState (QWebFrame*, QWebHistoryItem*)
	{
		const auto& histUrl = page ()->history ()->currentItem ().url ();
		if (histUrl != url ())
			remakeURL (histUrl);
	}

	void CustomWebView::handlePrintRequested (QWebFrame *frame)
	{
		PrintImpl (false, frame);
	}
}
}
