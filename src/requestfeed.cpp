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
#include "requestfeed.h"
#include "VersionNo.h"
#include "mainapplication.h"

#include <QDebug>
#ifdef HAVE_QT5
#include <QWebPage>
#else
#include <qwebkitversion.h>
#endif
#include <QtSql>
#include <qzregexp.h>

#define REPLY_MAX_COUNT 10

RequestFeed::RequestFeed(int timeoutRequest, int numberRequests,
                         int numberRepeats, QObject *parent)
  : QObject(parent)
  , timeoutRequest_(timeoutRequest)
  , numberRequests_(numberRequests)
  , numberRepeats_(numberRepeats)
{
  setObjectName("requestFeed_");

  timeout_ = new QTimer(this);
  timeout_->setInterval(1000);
  connect(timeout_, SIGNAL(timeout()), this, SLOT(slotRequestTimeout()));

  getUrlTimer_ = new QTimer(this);
  getUrlTimer_->setSingleShot(true);
  getUrlTimer_->setInterval(50);
  connect(getUrlTimer_, SIGNAL(timeout()), this, SLOT(getQueuedUrl()));

  networkManager_ = new NetworkManager(true, this);
  connect(networkManager_, SIGNAL(finished(QNetworkReply*)),
          this, SLOT(finished(QNetworkReply*)));

  connect(this, SIGNAL(signalHead(QUrl,int,QString,QDateTime,int)),
          SLOT(slotHead(QUrl,int,QString,QDateTime,int)),
          Qt::QueuedConnection);
  connect(this, SIGNAL(signalGet(QUrl,int,QString,QDateTime,int)),
          SLOT(slotGet(QUrl,int,QString,QDateTime,int)),
          Qt::QueuedConnection);
}

RequestFeed::~RequestFeed()
{

}

void RequestFeed::disconnectObjects()
{
  disconnect(this);
  networkManager_->disconnect(networkManager_);
}

/** @brief Put URL in request queue
 *----------------------------------------------------------------------------*/
void RequestFeed::requestUrl(int id, QString urlString,
                              QDateTime date, QString userInfo)
{
  if (!timeout_->isActive())
    timeout_->start();

  idsQueue_.enqueue(id);
  feedsQueue_.enqueue(urlString);
  dateQueue_.enqueue(date);
  userInfo_.enqueue(userInfo);

  if (!getUrlTimer_->isActive())
    getUrlTimer_->start();

  qDebug() << "urlsQueue_ <<" << urlString << "count=" << feedsQueue_.count();
}

void RequestFeed::stopRequest()
{
  while (!feedsQueue_.isEmpty()) {
    int feedId = idsQueue_.dequeue();
    QString feedUrl = feedsQueue_.dequeue();
    dateQueue_.clear();
    userInfo_.clear();

    emit getUrlDone(feedsQueue_.count(), feedId, feedUrl);
  }
}

/** @brief Process request queue on timer timeouts
 *----------------------------------------------------------------------------*/
void RequestFeed::getQueuedUrl()
{
  if ((numberRequests_ <= currentFeeds_.size()) ||
      (REPLY_MAX_COUNT <= currentFeeds_.size())) {
    getUrlTimer_->start();
    return;
  }

  if (!feedsQueue_.isEmpty()) {
    getUrlTimer_->start();
    QString feedUrl = feedsQueue_.head();

    if (hostList_.contains(QUrl(feedUrl).host())) {
      foreach (QString url, currentFeeds_) {
        if (QUrl(url).host() == QUrl(feedUrl).host()) {
          return;
        }
      }
    }
    int feedId = idsQueue_.dequeue();
    feedUrl = feedsQueue_.dequeue();

    emit setStatusFeed(feedId, "1 Update");

    QUrl getUrl = QUrl::fromEncoded(feedUrl.toUtf8());
    QString userInfo = userInfo_.dequeue();
    if (!userInfo.isEmpty()) {
      getUrl.setUserInfo(userInfo);
//      getUrl.addQueryItem("auth", getUrl.scheme());
    }
    QDateTime currentDate = dateQueue_.dequeue();

    if (currentDate.isValid())
      emit signalHead(getUrl, feedId, feedUrl, currentDate);
    else
      emit signalGet(getUrl, feedId, feedUrl, currentDate);

    qDebug() << "urlsQueue_ >>" << feedUrl << "count=" << feedsQueue_.count();
  }
}

/** @brief Prepare and send network request to get head
 *----------------------------------------------------------------------------*/
void RequestFeed::slotHead(const QUrl &getUrl, const int &id, const QString &feedUrl,
                            const QDateTime &date, const int &count)
{
  qDebug() << objectName() << "::head:" << getUrl.toEncoded() << "feed:" << feedUrl;
  QNetworkRequest request(getUrl);
  QString userAgent = QString("Mozilla/5.0 (Windows NT 6.1) AppleWebKit/%1 (KHTML, like Gecko) QuiteRSS/%2 Safari/%1").
      arg(qWebKitVersion()).arg(STRPRODUCTVER);
  request.setRawHeader("User-Agent", userAgent.toUtf8());
  request.setRawHeader("Accept-Language", "*,en-us;q=0.8,en;q=0.6");

  currentUrls_.append(getUrl);
  currentIds_.append(id);
  currentFeeds_.append(feedUrl);
  currentDates_.append(date);
  currentCount_.append(count);
  currentHead_.append(true);
  currentTime_.append(timeoutRequest_);

  QNetworkReply *reply = networkManager_->head(request);
  reply->setProperty("feedReply", QVariant(true));
  requestUrl_.append(reply->url());
  networkReply_.append(reply);
}

/** @brief Prepare and send network request to get all data
 *----------------------------------------------------------------------------*/
void RequestFeed::slotGet(const QUrl &getUrl, const int &id, const QString &feedUrl,
                           const QDateTime &date, const int &count)
{
  qDebug() << objectName() << "::get:" << getUrl.toEncoded() << "feed:" << feedUrl;
  QNetworkRequest request(getUrl);
  request.setRawHeader("Accept", "application/atom+xml,application/rss+xml;q=0.9,application/xml;q=0.8,text/xml;q=0.7,*/*;q=0.6");
  request.setRawHeader("Accept-Language", "*,en-us;q=0.8,en;q=0.6");
  QString userAgent = QString("Mozilla/5.0 (Windows NT 6.1) AppleWebKit/%1 (KHTML, like Gecko) QuiteRSS/%2 Safari/%1").
      arg(qWebKitVersion()).arg(STRPRODUCTVER);
  request.setRawHeader("User-Agent", userAgent.toUtf8());

  currentUrls_.append(getUrl);
  currentIds_.append(id);
  currentFeeds_.append(feedUrl);
  currentDates_.append(date);
  currentCount_.append(count);
  currentHead_.append(false);
  currentTime_.append(timeoutRequest_);

  QNetworkReply *reply = networkManager_->get(request);
  reply->setProperty("feedReply", QVariant(true));
  requestUrl_.append(reply->url());
  networkReply_.append(reply);
}

/** @brief Process network reply
 *----------------------------------------------------------------------------*/
void RequestFeed::finished(QNetworkReply *reply)
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
    int feedId    = currentIds_.takeAt(currentReplyIndex);
    QString feedUrl    = currentFeeds_.takeAt(currentReplyIndex);
    QDateTime feedDate = currentDates_.takeAt(currentReplyIndex);
    int count = currentCount_.takeAt(currentReplyIndex) + 1;
    bool headOk = currentHead_.takeAt(currentReplyIndex);

    if (reply->error() != QNetworkReply::NoError) {
      qDebug() << "  error retrieving RSS feed:" << reply->error() << reply->errorString();
      if (!headOk) {
        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
          emit getUrlDone(-2, feedId, feedUrl, tr("Server requires authentication!"));
        else if (reply->error() == QNetworkReply::ContentNotFoundError)
          emit getUrlDone(-5, feedId, feedUrl, tr("Server replied: Not Found!"));
        else {
          if (reply->errorString().contains("Service Temporarily Unavailable")) {
            if (!hostList_.contains(QUrl(feedUrl).host())) {
              hostList_.append(QUrl(feedUrl).host());
              count--;
            }
          }

          if (count < numberRepeats_) {
            emit signalGet(replyUrl, feedId, feedUrl, feedDate, count);
          } else {
            emit getUrlDone(-1, feedId, feedUrl, QString("%1 (%2)").arg(reply->errorString()).arg(reply->error()));
          }
        }
      } else {
        emit signalGet(replyUrl, feedId, feedUrl, feedDate);
      }
    } else {
      QUrl redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      if (redirectionTarget.isValid()) {
        if (count < (numberRepeats_ + 3)) {
          if (headOk && (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 302)) {
            emit signalGet(replyUrl, feedId, feedUrl, feedDate);
          } else {
            QString host(QUrl::fromEncoded(feedUrl.toUtf8()).host());
            if (reply->operation() == QNetworkAccessManager::HeadOperation) {
              qDebug() << objectName() << "  head redirect...";
              if (redirectionTarget.host().isEmpty())
                redirectionTarget.setUrl("http://" + host + redirectionTarget.toString());
              if (redirectionTarget.scheme().isEmpty())
                redirectionTarget.setScheme(QUrl(feedUrl).scheme());
              emit signalHead(redirectionTarget, feedId, feedUrl, feedDate, count);
            } else {
              qDebug() << objectName() << "  get redirect...";
              if (redirectionTarget.host().isEmpty())
                redirectionTarget.setUrl("http://" + host + redirectionTarget.toString());
              if (redirectionTarget.scheme().isEmpty())
                redirectionTarget.setScheme(QUrl(feedUrl).scheme());
              emit signalGet(redirectionTarget, feedId, feedUrl, feedDate, count);
            }
          }
        } else {
          emit getUrlDone(-4, feedId, feedUrl, tr("Redirect error!"));
        }
      } else {
        QDateTime replyDate = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime();
        QDateTime replyLocalDate = QDateTime(replyDate.date(), replyDate.time());

        qDebug() << feedDate << replyDate << replyLocalDate;
        qDebug() << feedDate.toMSecsSinceEpoch() << replyDate.toMSecsSinceEpoch() << replyLocalDate.toMSecsSinceEpoch();
        if ((reply->operation() == QNetworkAccessManager::HeadOperation) &&
            ((!feedDate.isValid()) || (!replyLocalDate.isValid()) ||
             (feedDate < replyLocalDate) || !replyDate.toMSecsSinceEpoch())) {
          emit signalGet(replyUrl, feedId, feedUrl, feedDate);
        }
        else {
          QString codecName;
          QzRegExp rx("charset=([^\t]+)$", Qt::CaseInsensitive);
          int pos = rx.indexIn(reply->header(QNetworkRequest::ContentTypeHeader).toString());
          if (pos > -1) {
            codecName = rx.cap(1);
          }

          QByteArray data = reply->readAll();
          data = data.trimmed();

          rx.setPattern("&(?!([a-z0-9#]+;))");
          pos = 0;
          while ((pos = rx.indexIn(QString::fromLatin1(data), pos)) != -1) {
            data.replace(pos, 1, "&amp;");
            pos += 1;
          }

          data.replace("<br>", "<br/>");

          if (data.indexOf("</rss>") > 0)
            data.resize(data.indexOf("</rss>") + 6);
          if (data.indexOf("</feed>") > 0)
            data.resize(data.indexOf("</feed>") + 7);
          if (data.indexOf("</rdf:RDF>") > 0)
            data.resize(data.indexOf("</rdf:RDF>") + 10);

          emit getUrlDone(feedsQueue_.count(), feedId, feedUrl, "", data, replyLocalDate, codecName);
        }
      }
    }
  } else {
    qCritical() << "Request Url error: " << replyUrl.toString() << reply->errorString();
  }

  int replyIndex = requestUrl_.indexOf(replyUrl);
  if (replyIndex >= 0) {
    requestUrl_.removeAt(replyIndex);
    networkReply_.removeAt(replyIndex);
  }

  reply->abort();
  reply->deleteLater();
}

/** @brief Timeout to delete network requests which has no answer
 *----------------------------------------------------------------------------*/
void RequestFeed::slotRequestTimeout()
{
  for (int i = currentTime_.count() - 1; i >= 0; i--) {
    int time = currentTime_.at(i) - 1;
    if (time <= 0) {
      QUrl url = currentUrls_.takeAt(i);
      int feedId    = currentIds_.takeAt(i);
      QString feedUrl = currentFeeds_.takeAt(i);
      QDateTime feedDate = currentDates_.takeAt(i);
      int count = currentCount_.takeAt(i) + 1;
      currentTime_.removeAt(i);
      currentHead_.removeAt(i);

      int replyIndex = requestUrl_.indexOf(url);
      if (replyIndex >= 0) {
        QUrl replyUrl = requestUrl_.takeAt(replyIndex);
        QNetworkReply *reply = networkReply_.takeAt(replyIndex);
        reply->deleteLater();

        if (count < numberRepeats_) {
          emit signalGet(replyUrl, feedId, feedUrl, feedDate, count);
        } else {
          emit getUrlDone(-3, feedId, feedUrl, tr("Request timeout!"));
        }
      }
    } else {
      currentTime_.replace(i, time);
    }
  }
}
