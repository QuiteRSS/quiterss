#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QNetworkAccessManager>

class NetworkManager : public QNetworkAccessManager
{
  Q_OBJECT
public:
  explicit NetworkManager(QObject* parent, QNetworkCookieJar *cookieJar);
  ~NetworkManager();

signals:
  void signalAuthentication(QNetworkReply* reply, QAuthenticator* auth);

private slots:
  void handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors);

};

#endif // NETWORKMANAGER_H
