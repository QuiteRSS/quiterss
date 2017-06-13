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
#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QNetworkAccessManager>

class DownloadItem;

class DownloadManager : public QWidget
{
  Q_OBJECT

public:
  explicit DownloadManager(QWidget *parent = 0);
  ~DownloadManager();

  void download(const QNetworkRequest &request);
  void handleUnsupportedContent(QNetworkReply *reply, bool askDownloadLocation);
  void startExternalApp(const QString &executable, const QUrl &url);
  void retranslateStrings();

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
  QListWidget *listWidget_;
  QAction *listClaerAct_;
  QTimer updateInfoTimer_;

};

#endif // DOWNLOADMANAGER_H
