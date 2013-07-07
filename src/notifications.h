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

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QtSql>

class NewsItem : public QWidget
{
  Q_OBJECT
public:
  NewsItem(int idFeed, int idNews, int width, QWidget * parent = 0)
    : QWidget(parent)
    , feedId_(idFeed)
    , newsId_(idNews)
    , read_(false)
  {
    setCursor(Qt::PointingHandCursor);

    iconNews_ = new QLabel(this);
    QPixmap icon(":/images/feed");
    iconNews_->setPixmap(icon);
    iconNews_->setFixedSize(icon.size());
    titleNews_ = new QLabel(this);
    titleNews_->setStyleSheet("QLabel:hover {color: #1155CC;}");

    readButton_ = new QToolButton(this);
    readButton_->setIcon(QIcon(":/images/bulletUnread"));
    readButton_->setToolTip(tr("Mark Read/Unread"));
    readButton_->setAutoRaise(true);
    readButton_->hide();

    QToolButton *openExternalBrowserButton = new QToolButton(this);
    openExternalBrowserButton->setIcon(QIcon(":/images/openBrowser"));
    openExternalBrowserButton->setAutoRaise(true);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setMargin(0);
    buttonsLayout->setSpacing(5);
    buttonsLayout->addWidget(iconNews_);
    buttonsLayout->addWidget(titleNews_, 1);
    buttonsLayout->addWidget(readButton_);
    buttonsLayout->addWidget(openExternalBrowserButton);

    setLayout(buttonsLayout);
    installEventFilter(this);

    titleNews_->setFixedWidth(width);

    connect(readButton_, SIGNAL(clicked()),
            this, SLOT(markRead()));
    connect(openExternalBrowserButton, SIGNAL(clicked()),
            this, SLOT(openExternalBrowser()));
  }
  QLabel *iconNews_;
  QLabel *titleNews_;

signals:
  void signalOpenExternalBrowser(const QUrl &url);
  void signalMarkRead(int);
  void signalTitleClicked(int, int);

protected:
  bool eventFilter(QObject *obj, QEvent *event)
  {
    if(event->type() == QEvent::MouseButtonPress) {
      emit signalTitleClicked(feedId_, newsId_);
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
    read_ = !read_;
    if (read_)
      readButton_->setIcon(QIcon(":/images/bulletRead"));
    else
      readButton_->setIcon(QIcon(":/images/bulletUnread"));
    emit signalMarkRead(newsId_);
  }

private:
  int feedId_;
  int newsId_;
  bool read_;
  QToolButton *readButton_;

};

class NotificationWidget : public QWidget
{
  Q_OBJECT
public:
  NotificationWidget(QList<int> idFeedList,
                     QList<int> cntNewNewsList,
                     QWidget *parentWidget, QWidget *parent = 0);

signals:
  void signalShow();
  void signalDelete();
  void signalOpenNews(int feedId, int newsId);
  void signalOpenExternalBrowser(const QUrl &url);

protected:
  virtual void showEvent(QShowEvent*);
  bool eventFilter(QObject *obj, QEvent *event);
  virtual void enterEvent(QEvent*);
  virtual void leaveEvent(QEvent*);

private slots:
  void nextPage();
  void previousPage();
  void markRead(int id);

private:
  QList<int> idFeedList_;
  QList<int> cntNewNewsList_;

  QLabel *iconTitle_;
  QLabel *textTitle_;
  QToolButton *closeButton_;
  QStackedWidget *stackedWidget_;
  QLabel *numPage_;
  QToolButton *leftButton_;
  QToolButton *rightButton_;

  QTimer *showTimer_;
  int timeShowNews_;
  int position_;

};

#endif // NOTIFICATIONS_H
