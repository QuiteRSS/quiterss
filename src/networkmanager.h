#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QNetworkAccessManager>

class NetworkManager : public QNetworkAccessManager
{
  Q_OBJECT
public:
  explicit NetworkManager(QObject* parent = 0);
  ~NetworkManager();

private slots:
  void handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors);

};

#endif // NETWORKMANAGER_H
