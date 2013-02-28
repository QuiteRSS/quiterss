#include <QDebug>
#include "parsethread.h"

ParseThread::ParseThread(QObject *parent, QString dataDirPath)
  : QThread(parent),
    dataDirPath_(dataDirPath)
{
  qDebug() << "ParseThread::constructor";

  setObjectName("parseThread_");
  start(LowestPriority);
}

ParseThread::~ParseThread()
{
  qDebug() << "ParseThread::~destructor";
}

/*virtual*/ void ParseThread::run()
{
  parseObject_ = new ParseObject(dataDirPath_);
  connect(parent(), SIGNAL(xmlReadyParse(QByteArray,QString,QDateTime)),
          parseObject_, SLOT(parseXml(QByteArray,QString,QDateTime)));
  connect(parseObject_, SIGNAL(feedUpdated(int, bool, int)),
          parent(), SLOT(slotUpdateFeed(int, bool, int)));

  qRegisterMetaType<FeedCountStruct>("FeedCountStruct");
  connect(parseObject_, SIGNAL(feedCountsUpdate(FeedCountStruct)),
          parent(), SLOT(slotFeedCountsUpdate(FeedCountStruct)));

  exec();
}
