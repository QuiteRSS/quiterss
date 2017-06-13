/* =============================================================================
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
* =========================================================================== */
#include "locationbar.h"
#include "rssdetectionwidget.h"

#include <QEvent>
#include <QLayout>

LocationBar::LocationBar(WebView *view, QWidget *parent)
  : QLineEdit(parent)
  , view_(view)
  , focus_(false)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setStyleSheet("QLineEdit {margin-bottom: 1px; padding: 0px 3px 0px 3px;}");

  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  rightWidget_ = new QWidget(this);
  rightWidget_->resize(0, 0);
  QHBoxLayout *rightLayout = new QHBoxLayout(rightWidget_);
  rightLayout->setContentsMargins(0, 0, 2, 0);

  rssButton_ = new QToolButton(this);
  rssButton_->setToolTip("RSS");
  rssButton_->setFocusPolicy(Qt::NoFocus);
  rssButton_->setCursor(Qt::ArrowCursor);
  rssButton_->setFocusPolicy(Qt::ClickFocus);
  rssButton_->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  QPixmap pixmap(":/images/feed");
  rssButton_->setIcon(QIcon(pixmap));
  rightLayout->addWidget(rssButton_, 0, Qt::AlignVCenter | Qt::AlignRight);

  mainLayout->addStretch(1);
  mainLayout->addWidget(rightWidget_, 0, Qt::AlignVCenter | Qt::AlignRight);

  rssButton_->hide();

  connect(rssButton_, SIGNAL(clicked()), this, SLOT(rssIconClicked()));
}

void LocationBar::mouseReleaseEvent(QMouseEvent *event)
{
  if (focus_) {
    selectAll();
    focus_ = false;
  }
  QLineEdit::mouseReleaseEvent(event);
}

void LocationBar::focusInEvent(QFocusEvent *event)
{
  focus_ = true;
  QLineEdit::focusInEvent(event);
}

void LocationBar::updateTextMargins()
{
  int left = 0;
  int right = rightWidget_->sizeHint().width();
  int top = 0;
  int bottom = 0;
  setTextMargins(left, top, right, bottom);
}

void LocationBar::showRssIcon(bool show)
{
  rssButton_->setVisible(show);

  updateTextMargins();
}

void LocationBar::rssIconClicked()
{
  RSSDetectionWidget *rssWidget = new RSSDetectionWidget(view_, parentWidget());
  rssWidget->showAt(parentWidget());
}
