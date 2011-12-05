#include "updatethread.h"

UpdateThread::UpdateThread(QObject *parent) :
    QThread(parent)
{
}

UpdateThread::~UpdateThread()
{
  qDebug("UpdateThread::~UpdateThread");
}

void UpdateThread::run()
{
  qDebug("UpdateThread::run()");
//  exec();
  return;
}
