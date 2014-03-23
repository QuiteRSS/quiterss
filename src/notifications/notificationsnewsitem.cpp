/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2014 QuiteRSS Team <quiterssteam@gmail.com>
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
#include "notificationsnewsitem.h"

NewsItem::NewsItem(int idFeed, int idNews, int width, QWidget * parent)
  : QWidget(parent)
  , feedId_(idFeed)
  , newsId_(idNews)
  , read_(false)
{
  setCursor(Qt::PointingHandCursor);

  textLabel_ = new QLabel(this);
  textLabel_->setFixedWidth(width);
  textLabel_->setStyleSheet("QLabel:hover {color: #1155CC;}");

  readButton_ = new ToolButton(this);
  readButton_->setIcon(QIcon(":/images/bulletUnread"));
  readButton_->setToolTip(tr("Mark Read/Unread"));
  readButton_->setAutoRaise(true);

  ToolButton *openExternalBrowserButton = new ToolButton(this);
  openExternalBrowserButton->setIcon(QIcon(":/images/openBrowser"));
  openExternalBrowserButton->setAutoRaise(true);

  ToolButton *deleteButton = new ToolButton(this);
  deleteButton->setIcon(QIcon(":/images/editClear"));
  deleteButton->setToolTip(tr("Delete News"));
  deleteButton->setAutoRaise(true);

  QHBoxLayout *mainLayout = new QHBoxLayout();
  mainLayout->setMargin(0);
  mainLayout->setSpacing(1);
  mainLayout->addWidget(readButton_);
  mainLayout->addWidget(textLabel_);
  mainLayout->addStretch();
  mainLayout->addWidget(openExternalBrowserButton);
  mainLayout->addWidget(deleteButton);
  setLayout(mainLayout);

  connect(readButton_, SIGNAL(clicked()), this, SLOT(markRead()));
  connect(openExternalBrowserButton, SIGNAL(clicked()), this, SLOT(openExternalBrowser()));
  connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteNews()));

  installEventFilter(this);
}

NewsItem::~NewsItem()
{

}

void NewsItem::setText(const QString &text) {
  QString titleStr = textLabel_->fontMetrics().elidedText(
        text, Qt::ElideRight, textLabel_->sizeHint().width());
  textLabel_->setText(titleStr);
  textLabel_->setToolTip(text);
}

void NewsItem::setFontText(const QFont & font) {
  textLabel_->setFont(font);
}

void NewsItem::setColorText(const QString &color) {
  textLabel_->setStyleSheet(QString(
                              "QLabel:hover {color: #1155CC;}"
                              "QLabel {color: %1;}").
                            arg(color));
}

bool NewsItem::eventFilter(QObject *obj, QEvent *event)
{
  if((event->type() == QEvent::MouseButtonPress) && isEnabled()) {
    emit signalTitleClicked(feedId_, newsId_);
    return true;
  } else {
    return QObject::eventFilter(obj, event);
  }
}

void NewsItem::openExternalBrowser()
{
  read_ = 1;
  readButton_->setIcon(QIcon(":/images/bulletRead"));
  QFont font = textLabel_->font();
  font.setBold(false);
  textLabel_->setFont(font);
  emit signalMarkRead(feedId_, newsId_, read_);

  QString linkString;
  QSqlQuery q;
  q.exec(QString("SELECT link_href, link_alternate FROM news WHERE id=='%1'").arg(newsId_));
  if (q.next()) {
    linkString = q.value(0).toString();
    if (linkString.isEmpty()) {
      linkString = q.value(1).toString();
    }
  }
  emit signalOpenExternalBrowser(linkString.simplified());
}

void NewsItem::markRead()
{
  QFont font = textLabel_->font();
  read_ = !read_;
  if (read_) {
    readButton_->setIcon(QIcon(":/images/bulletRead"));
    font.setBold(false);
  }
  else {
    readButton_->setIcon(QIcon(":/images/bulletUnread"));
    font.setBold(true);
  }
  textLabel_->setFont(font);
  emit signalMarkRead(feedId_, newsId_, read_);
}

void NewsItem::deleteNews()
{
  QFont font = textLabel_->font();
  font.setBold(false);
  textLabel_->setFont(font);
  setDisabled(true);
  emit signalDeleteNews(feedId_, newsId_);
}
