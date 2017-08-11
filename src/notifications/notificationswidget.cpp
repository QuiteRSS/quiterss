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
#include "notificationswidget.h"

#include "mainapplication.h"
#include "notificationsfeeditem.h"
#include "notificationsnewsitem.h"
#include "optionsdialog.h"

NotificationWidget::NotificationWidget(QList<int> idFeedList,
                                       QList<int> cntNewNewsList,
                                       QList<int> idColorList,
                                       QStringList colorList,
                                       QWidget *parentWidget,
                                       QWidget *parent)
  : QWidget(parent)
  , idFeedList_(idFeedList)
  , cntAllNews_(0)
  , cntReadNews_(0)
{
  Qt::WindowFlags flags;
#ifdef Q_OS_MAC
  flags = Qt::FramelessWindowHint | Qt::SplashScreen | Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint;
#else
  flags = Qt::FramelessWindowHint | Qt::Tool | Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint;
#endif
  setWindowFlags(flags);

  setAttribute(Qt::WA_TranslucentBackground);
  setFocusPolicy(Qt::NoFocus);
  setAttribute(Qt::WA_AlwaysShowToolTips);

  int numberItems;
  int widthList;
  QString fontFamily;
  int fontSize;
  QString textColor;
  QString backgroundColor;
  QString linkColor;
  bool showTitlesFeedsNotify;
  bool showIconFeedNotify;
  bool showButtonMarkAllNotify;
  bool showButtonMarkReadNotify;
  bool showButtonExBrowserNotify;
  bool showButtonDeleteNotify;

  if (idFeedList.count()) {
    screen_ = mainApp->mainWindow()->screenNotify_;
    position_ = mainApp->mainWindow()->positionNotify_;
    transparency_ = mainApp->mainWindow()->transparencyNotify_;
    timeShowNews_ = mainApp->mainWindow()->timeShowNewsNotify_;
    numberItems = mainApp->mainWindow()->countShowNewsNotify_;
    widthList = mainApp->mainWindow()->widthTitleNewsNotify_;
    fontFamily = mainApp->mainWindow()->notificationFontFamily_;
    fontSize = mainApp->mainWindow()->notificationFontSize_;
    textColor = mainApp->mainWindow()->notifierTextColor_;
    backgroundColor = mainApp->mainWindow()->notifierBackgroundColor_;
    linkColor = mainApp->mainWindow()->linkColor_;

    showTitlesFeedsNotify = mainApp->mainWindow()->showTitlesFeedsNotify_;
    showIconFeedNotify = mainApp->mainWindow()->showIconFeedNotify_;
    showButtonMarkAllNotify = mainApp->mainWindow()->showButtonMarkAllNotify_;
    showButtonMarkReadNotify = mainApp->mainWindow()->showButtonMarkReadNotify_;
    showButtonExBrowserNotify = mainApp->mainWindow()->showButtonExBrowserNotify_;
    showButtonDeleteNotify = mainApp->mainWindow()->showButtonDeleteNotify_;
  } else {
    OptionsDialog *options = qobject_cast<OptionsDialog*>(parentWidget);
    screen_ = options->screenNotify_->currentIndex()-1;
    position_ = options->positionNotify_->currentIndex();
    transparency_ = options->transparencyNotify_->value();
    timeShowNews_ = options->timeShowNewsNotify_->value();
    numberItems = options->countShowNewsNotify_->value();
    widthList = options->widthTitleNewsNotify_->value();
    fontFamily = options->fontsTree_->topLevelItem(4)->text(2).section(", ", 0, 0);
    fontSize = options->fontsTree_->topLevelItem(4)->text(2).section(", ", 1).toInt();
    textColor = options->colorsTree_->topLevelItem(21)->text(1);
    backgroundColor = options->colorsTree_->topLevelItem(22)->text(1);
    linkColor = options->colorsTree_->topLevelItem(6)->text(1);

    showTitlesFeedsNotify = options->showTitlesFeedsNotify_->isChecked();
    showIconFeedNotify = options->showIconFeedNotify_->isChecked();
    showButtonMarkAllNotify = options->showButtonMarkAllNotify_->isChecked();
    showButtonMarkReadNotify = options->showButtonMarkReadNotify_->isChecked();
    showButtonExBrowserNotify = options->showButtonExBrowserNotify_->isChecked();
    showButtonDeleteNotify = options->showButtonDeleteNotify_->isChecked();

    for (int i = 0; i < 10; i++) {
      cntNewNewsList << 9;
    }
  }

  if (screen_ == -1) {
    screen_ = QApplication::desktop()->screenNumber(mainApp->mainWindow());
  }

  int transparency = 255*(100-transparency_)/100;
  QColor color(backgroundColor);
  int redColor = color.red()-20;
  if (redColor < 0) redColor = 0;
  int greenColor = color.green()-20;
  if (greenColor < 0) greenColor = 0;
  int blueColor = color.blue()-20;
  if (blueColor < 0) blueColor = 0;

  iconTitle_ = new QLabel(this);
  iconTitle_->setPixmap(QPixmap(":/images/quiterss16"));
  textTitle_ = new QLabel(this);

  closeButton_ = new QToolButton(this);
  closeButton_->setToolTip(tr("Close"));
  closeButton_->setStyleSheet(
        "QToolButton { border: none; padding: 0px; "
        "image: url(:/images/close); }"
        "QToolButton:hover {"
        "image: url(:/images/closeHover); }");

  QHBoxLayout *titleLayout = new QHBoxLayout();
  titleLayout->setMargin(4);
  titleLayout->setSpacing(5);
  titleLayout->addWidget(iconTitle_);
  titleLayout->addWidget(textTitle_, 1);
  titleLayout->addWidget(closeButton_);

  QWidget *titlePanel_ = new QWidget(this);
  titlePanel_->setCursor(Qt::PointingHandCursor);
  titlePanel_->setObjectName("titleNotification");
  titlePanel_->setLayout(titleLayout);
  titlePanel_->installEventFilter(this);
  titlePanel_->setStyleSheet(QString("#titleNotification { background: rgba(%1, %2, %3, %4); }").
                             arg(redColor).arg(greenColor).
                             arg(blueColor).arg(transparency));

  numPage_ = new QLabel(this);

  ToolButton *markAllReadButton_ = new ToolButton(this);
  markAllReadButton_->setToolTip(tr("Mark All News Read"));
  markAllReadButton_->setIcon(QIcon(":/images/markReadAll"));
  markAllReadButton_->setAutoRaise(true);
  prevButton_ = new ToolButton(this);
  prevButton_->setIcon(QIcon(":/images/moveLeft"));
  prevButton_->setToolTip(tr("Previous Page"));
  prevButton_->setEnabled(false);
  prevButton_->setAutoRaise(true);
  nextButton_ = new ToolButton(this);
  nextButton_->setIcon(QIcon(":/images/moveRight"));
  nextButton_->setToolTip(tr("Next Page"));
  nextButton_->setAutoRaise(true);

  QHBoxLayout *bottomLayout = new QHBoxLayout();
  bottomLayout->setMargin(2);
  bottomLayout->setSpacing(1);
  bottomLayout->addSpacing(3);
  bottomLayout->addWidget(numPage_);
  bottomLayout->addStretch(1);
  if (showButtonMarkAllNotify) {
    bottomLayout->addWidget(markAllReadButton_);
    bottomLayout->addSpacing(5);
  }
  bottomLayout->addWidget(prevButton_);
  bottomLayout->addWidget(nextButton_);
  bottomLayout->addSpacing(3);

  QWidget *bottomPanel_ = new QWidget(this);
  bottomPanel_->setObjectName("bottomNotification");
  bottomPanel_->setLayout(bottomLayout);
  bottomPanel_->setStyleSheet(QString("#bottomNotification { background: rgba(%1, %2, %3, %4); }").
                              arg(redColor).arg(greenColor).
                              arg(blueColor).arg(transparency));

  stackedWidget_ = new QStackedWidget(this);

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setMargin(1);
  mainLayout->setSpacing(0);
  mainLayout->addWidget(titlePanel_);
  mainLayout->addWidget(stackedWidget_);
  mainLayout->addWidget(bottomPanel_);

  QWidget *mainWidget = new QWidget(this);
  mainWidget->setObjectName("notificationWidget");
  mainWidget->setLayout(mainLayout);
  mainWidget->setMouseTracking(true);
  mainWidget->setStyleSheet(QString("#notificationWidget { background: rgba(%1, %2, %3, %4); }").
                            arg(color.red()).arg(color.green()).
                            arg(color.blue()).arg(transparency));

  QVBoxLayout *layout = new QVBoxLayout();
  layout->setMargin(0);
  layout->addWidget(mainWidget);

  setLayout(layout);

  addPage(false);

  if (!idFeedList.isEmpty()) {
    int countItems = 0;
    for (int i = 0; i < idFeedList.count(); i++) {
      QSqlQuery q;
      QString qStr;
      QString titleFeed;
      QPixmap icon;
      int idFeed = idFeedList[i];
      int cntNews;

      qStr = QString("SELECT text, image, newCount FROM feeds WHERE id=='%1'").
          arg(idFeed);
      q.exec(qStr);
      if (q.next()) {
        cntNews = q.value(2).toInt();
        if (!cntNews)
          continue;

        titleFeed = q.value(0).toString();
        QByteArray byteArray = q.value(1).toByteArray();
        if (!byteArray.isNull()) {
          icon.loadFromData(QByteArray::fromBase64(byteArray));
        } else {
          icon.load(":/images/feed");
        }
      } else {
        continue;
      }
      cntAllNews_ = cntAllNews_ + cntNewNewsList[i];

      if (showTitlesFeedsNotify) {
        if (countItems >= (numberItems - 1)) {
          countItems = 1;
          addPage();
        } else countItems++;

        FeedItem *feedItem = new FeedItem(widthList, this);
        feedItem->setIcon(icon);
        feedItem->setFontTitle(QFont(fontFamily, fontSize, -1, true));
        feedItem->setColorText(textColor);
        feedItem->setTitle(titleFeed, cntNewNewsList[i]);
        pageLayout_->addWidget(feedItem);
      }

      int cntNewNews = 0;

      qStr = QString("SELECT id, title FROM news WHERE new=1 AND feedId=='%1' ORDER BY received DESC").
          arg(idFeed);
      q.exec(qStr);
      while (q.next()) {
        if (cntNewNews >= cntNewNewsList[i]) break;
        else cntNewNews++;

        if (countItems >= numberItems) {
          addPage();
          if (showTitlesFeedsNotify) {
            countItems = 2;

            FeedItem *feedItem = new FeedItem(widthList, this);
            feedItem->setIcon(icon);
            feedItem->setFontTitle(QFont(fontFamily, fontSize, -1, true));
            feedItem->setColorText(textColor);
            feedItem->setTitle(titleFeed, cntNewNewsList[i]);
            pageLayout_->addWidget(feedItem);
          } else {
            countItems = 1;
          }
        } else countItems++;

        int idNews = q.value(0).toInt();
        idNewsList_.append(idNews);
        NewsItem *newsItem = new NewsItem(idFeed, idNews, widthList, this);
        newsItem->setFontText(QFont(fontFamily, fontSize, QFont::Bold));
        newsItem->setText(q.value(1).toString());
        int index = idColorList.indexOf(idNews);
        if (index != -1)
          newsItem->setColorText(colorList.at(index), linkColor);
        else
          newsItem->setColorText(textColor, linkColor);
        newsItem->iconLabel_->setPixmap(icon);
        newsItem->iconLabel_->setToolTip(titleFeed);
        newsItem->readButton_->setVisible(showButtonMarkReadNotify);
        newsItem->iconLabel_->setVisible(showIconFeedNotify);
        newsItem->openExternalBrowserButton_->setVisible(showButtonExBrowserNotify);
        newsItem->deleteButton_->setVisible(showButtonDeleteNotify);
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
      if (showTitlesFeedsNotify) {
        if (countItems >= (numberItems - 1)) {
          countItems = 1;
          addPage();
        } else countItems++;

        FeedItem *feedItem = new FeedItem(widthList, this);
        feedItem->setIcon(QPixmap(":/images/feed"));
        feedItem->setFontTitle(QFont(fontFamily, fontSize, -1, true));
        feedItem->setColorText(textColor);
        feedItem->setTitle(QString("Title Feed %1").arg(i+1), 9);
        pageLayout_->addWidget(feedItem);
      }

      for (int y = 0; y < cntNewNewsList.at(i); y++) {
        if (countItems >= numberItems) {
          addPage();
          if (showTitlesFeedsNotify) {
            countItems = 2;

            FeedItem *feedItem = new FeedItem(widthList, this);
            feedItem->setIcon(QPixmap(":/images/feed"));
            feedItem->setFontTitle(QFont(fontFamily, fontSize, -1, true));
            feedItem->setColorText(textColor);
            feedItem->setTitle(QString("Title Feed %1").arg(i+1), 9);
            pageLayout_->addWidget(feedItem);
          } else {
            countItems = 1;
          }
        } else countItems++;

        NewsItem *newsItem = new NewsItem(0, 0, widthList, this);
        newsItem->setFontText(QFont(fontFamily, fontSize, QFont::Bold));
        newsItem->setColorText(textColor, linkColor);
        newsItem->setText("Test News Test News Test News Test News Test News");
        newsItem->iconLabel_->setPixmap(QPixmap(":/images/feed"));
        newsItem->iconLabel_->setToolTip(QString("Title Feed %1").arg(i+1));
        newsItem->readButton_->setVisible(showButtonMarkReadNotify);
        newsItem->iconLabel_->setVisible(showIconFeedNotify);
        newsItem->openExternalBrowserButton_->setVisible(showButtonExBrowserNotify);
        newsItem->deleteButton_->setVisible(showButtonDeleteNotify);
        pageLayout_->addWidget(newsItem);
      }
    }
    foreach (int cntNews, cntNewNewsList) {
      cntAllNews_ = cntAllNews_ + cntNews;
    }
  }

  textTitle_->setText(QString(tr("Incoming News: %1")).arg(cntAllNews_));
  if ((cntAllNews_ + idFeedList.count()) > numberItems) nextButton_->setEnabled(true);
  else nextButton_->setEnabled(false);

  pageLayout_->addStretch();
  numPage_->setText(QString(tr("Page %1 of %2").arg("1").arg(stackedWidget_->count())));

  showTimer_ = new QTimer(this);
  connect(showTimer_, SIGNAL(timeout()),
          this, SIGNAL(signalClose()));
  connect(closeButton_, SIGNAL(clicked()),
          this, SIGNAL(signalClose()));
  connect(prevButton_, SIGNAL(clicked()),
          this, SLOT(previousPage()));
  connect(nextButton_, SIGNAL(clicked()),
          this, SLOT(nextPage()));
  connect(markAllReadButton_, SIGNAL(clicked()),
          this, SIGNAL(signalMarkAllRead()));
  connect(markAllReadButton_, SIGNAL(clicked()),
          this, SIGNAL(signalClose()));

  if (timeShowNews_ != 0)
    showTimer_->start(timeShowNews_*1000);
}

NotificationWidget::~NotificationWidget()
{

}

void NotificationWidget::showEvent(QShowEvent*)
{
  QPoint point;
  switch (position_) {
  case 0:
    point = QPoint(QApplication::desktop()->availableGeometry(screen_).topLeft().x(),
                   QApplication::desktop()->availableGeometry(screen_).topLeft().y());
    break;
  case 1:
    point = QPoint(QApplication::desktop()->availableGeometry(screen_).topRight().x()-width(),
                   QApplication::desktop()->availableGeometry(screen_).topRight().y());
    break;
  case 2:
    point = QPoint(QApplication::desktop()->availableGeometry(screen_).bottomLeft().x(),
                   QApplication::desktop()->availableGeometry(screen_).bottomLeft().y()-height());
    break;
  default:
    point = QPoint(QApplication::desktop()->availableGeometry(screen_).bottomRight().x()-width(),
                   QApplication::desktop()->availableGeometry(screen_).bottomRight().y()-height());
    break;
  }
  move(point);
}

bool NotificationWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::MouseButtonPress) {
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
    nextButton_->setEnabled(false);
  if (stackedWidget_->currentIndex() != 0)
    prevButton_->setEnabled(true);
  numPage_->setText(QString(tr("Page %1 of %2").
                            arg(stackedWidget_->currentIndex()+1).
                            arg(stackedWidget_->count())));
}

void NotificationWidget::previousPage()
{
  stackedWidget_->setCurrentIndex(stackedWidget_->currentIndex()-1);
  if (stackedWidget_->currentIndex() == 0)
    prevButton_->setEnabled(false);
  if (stackedWidget_->currentIndex()+1 != stackedWidget_->count())
    nextButton_->setEnabled(true);
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
  idNewsList_.removeOne(newsId);
  emit signalDeleteNews(feedId, newsId);
}

void NotificationWidget::slotMarkAllRead()
{
}
