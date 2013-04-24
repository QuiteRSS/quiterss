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
#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <QtGui>
#include <QtSql>

class NewsItem : public QWidget
{
  Q_OBJECT
public:
  NewsItem(int idFeed, int parIdFeed, int idNews, int width, QWidget * parent = 0) :
    QWidget(parent), feedId_(idFeed), feedParId_(parIdFeed), newsId_(idNews)
  {
    read = false;

    setCursor(Qt::PointingHandCursor);

    iconNews = new QLabel(this);
    QPixmap icon(":/images/feed");
    iconNews->setPixmap(icon);
    iconNews->setFixedSize(icon.size());
    titleNews = new QLabel(this);
    titleNews->setStyleSheet("QLabel:hover {color: #1155CC;}");

    readButton = new QToolButton(this);
    readButton->setIcon(QIcon(":/images/bulletUnread"));
    readButton->setToolTip(tr("Mark Read/Unread"));
    readButton->setAutoRaise(true);
    readButton->hide();

    QToolButton *openExternalBrowserButton = new QToolButton(this);
    openExternalBrowserButton->setIcon(QIcon(":/images/openBrowser"));
    openExternalBrowserButton->setAutoRaise(true);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setMargin(0);
    buttonsLayout->setSpacing(5);
    buttonsLayout->addWidget(iconNews);
    buttonsLayout->addWidget(titleNews, 1);
    buttonsLayout->addWidget(readButton);
    buttonsLayout->addWidget(openExternalBrowserButton);

    setLayout(buttonsLayout);
    installEventFilter(this);

    titleNews->setFixedWidth(width);

    connect(readButton, SIGNAL(clicked()),
            this, SLOT(markRead()));
    connect(openExternalBrowserButton, SIGNAL(clicked()),
            this, SLOT(openExternalBrowser()));
  }
  int feedId_;
  int feedParId_;
  int newsId_;
  QLabel *iconNews;
  QLabel *titleNews;
  QToolButton *readButton;
  bool read;

protected:
  bool eventFilter(QObject *obj, QEvent *event)
  {
    if(event->type() == QEvent::MouseButtonPress) {
      emit signalTitleClicked(feedId_, feedParId_, newsId_);
      return true;
    } else {
      return QObject::eventFilter(obj, event);
    }
  }

private slots:
  void openExternalBrowser()
  {
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

  void markRead()
  {
    read = !read;
    if (read)
      readButton->setIcon(QIcon(":/images/bulletRead"));
    else
      readButton->setIcon(QIcon(":/images/bulletUnread"));
    emit signalMarkRead(newsId_);
  }

signals:
  void signalOpenExternalBrowser(const QUrl &url);
  void signalMarkRead(int);
  void signalTitleClicked(int, int, int);

};

class NotificationWidget : public QWidget
{
  Q_OBJECT
public:
  NotificationWidget(QList<int> idFeedList,
                     QList<int> cntNewNewsList,
                     int countShowNews, int timeShowNews, int widthTitleNews,
                     QString fontFamily, int fontSize,
                     QWidget * parent = 0);

protected:
  virtual void showEvent(QShowEvent*);
  bool eventFilter(QObject *obj, QEvent *event);
  /*virtual*/ void enterEvent(QEvent*);
  /*virtual*/ void leaveEvent(QEvent*);

private:
  QLabel *iconTitle_;
  QLabel *textTitle_;
  QToolButton *closeButton_;
  QStackedWidget *stackedWidget_;
  QLabel *numPage_;
  QToolButton *leftButton_;
  QToolButton *rightButton_;

  QTimer *showTimer_;

  QList<int> idFeedList_;
  QList<int> cntNewNewsList_;
  int countShowNews_;
  int timeShowNews_;
  int widthTitleNews_;

private slots:
  void nextPage();
  void previousPage();
  void markRead(int id);


signals:
  void signalShow();
  void signalDelete();
  void signalOpenNews(int feedId, int feedParId, int newsId);
  void signalOpenExternalBrowser(const QUrl &url);

};

#endif // NOTIFICATIONS_H
