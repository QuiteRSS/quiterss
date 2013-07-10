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
#include "updatethread.h"
#include "rsslisting.h"

#include <QDebug>

UpdateThread::UpdateThread(QObject *parent, int requestTimeout, int replyCount)
  : QThread(parent)
  , updateObject_(NULL)
  , requestTimeout_(requestTimeout)
  , replyCount_(replyCount)
{
  qDebug() << "UpdateThread::constructor";

  setObjectName("updateFeedsThread_");
  start(LowPriority);
}

UpdateThread::~UpdateThread()
{
  qDebug() << "UpdateThread::~destructor";
}

/*virtual*/ void UpdateThread::run()
{
  updateObject_ = new UpdateObject(requestTimeout_, replyCount_);

  QObject *parent_ = parent();
  while(parent_->parent()) {
    parent_ = parent_->parent();
  }
  RSSListing *rssl = qobject_cast<RSSListing*>(parent_);
  updateObject_->networkManager_->setCookieJar(rssl->cookieJar_);

  connect(parent(), SIGNAL(signalRequestUrl(QString,QDateTime,QString)),
          updateObject_, SLOT(requestUrl(QString,QDateTime,QString)));
  connect(updateObject_, SIGNAL(getUrlDone(int,QString,QString,QByteArray,QDateTime)),
          parent(), SLOT(getUrlDone(int,QString,QString,QByteArray,QDateTime)));
  connect(updateObject_->networkManager_,
          SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          parent(), SLOT(slotAuthentication(QNetworkReply*,QAuthenticator*)),
          Qt::BlockingQueuedConnection);

  exec();
}
