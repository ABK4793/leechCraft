/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "termtab.h"
#include <QVBoxLayout>
#include <QToolBar>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QToolButton>
#include <QShortcut>
#include <QFontDialog>
#include <QUrl>
#include <QProcessEnvironment>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QTimer>
#include <QKeyEvent>
#include <QtDebug>
#include <QDir>
#include <qtermwidget.h>
#include <util/xpc/util.h>
#include <util/xpc/stddatafiltermenucreator.h>
#include <util/shortcuts/shortcutmanager.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/core/iiconthememanager.h>
#include "xmlsettingsmanager.h"
#include "processgraphbuilder.h"
#include "closedialog.h"
#include "colorschemesmanager.h"

namespace LC::Eleeminator
{
	namespace
	{
#ifdef Q_OS_MAC
		void FixCommandControl (QKeyEvent *ev)
		{
			auto mods = ev->modifiers ();
			const bool hasCtrl = mods & Qt::ControlModifier;
			const bool hasCmd = mods & Qt::MetaModifier;
			bool changed = false;
			if (hasCtrl != hasCmd)
			{
				if (hasCtrl)
				{
					mods |= Qt::MetaModifier;
					mods &= ~Qt::ControlModifier;
				}
				else
				{
					mods |= Qt::ControlModifier;
					mods &= ~Qt::MetaModifier;
				}
				changed = true;
			}

			auto key = ev->key ();
			if (key == Qt::Key_Control)
			{
				key = Qt::Key_Meta;
				changed = true;
			}
			else if (key == Qt::Key_Meta)
			{
				key = Qt::Key_Control;
				changed = true;
			}

			if (changed)
				*ev = QKeyEvent
				{
					ev->type (),
					key,
					mods,
					ev->text (),
					ev->isAutoRepeat (),
					static_cast<ushort> (ev->count ())
				};
		}
#endif
		void SendUrl (const QUrl& url)
		{
			const auto& entity = Util::MakeEntity (url, {}, TaskParameter::FromUserInitiated);
			GetProxyHolder ()->GetEntityManager ()->HandleEntity (entity);
		}

		Qt::KeyboardModifier GetModifier (const QString& str)
		{
			if (str == "Ctrl")
				return Qt::ControlModifier;
			else if (str == "Alt")
				return Qt::AltModifier;
			else if (str == "Shift")
				return Qt::ShiftModifier;
			else if (str == "Meta")
				return Qt::MetaModifier;
			else
				return Qt::NoModifier;
		}

		void HandleUrlActivatedInTerm (const QUrl& url)
		{
			const auto modifiers = QApplication::keyboardModifiers ();
			const auto neededMod = GetModifier (XmlSettingsManager::Instance ().property ("LinkActivationModifier").toString ());
			if ((modifiers & neededMod) == neededMod)
				SendUrl (url);
		}

		void SetEnvironment (QTermWidget& term)
		{
			auto systemEnv = QProcessEnvironment::systemEnvironment ();
			if (systemEnv.value ("TERM") != "xterm")
				systemEnv.remove ("TERM");
			if (!systemEnv.contains ("TERM"))
			{
				systemEnv.insert ("TERM", "xterm");
				term.setEnvironment (systemEnv.toStringList ());
			}
		}
	}

	TermTab::TermTab (Util::ShortcutManager *scMgr,
			const TabClassInfo& tc, ColorSchemesManager *colorSchemesMgr, QObject *plugin)
	: TC_ (tc)
	, ParentPlugin_ { plugin }
	, Toolbar_ { new QToolBar { tr ("Terminal toolbar") } }
	, Term_ { *new QTermWidget { false } }
	, ColorSchemesMgr_ { colorSchemesMgr }
	{
		auto lay = new QVBoxLayout;
		lay->setContentsMargins (0, 0, 0, 0);
		setLayout (lay);

		lay->addWidget (&Term_);

		Term_.setFlowControlEnabled (true);
		Term_.setFlowControlWarningEnabled (true);
		Term_.setScrollBarPosition (QTermWidget::ScrollBarRight);

#ifdef Q_OS_MAC
		connect (&Term_,
				&QTermWidget::termKeyPressed,
				&Term_,
				&FixCommandControl);
#endif

		SetEnvironment (Term_);

		Term_.startShellProgram ();

		connect (&Term_,
				&QTermWidget::finished,
				this,
				&TermTab::RemoveTab);

		connect (&Term_,
				&QTermWidget::urlActivated,
				this,
				[] (const QUrl& url, bool fromMenu)
				{
					if (fromMenu)
						SendUrl (url);
					else
						HandleUrlActivatedInTerm (url);
				});

		const auto& savedFontVar = XmlSettingsManager::Instance ().property ("Font");
		if (!savedFontVar.isNull () && savedFontVar.canConvert<QFont> ())
			Term_.setTerminalFont (savedFontVar.value<QFont> ());

		QTimer::singleShot (0,
				&Term_,
				qOverload<> (&QTermWidget::setFocus));

		SetupToolbar (scMgr);
		SetupShortcuts (scMgr);

		Term_.setContextMenuPolicy (Qt::CustomContextMenu);
		connect (&Term_,
				SIGNAL (customContextMenuRequested (QPoint)),
				this,
				SLOT (handleTermContextMenu (QPoint)));

		connect (&Term_,
				SIGNAL (bell (QString)),
				this,
				SLOT (handleBell (QString)));

		auto timer = new QTimer { this };
		connect (timer,
				SIGNAL (timeout ()),
				this,
				SLOT (updateTitle ()));
		timer->start (3000);

		XmlSettingsManager::Instance ().RegisterObject ({ "FiniteHistory", "HistorySize" },
				this,
				"setHistorySettings");
		setHistorySettings ();
	}

	TabClassInfo TermTab::GetTabClassInfo () const
	{
		return TC_;
	}

	QObject* TermTab::ParentMultiTabs ()
	{
		return ParentPlugin_;
	}

	QToolBar* TermTab::GetToolBar () const
	{
		return Toolbar_;
	}

	void TermTab::Remove ()
	{
		const ProcessGraphBuilder builder { Term_.getShellPID () };
		if (!builder.IsEmpty ())
		{
			CloseDialog dia { builder.CreateModel (), this };
			if (dia.exec () != QDialog::Accepted)
				return;
		}

		RemoveTab ();
	}

	void TermTab::TabMadeCurrent ()
	{
		IsTabCurrent_ = true;
	}

	void TermTab::TabLostCurrent ()
	{
		IsTabCurrent_ = false;
	}

	void TermTab::SetupToolbar (Util::ShortcutManager *manager)
	{
		SetupColorsButton ();
		SetupFontsButton ();

		Toolbar_->addSeparator ();

		const auto clearAct = Toolbar_->addAction (tr ("Clear window"));
		clearAct->setProperty ("ActionIcon", "edit-clear");
		connect (clearAct,
				&QAction::triggered,
				&Term_,
				&QTermWidget::clear);
		manager->RegisterAction ("org.LeechCraft.Eleeminator.Clear", clearAct);
	}

	void TermTab::SetupColorsButton ()
	{
		auto colorMenu = new QMenu { tr ("Color scheme"), this };
		colorMenu->menuAction ()->setProperty ("ActionIcon", "fill-color");
		connect (colorMenu,
				SIGNAL (triggered (QAction*)),
				this,
				SLOT (setColorScheme (QAction*)));
		connect (colorMenu,
				SIGNAL (hovered (QAction*)),
				this,
				SLOT (previewColorScheme (QAction*)));
		connect (colorMenu,
				SIGNAL (aboutToHide ()),
				this,
				SLOT (stopColorSchemePreview ()));

		const auto& lastScheme = XmlSettingsManager::Instance ()
				.Property ("LastColorScheme", "Linux").toString ();

		const auto colorActionGroup = new QActionGroup { colorMenu };
		for (const auto& colorScheme : ColorSchemesMgr_->GetSchemes ())
		{
			auto act = colorMenu->addAction (colorScheme.Name_);
			act->setCheckable (true);
			act->setProperty ("ER/ColorScheme", colorScheme.ID_);

			if (colorScheme.ID_ == lastScheme)
			{
				act->setChecked (true);
				setColorScheme (act);
			}

			colorActionGroup->addAction (act);
		}

		auto colorButton = new QToolButton { Toolbar_ };
		colorButton->setPopupMode (QToolButton::InstantPopup);
		colorButton->setMenu (colorMenu);
		colorButton->setProperty ("ActionIcon", "fill-color");

		Toolbar_->addWidget (colorButton);
	}

	void TermTab::SetupFontsButton ()
	{
		const auto action = Toolbar_->addAction (tr ("Select font..."),
				this, SLOT (selectFont ()));
		action->setProperty ("ActionIcon", "preferences-desktop-font");
	}

	void TermTab::SetupShortcuts (Util::ShortcutManager *manager)
	{
		auto copySc = new QShortcut { { "Ctrl+Shift+C" }, &Term_, &Term_, &QTermWidget::copyClipboard };
		manager->RegisterShortcut ("org.LeechCraft.Eleeminator.Copy", {}, copySc);

		auto pasteSc = new QShortcut { { "Ctrl+Shift+V" }, &Term_, &Term_, &QTermWidget::pasteClipboard };
		manager->RegisterShortcut ("org.LeechCraft.Eleeminator.Paste", {}, pasteSc);

		auto closeSc = new QShortcut { { "Ctrl+Shift+W" }, &Term_, this, &TermTab::Remove };
		manager->RegisterShortcut ("org.LeechCraft.Eleeminator.Close", {}, closeSc);
	}

	void TermTab::AddUrlActions (QMenu& menu, const QPoint& point)
	{
		const auto hotspot = Term_.getHotSpotAt (point);
		if (!hotspot)
			return;

		if (hotspot->type () != Filter::HotSpot::Link)
			return;

		const auto urlHotSpot = static_cast<const Konsole::UrlFilter::HotSpot*> (hotspot);
		const auto& cap = urlHotSpot->capturedTexts ().value (0);
		if (cap.isEmpty ())
			return;

		const auto itm = GetProxyHolder ()->GetIconThemeManager ();
		menu.addAction (itm->GetIcon ("document-open-remote"),
				tr ("Open URL"),
				this,
				[cap]
				{
					const auto& url = QUrl::fromEncoded (cap.toUtf8 ());
					const auto& entity = Util::MakeEntity (url, {}, TaskParameter::FromUserInitiated);
					GetProxyHolder ()->GetEntityManager ()->HandleEntity (entity);
				});
		menu.addAction (tr ("Copy URL"),
				this,
				[cap] { QApplication::clipboard ()->setText (cap, QClipboard::Clipboard); });
		menu.addSeparator ();
	}

	void TermTab::AddLocalFileActions (QMenu& menu, const QString& selected)
	{
		if (selected.isEmpty ())
			return;

		const QDir workingDir { Term_.workingDirectory () };
		if (!workingDir.exists (selected))
			return;

		const auto& localUrl = QUrl::fromLocalFile (workingDir.filePath (selected));
		menu.addAction (tr ("Open file"),
				this,
				[localUrl]
				{
					const auto& entity = Util::MakeEntity (localUrl, {}, OnlyHandle | FromUserInitiated);
					GetProxyHolder ()->GetEntityManager ()->HandleEntity (entity);
				});
		menu.addAction (tr ("Open file externally"),
				this,
				[localUrl]
				{
					QDesktopServices::openUrl (localUrl);
				});
		menu.addSeparator ();

		new Util::StdDataFilterMenuCreator { localUrl, GetProxyHolder ()->GetEntityManager (), &menu };
	}

	void TermTab::setHistorySettings ()
	{
		const bool isFinite = XmlSettingsManager::Instance ().property ("FiniteHistory").toBool ();
		const auto linesCount = isFinite ?
				XmlSettingsManager::Instance ().property ("HistorySize").toInt () :
				-1;
		Term_.setHistorySize (linesCount);
	}

	void TermTab::handleTermContextMenu (const QPoint& point)
	{
		QMenu menu;

		AddUrlActions (menu, point);

		const auto& selected = Term_.selectedText ();
		AddLocalFileActions (menu, selected);

		const auto itm = GetProxyHolder ()->GetIconThemeManager ();

		const auto copyAct = menu.addAction (itm->GetIcon ("edit-copy"),
				tr ("Copy selected text"),
				&Term_,
				&QTermWidget::copyClipboard);
		copyAct->setEnabled (!Term_.selectedText ().isEmpty ());

		const auto pasteAct = menu.addAction (itm->GetIcon ("edit-paste"),
				tr ("Paste from clipboard"),
				&Term_,
				&QTermWidget::pasteClipboard);
		pasteAct->setEnabled (!QApplication::clipboard ()->text (QClipboard::Clipboard).isEmpty ());

		new Util::StdDataFilterMenuCreator { selected, GetProxyHolder ()->GetEntityManager (), &menu };

		menu.exec (Term_.mapToGlobal (point));
	}

	void TermTab::setColorScheme (QAction *schemeAct)
	{
		const auto& colorScheme = schemeAct->property ("ER/ColorScheme").toString ();
		if (colorScheme.isEmpty ())
		{
			qWarning () << Q_FUNC_INFO
					<< "empty color scheme for"
					<< schemeAct;
			return;
		}

		schemeAct->setChecked (true);

		Term_.setColorScheme (colorScheme);
		CurrentColorScheme_ = colorScheme;

		XmlSettingsManager::Instance ().setProperty ("LastColorScheme", colorScheme);
	}

	void TermTab::previewColorScheme (QAction *schemeAct)
	{
		const auto& colorScheme = schemeAct->property ("ER/ColorScheme").toString ();
		if (colorScheme.isEmpty ())
		{
			qWarning () << Q_FUNC_INFO
					<< "empty color scheme for"
					<< schemeAct;
			return;
		}

		Term_.setColorScheme (colorScheme);
	}

	void TermTab::stopColorSchemePreview ()
	{
		Term_.setColorScheme (CurrentColorScheme_);
	}

	void TermTab::selectFont ()
	{
		const auto& currentFont = Term_.getTerminalFont ();
		auto savedFont = XmlSettingsManager::Instance ()
				.Property ("Font", QVariant::fromValue (currentFont)).value<QFont> ();

		bool ok = false;
		const auto& font = QFontDialog::getFont (&ok, currentFont, this);
		if (!ok)
			return;

		Term_.setTerminalFont (font);

		XmlSettingsManager::Instance ().setProperty ("Font", QVariant::fromValue (font));
	}

	void TermTab::updateTitle ()
	{
		auto cwd = Term_.workingDirectory ();
		while (cwd.endsWith ('/'))
			cwd.chop (1);

		const auto& tree = ProcessGraphBuilder { Term_.getShellPID () }.GetProcessTree ();
		const auto& processName = tree.Children_.isEmpty () ?
				tree.Command_ :
				tree.Children_.value (0).Command_;

		const auto& title = cwd.isEmpty () ?
				processName :
				(cwd.section ('/', -1, -1) + " : " + processName);
		emit changeTabName (title);
	}

	void TermTab::handleBell (const QString&)
	{
		auto e = Util::MakeAN ("Eleeminator", tr ("Bell in terminal."), Priority::Info,
				"org.LeechCraft.Eleeminator", AN::CatTerminal, AN::TypeTerminalBell,
				"org.LeechCraft.Eleeminator.BellEvent",
				{ "Eleeminator", tr ("Bell") });
		e.Mime_ += "+advanced";
		e.Additional_ [AN::Field::TerminalActive] = IsTabCurrent_;
		GetProxyHolder ()->GetEntityManager ()->HandleEntity (e);
	}

	void TermTab::RemoveTab ()
	{
		emit removeTab ();
		deleteLater ();
	}
}
