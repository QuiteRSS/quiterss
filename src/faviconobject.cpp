/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2020 QuiteRSS Team <quiterssteam@gmail.com>
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
#include "faviconobject.h"
#include "VersionNo.h"
#include "mainapplication.h"
#include "globals.h"

#include <QDebug>
#include <QtSql>
#include <qzregexp.h>

#define REPLY_MAX_COUNT 4
#define REQUEST_TIMEOUT 30

FaviconObject::FaviconObject(QObject *parent)
  : QObject(parent)
  , networkManager_(NULL)
{
  setObjectName("faviconObject_");

  timeout_ = new QTimer(this);
  timeout_->setInterval(1000);
  connect(timeout_, SIGNAL(timeout()), this, SLOT(slotRequestTimeout()));

  getUrlTimer_ = new QTimer(this);
  getUrlTimer_->setSingleShot(true);
  getUrlTimer_->setInterval(20);
  connect(getUrlTimer_, SIGNAL(timeout()), this, SLOT(getQueuedUrl()));

  connect(this, SIGNAL(signalGet(QUrl,QString,int)),
          SLOT(slotGet(QUrl,QString,int)));
}

void FaviconObject::disconnectObjects()
{
  disconnect(this);
  if (networkManager_)
    networkManager_->disconnect(networkManager_);
}

/** @brief Put requested URL in request queue
 *----------------------------------------------------------------------------*/
void FaviconObject::requestUrl(QString urlString, QString feedUrl)
{
  if (!networkManager_) {
    networkManager_ = new NetworkManager(true, this);
    connect(networkManager_, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(finished(QNetworkReply*)));
  }

  if (!timeout_->isActive())
    timeout_->start();

  urlsQueue_.enqueue(urlString);
  feedsQueue_.enqueue(feedUrl);

  if (!getUrlTimer_->isActive())
    getUrlTimer_->start();
}

/** @brief Process request queue by timer
 *----------------------------------------------------------------------------*/
void FaviconObject::getQueuedUrl()
{
  if (currentFeeds_.size() >= REPLY_MAX_COUNT) {
    getUrlTimer_->start();
    return;
  }

  if (!urlsQueue_.isEmpty()) {
    getUrlTimer_->start();

    QString feedUrl = feedsQueue_.head();

    if (hostList_.contains(QUrl(feedUrl).host())) {
      foreach (QString url, currentFeeds_) {
        if (QUrl(url).host() == QUrl(feedUrl).host()) {
          return;
        }
      }
    }

    QString urlString = urlsQueue_.dequeue();
    feedUrl = feedsQueue_.dequeue();

    QUrl url = QUrl::fromEncoded(urlString.toUtf8());
    url.setUrl(QString("%1://%2").arg(url.scheme()).arg(url.host()));
    if (!url.isValid()) {
      url = QUrl::fromEncoded(feedUrl.toUtf8());
      url.setUrl(QString("%1://%2").arg(url.scheme()).arg(url.host()));
    }
    emit signalGet(url, feedUrl, 0);
  }
}

/** @brief Prepare and send network request to receive all data
 *----------------------------------------------------------------------------*/
void FaviconObject::slotGet(const QUrl &getUrl, const QString &feedUrl, const int &cnt)
{
  QNetworkRequest request(getUrl);
  request.setRawHeader("User-Agent", globals.userAgent().toUtf8());

  currentUrls_.append(getUrl);
  currentFeeds_.append(feedUrl);
  currentCntRequests_.append(cnt);
  currentTime_.append(REQUEST_TIMEOUT);

  QNetworkReply *reply = networkManager_->get(request);
  reply->setProperty("feedReply", QVariant(true));
  requestUrl_.append(reply->url());
  networkReply_.append(reply);
}

/** @brief Finish network request processing
 *----------------------------------------------------------------------------*/
void FaviconObject::finished(QNetworkReply *reply)
{
  int currentReplyIndex = currentUrls_.indexOf(reply->url());
  if (currentReplyIndex >= 0) {
    currentTime_.removeAt(currentReplyIndex);
    QUrl url = currentUrls_.takeAt(currentReplyIndex);
    QString feedUrl = currentFeeds_.takeAt(currentReplyIndex);
    int cntRequests = currentCntRequests_.takeAt(currentReplyIndex);

    if ((reply->error() == QNetworkReply::NoError) || (reply->error() == QNetworkReply::UnknownContentError)) {
      QUrl redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      if (redirectionTarget.isValid()) {
        if ((cntRequests == 0) || (cntRequests == 1) || (cntRequests == 3)) {
          if (redirectionTarget.host().isNull()) {
            if (redirectionTarget.toString().left(1) == "/")
              redirectionTarget.setUrl(url.scheme()+"://"+url.host()+redirectionTarget.toString());
            else
              redirectionTarget.setUrl(url.scheme()+"://"+url.host()+"/"+redirectionTarget.toString());
          }
          emit signalGet(redirectionTarget, feedUrl, cntRequests+2);
        }
      } else {
        QByteArray data = reply->readAll();
        if (!data.isNull()) {
          if ((cntRequests == 0) || (cntRequests == 2)) {
            QString linkFavicon;
            QString str = QString::fromUtf8(data);
            if (str.contains("<html", Qt::CaseInsensitive)) {
              QzRegExp rx("<link[^>]+rel=['\"]shortcut icon['\"][^>]+>",
                          Qt::CaseInsensitive);
              int pos = rx.indexIn(str);
              if (pos == -1) {
                rx = QzRegExp("<link[^>]+rel=['\"]icon['\"][^>]+>",
                              Qt::CaseInsensitive);
                pos = rx.indexIn(str);
              }
              if (pos > -1) {
                str = rx.cap(0);
                rx.setPattern("href=\"([^\"]+)");
                pos = rx.indexIn(str);
                if (pos == -1) {
                  rx.setPattern("href='([^']+)");
                  pos = rx.indexIn(str);
                }
                if (pos > -1) {
                  linkFavicon = rx.cap(1).simplified();
                  QUrl urlFavicon(linkFavicon);
                  if (urlFavicon.host().isEmpty()) {
                    urlFavicon.setHost(url.host());
                  }
                  if (urlFavicon.scheme().isEmpty()) {
                    urlFavicon.setScheme(url.scheme());
                  }
                  linkFavicon = urlFavicon.toString().simplified();
                  qDebug() << "Favicon URL:" << linkFavicon;
                  emit signalGet(linkFavicon, feedUrl, cntRequests+1);
                }
              }
            }
            if (linkFavicon.isEmpty()) {
              if ((cntRequests == 0) || (cntRequests == 2)) {
                QString link = QString("%1://%2/favicon.ico").arg(url.scheme()).arg(url.host());
                emit signalGet(link, feedUrl, cntRequests+1);
              }
            }
          } else {
            // Emit receiced data in main thread
            QFileInfo info(url.path());
            emit signalIconRecived(feedUrl, data, info.suffix());
          }
        } else {
          if ((cntRequests == 0) || (cntRequests == 2)) {
            QString link = QString("%1://%2/favicon.ico").arg(url.scheme()).arg(url.host());
            emit signalGet(link, feedUrl, cntRequests+1);
          }
        }
      }
    } else {
      if (reply->errorString().contains("Service Temporarily Unavailable")) {
        if (!hostList_.contains(QUrl(feedUrl).host())) {
          hostList_.append(QUrl(feedUrl).host());
        }
      }

      if ((cntRequests == 0) || (cntRequests == 1)) {
        QString link = QString("%1://%2").arg(url.scheme()).arg(url.host());
        emit signalGet(link, feedUrl, 2);
        qDebug() << "Request Url error: " << reply->url().toString() << reply->errorString();
      }
    }
  } else {
    qCritical() << "Request Url error: " << reply->url().toString() << reply->errorString();
  }

  int replyIndex = requestUrl_.indexOf(reply->url());
  if (replyIndex >= 0) {
    requestUrl_.removeAt(replyIndex);
    networkReply_.removeAt(replyIndex);
  }
  reply->abort();
  reply->deleteLater();
}

/** @brief Timeout to delete requests without answer from server
 *----------------------------------------------------------------------------*/
void FaviconObject::slotRequestTimeout()
{
  for (int i = currentTime_.count() - 1; i >= 0; i--) {
    int time = currentTime_.at(i) - 1;
    if (time <= 0) {
      QUrl url = currentUrls_.takeAt(i);
      QString feedUrl = currentFeeds_.takeAt(i);
      int cntRequests = currentCntRequests_.takeAt(i);
      currentTime_.removeAt(i);

      int replyIndex = requestUrl_.indexOf(url);
      if (replyIndex >= 0) {
        requestUrl_.removeAt(replyIndex);
        QNetworkReply *reply = networkReply_.takeAt(replyIndex);
        reply->deleteLater();

        if (cntRequests == 0) {
          emit signalGet(url, feedUrl, 2);
        }
      }
    } else {
      currentTime_.replace(i, time);
    }
  }
}
