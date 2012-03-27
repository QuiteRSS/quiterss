#include "faviconloader.h"

FaviconLoader::FaviconLoader(QObject *pParent)
  :QThread(pParent)
{
  qDebug() << "FaviconLoader::constructor";
  start();
}

FaviconLoader::~FaviconLoader()
{
  qDebug() << "FaviconLoader::~destructor";
}

/*virtual*/ void FaviconLoader::run()
{
  getUrlTimer_ = new QTimer();
  getUrlTimer_->setSingleShot(true);
  connect(this, SIGNAL(startGetUrlTimer()), getUrlTimer_, SLOT(start()));
  connect(getUrlTimer_, SIGNAL(timeout()), this, SLOT(getQueuedUrl()));

  updateObject_ = new UpdateObject();
  connect(this, SIGNAL(signalGet(QNetworkRequest)),
          updateObject_, SLOT(slotGet(QNetworkRequest)));
  connect(updateObject_, SIGNAL(signalFinished(QNetworkReply*)),
          this, SLOT(slotFinished(QNetworkReply*)));

  exec();
}

void FaviconLoader::requestUrl(const QUrl &url, const QUrl &feedUrl)
{
  urlsQueue_.enqueue(url);
  feedsQueue_.enqueue(feedUrl);
}

void FaviconLoader::getQueuedUrl()
{
  if (currentFeeds_.size() >= 1) return;

  if (!urlsQueue_.isEmpty()) {
    QUrl url = urlsQueue_.dequeue();
    QUrl feedUrl = feedsQueue_.dequeue();
    if (url.isValid()) {
      QUrl getUrl(QString("http://favicon.yandex.ru/favicon/%1").
                  arg(url.host()));
      get(getUrl, feedUrl);
    } else {
      QUrl getUrl(QString("http://favicon.yandex.ru/favicon/%1").
                  arg(feedUrl.host()));
      get(getUrl, feedUrl);
    }
    if (currentFeeds_.size() < 1) emit startGetUrlTimer();
  }
}

void FaviconLoader::get(const QUrl &getUrl, const QUrl &feedUrl)
{
  emit signalGet(QNetworkRequest(getUrl));

  currentUrls_.append(getUrl);
  currentFeeds_.append(feedUrl);
}

void FaviconLoader::slotFinished(QNetworkReply *reply)
{
  int currentReplyIndex = currentUrls_.indexOf(reply->url());
  currentUrls_.removeAt(currentReplyIndex);
  QUrl feedUrl = currentFeeds_.takeAt(currentReplyIndex);

  if(reply->error() == QNetworkReply::NoError) {
    QByteArray favico = reply->readAll();
    if(!favico.isNull() && (favico.size() != 178) && (favico.size() != 43)) {
      emit signalIconRecived(feedUrl.toString(), favico);
    }
  }
  getQueuedUrl();
  reply->deleteLater();
}
