#include "updatethread.h"

UpdateThread::UpdateThread(QObject *parent) :
    QThread(parent), currentReply_(0)
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

/*! \brief ѕостановка сетевого адреса в очередь запросов **********************/
void UpdateThread::getUrl(const QUrl &url)
{
  urlsQueue_.enqueue(url);
  qDebug() << "urlsQueue_ <<" << url << "count=" << urlsQueue_.count();
  getQueuedUrl();
}

/*! \brief ќбработка очереди запросов *****************************************/
void UpdateThread::getQueuedUrl()
{
  if (!currentUrl_.isEmpty()) return;

  if (!urlsQueue_.isEmpty()) {
    currentUrl_ = urlsQueue_.dequeue();
    qDebug() << "urlsQueue_ >>" << currentUrl_ << "count=" << urlsQueue_.count();
    get(currentUrl_);
  } else {
    qDebug() << "urlsQueue_ -- count=" << urlsQueue_.count();
    emit getUrlDone(urlsQueue_.count());
  }
}

/*! \brief »нициаци€ сетевого запроса и подсоединение сигналов ****************/
void UpdateThread::get(const QUrl &url)
{
  qDebug() << objectName() << "::get:" << url;
  QNetworkRequest request(url);
  if (currentReply_) {
      currentReply_->disconnect(this);
      currentReply_->deleteLater();
  }
  currentReply_ = manager_.get(request);
  connect(currentReply_, SIGNAL(readyRead()), this, SLOT(readyRead()));
  connect(currentReply_, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
  connect(currentReply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));

//  start(QThread::LowPriority);
}

/*! \brief „тение данных прин€тых из сети
 *
 *   We read all the available data, and pass it to the XML
 *   stream reader. Then we call the XML parsing function.
 ******************************************************************************/
void UpdateThread::readyRead()
{
  int statusCode = currentReply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (statusCode >= 200 && statusCode < 300) {
    QByteArray data = currentReply_->readAll();
    emit readedXml(data, currentUrl_);
  }
}

/*! \brief ќбработка событи€ изменени€ метаданных интернет-запроса ************/
void UpdateThread::metaDataChanged()
{
  QUrl redirectionTarget = currentReply_->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
  if (redirectionTarget.isValid()) {
    qDebug() << objectName() << "get redirect...";
    get(redirectionTarget);
  }
}

/*! \brief ќбработка ошибки html-запроса **************************************/
void UpdateThread::error(QNetworkReply::NetworkError)
{
  qDebug() << objectName() << "::error retrieving RSS feed";
  currentReply_->disconnect(this);
  currentReply_->deleteLater();
  currentReply_ = 0;
  emit getUrlDone(-1);
  currentUrl_.clear();
}

/*! \brief «авершение обработки сетевого запроса

    The default behavior is to keep the text edit read only.

    If an error has occurred, the user interface is made available
    to the user for further input, allowing a new fetch to be
    started.

    If the HTTP get request has finished, we make the
    user interface available to the user for further input.
 ******************************************************************************/
void UpdateThread::finished(QNetworkReply *reply)
{
  Q_UNUSED(reply);
  qDebug() << objectName() << "::finished";
  emit getUrlDone(urlsQueue_.count());
  currentUrl_.clear();
  getQueuedUrl();
}

void UpdateThread::setProxy(const QNetworkProxy proxy)
{
  networkProxy_ = proxy;
  if (QNetworkProxy::DefaultProxy == networkProxy_.type())
    QNetworkProxyFactory::setUseSystemConfiguration(true);
  else
    QNetworkProxy::setApplicationProxy(networkProxy_);
}
