#include <QDebug>
#include <QtSql>
#include "faviconobject.h"

#define REPLY_MAX_COUNT 4
#define REQUEST_TIMEOUT 30

FaviconObject::FaviconObject(QObject *parent)
  : QObject(parent)
{
  setObjectName("faviconObject_");

  QTimer *timeout_ = new QTimer();
  connect(timeout_, SIGNAL(timeout()), this, SLOT(slotRequestTimeout()));
  timeout_->start(1000);

  QTimer *getUrlTimer = new QTimer();
  getUrlTimer->setSingleShot(true);
  connect(getUrlTimer, SIGNAL(timeout()), this, SLOT(getQueuedUrl()));
  connect(this, SIGNAL(startTimer()), getUrlTimer, SLOT(start()));

  connect(this, SIGNAL(signalGet(QUrl,QString,int)),
          SLOT(slotGet(QUrl,QString,int)));

  networkManager_ = new NetworkManager(this);
  connect(networkManager_, SIGNAL(finished(QNetworkReply*)),
          this, SLOT(finished(QNetworkReply*)));
}

/** @brief Постановка сетевого адреса в очередь запросов
 *----------------------------------------------------------------------------*/
void FaviconObject::requestUrl(const QString &urlString, const QString &feedUrl)
{
  urlsQueue_.enqueue(urlString);
  feedsQueue_.enqueue(feedUrl);
  emit startTimer();
}

/** @brief Обработка очереди запросов по таймеру
 *----------------------------------------------------------------------------*/
void FaviconObject::getQueuedUrl()
{
  if (currentFeeds_.size() >= REPLY_MAX_COUNT) {
    emit startTimer();
    return;
  }

  if (!urlsQueue_.isEmpty()) {
    QString urlString = urlsQueue_.dequeue();
    QString feedUrl = feedsQueue_.dequeue();

    QUrl url = QUrl::fromEncoded(urlString.toLocal8Bit());
    if (!url.isValid()) {
      url = QUrl::fromEncoded(feedUrl.toLocal8Bit());
    }
    QUrl getUrl(QString("%1://%2/favicon.ico").
                arg(url.scheme()).arg(url.host()));
    emit signalGet(getUrl, feedUrl, 0);
    emit startTimer();
  }
}

/** @brief Подготовка и отправка сетевого запроса для получения всех данных
 *----------------------------------------------------------------------------*/
void FaviconObject::slotGet(const QUrl &getUrl, const QString &feedUrl, const int &cnt)
{
  QNetworkRequest request(getUrl);
  request.setRawHeader("User-Agent", "Opera/9.80 (Windows NT 6.1) Presto/2.12.388 Version/12.12");

  currentUrls_.append(getUrl);
  currentFeeds_.append(feedUrl);
  currentCntRequests_.append(cnt);
  currentTime_.append(REQUEST_TIMEOUT);

  QNetworkReply *reply = networkManager_->get(request);
  requestUrl_.append(reply->url());
  networkReply_.append(reply);
}

/** @brief Завершение обработки сетевого запроса
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

/** @brief Тайм-аут для удаления запросов, если нет ответа от сервера
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
