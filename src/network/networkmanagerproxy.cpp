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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "networkmanagerproxy.h"
#include "webpage.h"
#include "cookiejar.h"
#include "mainapplication.h"

#include <QNetworkRequest>

NetworkManagerProxy::NetworkManagerProxy(QObject* parent)
  : QNetworkAccessManager(parent)
  , page_(0)
{
  setCookieJar(mainApp->cookieJar());

  // CookieJar is shared between NetworkManagers
  mainApp->cookieJar()->setParent(0);
}

void NetworkManagerProxy::setPrimaryNetworkAccessManager(NetworkManager* manager)
{
  Q_ASSERT(manager);

  connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          manager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          Qt::BlockingQueuedConnection);
  connect(this, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
          manager, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
          Qt::BlockingQueuedConnection);
  connect(this, SIGNAL(finished(QNetworkReply*)),
          manager, SIGNAL(finished(QNetworkReply*)));
  connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
          manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)));
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

void NetworkManagerProxy::disconnectObjects()
{
  page_ = 0;

  disconnect(mainApp->networkManager());
}
