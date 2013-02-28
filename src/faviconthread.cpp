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
  connect(faviconObject_, SIGNAL(signalIconRecived(QString, const QByteArray&)),
          parent(), SLOT(slotIconFeedPreparing(QString, const QByteArray&)));
  connect(parent(), SIGNAL(signalIconFeedReady(QString, const QByteArray&)),
          faviconObject_, SLOT(slotIconSave(QString, const QByteArray&)));
  connect(faviconObject_, SIGNAL(signalIconUpdate(int, int, const QByteArray&)),
          parent(), SLOT(slotIconFeedUpdate(int, int, const QByteArray&)));

  exec();
}
