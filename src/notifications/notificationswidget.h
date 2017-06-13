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
#ifndef NOTIFICATIONSWIDGET_H
#define NOTIFICATIONSWIDGET_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QtSql>

#include "toolbutton.h"

class NotificationWidget : public QWidget
{
  Q_OBJECT
public:
  NotificationWidget(QList<int> idFeedList,
                     QList<int> cntNewNewsList,
                     QList<int> idColorList,
                     QStringList colorList,
                     QWidget *parentWidget,
                     QWidget *parent = 0);
  ~NotificationWidget();

  QList<int> idFeedList() { return idFeedList_; }
  QList<int> idNewsList() { return idNewsList_; }

signals:
  void signalShow();
  void signalClose();
  void signalOpenNews(int feedId, int newsId);
  void signalOpenExternalBrowser(const QUrl &url);
  void signalMarkRead(int feedId, int newsId, int read);
  void signalDeleteNews(int feedId, int newsId);
  void signalMarkAllRead();

protected:
  void showEvent(QShowEvent*);
  bool eventFilter(QObject *obj, QEvent *event);
  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);

private slots:
  void nextPage();
  void previousPage();
  void slotMarkRead(int feedId, int newsId, int read);
  void slotDeleteNews(int feedId, int newsId);
  void slotMarkAllRead();

private:
  void addPage(bool next = true);

  QLabel *iconTitle_;
  QLabel *textTitle_;
  QToolButton *closeButton_;
  QStackedWidget *stackedWidget_;
  QVBoxLayout *pageLayout_;
  QLabel *numPage_;
  ToolButton *prevButton_;
  ToolButton *nextButton_;

  QList<int> idFeedList_;
  QList<int> idNewsList_;
  QTimer *showTimer_;
  int timeShowNews_;
  int screen_;
  int position_;
  int transparency_;
  int cntAllNews_;
  int cntReadNews_;

};

#endif // NOTIFICATIONSWIDGET_H
