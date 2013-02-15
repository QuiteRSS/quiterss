#include <QDebug>
#include "updatethread.h"

UpdateThread::UpdateThread(QObject *parent, int requestTimeout) :
  QThread(parent),
  requestTimeout_(requestTimeout)
{
  qDebug() << "UpdateThread::constructor";

  setObjectName("updateFeedsThread_");
  start();
}

UpdateThread::~UpdateThread()
{
  qDebug() << "UpdateThread::~destructor";
}

void UpdateThread::run()
{
  updateObject_ = new UpdateObject(requestTimeout_);

  connect(parent(), SIGNAL(signalRequestUrl(QString,QDateTime,QString)),
          updateObject_, SLOT(requestUrl(QString,QDateTime,QString)));
  connect(updateObject_, SIGNAL(getUrlDone(int,QString,QByteArray,QDateTime)),
          parent(), SLOT(getUrlDone(int,QString,QByteArray,QDateTime)));
  connect(updateObject_->networkManager_,
          SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          parent(), SLOT(slotAuthentication(QNetworkReply*,QAuthenticator*)),
          Qt::BlockingQueuedConnection);

  exec();
}
