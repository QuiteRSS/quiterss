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

UpdateFeeds::UpdateFeeds(QObject *parent)
  : QObject(parent)
  , updateObject_(NULL)
{
  QObject *parent_ = parent;
  while(parent_->parent()) {
    parent_ = parent_->parent();
  }
  RSSListing *rssl = qobject_cast<RSSListing*>(parent_);

  int timeoutRequest = rssl->settings_->value("Settings/timeoutRequest", 15).toInt();
  int numberRequest = rssl->settings_->value("Settings/numberRequest", 10).toInt();
  int numberRepeats = rssl->settings_->value("Settings/numberRepeats", 2).toInt();

  updateObject_ = new UpdateObject(timeoutRequest, numberRequest, numberRepeats);
  updateObject_->networkManager_->setCookieJar(rssl->cookieJar_);

  connect(this, SIGNAL(signalRequestUrl(int,QString,QDateTime,QString)),
          updateObject_, SLOT(requestUrl(int,QString,QDateTime,QString)));
  connect(updateObject_, SIGNAL(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)),
          parent, SLOT(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)),
          Qt::QueuedConnection);
  connect(updateObject_, SIGNAL(setStatusFeed(int,QString)),
          parent, SLOT(setStatusFeed(int,QString)));
  connect(updateObject_->networkManager_,
          SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          parent, SLOT(slotAuthentication(QNetworkReply*,QAuthenticator*)),
          Qt::BlockingQueuedConnection);

  parseObject_ = new ParseObject(parent);
  connect(parent, SIGNAL(xmlReadyParse(QByteArray,int,QDateTime,QString)),
          parseObject_, SLOT(parseXml(QByteArray,int,QDateTime,QString)));
  connect(parseObject_, SIGNAL(feedUpdated(int,bool,int,QString,bool)),
          parent, SLOT(slotUpdateFeed(int,bool,int,QString,bool)),
          Qt::BlockingQueuedConnection);

  qRegisterMetaType<FeedCountStruct>("FeedCountStruct");
  connect(parseObject_, SIGNAL(feedCountsUpdate(FeedCountStruct)),
          parent, SLOT(slotFeedCountsUpdate(FeedCountStruct)),
          Qt::QueuedConnection);

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
