#include "faviconloader.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QDebug>

FaviconLoader::FaviconLoader(QObject *pParent)
  :QThread(pParent)
{
}

FaviconLoader::~FaviconLoader()
{
}

/*virtual*/ void FaviconLoader::run()
{
  QUrl url(strTargetUrl_);
  QNetworkRequest myRequest(url);

  QNetworkAccessManager networkMgr;
  connect(&networkMgr, SIGNAL(finished(QNetworkReply*)),
          this, SLOT(slotRecived(QNetworkReply*)));

  networkMgr.get(myRequest);

  exec();
}

void FaviconLoader::setTargetUrl(const QString& strUrl)
{
  strFeedUrl_ = strUrl;
  strTargetUrl_ = QString("http://favicon.yandex.ru/favicon/%1").
      arg(QUrl(strUrl).host());
  start();
}

void FaviconLoader::slotRecived(QNetworkReply *pReplay)
{
  if(pReplay->error() == QNetworkReply::NoError) {
    QByteArray favico;
    favico = pReplay->readAll();
    if(!favico.isNull()) {
      emit signalIconRecived(strFeedUrl_, favico);
    }
  }

  exit(0);
}
