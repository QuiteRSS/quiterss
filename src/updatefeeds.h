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
#ifndef UPDATEFEEDS_H
#define UPDATEFEEDS_H

#include <QThread>

#include "requestfeed.h"
#include "parseobject.h"

class UpdateObject;

class UpdateFeeds : public QObject
{
  Q_OBJECT
public:
  explicit UpdateFeeds(QObject *parent, bool add = false);
  ~UpdateFeeds();

  UpdateObject *updateObject_;
  RequestFeed *requestFeed_;
  ParseObject *parseObject_;
  QThread *getFeedThread_;
  QThread *updateFeedThread_;

};

class UpdateObject : public QObject
{
  Q_OBJECT
public:
  explicit UpdateObject(QObject *parent = 0);


public slots:
  void slotGetFeed(int feedId, QString feedUrl, QDateTime date, int auth);
  void slotGetFeedsFolder(QString query);
  void slotGetAllFeeds();
  void slotImportFeeds(QByteArray xmlData);
  void getUrlDone(int result, int feedId, QString feedUrlStr,
                  QString error, QByteArray data,
                  QDateTime dtReply, QString codecName);
  void finishUpdate(int feedId, bool changed, int newCount, QString status);
  void slotNextUpdateFeed();

signals:
  void showProgressBar(int value);
  void loadProgress(int value, bool clear = false);
  void signalMessageStatusBar(QString message, int timeout = 0);
  void signalUpdateFeedsModel();
  void signalRequestUrl(int feedId, QString urlString,
                        QDateTime date, QString userInfo);
  void xmlReadyParse(QByteArray data, int feedId,
                     QDateTime dtReply, QString codecName);
  void feedUpdated(int feedId, bool changed, int newCount, QString status, bool finish);
  void signalUpdateModel(bool checkFilter = true);

private slots:
  bool addFeedInQueue(int feedId, const QString &feedUrl,
                      const QDateTime &date, int auth);

private:
  QList<int> feedIdList_;
  int updateFeedsCount_;
  QTimer *updateModelTimer_;

};

#endif // UPDATEFEEDS_H
