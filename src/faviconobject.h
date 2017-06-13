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
#ifndef FAVICONOBJECT_H
#define FAVICONOBJECT_H

#include <QDateTime>
#include <QObject>
#include <QQueue>
#include <QNetworkReply>
#include <QTimer>

#include "networkmanager.h"

class FaviconObject : public QObject
{
  Q_OBJECT

public:
  explicit FaviconObject(QObject *parent = 0);

  void disconnectObjects();

public slots:
  void requestUrl(QString urlString, QString feedUrl);
  void slotGet(const QUrl &getUrl, const QString &feedUrl, const int &cnt);

signals:
  void startTimer();
  void signalGet(const QUrl &getUrl, const QString &feedUrl, const int &cnt);
  void signalIconRecived(QString feedUrl, QByteArray byteArray, QString format);

private slots:
  void getQueuedUrl();
  void finished(QNetworkReply *reply);
  void slotRequestTimeout();

private:
  NetworkManager *networkManager_;

  QQueue<QString> urlsQueue_;
  QQueue<QString> feedsQueue_;

  QTimer *timeout_;
  QTimer *getUrlTimer_;
  QList<QUrl> currentUrls_;
  QList<QString> currentFeeds_;
  QList<int> currentCntRequests_;
  QList<int> currentTime_;
  QList<QUrl> requestUrl_;
  QList<QNetworkReply*> networkReply_;
  QList<QString> hostList_;

};

#endif // FAVICONOBJECT_H
