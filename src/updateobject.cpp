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

#include <QDebug>

#define REPLY_MAX_COUNT 10

UpdateObject::UpdateObject(int requestTimeout, QObject *parent)
  : QObject(parent)
  , requestTimeout_(requestTimeout)
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

  timer_.start();
}

/** @brief Постановка сетевого адреса в очередь запросов
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

/** @brief Обработка очереди запросов по таймеру
 *----------------------------------------------------------------------------*/
void UpdateObject::getQueuedUrl()
{
  if (REPLY_MAX_COUNT <= currentFeeds_.size()) {
    getUrlTimer_->start();
    return;
  }

  if (!feedsQueue_.isEmpty()) {
    getUrlTimer_->start();

    QString feedUrl = feedsQueue_.dequeue();
    QUrl getUrl = QUrl::fromEncoded(feedUrl.toLocal8Bit());
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

/** @brief Подготовка и отправка сетевого запроса для получения заголовка
 *----------------------------------------------------------------------------*/
void UpdateObject::slotHead(const QUrl &getUrl, const QString &feedUrl,
                            const QDateTime &date, const int &count)
{
  qDebug() << objectName() << "::head:" << getUrl << "feed:" << feedUrl;
  QNetworkRequest request(getUrl);
  request.setRawHeader("User-Agent", "Opera/9.80 (Windows NT 6.1) Presto/2.10.229 Version/11.62");
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

/** @brief Подготовка и отправка сетевого запроса для получения всех данных
 *----------------------------------------------------------------------------*/
void UpdateObject::slotGet(const QUrl &getUrl, const QString &feedUrl,
                           const QDateTime &date, const int &count)
{
  qDebug() << objectName() << "::get:" << getUrl << "feed:" << feedUrl;
  QNetworkRequest request(getUrl);
  request.setRawHeader("User-Agent", "Opera/9.80 (Windows NT 6.1) Presto/2.12.388 Version/12.12");
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

/** @brief Завершение обработки сетевого запроса

    The default behavior is to keep the text edit read only.

    If an error has occurred, the user interface is made available
    to the user for further input, allowing a new fetch to be
    started.

    If the HTTP get request has finished, we make the
    user interface available to the user for further input.
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
      qDebug() << "  error retrieving RSS feed:" << reply->error();
      if (!headOk) {
        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
          emit getUrlDone(-2, feedUrl);
        else
          emit getUrlDone(-1, feedUrl);
      } else {
        emit signalGet(replyUrl, feedUrl, feedDate);
      }
    } else {
      QUrl redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      if (redirectionTarget.isValid()) {
        if (count < 5) {
          QString host(QUrl::fromEncoded(feedUrl.toLocal8Bit()).host());
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
          emit getUrlDone(-4, feedUrl);
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
          QByteArray data = reply->readAll();

          emit getUrlDone(feedsQueue_.count(), feedUrl, data, replyLocalDate);
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

/** @brief Тайм-аут для удаления запросов, если нет ответа от сервера
 *----------------------------------------------------------------------------*/
void UpdateObject::slotRequestTimeout()
{
  for (int i = currentTime_.count() - 1; i >= 0; i--) {
    int time = currentTime_.at(i) - 1;
    if (time <= 0) {
      QUrl url = currentUrls_.takeAt(i);
      QString feedUrl = currentFeeds_.takeAt(i);
      currentTime_.removeAt(i);
      currentDates_.removeAt(i);
      currentHead_.removeAt(i);

      int replyIndex = requestUrl_.indexOf(url);
      requestUrl_.removeAt(replyIndex);
      networkReply_.takeAt(replyIndex)->deleteLater();
      emit getUrlDone(-3, feedUrl);
    } else {
      currentTime_.replace(i, time);
    }
  }
}
