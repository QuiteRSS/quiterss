#include <QDebug>
#include "updatethread.h"

UpdateThread::UpdateThread(QObject *parent, int requestTimeout) :
  QThread(parent),
  requestTimeout_(requestTimeout)
{
  qDebug() << "UpdateThread::constructor";
  start();
}

UpdateThread::~UpdateThread()
{
  qDebug() << "UpdateThread::~destructor";
}

void UpdateThread::run()
{
  QTimer timeout_;
  connect(&timeout_, SIGNAL(timeout()), this, SLOT(slotRequestTimeout()));
  timeout_.start(1000);

  QTimer getUrlTimer_;
  getUrlTimer_.setSingleShot(true);
  connect(this, SIGNAL(startGetUrlTimer()), &getUrlTimer_, SLOT(start()));
  connect(&getUrlTimer_, SIGNAL(timeout()), this, SLOT(getQueuedUrl()));

  updateObject_ = new UpdateObject();
  connect(this, SIGNAL(signalHead(QNetworkRequest)),
          updateObject_, SLOT(slotHead(QNetworkRequest)));
  connect(this, SIGNAL(signalGet(QNetworkRequest)),
          updateObject_, SLOT(slotGet(QNetworkRequest)));
  connect(updateObject_, SIGNAL(signalNetworkReply(QNetworkReply*)),
          this, SLOT(slotNetworkReply(QNetworkReply*)));
  connect(updateObject_, SIGNAL(signalFinished(QNetworkReply*)),
          this, SLOT(finished(QNetworkReply*)));
  connect(updateObject_->networkManager_,
          SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          this, SIGNAL(signalAuthentication(QNetworkReply*,QAuthenticator*)),
          Qt::BlockingQueuedConnection);

  exec();
}

/*! \brief Постановка сетевого адреса в очередь запросов **********************/
void UpdateThread::requestUrl(const QString &urlString, const QDateTime &date,
                              const QString	&userInfo)
{
  feedsQueue_.enqueue(urlString);
  dateQueue_.enqueue(date);
  userInfo_.enqueue(userInfo);
  qDebug() << "urlsQueue_ <<" << urlString << "count=" << feedsQueue_.count();
}

/*! \brief Обработка очереди запросов *****************************************/
void UpdateThread::getQueuedUrl()
{
  if (REPLY_MAX_COUNT <= currentFeeds_.size()) return;

  if (!feedsQueue_.isEmpty()) {
    QString feedUrl = feedsQueue_.dequeue();
    QUrl getUrl = QUrl::fromEncoded(feedUrl.toLocal8Bit());
    QString userInfo = userInfo_.dequeue();
    if (!userInfo.isEmpty()) {
      getUrl.setUserInfo(userInfo);
      getUrl.addQueryItem("auth", "http");
    }
    QDateTime currentDate = dateQueue_.dequeue();
    qDebug() << "urlsQueue_ >>" << feedUrl << "count=" << feedsQueue_.count();
    head(getUrl, feedUrl, currentDate);
    if (currentFeeds_.size() < REPLY_MAX_COUNT) emit startGetUrlTimer();
  }
}

/*! \brief Инициация сетевого запроса и подсоединение сигналов ****************/
void UpdateThread::get(const QUrl &getUrl, const QString &feedUrl, const QDateTime &date)
{
  qDebug() << objectName() << "::get:" << getUrl << "feed:" << feedUrl;
  QNetworkRequest request(getUrl);
  request.setRawHeader("User-Agent", "Opera/9.80 (Windows NT 6.1) Presto/2.12.388 Version/12.12");
  qDebug() << "request.rawHeaderList:" << request.rawHeaderList();
  emit signalGet(request);
  currentUrls_.append(getUrl);
  currentFeeds_.append(feedUrl);
  currentDates_.append(date);
  currentHead_.append(false);
  currentTime_.append(requestTimeout_);
}

void UpdateThread::head(const QUrl &getUrl, const QString &feedUrl, const QDateTime &date)
{
  qDebug() << objectName() << "::head:" << getUrl << "feed:" << feedUrl;
  QNetworkRequest request(getUrl);
  request.setRawHeader("User-Agent", "Opera/9.80 (Windows NT 6.1) Presto/2.10.229 Version/11.62");
  qDebug() << "request.rawHeaderList:" << request.rawHeaderList();
  emit signalHead(request);
  currentUrls_.append(getUrl);
  currentFeeds_.append(feedUrl);
  currentDates_.append(date);
  currentHead_.append(true);
  currentTime_.append(requestTimeout_);
}

/*! \brief Завершение обработки сетевого запроса

    The default behavior is to keep the text edit read only.

    If an error has occurred, the user interface is made available
    to the user for further input, allowing a new fetch to be
    started.

    If the HTTP get request has finished, we make the
    user interface available to the user for further input.
 *******************************************************************************/
void UpdateThread::finished(QNetworkReply *reply)
{
  if (!reply) {
    qCritical() << "*01";
    return;
  }
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
    bool headOk = currentHead_.takeAt(currentReplyIndex);

    if (reply->error() != QNetworkReply::NoError) {
      qDebug() << "  error retrieving RSS feed:" << reply->error();
      if (!headOk) {
        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
          emit getUrlDone(-2);
        else
          emit getUrlDone(-1);
        getQueuedUrl();
      } else {
        get(replyUrl, feedUrl, feedDate);
      }
    }
    else {
      QUrl redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      if (redirectionTarget.isValid()) {
        QString host(QUrl::fromEncoded(feedUrl.toLocal8Bit()).host());
        if (reply->operation() == QNetworkAccessManager::HeadOperation) {
          qDebug() << objectName() << "  head redirect...";
          if (redirectionTarget.host().isNull())
            redirectionTarget.setUrl("http://" + host + redirectionTarget.toString());
          head(redirectionTarget, feedUrl, feedDate);
        } else {
          qDebug() << objectName() << "  get redirect...";
          if (redirectionTarget.host().isNull())
            redirectionTarget.setUrl("http://" + host + redirectionTarget.toString());
          get(redirectionTarget, feedUrl, feedDate);
        }
      } else {
        QDateTime replyDate = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime();
        QDateTime replyLocalDate = QDateTime(replyDate.date(), replyDate.time());

        qDebug() << feedDate << replyDate << replyLocalDate;
        qDebug() << feedDate.toMSecsSinceEpoch() << replyDate.toMSecsSinceEpoch() << replyLocalDate.toMSecsSinceEpoch();
        if ((reply->operation() == QNetworkAccessManager::HeadOperation) &&
            ((!feedDate.isValid()) || (!replyLocalDate.isValid()) || (feedDate < replyLocalDate))) {
          get(replyUrl, feedUrl, feedDate);
        }
        else {
          QByteArray data = reply->readAll();

          emit getUrlDone(feedsQueue_.count(), feedUrl, data, replyLocalDate);
          getQueuedUrl();
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

void UpdateThread::setProxy(const QNetworkProxy proxy)
{
  networkProxy_ = proxy;
  if (QNetworkProxy::DefaultProxy == networkProxy_.type())
    QNetworkProxyFactory::setUseSystemConfiguration(true);
  else
    QNetworkProxy::setApplicationProxy(networkProxy_);
}

void UpdateThread::slotNetworkReply(QNetworkReply *reply)
{
  requestUrl_.append(reply->url());
  networkReply_.append(reply);
}

void UpdateThread::slotRequestTimeout()
{
  for (int i = currentTime_.count() - 1; i >= 0; i--) {
    int time = currentTime_.at(i) - 1;
    if (time <= 0) {
      QUrl url = currentUrls_.takeAt(i);
      currentTime_.removeAt(i);
      currentFeeds_.removeAt(i);
      currentDates_.removeAt(i);
      currentHead_.removeAt(i);

      int replyIndex = requestUrl_.indexOf(url);
      requestUrl_.removeAt(replyIndex);
      networkReply_.takeAt(replyIndex)->deleteLater();
      emit getUrlDone(-3);
      getQueuedUrl();
    } else {
      currentTime_.replace(i, time);
    }
  }
}
