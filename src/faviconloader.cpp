#include "faviconloader.h"
#include  <QDebug>
#include <QPixmap>
#include <QBuffer>

FaviconLoader::FaviconLoader(QObject *pParent)
  : QThread(pParent)
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

void FaviconLoader::slotRequestUrl(const QString &urlString, const QUrl &feedUrl)
{
  urlsQueue_.enqueue(QUrl::fromEncoded(urlString.toLocal8Bit()));
  feedsQueue_.enqueue(feedUrl);
}

void FaviconLoader::getQueuedUrl()
{
  if (currentFeeds_.size() >= 2) return;

  if (!urlsQueue_.isEmpty()) {
    QUrl url = urlsQueue_.dequeue();
    QUrl feedUrl = feedsQueue_.dequeue();
    if (url.isValid()) {
      QUrl getUrl(QString("%1://%2/favicon.ico").
                  arg(url.scheme()).
                  arg(url.host()));
      get(getUrl, feedUrl, 0);
    } else {
      QUrl getUrl(QString("%1://%2/favicon.ico").
                  arg(feedUrl.scheme()).
                  arg(feedUrl.host()));
      get(getUrl, feedUrl, 0);
    }
    if (currentFeeds_.size() < 2) emit startGetUrlTimer();
  }
}

void FaviconLoader::get(const QUrl &getUrl,
                        const QUrl &feedUrl, const int &cntRequests)
{
  QNetworkRequest request(getUrl);
  request.setRawHeader("User-Agent", "Opera/9.80 (Windows NT 6.1) Presto/2.10.229 Version/11.62");
  emit signalGet(request);

  currentUrls_.append(getUrl);
  currentFeeds_.append(feedUrl);
  currentCntRequests_.append(cntRequests);
}

void FaviconLoader::slotFinished(QNetworkReply *reply)
{
  int currentReplyIndex = currentUrls_.indexOf(reply->url());
  QUrl url = currentUrls_.takeAt(currentReplyIndex);
  QUrl feedUrl = currentFeeds_.takeAt(currentReplyIndex);
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
              get(linkFavicon, feedUrl, cntRequests+1);
            }
          }
        }
      } else {
        QUrl redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (redirectionTarget.isValid()) {
          if (cntRequests == 0) {
            if (redirectionTarget.host().isNull())
              redirectionTarget.setUrl("http://"+url.host()+redirectionTarget.toString());
            get(redirectionTarget, feedUrl, 2);
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
              QSqlQuery q;
              int feedId = 0;
              q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
              q.bindValue(":xmlUrl", feedUrl.toString());
              q.exec();
              if (q.next()) feedId = q.value(0).toInt();

              q.prepare("UPDATE feeds SET image = ? WHERE id == ?");
              q.addBindValue(faviconData.toBase64());
              q.addBindValue(feedId);
              q.exec();

              int cnt = 1;
              if (!feedsQueue_.count() && !currentFeeds_.count())
                cnt = 0;
              emit signalIconRecived(feedId, faviconData, cnt);
            }
            buffer.close();
          } else if (cntRequests == 0) {
            QString link = QString("%1://%2").arg(url.scheme()).arg(url.host());
            get(link, feedUrl, 1);
          }
        }
      }
    } else {
      if (cntRequests == 0) {
        QString link = QString("http://%1").arg(url.host());
        get(link, feedUrl, 1);
      }
    }
  } else {
    if ((cntRequests == 0) || (cntRequests == 2)) {
      QString link = QString("http://%1").arg(url.host());
      get(link, feedUrl, cntRequests+1);
    }
  }
  getQueuedUrl();
  reply->deleteLater();
}
