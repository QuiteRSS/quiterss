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
#include "updateobject.h"
#include "VersionNo.h"

#include <QDebug>
#ifdef HAVE_QT5
#include <QWebPage>
#else
#include <qwebkitversion.h>
#endif
#include <QtSql>

#define REPLY_MAX_COUNT 10

UpdateObject::UpdateObject(int requestTimeout, int replyCount, int numberRepeats, QObject *parent)
  : QObject(parent)
  , requestTimeout_(requestTimeout)
  , replyCount_(replyCount)
  , numberRepeats_(numberRepeats)
{
  setObjectName("updateObject_");

  QTimer *timeout = new QTimer();
  connect(timeout, SIGNAL(timeout()), this, SLOT(slotRequestTimeout()));
  timeout->start(1000);

  getUrlTimer_ = new QTimer();
  getUrlTimer_->setSingleShot(true);
  connect(getUrlTimer_, SIGNAL(timeout()), this, SLOT(getQueuedUrl()));

  networkManager_ = new NetworkManager(this);
  connect(networkManager_, SIGNAL(finished(QNetworkReply*)),
          this, SLOT(finished(QNetworkReply*)));

  connect(this, SIGNAL(signalHead(QUrl,QString,QDateTime,int)),
          SLOT(slotHead(QUrl,QString,QDateTime,int)));
  connect(this, SIGNAL(signalGet(QUrl,QString,QDateTime,int)),
          SLOT(slotGet(QUrl,QString,QDateTime,int)));
}

/** @brief Put URL in request queue
 *----------------------------------------------------------------------------*/
void UpdateObject::requestUrl(const QString &urlString, const QDateTime &date,
                              const QString	&userInfo)
{
  feedsQueue_.enqueue(urlString);
  dateQueue_.enqueue(date);
  userInfo_.enqueue(userInfo);

  getUrlTimer_->start();

  qDebug() << "urlsQueue_ <<" << urlString << "count=" << feedsQueue_.count();
}

/** @brief Process request queue on timer timeouts
 *----------------------------------------------------------------------------*/
void UpdateObject::getQueuedUrl()
{
  if ((replyCount_ <= currentFeeds_.size()) ||
      (REPLY_MAX_COUNT <= currentFeeds_.size())) {
    getUrlTimer_->start();
    return;
  }

  if (!feedsQueue_.isEmpty()) {
    getUrlTimer_->start(50);
    QString feedUrl = feedsQueue_.head();

    int feedId = -1;
    QSqlQuery q;
    q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
    q.bindValue(":xmlUrl", feedUrl);
    q.exec();
    if (q.next()) feedId = q.value(0).toInt();
    emit setStatusFeed(feedId, "1 Update");

    if (hostList_.contains(QUrl(feedUrl).host())) {
      int count = 0;
      foreach (QString url, currentFeeds_) {
        if (QUrl(url).host() == QUrl(feedUrl).host()) {
          if (++count > 1) {
            return;
          }
        }
      }
    }

    feedUrl = feedsQueue_.dequeue();
    QUrl getUrl = QUrl::fromEncoded(feedUrl.toUtf8());
    QString userInfo = userInfo_.dequeue();
    if (!userInfo.isEmpty()) {
      getUrl.setUserInfo(userInfo);
//      getUrl.addQueryItem("auth", getUrl.scheme());
    }
    QDateTime currentDate = dateQueue_.dequeue();

    if (currentDate.isValid())
      emit signalHead(getUrl, feedUrl, currentDate);
    else
      emit signalGet(getUrl, feedUrl, currentDate);

    qDebug() << "urlsQueue_ >>" << feedUrl << "count=" << feedsQueue_.count();
  }
}

/** @brief Prepare and send network request to get head
 *----------------------------------------------------------------------------*/
void UpdateObject::slotHead(const QUrl &getUrl, const QString &feedUrl,
                            const QDateTime &date, const int &count)
{
  qDebug() << objectName() << "::head:" << getUrl.toEncoded() << "feed:" << feedUrl;
  QNetworkRequest request(getUrl);
  QString userAgent = QString("Mozilla/5.0 (Windows NT 6.1) AppleWebKit/%1 (KHTML, like Gecko) QuiteRSS/%2 Safari/%1").
      arg(qWebKitVersion()).arg(STRPRODUCTVER);
  request.setRawHeader("User-Agent", userAgent.toUtf8());
  request.setRawHeader("Accept-Language", "en-us,en");

  currentUrls_.append(getUrl);
  currentFeeds_.append(feedUrl);
  currentDates_.append(date);
  currentCount_.append(count);
  currentHead_.append(true);
  currentTime_.append(requestTimeout_);

  QNetworkReply *reply = networkManager_->head(request);
  requestUrl_.append(reply->url());
  networkReply_.append(reply);
}

/** @brief Prepare and send network request to get all data
 *----------------------------------------------------------------------------*/
void UpdateObject::slotGet(const QUrl &getUrl, const QString &feedUrl,
                           const QDateTime &date, const int &count)
{
  qDebug() << objectName() << "::get:" << getUrl.toEncoded() << "feed:" << feedUrl;
  QNetworkRequest request(getUrl);
  QString userAgent = QString("Mozilla/5.0 (Windows NT 6.1) AppleWebKit/%1 (KHTML, like Gecko) QuiteRSS/%2 Safari/%1").
      arg(qWebKitVersion()).arg(STRPRODUCTVER);
  request.setRawHeader("User-Agent", userAgent.toUtf8());
  request.setRawHeader("Accept-Language", "en-us,en");

  currentUrls_.append(getUrl);
  currentFeeds_.append(feedUrl);
  currentDates_.append(date);
  currentCount_.append(count);
  currentHead_.append(false);
  currentTime_.append(requestTimeout_);

  QNetworkReply *reply = networkManager_->get(request);
  requestUrl_.append(reply->url());
  networkReply_.append(reply);
}

/** @brief Process network reply
 *----------------------------------------------------------------------------*/
void UpdateObject::finished(QNetworkReply *reply)
{
  QUrl replyUrl = reply->url();

  qDebug() << "reply.finished():" << replyUrl.toString();
  qDebug() << reply->header(QNetworkRequest::ContentTypeHeader);
  qDebug() << reply->header(QNetworkRequest::ContentLengthHeader);
  qDebug() << reply->header(QNetworkRequest::LocationHeader);
  qDebug() << reply->header(QNetworkRequest::LastModifiedHeader);
  qDebug() << reply->header(QNetworkRequest::CookieHeader);
  qDebug() << reply->header(QNetworkRequest::SetCookieHeader);

  int currentReplyIndex = currentUrls_.indexOf(replyUrl);

  if (currentReplyIndex >= 0) {
    currentTime_.removeAt(currentReplyIndex);
    currentUrls_.removeAt(currentReplyIndex);
    QString feedUrl    = currentFeeds_.takeAt(currentReplyIndex);
    QDateTime feedDate = currentDates_.takeAt(currentReplyIndex);
    int count = currentCount_.takeAt(currentReplyIndex) + 1;
    bool headOk = currentHead_.takeAt(currentReplyIndex);

    if (reply->error() != QNetworkReply::NoError) {
      qDebug() << "  error retrieving RSS feed:" << reply->error() << reply->errorString();
      if (!headOk) {
        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
          emit getUrlDone(-2, feedUrl, tr("Server requires authentication!"));
        else if (reply->error() == QNetworkReply::ContentNotFoundError)
          emit getUrlDone(-5, feedUrl, tr("Server replied: Not Found!"));
        else {
          if (reply->errorString().contains("Service Temporarily Unavailable")) {
            if (!hostList_.contains(QUrl(feedUrl).host())) {
              hostList_.append(QUrl(feedUrl).host());
              count--;
            }
          }

          if (count < numberRepeats_) {
            emit signalGet(replyUrl, feedUrl, feedDate, count);
          } else {
            emit getUrlDone(-1, feedUrl, QString("%1 (%2)").arg(reply->errorString()).arg(reply->error()));
          }
        }
      } else {
        emit signalGet(replyUrl, feedUrl, feedDate);
      }
    } else {
      QUrl redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      if (redirectionTarget.isValid()) {
        if (count < (numberRepeats_ + 3)) {
          QString host(QUrl::fromEncoded(feedUrl.toUtf8()).host());
          if (reply->operation() == QNetworkAccessManager::HeadOperation) {
            qDebug() << objectName() << "  head redirect...";
            if (redirectionTarget.host().isNull())
              redirectionTarget.setUrl("http://" + host + redirectionTarget.toString());
            emit signalHead(redirectionTarget, feedUrl, feedDate, count);
          } else {
            qDebug() << objectName() << "  get redirect...";
            if (redirectionTarget.host().isNull())
              redirectionTarget.setUrl("http://" + host + redirectionTarget.toString());
            emit signalGet(redirectionTarget, feedUrl, feedDate, count);
          }
        } else {
          emit getUrlDone(-4, feedUrl, tr("Redirect error!"));
        }
      } else {
        QDateTime replyDate = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime();
        QDateTime replyLocalDate = QDateTime(replyDate.date(), replyDate.time());

        qDebug() << feedDate << replyDate << replyLocalDate;
        qDebug() << feedDate.toMSecsSinceEpoch() << replyDate.toMSecsSinceEpoch() << replyLocalDate.toMSecsSinceEpoch();
        if ((reply->operation() == QNetworkAccessManager::HeadOperation) &&
            ((!feedDate.isValid()) || (!replyLocalDate.isValid()) || (feedDate < replyLocalDate))) {
          emit signalGet(replyUrl, feedUrl, feedDate);
        }
        else {
          QByteArray data = reply->readAll().trimmed();

          emit getUrlDone(feedsQueue_.count(), feedUrl, "", data, replyLocalDate);
        }
      }
    }
  } else {
    qCritical() << "Request Url error: " << replyUrl.toString() << reply->errorString();
  }

  int replyIndex = requestUrl_.indexOf(replyUrl);
  if (replyIndex >= 0) {
    requestUrl_.removeAt(replyIndex);
    networkReply_.takeAt(replyIndex)->deleteLater();
  }
}

/** @brief Timeout to delete network requests wich has no answer
 *----------------------------------------------------------------------------*/
void UpdateObject::slotRequestTimeout()
{
  for (int i = currentTime_.count() - 1; i >= 0; i--) {
    int time = currentTime_.at(i) - 1;
    if (time <= 0) {
      QUrl url = currentUrls_.takeAt(i);
      QString feedUrl = currentFeeds_.takeAt(i);
      QDateTime feedDate = currentDates_.takeAt(i);
      int count = currentCount_.takeAt(i) + 1;
      currentTime_.removeAt(i);
      currentHead_.removeAt(i);

      int replyIndex = requestUrl_.indexOf(url);
      QUrl replyUrl = requestUrl_.takeAt(replyIndex);
      networkReply_.takeAt(replyIndex)->deleteLater();
      if (count < numberRepeats_) {
        emit signalGet(replyUrl, feedUrl, feedDate, count);
      } else {
        emit getUrlDone(-3, feedUrl, tr("Request timeout!"));
      }
    } else {
      currentTime_.replace(i, time);
    }
  }
}
