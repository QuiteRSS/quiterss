#ifndef UPDATEOBJECT_H
#define UPDATEOBJECT_H

#include "networkmanager.h"
#include <QObject>

class UpdateObject : public QObject
{
  Q_OBJECT

public:
  explicit UpdateObject(QObject *parent = 0);
  NetworkManager *networkManager_;
  
signals:
  void signalFinished(QNetworkReply *reply);
  void signalNetworkReply(QNetworkReply *reply);
  
public slots:
  void slotHead(const QNetworkRequest &request);
  void slotGet(const QNetworkRequest &request);
  
};

#endif // UPDATEOBJECT_H
