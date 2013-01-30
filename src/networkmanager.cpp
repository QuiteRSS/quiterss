#include "networkmanager.h"
#include <QNetworkReply>
#include <QSslError>
#include <QDebug>


NetworkManager::NetworkManager(QObject* parent, QNetworkCookieJar *cookieJar)
  : QNetworkAccessManager(parent)
{
  setCookieJar(cookieJar);

  connect(this, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)),
          SLOT(handleSslErrors(QNetworkReply*, const QList<QSslError> &)));
  connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          SIGNAL(signalAuthentication(QNetworkReply*,QAuthenticator*)));
}

NetworkManager::~NetworkManager()
{
}

void NetworkManager::handleSslErrors(QNetworkReply* reply,
                                     const QList<QSslError> &errors)
{
  qDebug() << "handleSslErrors: ";
  foreach (QSslError e, errors) {
    qDebug() << "ssl error: " << e.errorString();
  }

  reply->ignoreSslErrors();
}
