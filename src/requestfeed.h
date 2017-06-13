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
#ifndef REQUESTFEED_H
#define REQUESTFEED_H

#include <QDateTime>
#include <QObject>
#include <QQueue>
#include <QNetworkReply>
#include <QTimer>
#include <QElapsedTimer>

#include "networkmanager.h"

class RequestFeed : public QObject
{
  Q_OBJECT
public:
  explicit RequestFeed(int timeoutRequest, int numberRequests,
                       int numberRepeats, QObject *parent = 0);
  ~RequestFeed();

  void disconnectObjects();

public slots:
  void requestUrl(int id, QString urlString, QDateTime date, QString userInfo = "");
  void stopRequest();
  void slotHead(const QUrl &getUrl, const int &id, const QString &feedUrl,
                const QDateTime &date, const int &count);
  void slotGet(const QUrl &getUrl, const int &id, const QString &feedUrl,
               const QDateTime &date, const int &count);

signals:
  void getUrlDone(int result, int feedId, QString feedUrl = "",
                  QString error = "", QByteArray data = NULL,
                  QDateTime dtReply = QDateTime(), QString codecName = "");
  void signalHead(const QUrl &getUrl, const int &id, const QString &feedUrl,
                  const QDateTime &date, const int &count = 0);
  void signalGet(const QUrl &getUrl, const int &id, const QString &feedUrl,
                 const QDateTime &date, const int &count = 0);
  void setStatusFeed(int feedId, QString status);

private slots:
  void getQueuedUrl();
  void finished(QNetworkReply *reply);
  void slotRequestTimeout();

private:
  NetworkManager *networkManager_;

  int timeoutRequest_;
  int numberRequests_;
  int numberRepeats_;
  QTimer *timeout_;
  QTimer *getUrlTimer_;

  QQueue<int> idsQueue_;
  QQueue<QString> feedsQueue_;
  QQueue<QDateTime> dateQueue_;
  QQueue<QString> userInfo_;

  QList<QUrl> currentUrls_;
  QList<int> currentIds_;
  QList<QString> currentFeeds_;
  QList<QDateTime> currentDates_;
  QList<int> currentCount_;
  QList<bool> currentHead_;
  QList<int> currentTime_;
  QList<QUrl> requestUrl_;
  QList<QNetworkReply*> networkReply_;
  QList<QString> hostList_;

};

#endif // REQUESTFEED_H
