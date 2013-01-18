#ifndef UPDATEOBJECT_H
#define UPDATEOBJECT_H

#include <QNetworkAccessManager>
#include <QObject>

class UpdateObject : public QObject
{
  Q_OBJECT

public:
  explicit UpdateObject(QObject *parent = 0);
  QNetworkAccessManager manager_;
  
signals:
  void signalFinished(QNetworkReply *reply);
  
public slots:
  void slotHead(const QNetworkRequest &request);
  void slotGet(const QNetworkRequest &request);

private slots:
  void handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors);
  
};

#endif // UPDATEOBJECT_H
