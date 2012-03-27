#ifndef UPDATEOBJECT_H
#define UPDATEOBJECT_H

#include <QNetworkAccessManager>
#include <QObject>

class UpdateObject : public QObject
{
  Q_OBJECT
  QNetworkAccessManager manager_;

public:
  explicit UpdateObject(QObject *parent = 0);
  
signals:
  void signalFinished(QNetworkReply *reply);
  
public slots:
  void slotHead(const QNetworkRequest &request);
  void slotGet(const QNetworkRequest &request);
  
};

#endif // UPDATEOBJECT_H
