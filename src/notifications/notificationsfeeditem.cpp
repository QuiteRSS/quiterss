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
#include "notificationsfeeditem.h"

FeedItem::FeedItem(int width, QWidget * parent)
  : QWidget(parent)
{
  iconLabel_ = new QLabel(this);
  titleLabel_ = new QLabel(this);
  titleLabel_->setFixedWidth(width + 21*1);

  QWidget *mainWidget = new QWidget(this);
  mainWidget->setObjectName("feedItemNotification");
  QHBoxLayout *layout = new QHBoxLayout(mainWidget);
  layout->setMargin(4);
  layout->setSpacing(5);
  layout->addWidget(iconLabel_);
  layout->addWidget(titleLabel_);
  layout->addStretch();

  QHBoxLayout *mainLayout = new QHBoxLayout();
  mainLayout->setMargin(0);
  mainLayout->addWidget(mainWidget);
  setLayout(mainLayout);
}

FeedItem::~FeedItem()
{

}

void FeedItem::setIcon(const QPixmap &icon)
{
  iconLabel_->setPixmap(icon);
}

void FeedItem::setTitle(const QString &text, int cntNews)
{
  int wight = titleLabel_->fontMetrics().width(QString(" (%1)").arg(cntNews));
  QString titleStr = titleLabel_->fontMetrics().elidedText(
        text, Qt::ElideRight, titleLabel_->sizeHint().width() - wight);
  titleLabel_->setText(QString("%1 (%2)").arg(titleStr).arg(cntNews));
}

void FeedItem::setFontTitle(const QFont &font)
{
  titleLabel_->setFont(font);
}

void FeedItem::setColorText(const QColor &color)
{
  titleLabel_->setStyleSheet(QString("QLabel {color: %1;}").arg(color.name()));
}
