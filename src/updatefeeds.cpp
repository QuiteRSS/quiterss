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
#include "updatefeeds.h"
#include "rsslisting.h"

#include <QDebug>

UpdateFeeds::UpdateFeeds(QObject *parent, bool add)
  : QObject(parent)
  , requestFeed(NULL)
{
  QObject *parent_ = parent;
  while(parent_->parent()) {
    parent_ = parent_->parent();
  }
  RSSListing *rssl = qobject_cast<RSSListing*>(parent_);

  int timeoutRequest = rssl->settings_->value("Settings/timeoutRequest", 15).toInt();
  int numberRequest = rssl->settings_->value("Settings/numberRequest", 10).toInt();
  int numberRepeats = rssl->settings_->value("Settings/numberRepeats", 2).toInt();

  requestFeed = new RequestFeed(timeoutRequest, numberRequest, numberRepeats);
  requestFeed->networkManager_->setCookieJar(rssl->cookieJar_);

  parseObject_ = new ParseObject(parent);

  if (add) {
    connect(parent, SIGNAL(signalRequestUrl(int,QString,QDateTime,QString)),
            requestFeed, SLOT(requestUrl(int,QString,QDateTime,QString)));
    connect(requestFeed, SIGNAL(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)),
            parent, SLOT(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)),
            Qt::QueuedConnection);

    connect(parent, SIGNAL(xmlReadyParse(QByteArray,int,QDateTime,QString)),
            parseObject_, SLOT(parseXml(QByteArray,int,QDateTime,QString)));
    connect(parseObject_, SIGNAL(feedUpdated(int,bool,int,QString,bool)),
            parent, SLOT(slotUpdateFeed(int,bool,int,QString,bool)),
            Qt::QueuedConnection);
  } else {
    connect(parseObject_, SIGNAL(signalRequestUrl(int,QString,QDateTime,QString)),
            requestFeed, SLOT(requestUrl(int,QString,QDateTime,QString)));
    connect(requestFeed, SIGNAL(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)),
            parseObject_, SLOT(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)));
    connect(requestFeed, SIGNAL(setStatusFeed(int,QString)),
            parent, SLOT(setStatusFeed(int,QString)));

    connect(parent, SIGNAL(signalGetFeed(int,QString,QDateTime,int)),
            parseObject_, SLOT(slotGetFeed(int,QString,QDateTime,int)));
    connect(parent, SIGNAL(signalGetFeedsFolder(QString)),
            parseObject_, SLOT(slotGetFeedsFolder(QString)));
    connect(parent, SIGNAL(signalGetAllFeeds()),
            parseObject_, SLOT(slotGetAllFeeds()));
    connect(parent, SIGNAL(signalImportFeeds(QByteArray)),
            parseObject_, SLOT(slotImportFeeds(QByteArray)));
    connect(parseObject_, SIGNAL(showProgressBar(int)),
            parent, SLOT(showProgressBar(int)),
            Qt::QueuedConnection);
    connect(parseObject_, SIGNAL(loadProgress(int)),
            parent, SLOT(slotSetValue(int)),
            Qt::QueuedConnection);
    connect(parseObject_, SIGNAL(signalMessageStatusBar(QString,int)),
            parent, SLOT(showMessageStatusBar(QString,int)));
    connect(parseObject_, SIGNAL(signalUpdateFeedsModel()),
            parent, SLOT(feedsModelReload()),
            Qt::BlockingQueuedConnection);
    connect(parent, SIGNAL(xmlReadyParse(QByteArray,int,QDateTime,QString)),
            parseObject_, SLOT(parseXml(QByteArray,int,QDateTime,QString)));
    connect(parseObject_, SIGNAL(feedUpdated(int,bool,int,QString,bool)),
            parent, SLOT(slotUpdateFeed(int,bool,int,QString,bool)),
            Qt::BlockingQueuedConnection);

    qRegisterMetaType<FeedCountStruct>("FeedCountStruct");
    connect(parseObject_, SIGNAL(feedCountsUpdate(FeedCountStruct)),
            parent, SLOT(slotFeedCountsUpdate(FeedCountStruct)),
            Qt::QueuedConnection);
  }

  connect(requestFeed->networkManager_,
          SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          parent, SLOT(slotAuthentication(QNetworkReply*,QAuthenticator*)),
          Qt::BlockingQueuedConnection);

  getFeedThread_ = new QThread();
  parseFeedThread_ = new QThread();
  requestFeed->moveToThread(getFeedThread_);
  parseObject_->moveToThread(parseFeedThread_);

  getFeedThread_->start(QThread::LowPriority);
  parseFeedThread_->start(QThread::LowPriority);
}

UpdateFeeds::~UpdateFeeds()
{
  getFeedThread_->exit();
  getFeedThread_->wait();
  getFeedThread_->deleteLater();

  parseFeedThread_->exit();
  parseFeedThread_->wait();
  parseFeedThread_->deleteLater();
}
