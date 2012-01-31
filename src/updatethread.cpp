#include "updatethread.h"

UpdateThread::UpdateThread(QObject *parent) :
    QThread(parent)
{
  connect(&manager_, SIGNAL(finished(QNetworkReply*)),
      this, SLOT(finished(QNetworkReply*)));

  qDebug() << "UpdateThread::constructor";
}

UpdateThread::~UpdateThread()
{
  qDebug() << "UpdateThread::~destructor";
}

void UpdateThread::run()
{
  qDebug() << "UpdateThread::run()";
  return;
}

/*! \brief Постановка сетевого адреса в очередь запросов **********************/
void UpdateThread::getUrl(const QUrl &url, const QDateTime &date)
{
  urlsQueue_.enqueue(url);
  dateQueue_.enqueue(date);
  qDebug() << "urlsQueue_ <<" << url << "count=" << urlsQueue_.count();
  getQueuedUrl();
}

/*! \brief Обработка очереди запросов *****************************************/
void UpdateThread::getQueuedUrl()
{
  if (REPLY_MAX_COUNT <= currentUrls_.size()) return;

  if (!urlsQueue_.isEmpty()) {
    QUrl currentUrl = urlsQueue_.dequeue();
    QDateTime currentDate = dateQueue_.dequeue();
    qDebug() << "urlsQueue_ >>" << currentUrl << "count=" << urlsQueue_.count();
    head(currentUrl, currentDate);
  } else {
    qDebug() << "urlsQueue_ -- count=" << urlsQueue_.count();
    emit getUrlDone(urlsQueue_.count());
  }
}

/*! \brief Инициация сетевого запроса и подсоединение сигналов ****************/
void UpdateThread::get(const QUrl &url, const QDateTime &date)
{
  qDebug() << objectName() << "::get:" << url;
  QNetworkRequest request(url);
//  if (currentReply_) {
//      currentReply_->disconnect(this);
//      currentReply_->deleteLater();
//  }
//  currentData_.clear();
  QNetworkReply *reply = manager_.get(request);
//  connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
//  connect(reply, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
//  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
  currentReplies_.append(reply);
  currentUrls_.append(url);
  currentDates_.append(date);

//  start(QThread::LowPriority);
}

void UpdateThread::head(const QUrl &url, const QDateTime &date)
{
  qDebug() << objectName() << "::head:" << url;
  QNetworkRequest request(url);
//  if (currentReply_) {
//    currentReply_->disconnect(this);
//    currentReply_->deleteLater();
//  }
//  currentData_.clear();
  QNetworkReply *reply = manager_.head(request);
//  connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
//  connect(reply, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
//  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
  currentReplies_.append(reply);
  currentUrls_.append(url);
  currentDates_.append(date);

  if (currentReplies_.size() < REPLY_MAX_COUNT) getQueuedUrl();
  //  start(QThread::LowPriority);
}

/*! \brief Чтение данных принятых из сети
 *
 *   We read all the available data, and pass it to the XML
 *   stream reader. Then we call the XML parsing function.
 ******************************************************************************/
//void UpdateThread::readyRead()
//{
//  int statusCode = currentReply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
//  if (statusCode >= 200 && statusCode < 300) {
//    QByteArray data = currentReply_->readAll();
//    currentData_.append(data);
//    emit readedXml(data, currentUrl_);
//  }
//}

/*! \brief Обработка события изменения метаданных интернет-запроса ************/
//void UpdateThread::metaDataChanged()
//{
//  QUrl redirectionTarget = currentReply_->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
//  if (redirectionTarget.isValid()) {
//    if (currentReply_->operation() == QNetworkAccessManager::HeadOperation) {
//      qDebug() << objectName() << "head redirect...";
//      head(redirectionTarget);
//    } else {
//      qDebug() << objectName() << "get redirect...";
//      get(redirectionTarget);
//    }
//  }
//}

/*! \brief Обработка ошибки html-запроса **************************************/
//void UpdateThread::error(QNetworkReply::NetworkError)
//{
//  qDebug() << objectName() << "::error retrieving RSS feed";
//  currentReply_->disconnect(this);
//  currentReply_->deleteLater();
//  currentReply_ = 0;
//  emit getUrlDone(-1);
//  currentUrl_.clear();
//}

/*! \brief Завершение обработки сетевого запроса

    The default behavior is to keep the text edit read only.

    If an error has occurred, the user interface is made available
    to the user for further input, allowing a new fetch to be
    started.

    If the HTTP get request has finished, we make the
    user interface available to the user for further input.
 *********************************r*********************************************/
void UpdateThread::finished(QNetworkReply *reply)
{
  qDebug() << "reply.finished():" << reply->url().toString();
  qDebug() << reply->header(QNetworkRequest::ContentTypeHeader);
  qDebug() << reply->header(QNetworkRequest::ContentLengthHeader);
  qDebug() << reply->header(QNetworkRequest::LocationHeader);
  qDebug() << reply->header(QNetworkRequest::LastModifiedHeader);
  qDebug() << reply->header(QNetworkRequest::CookieHeader);
  qDebug() << reply->header(QNetworkRequest::SetCookieHeader);

  int currentReplyIndex = currentReplies_.indexOf(reply);
  QUrl currentUrl       = currentUrls_.takeAt(currentReplyIndex);
  QDateTime currentDate = currentDates_.takeAt(currentReplyIndex);
  currentReplies_.removeAll(reply);
  reply->deleteLater();

  if (reply->error() != QNetworkReply::NoError) {
    qDebug() << "  error retrieving RSS feed:" << reply->error();
    emit getUrlDone(-1);
    getQueuedUrl();
  }
  else {
    QUrl redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectionTarget.isValid()) {
      if (reply->operation() == QNetworkAccessManager::HeadOperation) {
        qDebug() << objectName() << "  head redirect...";
        head(redirectionTarget, currentDate);
      } else {
        qDebug() << objectName() << "  get redirect...";
        get(redirectionTarget, currentDate);
      }
    } else {
      QDateTime dtReply = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime();
      QDateTime dtReplyLocal = QDateTime(dtReply.date(), dtReply.time());

      qDebug() << currentDate << dtReply << dtReplyLocal;
      qDebug() << currentDate.toMSecsSinceEpoch() << dtReply.toMSecsSinceEpoch() << dtReplyLocal.toMSecsSinceEpoch();
      if ((reply->operation() == QNetworkAccessManager::HeadOperation) &&
          ((!currentDate.isValid()) || (!dtReplyLocal.isValid()) || (currentDate < dtReplyLocal))) {
        get(reply->url(), currentDate);
      }
      else {
        QByteArray data = reply->readAll();
        emit readedXml(data, currentUrl);
        emit getUrlDone(urlsQueue_.count(), dtReplyLocal);

        getQueuedUrl();
      }
    }
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
