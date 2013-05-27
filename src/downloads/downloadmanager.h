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
#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QtGui>
#include <QNetworkAccessManager>

#include "networkmanager.h"

class DownloadItem;
class RSSListing;

class DownloadManager : public QWidget
{
  Q_OBJECT

public:
  explicit DownloadManager(QWidget *parent);
  ~DownloadManager();

  void download(const QNetworkRequest &request);
  void handleUnsupportedContent(QNetworkReply *reply, bool askDownloadLocation);
  void startExternalApp(const QString &executable, const QUrl &url);

  NetworkManager *networkManager_;
  QAction *listClaerAct_;

public slots:
  void ftpAuthentication(const QUrl &url, QAuthenticator *auth);

signals:
  void signalItemCreated(QListWidgetItem* item, DownloadItem* downItem);
  void signalShowDownloads(bool activate);
  void signalUpdateInfo(const QString &text);

private slots:
  QString getFileName(QNetworkReply* reply);
  void itemCreated(QListWidgetItem* item, DownloadItem* downItem);
  void clearList();
  void deleteItem(DownloadItem* item);
  void updateInfo();

private:
  RSSListing *rssl_;
  QListWidget *listWidget_;
  QTimer updateInfoTimer_;

};

#endif // DOWNLOADMANAGER_H
