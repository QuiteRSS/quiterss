/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2014 QuiteRSS Team <quiterssteam@gmail.com>
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
#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QNetworkAccessManager>

class AdBlockManager;

class NetworkManager : public QNetworkAccessManager
{
  Q_OBJECT
public:
  explicit NetworkManager(QObject *parent = 0);
  ~NetworkManager();

  QNetworkReply *createRequest(QNetworkAccessManager::Operation op,
                               const QNetworkRequest &request,
                               QIODevice *outgoingData);

private slots:
  void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
  AdBlockManager *adblockManager_;

};

#endif // NETWORKMANAGER_H
