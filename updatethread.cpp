#include "updatethread.h"

UpdateThread::UpdateThread(QObject *parent) :
    QThread(parent), currentReply_(0)
{
  networkProxy_.setHostName("10.0.0.172");
  networkProxy_.setPort(3150);
  manager_.setProxy(networkProxy_);

  connect(&manager_, SIGNAL(finished(QNetworkReply*)),
      this, SLOT(finished(QNetworkReply*)));

  qDebug() << objectName() << "::constructor";
}

UpdateThread::~UpdateThread()
{
  qDebug() << objectName() << "::~destructor";
}

void UpdateThread::run()
{
  qDebug() << objectName() << "::run()";
//  exec();
  return;
}

/*! \brief »нициаци€ сетевого запроса и подсоединение сигналов ****************/
void UpdateThread::getUrl(const QUrl &url)
{
  currentUrl_ = url;
  get(currentUrl_);
}

/*! \brief »нициаци€ сетевого запроса и подсоединение сигналов ****************/
void UpdateThread::get(const QUrl &url)
{
  qDebug() << objectName() << "get:" << url;
  QNetworkRequest request(url);
  if (currentReply_) {
      currentReply_->disconnect(this);
      currentReply_->deleteLater();
  }
  currentReply_ = manager_.get(request);
  connect(currentReply_, SIGNAL(readyRead()), this, SLOT(readyRead()));
  connect(currentReply_, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
  connect(currentReply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));

  start(QThread::LowPriority);
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
  qDebug() << objectName() << "error retrieving RSS feed";
//  statusBar()->showMessage("error retrieving RSS feed", 3000);
  currentReply_->disconnect(this);
  currentReply_->deleteLater();
  currentReply_ = 0;
  emit getUrlDone(-1);
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
//  addFeedAct_->setEnabled(true);
//  deleteFeedAct_->setEnabled(true);
//  feedsView_->setEnabled(true);
  qDebug() << objectName() << "::finished";
  emit getUrlDone(0);
}

void UpdateThread::setProxyType(QNetworkProxy::ProxyType type)
{
  networkProxy_.setType(type);
  QNetworkProxy::setApplicationProxy(networkProxy_);
}
