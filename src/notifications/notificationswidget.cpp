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
#include "notificationswidget.h"
#include "notificationsfeeditem.h"
#include "notificationsnewsitem.h"
#include "optionsdialog.h"
#include "rsslisting.h"

NotificationWidget::NotificationWidget(QList<int> idFeedList,
                                       QList<int> cntNewNewsList,
                                       QList<int> idColorList,
                                       QStringList colorList,
                                       QWidget *parentWidget,
                                       QWidget *parent)
  : QWidget(parent,  Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
  , cntAllNews_(0)
  , cntReadNews_(0)
{
  setAttribute(Qt::WA_TranslucentBackground);
  setFocusPolicy(Qt::NoFocus);
  setAttribute(Qt::WA_AlwaysShowToolTips);

  int numberItems;
  int widthList;
  QString fontFamily;
  int fontSize;

  if (idFeedList.count()) {
    RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget);
    position_ = rssl_->positionNotify_;
    timeShowNews_ = rssl_->timeShowNewsNotify_;
    numberItems = rssl_->countShowNewsNotify_;
    widthList = rssl_->widthTitleNewsNotify_;
    fontFamily = rssl_->notificationFontFamily_;
    fontSize = rssl_->notificationFontSize_;
  } else {
    OptionsDialog *options = qobject_cast<OptionsDialog*>(parentWidget);
    position_ = options->positionNotify_->currentIndex();
    timeShowNews_ = options->timeShowNewsNotify_->value();
    numberItems = options->countShowNewsNotify_->value();
    widthList = options->widthTitleNewsNotify_->value();
    fontFamily = options->fontsTree_->topLevelItem(4)->text(2).section(", ", 0, 0);
    fontSize = options->fontsTree_->topLevelItem(4)->text(2).section(", ", 1).toInt();

    for (int i = 0; i < 10; i++) {
      cntNewNewsList << 9;
    }
  }

  iconTitle_ = new QLabel(this);
  iconTitle_->setPixmap(QPixmap(":/images/quiterss16"));
  textTitle_ = new QLabel(this);

  closeButton_ = new QToolButton(this);
  closeButton_->setStyleSheet(
        "QToolButton { border: none; padding: 0px; "
        "image: url(:/images/close); }"
        "QToolButton:hover {"
        "image: url(:/images/closeHover); }");

  QHBoxLayout *titleLayout = new QHBoxLayout();
  titleLayout->setMargin(5);
  titleLayout->setSpacing(5);
  titleLayout->addWidget(iconTitle_);
  titleLayout->addWidget(textTitle_, 1);
  titleLayout->addWidget(closeButton_);

  QWidget *titlePanel_ = new QWidget(this);
  titlePanel_->setCursor(Qt::PointingHandCursor);
  titlePanel_->setObjectName("titleNotification");
  titlePanel_->setLayout(titleLayout);
  titlePanel_->installEventFilter(this);

  numPage_ = new QLabel(this);

  leftButton_ = new QToolButton(this);
  leftButton_->setIcon(QIcon(":/images/moveLeft"));
  leftButton_->setCursor(Qt::PointingHandCursor);
  leftButton_->setEnabled(false);
  rightButton_ = new QToolButton(this);
  rightButton_->setIcon(QIcon(":/images/moveRight"));
  rightButton_->setCursor(Qt::PointingHandCursor);

  QHBoxLayout *bottomLayout = new QHBoxLayout();
  bottomLayout->setMargin(2);
  bottomLayout->setSpacing(5);
  bottomLayout->addSpacing(3);
  bottomLayout->addWidget(numPage_);
  bottomLayout->addStretch(1);
  bottomLayout->addWidget(leftButton_);
  bottomLayout->addWidget(rightButton_);
  bottomLayout->addSpacing(3);

  QWidget *bottomPanel_ = new QWidget(this);
  bottomPanel_->setObjectName("bottomNotification");
  bottomPanel_->setLayout(bottomLayout);

  stackedWidget_ = new QStackedWidget(this);

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setMargin(2);
  mainLayout->setSpacing(0);
  mainLayout->addWidget(titlePanel_);
  mainLayout->addWidget(stackedWidget_);
  mainLayout->addWidget(bottomPanel_);

  QWidget *mainWidget = new QWidget(this);
  mainWidget->setObjectName("notificationWidget");
  mainWidget->setLayout(mainLayout);
  mainWidget->setMouseTracking(true);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->setMargin(0);
  layout->addWidget(mainWidget);

  setLayout(layout);

  foreach (int cntNews, cntNewNewsList) {
    cntAllNews_ = cntAllNews_ + cntNews;
  }
  textTitle_->setText(QString(tr("Incoming News: %1")).arg(cntAllNews_));

  if ((cntAllNews_ + idFeedList.count()) > numberItems) rightButton_->setEnabled(true);
  else rightButton_->setEnabled(false);

  addPage(false);

  if (!idFeedList.isEmpty()) {
    QSqlQuery q;
    int countItems = 0;
    for (int i = 0; i < idFeedList.count(); i++) {
      int idFeed = idFeedList[i];
      QString qStr = QString("SELECT text, image FROM feeds WHERE id=='%1'").
          arg(idFeed);
      q.exec(qStr);
      QString titleFeed;
      QPixmap icon;
      if (q.next()) {
        titleFeed = q.value(0).toString();
        QByteArray byteArray = q.value(1).toByteArray();
        if (!byteArray.isNull()) {
          icon.loadFromData(QByteArray::fromBase64(byteArray));
        } else {
          icon.load(":/images/feed");
        }
      }

      if (countItems >= (numberItems - 1)) {
        countItems = 1;
        addPage();
      } else countItems++;

      FeedItem *feedItem = new FeedItem(widthList, this);
      feedItem->setIcon(icon);
      feedItem->setFontTitle(QFont(fontFamily, fontSize, -1, true));
      feedItem->setTitle(titleFeed, cntNewNewsList[i]);
      pageLayout_->addWidget(feedItem);

      int cntNewNews = 0;
      qStr = QString("SELECT id, title FROM news WHERE new=1 AND feedId=='%1'").
          arg(idFeed);
      q.exec(qStr);
      while (q.next()) {
        if (cntNewNews >= cntNewNewsList[i]) break;
        else cntNewNews++;

        if (countItems >= numberItems) {
          countItems = 1;
          addPage();

          FeedItem *feedItem = new FeedItem(widthList, this);
          feedItem->setIcon(icon);
          feedItem->setFontTitle(QFont(fontFamily, fontSize, -1, true));
          feedItem->setTitle(titleFeed, cntNewNewsList[i]);
          pageLayout_->addWidget(feedItem);
        } else countItems++;

        int idNews = q.value(0).toInt();
        NewsItem *newsItem = new NewsItem(idFeed, idNews, widthList, this);
        newsItem->setFontText(QFont(fontFamily, fontSize, QFont::Bold));
        newsItem->setText(q.value(1).toString());
        int index = idColorList.indexOf(idNews);
        if (index != -1)
          newsItem->setColorText(colorList.at(index));
        pageLayout_->addWidget(newsItem);

        connect(newsItem, SIGNAL(signalMarkRead(int, int, int)),
                this, SLOT(slotMarkRead(int, int, int)));
        connect(newsItem, SIGNAL(signalTitleClicked(int, int)),
                this, SIGNAL(signalOpenNews(int, int)));
        connect(newsItem, SIGNAL(signalOpenExternalBrowser(QUrl)),
                this, SIGNAL(signalOpenExternalBrowser(QUrl)));
        connect(newsItem, SIGNAL(signalDeleteNews(int,int)),
                this, SLOT(slotDeleteNews(int, int)));
      }
    }
  }
  // Review
  else {
    int countItems = 0;
    for (int i = 0; i < 10; i++) {
      if (countItems >= (numberItems - 1)) {
        countItems = 1;
        addPage();
      } else countItems++;

      FeedItem *feedItem = new FeedItem(widthList, this);
      feedItem->setIcon(QPixmap(":/images/feed"));
      feedItem->setFontTitle(QFont(fontFamily, fontSize, -1, true));
      feedItem->setTitle(QString("Title Feed %1").arg(i+1), 9);
      pageLayout_->addWidget(feedItem);

      for (int y = 0; y < cntNewNewsList.at(i); y++) {
        if (countItems >= numberItems) {
          countItems = 1;
          addPage();

          FeedItem *feedItem = new FeedItem(widthList, this);
          feedItem->setIcon(QPixmap(":/images/feed"));
          feedItem->setFontTitle(QFont(fontFamily, fontSize, -1, true));
          feedItem->setTitle(QString("Title Feed %1").arg(i+1), 9);
          pageLayout_->addWidget(feedItem);
        } else countItems++;

        NewsItem *newsItem = new NewsItem(0, 0, widthList, this);
        newsItem->setFontText(QFont(fontFamily, fontSize, QFont::Bold));
        newsItem->setText("Test News Test News Test News Test News Test News");
        pageLayout_->addWidget(newsItem);
      }
    }
  }

  pageLayout_->addStretch();
  numPage_->setText(QString(tr("Page %1 of %2").arg("1").arg(stackedWidget_->count())));

  showTimer_ = new QTimer(this);
  connect(showTimer_, SIGNAL(timeout()),
          this, SIGNAL(signalClose()));
  connect(closeButton_, SIGNAL(clicked()),
          this, SIGNAL(signalClose()));
  connect(leftButton_, SIGNAL(clicked()),
          this, SLOT(previousPage()));
  connect(rightButton_, SIGNAL(clicked()),
          this, SLOT(nextPage()));

  if (timeShowNews_ != 0)
    showTimer_->start(timeShowNews_*1000);
}

NotificationWidget::~NotificationWidget()
{

}

void NotificationWidget::showEvent(QShowEvent*)
{
  QPoint point;
  switch(position_) {
  case 0:
    point = QPoint(QApplication::desktop()->availableGeometry(0).topLeft().x()+1,
                   QApplication::desktop()->availableGeometry(0).topLeft().y()+1);
    break;
  case 1:
    point = QPoint(QApplication::desktop()->availableGeometry(0).topRight().x()-width()-1,
                   QApplication::desktop()->availableGeometry(0).topRight().y()+1);
    break;
  case 2:
    point = QPoint(QApplication::desktop()->availableGeometry(0).bottomLeft().x()+1,
                   QApplication::desktop()->availableGeometry(0).bottomLeft().y()-height()-1);
    break;
  default:
    point = QPoint(QApplication::desktop()->availableGeometry(0).bottomRight().x()-width()-1,
                   QApplication::desktop()->availableGeometry(0).bottomRight().y()-height()-1);
    break;
  }
  move(point);
}

bool NotificationWidget::eventFilter(QObject *obj, QEvent *event)
{
  if(event->type() == QEvent::MouseButtonPress) {
    emit signalShow();
    emit signalClose();
    return true;
  } else {
    return QObject::eventFilter(obj, event);
  }
}

void NotificationWidget::enterEvent(QEvent*)
{
  showTimer_->stop();
}

void NotificationWidget::leaveEvent(QEvent*)
{
  if (timeShowNews_ != 0)
    showTimer_->start(timeShowNews_*1000);
}

void NotificationWidget::addPage(bool next)
{
  if (next) pageLayout_->addStretch();

  pageLayout_ = new QVBoxLayout();
  pageLayout_->setMargin(5);
  pageLayout_->setSpacing(0);
  QWidget *pageWidget = new QWidget(this);
  pageWidget->setLayout(pageLayout_);
  stackedWidget_->addWidget(pageWidget);
}

void NotificationWidget::nextPage()
{
  stackedWidget_->setCurrentIndex(stackedWidget_->currentIndex()+1);
  if (stackedWidget_->currentIndex()+1 == stackedWidget_->count())
    rightButton_->setEnabled(false);
  if (stackedWidget_->currentIndex() != 0)
    leftButton_->setEnabled(true);
  numPage_->setText(QString(tr("Page %1 of %2").
                            arg(stackedWidget_->currentIndex()+1).
                            arg(stackedWidget_->count())));
}

void NotificationWidget::previousPage()
{
  stackedWidget_->setCurrentIndex(stackedWidget_->currentIndex()-1);
  if (stackedWidget_->currentIndex() == 0)
    leftButton_->setEnabled(false);
  if (stackedWidget_->currentIndex()+1 != stackedWidget_->count())
    rightButton_->setEnabled(true);
  numPage_->setText(QString(tr("Page %1 of %2").
                            arg(stackedWidget_->currentIndex()+1).
                            arg(stackedWidget_->count())));
}

void NotificationWidget::slotMarkRead(int feedId, int newsId, int read)
{
  if (read) cntReadNews_++;
  else cntReadNews_--;
  if (cntReadNews_ == cntAllNews_) {
    emit signalClose();
  }
  emit signalMarkRead(feedId, newsId, read);
}

void NotificationWidget::slotDeleteNews(int feedId, int newsId)
{
  cntReadNews_++;
  if (cntReadNews_ == cntAllNews_) {
    emit signalClose();
  }
  emit signalDeleteNews(feedId, newsId);
}
