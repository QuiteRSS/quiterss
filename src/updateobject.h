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
#ifndef UPDATEOBJECT_H
#define UPDATEOBJECT_H

#include <QDateTime>
#include <QObject>
#include <QQueue>
#include <QNetworkReply>
#include <QTimer>
#include <QElapsedTimer>

#include "networkmanager.h"

class UpdateObject : public QObject
{
  Q_OBJECT
public:
  explicit UpdateObject(int requestTimeout, QObject *parent = 0);
  NetworkManager *networkManager_;

public slots:
  void requestUrl(const QString &urlString, const QDateTime &date, const QString &userInfo = "");
  void slotHead(const QUrl &getUrl, const QString &feedUrl, const QDateTime &date, const int &count);
  void slotGet(const QUrl &getUrl, const QString &feedUrl, const QDateTime &date, const int &count);

signals:
  void getUrlDone(const int &result, const QString &feedUrl = "",
                  const QByteArray &data = NULL, const QDateTime &dtReply = QDateTime());
  void signalHead(const QUrl &getUrl, const QString &feedUrl,
                  const QDateTime &date, const int &count = 0);
  void signalGet(const QUrl &getUrl, const QString &feedUrl,
                 const QDateTime &date, const int &count = 0);

private slots:
  void getQueuedUrl();
  void finished(QNetworkReply *reply);
  void slotRequestTimeout();

private:
  int requestTimeout_;
  QTimer *getUrlTimer_;
  QElapsedTimer timer_;

  QQueue<QString> feedsQueue_;
  QQueue<QDateTime> dateQueue_;
  QQueue<QString> userInfo_;

  QList<QUrl> currentUrls_;
  QList<QString> currentFeeds_;
  QList<QDateTime> currentDates_;
  QList<int> currentCount_;
  QList<bool> currentHead_;
  QList<int> currentTime_;
  QList<QUrl> requestUrl_;
  QList<QNetworkReply*> networkReply_;

};

#endif // UPDATEOBJECT_H
