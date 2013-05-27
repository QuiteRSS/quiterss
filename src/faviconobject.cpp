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
#include "faviconobject.h"

#include <QDebug>
#include <QtSql>

#define REPLY_MAX_COUNT 4
#define REQUEST_TIMEOUT 30

FaviconObject::FaviconObject(QObject *parent)
  : QObject(parent)
{
  setObjectName("faviconObject_");

  QTimer *timeout_ = new QTimer();
  connect(timeout_, SIGNAL(timeout()), this, SLOT(slotRequestTimeout()));
  timeout_->start(1000);

  getUrlTimer_ = new QTimer();
  getUrlTimer_->setSingleShot(true);
  connect(getUrlTimer_, SIGNAL(timeout()), this, SLOT(getQueuedUrl()));

  connect(this, SIGNAL(signalGet(QUrl,QString,int)),
          SLOT(slotGet(QUrl,QString,int)));

  networkManager_ = new NetworkManager(this);
  connect(networkManager_, SIGNAL(finished(QNetworkReply*)),
          this, SLOT(finished(QNetworkReply*)));
}

/** @brief Put requested URL in request queue
 *----------------------------------------------------------------------------*/
void FaviconObject::requestUrl(const QString &urlString, const QString &feedUrl)
{
  urlsQueue_.enqueue(urlString);
  feedsQueue_.enqueue(feedUrl);
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

    QString urlString = urlsQueue_.dequeue();
    QString feedUrl = feedsQueue_.dequeue();

    QUrl url = QUrl::fromEncoded(urlString.toUtf8());
    if (!url.isValid()) {
      url = QUrl::fromEncoded(feedUrl.toUtf8());
    }
    QUrl getUrl(QString("%1://%2/favicon.ico").
                arg(url.scheme()).arg(url.host()));
    emit signalGet(getUrl, feedUrl, 0);
  }
}

/** @brief Prepare and send network request to receive all data
 *----------------------------------------------------------------------------*/
void FaviconObject::slotGet(const QUrl &getUrl, const QString &feedUrl, const int &cnt)
{
  QNetworkRequest request(getUrl);
  request.setRawHeader("User-Agent", "Opera/9.80 (Windows NT 6.1) Presto/2.12.388 Version/12.12");
  request.setRawHeader("Accept-Language", "en-us,en");

  currentUrls_.append(getUrl);
  currentFeeds_.append(feedUrl);
  currentCntRequests_.append(cnt);
  currentTime_.append(REQUEST_TIMEOUT);

  QNetworkReply *reply = networkManager_->get(request);
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

    if(reply->error() == QNetworkReply::NoError) {
      QByteArray data = reply->readAll();
      if (!data.isNull()) {
        if ((cntRequests == 1) || (cntRequests == 3)) {
          QString str = QString::fromUtf8(data);
          if (str.contains("<html", Qt::CaseInsensitive)) {
            QString linkFavicon;
            QRegExp rx("<link[^>]+rel=\"shortcut icon\"[^>]+>",
                       Qt::CaseInsensitive, QRegExp::RegExp2);
            int pos = rx.indexIn(str);
            if (pos == -1) {
              rx = QRegExp("<link[^>]+rel=\"icon\"[^>]+>",
                           Qt::CaseInsensitive, QRegExp::RegExp2);
              pos = rx.indexIn(str);
            }
            if (pos > -1) {
              str = rx.cap(0);
              rx.setPattern("href=\"([^\"]+)");
              pos = rx.indexIn(str);
              if (pos > -1) {
                linkFavicon = rx.cap(1);
                QUrl urlFavicon(linkFavicon);
                if (urlFavicon.host().isEmpty()) {
                  urlFavicon.setHost(url.host());
                }
                if (urlFavicon.scheme().isEmpty()) {
                  urlFavicon.setScheme(url.scheme());
                }
                linkFavicon = urlFavicon.toString();
                qDebug() << "Favicon URL:" << linkFavicon;
                emit signalGet(linkFavicon, feedUrl, cntRequests+1);
              }
            }
          }
        } else {
          QUrl redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
          if (redirectionTarget.isValid()) {
            if (cntRequests == 0) {
              if (redirectionTarget.host().isNull())
                redirectionTarget.setUrl("http://"+url.host()+redirectionTarget.toString());
              emit signalGet(redirectionTarget, feedUrl, 2);
            }
          } else {
            // Emit receiced data in main thread
            emit signalIconRecived(feedUrl, data);
          }
        }
      } else {
        if ((cntRequests == 0) || (cntRequests == 2)) {
          QString link = QString("%1://%2").arg(url.scheme()).arg(url.host());
          emit signalGet(link, feedUrl, cntRequests+1);
        }
      }
    } else {
      if ((cntRequests == 0) || (cntRequests == 2)) {
        QString link = QString("%1://%2").arg(url.scheme()).arg(url.host());
        emit signalGet(link, feedUrl, cntRequests+1);
      }
    }
  } else {
    qCritical() << "Request Url error: " << reply->url().toString() << reply->errorString();
  }

  int replyIndex = requestUrl_.indexOf(reply->url());
  if (replyIndex >= 0) {
    requestUrl_.removeAt(replyIndex);
    networkReply_.takeAt(replyIndex)->deleteLater();
  }
}

/** @brief Timeout to delete requests without answer from server
 *----------------------------------------------------------------------------*/
void FaviconObject::slotRequestTimeout()
{
  for (int i = currentTime_.count() - 1; i >= 0; i--) {
    int time = currentTime_.at(i) - 1;
    if (time <= 0) {
      QUrl url = currentUrls_.takeAt(i);
      currentTime_.removeAt(i);
      currentFeeds_.removeAt(i);
      currentCntRequests_.removeAt(i);

      int replyIndex = requestUrl_.indexOf(url);
      requestUrl_.removeAt(replyIndex);
      networkReply_.takeAt(replyIndex)->deleteLater();
    } else {
      currentTime_.replace(i, time);
    }
  }
}

/** @brief Save icon in DB and emit signal to update it
 *----------------------------------------------------------------------------*/
void FaviconObject::slotIconSave(const QString &feedUrl, const QByteArray &faviconData)
{
  int feedId = 0;
  int feedParId = 0;

  QSqlQuery q;
  q.prepare("SELECT id, parentId FROM feeds WHERE xmlUrl LIKE :xmlUrl");
  q.bindValue(":xmlUrl", feedUrl);
  q.exec();
  if (q.next()) {
    feedId = q.value(0).toInt();
    feedParId = q.value(1).toInt();
  }

  q.prepare("UPDATE feeds SET image = ? WHERE id == ?");
  q.addBindValue(faviconData.toBase64());
  q.addBindValue(feedId);
  q.exec();

  emit signalIconUpdate(feedId, feedParId, faviconData);
}
