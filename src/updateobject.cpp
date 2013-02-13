#include "updateobject.h"

UpdateObject::UpdateObject(QObject *parent) :
  QObject(parent)
{
  networkManager_ = new NetworkManager();
  connect(networkManager_, SIGNAL(finished(QNetworkReply*)),
          this, SIGNAL(signalFinished(QNetworkReply*)));
}

void UpdateObject::slotHead(const QNetworkRequest &request)
{
  emit signalNetworkReply(networkManager_->head(request));
}

void UpdateObject::slotGet(const QNetworkRequest &request)
{
  emit signalNetworkReply(networkManager_->get(request));
}
