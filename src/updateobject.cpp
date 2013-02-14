#include "updateobject.h"

UpdateObject::UpdateObject(QObject *parent) :
  QObject(parent)
{
  networkManager_ = new NetworkManager();
  connect(networkManager_, SIGNAL(finished(QNetworkReply*)),
          this, SIGNAL(signalReplyFinished(QNetworkReply*)));
}

void UpdateObject::slotHead(const QNetworkRequest &request)
{
  QNetworkReply *reply = networkManager_->head(request);
  emit signalReplyCreated(reply);
}

void UpdateObject::slotGet(const QNetworkRequest &request)
{
  QNetworkReply *reply = networkManager_->get(request);
  emit signalReplyCreated(reply);
}
