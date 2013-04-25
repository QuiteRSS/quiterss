/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
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
