/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2014  David Rosca <nowrep@gmail.com>
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
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "networkmanagerproxy.h"
#include "webpage.h"
#include "cookiejar.h"
#include "mainapplication.h"

#include <QNetworkRequest>

NetworkManagerProxy::NetworkManagerProxy(WebPage *page, QObject* parent)
  : QNetworkAccessManager(parent)
  , page_(page)
{
  setCookieJar(mainApp->cookieJar());
  // CookieJar is shared between NetworkManagers
  mainApp->cookieJar()->setParent(0);

#ifndef QT_NO_NETWORKPROXY
  qRegisterMetaType<QNetworkProxy>("QNetworkProxy");
  qRegisterMetaType<QList<QSslError> >("QList<QSslError>");
#endif

  connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          mainApp->networkManager(), SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)));
  connect(this, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
          mainApp->networkManager(), SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
  connect(this, SIGNAL(finished(QNetworkReply*)),
          mainApp->networkManager(), SIGNAL(finished(QNetworkReply*)));

  if (page_) {
    connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
            mainApp->networkManager(), SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)));
  } else {
    connect(this, SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)),
            SLOT(slotSslError(QNetworkReply*, QList<QSslError>)));
  }
}

QNetworkReply* NetworkManagerProxy::createRequest(QNetworkAccessManager::Operation op,
                                                  const QNetworkRequest &request,
                                                  QIODevice* outgoingData)
{
  if (page_) {
    QNetworkRequest pageRequest = request;
    page_->populateNetworkRequest(pageRequest);
    return mainApp->networkManager()->createRequest(op, pageRequest, outgoingData);
  }
  return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

void NetworkManagerProxy::slotSslError(QNetworkReply *reply, QList<QSslError> errors)
{
  reply->ignoreSslErrors(errors);
}

void NetworkManagerProxy::disconnectObjects()
{
  page_ = 0;

  disconnect(this);
  disconnect(mainApp->networkManager());
}
