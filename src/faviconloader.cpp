#include "faviconloader.h"
#include  <QDebug>
#include <QPixmap>
#include <QBuffer>

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
      QUrl getUrl(QString("http://%1/favicon.ico").
                  arg(url.host()));
      get(getUrl, feedUrl);
    } else {
      QUrl getUrl(QString("http://%1/favicon.ico").
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
  QUrl url = currentUrls_.takeAt(currentReplyIndex);
  QUrl feedUrl = currentFeeds_.takeAt(currentReplyIndex);

  if(reply->error() == QNetworkReply::NoError) {
    QByteArray data = reply->readAll();
    if (!data.isNull()) {
      if (url.toString().indexOf("favicon") <= -1) {
        QString str = QString::fromUtf8(data);
        if (str.contains("<html", Qt::CaseInsensitive)) {
          QString linkFavicon;
          QRegExp rx("<link[^>]+favicon\\.[^>]+>",
                     Qt::CaseInsensitive, QRegExp::RegExp2);
          int pos = rx.indexIn(str);
          if (pos > -1) {
            str = rx.cap(0);
            rx.setPattern("href=\"([^\"]+)");
            pos = rx.indexIn(str);
            if (pos > -1) {
              linkFavicon = rx.cap(1);
              QUrl urlFavicon(linkFavicon);
              if (urlFavicon.host().isEmpty()) {
                urlFavicon.setScheme(url.scheme());
                urlFavicon.setHost(url.host());
              }
              linkFavicon = urlFavicon.toString();
              qDebug() << "Favicon URL:" << linkFavicon;
              get(linkFavicon, feedUrl);
            }
          }
        }
      } else {
        QPixmap icon;
        if (icon.loadFromData(data)) {
          icon = icon.scaled(16, 16, Qt::IgnoreAspectRatio,
                             Qt::SmoothTransformation);
          QByteArray faviconData;
          QBuffer    buffer(&faviconData);
          buffer.open(QIODevice::WriteOnly);
          if (icon.save(&buffer, "ICO")) {
            emit signalIconRecived(feedUrl.toString(), faviconData);
          }
        }
      }
    } else {
      if (url.isValid()) {
        if (url.toString().indexOf("favicon") > -1) {
          QString link = QString("http://%1").arg(url.host());
          get(link, feedUrl);
        }
      }
    }
  } else {
    if (url.isValid()) {
      if (url.toString().indexOf("favicon") > -1) {
        QString link = QString("http://%1").arg(url.host());
        get(link, feedUrl);
      }
    }
  }
  getQueuedUrl();
  reply->deleteLater();
}
