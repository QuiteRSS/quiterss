/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
/****************************************************************************
**
** Copyright (c) 2007 Trolltech ASA <info@trolltech.com>
**
** Use, modification and distribution is allowed without limitation,
** warranty, liability or support of any kind.
**
****************************************************************************/
#include "lineedit.h"

#include <QToolButton>
#include <QStyle>

LineEdit::LineEdit(QWidget *parent, const QString &text)
  : QLineEdit(parent)
  , textLabel_(0)
{
  clearButton = new QToolButton(this);
  clearButton->setFocusPolicy(Qt::NoFocus);
  QPixmap pixmap(":/images/editClear");
  clearButton->setIcon(QIcon(pixmap));
  clearButton->setIconSize(pixmap.size());
  clearButton->setCursor(Qt::ArrowCursor);
  clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  clearButton->hide();

  if (!text.isEmpty()) {
    textLabel_ = new QLabel(this);
    textLabel_->setStyleSheet("QLabel { color: gray; }");
    textLabel_->setText(text);
  }

  connect(clearButton, SIGNAL(clicked()), this, SLOT(slotClear()));
  connect(this, SIGNAL(textChanged(const QString&)),
          SLOT(updateClearButton(const QString&)));
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  setStyleSheet(QString("QLineEdit { padding-right: %1px; }").
                arg(clearButton->sizeHint().width() + frameWidth + 1));
  QSize msz = minimumSizeHint();
  setMinimumSize(qMax(msz.width(), clearButton->sizeHint().height() + frameWidth * 2 + 2),
                 qMax(msz.height(), clearButton->sizeHint().height() + frameWidth * 2 + 2));
}

void LineEdit::resizeEvent(QResizeEvent *)
{
  QSize sz;
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  if (textLabel_) {
    sz = textLabel_->sizeHint();
    textLabel_->move(frameWidth+3,
                     (rect().bottom() + 1 - sz.height())/2);
  }

  sz = clearButton->sizeHint();
  clearButton->move(rect().right() - frameWidth - sz.width(),
                    (rect().bottom() + 1 - sz.height())/2);
}

void LineEdit::focusInEvent(QFocusEvent *event)
{
  if (textLabel_)
    textLabel_->setVisible(false);

  QLineEdit::focusInEvent(event);
}

void LineEdit::focusOutEvent(QFocusEvent *event)
{
  if (text().isEmpty() && textLabel_)
    textLabel_->setVisible(true);

  QLineEdit::focusOutEvent(event);
}

void LineEdit::updateClearButton(const QString& text)
{
  clearButton->setVisible(!text.isEmpty());
}

void LineEdit::slotClear()
{
  clear();
  emit signalClear();
}


