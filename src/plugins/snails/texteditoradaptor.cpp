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

#include "texteditoradaptor.h"
#include <QTextEdit>

namespace LeechCraft
{
namespace Snails
{
	TextEditorAdaptor::TextEditorAdaptor (QTextEdit *edit)
	: QObject { edit }
	, Edit_ { edit }
	{
		connect (Edit_,
				SIGNAL (textChanged ()),
				this,
				SIGNAL (textChanged ()));
	}

	QString TextEditorAdaptor::GetContents (ContentType type) const
	{
		switch (type)
		{
		case ContentType::PlainText:
			return Edit_->toPlainText ();
		default:
			return {};
		}
	}

	void TextEditorAdaptor::SetContents (QString contents, ContentType type)
	{
		if (type == ContentType::PlainText)
			Edit_->setPlainText (contents);
	}

	QAction* TextEditorAdaptor::GetEditorAction (EditorAction)
	{
		return nullptr;
	}

	void TextEditorAdaptor::AppendAction (QAction*)
	{
	}

	void TextEditorAdaptor::AppendSeparator ()
	{
	}

	void TextEditorAdaptor::RemoveAction (QAction*)
	{
	}

	void TextEditorAdaptor::SetBackgroundColor (const QColor&, ContentType)
	{
	}

	QWidget* TextEditorAdaptor::GetWidget ()
	{
		return Edit_;
	}

	QObject* TextEditorAdaptor::GetQObject ()
	{
		return this;
	}

	bool TextEditorAdaptor::FindText (const QString& text)
	{
		return Edit_->find (text);
	}

	void TextEditorAdaptor::DeleteSelection ()
	{
		Edit_->textCursor ().deleteChar ();
	}

	void TextEditorAdaptor::SetFontFamily (FontFamily family, const QFont& font)
	{
		if (family != FontFamily::FixedFont)
			return;

		auto cursor = Edit_->textCursor ();
		cursor.select (QTextCursor::Document);

		auto fmt = cursor.charFormat ();

		auto newFont = font;
		newFont.setPixelSize (fmt.font ().pixelSize ());
		fmt.setFont (newFont);

		cursor.setCharFormat (fmt);
	}

	void TextEditorAdaptor::SetFontSize (FontSize type, int size)
	{
		if (type != FontSize::DefaultFixedFontSize)
			return;

		auto cursor = Edit_->textCursor ();
		cursor.select (QTextCursor::Document);

		auto fmt = cursor.charFormat ();

		auto font = fmt.font ();
		font.setPixelSize (size);
		fmt.setFont (font);

		cursor.setCharFormat (fmt);
	}
}
}
