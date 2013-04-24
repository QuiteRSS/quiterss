/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "findtext.h"

FindTextContent::FindTextContent(QWidget *parent)
  : QLineEdit(parent)
{
  findInNewsAct_ = new QAction(this);
  findInNewsAct_->setObjectName("findInNewsAct");
  findInNewsAct_->setIcon(QIcon(":/images/findInNews"));
  findInNewsAct_->setCheckable(true);
  findInNewsAct_->setChecked(true);

  findTitleAct_ = new QAction(this);
  findTitleAct_->setObjectName("findTitleAct");
  findTitleAct_->setIcon(QIcon(":/images/findInNews"));
  findTitleAct_->setCheckable(true);
  findAuthorAct_ = new QAction(this);
  findAuthorAct_->setObjectName("findAuthorAct");
  findAuthorAct_->setIcon(QIcon(":/images/findInNews"));
  findAuthorAct_->setCheckable(true);
  findCategoryAct_ = new QAction(this);
  findCategoryAct_->setObjectName("findCategoryAct");
  findCategoryAct_->setIcon(QIcon(":/images/findInNews"));
  findCategoryAct_->setCheckable(true);
  findContentAct_ = new QAction(this);
  findContentAct_->setObjectName("findContentAct");
  findContentAct_->setIcon(QIcon(":/images/findInNews"));
  findContentAct_->setCheckable(true);

  findInBrowserAct_ = new QAction(this);
  findInBrowserAct_->setObjectName("findInBrowserAct");
  findInBrowserAct_->setIcon(QIcon(":/images/findText"));
  findInBrowserAct_->setCheckable(true);

  findGroup_ = new QActionGroup(this);
  findGroup_->setExclusive(true);
  findGroup_->addAction(findInNewsAct_);
  findGroup_->addAction(findTitleAct_);
  findGroup_->addAction(findAuthorAct_);
  findGroup_->addAction(findCategoryAct_);
  findGroup_->addAction(findContentAct_);
  findGroup_->addAction(findInBrowserAct_);

  findMenu_ = new QMenu(this);
  findMenu_->addActions(findGroup_->actions());
  findMenu_->insertSeparator(findTitleAct_);
  findMenu_->insertSeparator(findInBrowserAct_);

  findButton = new QToolButton(this);
  findButton->setFocusPolicy(Qt::NoFocus);
  QPixmap findPixmap(":/images/selectFindInNews");
  findButton->setIcon(QIcon(findPixmap));
  findButton->setIconSize(findPixmap.size());
  findButton->setCursor(Qt::ArrowCursor);
  findButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");

  connect(findButton, SIGNAL(clicked()), this, SLOT(slotMenuFind()));
  connect(findGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(slotSelectFind(QAction*)));

  clearButton = new QToolButton(this);
  clearButton->setFocusPolicy(Qt::NoFocus);
  QPixmap pixmap(":/images/editClear");
  clearButton->setIcon(QIcon(pixmap));
  clearButton->setIconSize(pixmap.size());
  clearButton->setCursor(Qt::ArrowCursor);
  clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  clearButton->hide();
  connect(clearButton, SIGNAL(clicked()), this, SLOT(slotClear()));
  connect(this, SIGNAL(textChanged(const QString&)),
          SLOT(updateClearButton(const QString&)));

  findLabel_ = new QLabel(tr("Find in News"), this);
  findLabel_->setStyleSheet("QLabel { color: gray; }");

  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  setStyleSheet(QString("QLineEdit { padding-right: %1px; padding-left: %2px; }").
                arg(clearButton->sizeHint().width() + frameWidth + 1).
                arg(findButton->sizeHint().width() + frameWidth + 1));
  QSize msz = minimumSizeHint();
  setMinimumSize(
        qMax(msz.width(), clearButton->sizeHint().height() + findButton->sizeHint().height() + frameWidth * 2 + 2),
        qMax(msz.height(), clearButton->sizeHint().height() + frameWidth * 2 + 2));
}

void FindTextContent::retranslateStrings()
{
  findInNewsAct_->setText(tr("Find in News"));
  findTitleAct_->setText(tr("Find Title"));
  findAuthorAct_->setText(tr("Find Authors"));
  findCategoryAct_->setText(tr("Find Category"));
  findContentAct_->setText(tr("Find in Content"));
  findInBrowserAct_->setText(tr("Find in Browser"));
  findLabel_->setText(findGroup_->checkedAction()->text());
  if (findLabel_->isVisible()) {
    findLabel_->hide();
    findLabel_->show();
  }
}

void FindTextContent::resizeEvent(QResizeEvent *)
{
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  QSize sz = findButton->sizeHint();
  findButton->move(frameWidth+3,
                   (rect().bottom() + 1 - sz.height())/2);
  sz = findLabel_->sizeHint();
  findLabel_->move(frameWidth+findButton->sizeHint().width()+5,
                   (rect().bottom() + 1 - sz.height())/2);
  sz = clearButton->sizeHint();
    clearButton->move(rect().right() - frameWidth - sz.width(),
                      (rect().bottom() + 1 - sz.height())/2);
}

void FindTextContent::focusInEvent(QFocusEvent *event)
{
  findLabel_->setVisible(false);
  QLineEdit::focusInEvent(event);
}

void FindTextContent::focusOutEvent(QFocusEvent *event)
{
  if (text().isEmpty())
    findLabel_->setVisible(true);
  QLineEdit::focusOutEvent(event);
}

void FindTextContent::updateClearButton(const QString& text)
{
  clearButton->setVisible(!text.isEmpty());
}

void FindTextContent::slotClear()
{
  clear();
  emit signalClear();
}

void FindTextContent::slotMenuFind()
{
  findMenu_->popup(mapToGlobal(QPoint(0, height()-1)));
}

void FindTextContent::slotSelectFind(QAction *act)
{
  if (act->objectName() == "findInBrowserAct") {
    findButton->setIcon(QIcon(":/images/selectFindInBrowser"));
  } else {
    findButton->setIcon(QIcon(":/images/selectFindInNews"));
  }
  findLabel_->setText(act->text());
  emit signalSelectFind();
}
