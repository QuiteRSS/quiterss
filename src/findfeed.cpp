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
#include "findfeed.h"

FindFeed::FindFeed(QWidget *parent)
  : QLineEdit(parent)
{
  findNameAct_ = new QAction(this);
  findNameAct_->setObjectName("findNameAct");
  findNameAct_->setCheckable(true);
  findNameAct_->setChecked(true);
  findLinkAct_ = new QAction(this);
  findLinkAct_->setObjectName("findLinkAct");
  findLinkAct_->setCheckable(true);

  findButton_ = new QToolButton(this);
  findButton_->setFocusPolicy(Qt::NoFocus);
  QPixmap findPixmap(":/images/selectFindInBrowser");
  findButton_->setIcon(QIcon(findPixmap));
  findButton_->setIconSize(findPixmap.size());
  findButton_->setCursor(Qt::ArrowCursor);
  findButton_->setStyleSheet("QToolButton { border: none; padding: 0px; }");

  findGroup_ = new QActionGroup(this);
  findGroup_->setExclusive(true);
  findMenu_ = new QMenu(this);

  findMenu_->addAction(findNameAct_);
  findMenu_->addAction(findLinkAct_);
  findGroup_->addAction(findNameAct_);
  findGroup_->addAction(findLinkAct_);

  connect(findButton_, SIGNAL(clicked()), this, SLOT(slotMenuFind()));
  connect(findGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(slotSelectFind(QAction*)));

  clearButton_ = new QToolButton(this);
  clearButton_->setFocusPolicy(Qt::NoFocus);
  QPixmap pixmap(":/images/editClear");
  clearButton_->setIcon(QIcon(pixmap));
  clearButton_->setIconSize(pixmap.size());
  clearButton_->setCursor(Qt::ArrowCursor);
  clearButton_->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  clearButton_->hide();
  connect(clearButton_, SIGNAL(clicked()), this, SLOT(slotClear()));
  connect(this, SIGNAL(textChanged(const QString&)),
          SLOT(updateClearButton(const QString&)));

  findLabel_ = new QLabel(this);
  findLabel_->setStyleSheet("QLabel { color: gray; }");

  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  setStyleSheet(QString("QLineEdit { padding-right: %1px; padding-left: %2px; }").
                arg(clearButton_->sizeHint().width() + frameWidth + 1).
                arg(findButton_->sizeHint().width() + frameWidth + 1));
  QSize msz = minimumSizeHint();
  setMinimumSize(
        qMax(msz.width(), clearButton_->sizeHint().height() + findButton_->sizeHint().height() + frameWidth * 2 + 2),
        qMax(msz.height(), clearButton_->sizeHint().height() + frameWidth * 2 + 2));
}

void FindFeed::retranslateStrings()
{
  findNameAct_->setText(tr("Find Name"));
  findLinkAct_->setText(tr("Find Link"));
  findLabel_->setText(findGroup_->checkedAction()->text());
  if (findLabel_->isVisible()) {
    findLabel_->hide();
    findLabel_->show();
  }
}

void FindFeed::resizeEvent(QResizeEvent *)
{
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  QSize sz = findButton_->sizeHint();
  findButton_->move(frameWidth+3,
                   (rect().bottom() + 1 - sz.height())/2);
  sz = findLabel_->sizeHint();
  findLabel_->move(frameWidth+findButton_->sizeHint().width()+5,
                   (rect().bottom() + 1 - sz.height())/2);
  sz = clearButton_->sizeHint();
    clearButton_->move(rect().right() - frameWidth - sz.width(),
                      (rect().bottom() + 1 - sz.height())/2);
}

void FindFeed::focusInEvent(QFocusEvent *event)
{
  findLabel_->setVisible(false);
  QLineEdit::focusInEvent(event);
}

void FindFeed::focusOutEvent(QFocusEvent *event)
{
  if (text().isEmpty())
    findLabel_->setVisible(true);
  QLineEdit::focusOutEvent(event);
}

void FindFeed::updateClearButton(const QString& text)
{
  clearButton_->setVisible(!text.isEmpty());
}

void FindFeed::slotClear()
{
  clear();
  emit signalClear();
}

void FindFeed::slotMenuFind()
{
  findMenu_->popup(mapToGlobal(QPoint(0, height()-1)));
}

void FindFeed::slotSelectFind(QAction *act)
{
  findLabel_->setText(act->text());
  emit signalSelectFind();
}

