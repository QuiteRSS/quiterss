#include <QDebug>
#include <QPixmap>
#include <QBuffer>

#include "faviconthread.h"

FaviconThread::FaviconThread(QObject *parent)
  : QThread(parent)
{
  qDebug() << "FaviconLoader::constructor";

  setObjectName("faviconThread_");
  start(LowestPriority);
}

FaviconThread::~FaviconThread()
{
  qDebug() << "FaviconLoader::~destructor";
}

/*virtual*/ void FaviconThread::run()
{
  faviconObject_ = new FaviconObject();
  connect(parent(), SIGNAL(faviconRequestUrl(QString,QString)),
          faviconObject_, SLOT(requestUrl(QString,QString)));
  connect(faviconObject_, SIGNAL(signalIconRecived(QString, const QByteArray&, const int&)),
          parent(), SLOT(slotIconFeedLoad(QString, const QByteArray&, const int&)));

  exec();
}
