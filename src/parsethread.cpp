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
  parseTimer_ = new QTimer();
  parseTimer_->setSingleShot(true);
  connect(parseTimer_, SIGNAL(timeout()), this, SLOT(getQueuedXml()));
  connect(this, SIGNAL(startTimer()), parseTimer_, SLOT(start()));

  parseObject_ = new ParseObject(dataDirPath_);
  connect(this, SIGNAL(signalReadyParse(QByteArray,QString)),
          parseObject_, SLOT(slotParse(QByteArray,QString)));
  connect(parseObject_, SIGNAL(feedUpdated(int, bool, int)),
          this->parent(), SLOT(slotUpdateFeed(int, bool, int)));

  exec();
}

void ParseThread::parseXml(const QByteArray &data, const QString &feedUrl)
{
  feedsQueue_.enqueue(feedUrl);
  xmlsQueue_.enqueue(data);
  qDebug() << "xmlsQueue_ <<" << feedUrl << "count=" << xmlsQueue_.count();
  emit startTimer();
}

void ParseThread::getQueuedXml()
{
  if (!currentFeedUrl_.isEmpty()) return;

  while (!feedsQueue_.isEmpty()) {
    currentFeedUrl_ = feedsQueue_.dequeue();
    currentXml_ = xmlsQueue_.dequeue();
    qDebug() << "xmlsQueue_ >>" << currentFeedUrl_ << "count=" << xmlsQueue_.count();
    //    parse();
    emit signalReadyParse(currentXml_, currentFeedUrl_);
  }

  currentFeedUrl_.clear();
}
