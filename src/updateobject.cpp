#include "updateobject.h"

UpdateObject::UpdateObject(QObject *parent) :
  QObject(parent)
{
  connect(&manager_, SIGNAL(finished(QNetworkReply*)),
          this, SIGNAL(signalFinished(QNetworkReply*)));
}

void UpdateObject::slotHead(const QNetworkRequest &request)
{
  manager_.head(request);
}

void UpdateObject::slotGet(const QNetworkRequest &request)
{
  manager_.get(request);
}
