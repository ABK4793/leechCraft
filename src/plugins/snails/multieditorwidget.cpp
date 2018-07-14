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

#include "multieditorwidget.h"
#include <QAction>
#include <QActionGroup>
#include <QSignalMapper>
#include <util/sll/prelude.h>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/iinfo.h>
#include "core.h"
#include "texteditoradaptor.h"

namespace LeechCraft
{
namespace Snails
{
	MultiEditorWidget::MultiEditorWidget (QWidget *parent)
	: QWidget { parent }
	{
		Ui_.setupUi (this);
	}

	void MultiEditorWidget::SetupEditors (const std::function<void (QAction*)>& editorsHandler)
	{
		auto mapper = new QSignalMapper (this);
		connect (mapper,
				SIGNAL (mapped (int)),
				this,
				SLOT (handleEditorSelected (int)));

		const auto editorsGroup = new QActionGroup (this);
		auto addEditor = [&] (const QString& name, int index) -> void
		{
			auto action = new QAction (name, this);
			connect (action,
					SIGNAL (triggered ()),
					mapper,
					SLOT (map ()));
			editorsGroup->addAction (action);
			action->setCheckable (true);
			action->setChecked (true);
			mapper->setMapping (action, index);

			editorsHandler (action);
			Actions_ << action;
		};

		MsgEdits_ << std::make_shared<TextEditorAdaptor> (Ui_.PlainEdit_);

		addEditor (tr ("Plain text (internal)"), MsgEdits_.size () - 1);

		const auto& plugs = Core::Instance ().GetProxy ()->
				GetPluginsManager ()->GetAllCastableRoots<ITextEditor*> ();
		for (const auto plugObj : plugs)
		{
			const auto plug = qobject_cast<ITextEditor*> (plugObj);

			if (!plug->SupportsEditor (ContentType::HTML))
				continue;

			const std::shared_ptr<QWidget> w { plug->GetTextEditor (ContentType::HTML) };
			const auto edit = std::dynamic_pointer_cast<IEditorWidget> (w);
			if (!edit)
				continue;

			MsgEdits_ << edit;
			Ui_.EditorStack_->addWidget (w.get ());

			const auto& pluginName = qobject_cast<IInfo*> (plugObj)->GetName ();
			addEditor (tr ("Rich text (%1)").arg (pluginName), MsgEdits_.size () - 1);
		}

		Ui_.EditorStack_->setCurrentIndex (Ui_.EditorStack_->count () - 1);
	}

	ContentType MultiEditorWidget::GetCurrentEditorType () const
	{
		return Ui_.EditorStack_->currentIndex () ? ContentType::HTML : ContentType::PlainText;
	}

	IEditorWidget* MultiEditorWidget::GetCurrentEditor () const
	{
		return MsgEdits_.value (Ui_.EditorStack_->currentIndex ()).get ();
	}

	void MultiEditorWidget::SelectEditor (ContentType type)
	{
		int index = 0;
		switch (type)
		{
		case ContentType::PlainText:
			index = 0;
			break;
		case ContentType::HTML:
			index = 1;
			break;
		}

		if (index >= Actions_.size ())
			return;

		Actions_.value (index)->setChecked (true);
		handleEditorSelected (index);
	}

	QList<IEditorWidget*> MultiEditorWidget::GetAllEditors () const
	{
		return Util::Map (MsgEdits_, [] (auto ptr) { return ptr.get (); });
	}

	ContentType MultiEditorWidget::GetEditorType (IEditorWidget *editor) const
	{
		return editor == MsgEdits_.value (0).get () ?
				ContentType::PlainText :
				ContentType::HTML;
	}

	void MultiEditorWidget::handleEditorSelected (int index)
	{
		const auto previous = GetCurrentEditor ();

		Ui_.EditorStack_->setCurrentIndex (index);

		emit editorChanged (GetCurrentEditor (), previous);
	}
}
}
