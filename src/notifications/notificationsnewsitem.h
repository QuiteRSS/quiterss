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
#ifndef NOTIFICATIONSNEWSITEM_H
#define NOTIFICATIONSNEWSITEM_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QtSql>

#include "toolbutton.h"

class NewsItem : public QWidget
{
  Q_OBJECT
public:
  NewsItem(int idFeed, int idNews, int width, QWidget * parent = 0);
  ~NewsItem();

  void setText(const QString &text);
  void setFontText(const QFont & font);
  void setColorText(const QString &color, const QString &linkColor);

  QLabel *iconLabel_;
  ToolButton *readButton_;
  ToolButton *openExternalBrowserButton_;
  ToolButton *deleteButton_;

signals:
  void signalOpenExternalBrowser(const QUrl &url);
  void signalMarkRead(int feedId, int newsId, int read);
  void signalTitleClicked(int feedId, int newsId);
  void signalDeleteNews(int feedId, int newsId);

protected:
  bool eventFilter(QObject *obj, QEvent *event);

private slots:
  void openExternalBrowser();
  void markRead();
  void deleteNews();

private:
  int feedId_;
  int newsId_;
  bool read_;

  QLabel *textLabel_;

};

#endif // NOTIFICATIONSNEWSITEM_H
