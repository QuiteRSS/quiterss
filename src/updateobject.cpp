#include "updateobject.h"
#include <QDebug>
#include <QSslError>
#include <QNetworkReply>

UpdateObject::UpdateObject(QObject *parent) :
  QObject(parent)
{
  connect(&manager_, SIGNAL(finished(QNetworkReply*)),
          this, SIGNAL(signalFinished(QNetworkReply*)));
  connect(&manager_,
          SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )),
          this,
          SLOT(handleSslErrors(QNetworkReply*, const QList<QSslError> & )));
}

void UpdateObject::slotHead(const QNetworkRequest &request)
{
  manager_.head(request);
}

void UpdateObject::slotGet(const QNetworkRequest &request)
{
  manager_.get(request);
}

void UpdateObject::handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors)
{
    qDebug() << "handleSslErrors: ";
    foreach (QSslError e, errors) {
        qDebug() << "ssl error: " << e.errorString();
    }

    reply->ignoreSslErrors();
}
