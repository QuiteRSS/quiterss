#include "networkmanager.h"
#include <QNetworkReply>
#include <QSslError>
#include <QDebug>


NetworkManager::NetworkManager(QObject* parent)
  : QNetworkAccessManager(parent)
{
  connect(this, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)),
          SLOT(handleSslErrors(QNetworkReply*, const QList<QSslError> &)));
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
