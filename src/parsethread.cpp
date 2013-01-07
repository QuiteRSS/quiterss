#include <QDateTime>
#include <QDebug>
#include <QMessageBox>

#include "parsethread.h"

ParseThread::ParseThread(QObject *parent, QSqlDatabase *db) :
  QThread(parent)
{
  qDebug() << "ParseThread::constructor";
  db_ = db;
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

  parseObject_ = new ParseObject();
  connect(this, SIGNAL(signalReadyParse(QSqlDatabase*,QByteArray,QUrl)),
          parseObject_, SLOT(slotParse(QSqlDatabase*,QByteArray,QUrl)));
  connect(parseObject_, SIGNAL(feedUpdated(int, bool)),
          this->parent(), SLOT(slotUpdateFeed(int, bool)));

  exec();
}

void ParseThread::parseXml(const QByteArray &data, const QUrl &url)
{
  urlsQueue_.enqueue(url);
  xmlsQueue_.enqueue(data);
  qDebug() << "xmlsQueue_ <<" << url << "count=" << xmlsQueue_.count();
  emit startTimer();
}

void ParseThread::getQueuedXml()
{
  if (!currentUrl_.isEmpty()) return;

  while (!urlsQueue_.isEmpty()) {
    currentUrl_ = urlsQueue_.dequeue();
    currentXml_ = xmlsQueue_.dequeue();
    qDebug() << "xmlsQueue_ >>" << currentUrl_ << "count=" << xmlsQueue_.count();
    //    parse();
    emit signalReadyParse(db_, currentXml_, currentUrl_);
  }

  currentUrl_.clear();
}
