#include <QDateTime>
#include <QDebug>
#include <QMessageBox>

#include "parsethread.h"

ParseThread::ParseThread(QObject *parent, QString dataDirPath)
  : QThread(parent),
    dataDirPath_(dataDirPath)
{
  qDebug() << "ParseThread::constructor";
  start(LowestPriority);
}


ParseThread::~ParseThread()
{
  qDebug() << "ParseThread::~destructor";
}

void ParseThread::run()
{
  QTimer parseTimer;
  parseTimer.setSingleShot(true);
  connect(&parseTimer, SIGNAL(timeout()), this, SLOT(getQueuedXml()));
  connect(this, SIGNAL(startTimer()), &parseTimer, SLOT(start()));

  parseObject_ = new ParseObject(dataDirPath_);
  connect(this, SIGNAL(signalReadyParse(QByteArray,QString,QDateTime)),
          parseObject_, SLOT(slotParse(QByteArray,QString,QDateTime)));
  connect(parseObject_, SIGNAL(feedUpdated(int, bool, int)),
          this->parent(), SLOT(slotUpdateFeed(int, bool, int)));

  exec();
}

void ParseThread::parseXml(const QByteArray &data, const QString &feedUrl,
                           const QDateTime &dtReply)
{
  feedsQueue_.enqueue(feedUrl);
  xmlsQueue_.enqueue(data);
  dtReadyQueue_.enqueue(dtReply);
  qDebug() << "xmlsQueue_ <<" << feedUrl << "count=" << xmlsQueue_.count();
  emit startTimer();
}

void ParseThread::getQueuedXml()
{
  if (!currentFeedUrl_.isEmpty()) return;

  while (!feedsQueue_.isEmpty()) {
    currentFeedUrl_ = feedsQueue_.dequeue();
    currentXml_ = xmlsQueue_.dequeue();
    currentDtReady_ = dtReadyQueue_.dequeue();
    qDebug() << "xmlsQueue_ >>" << currentFeedUrl_ << "count=" << xmlsQueue_.count();

    emit signalReadyParse(currentXml_, currentFeedUrl_, currentDtReady_);
  }

  currentFeedUrl_.clear();
}
