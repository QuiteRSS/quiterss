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
/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2013  David Rosca <nowrep@gmail.com>
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
#ifndef DOWNLOADITEM_H
#define DOWNLOADITEM_H

#include <QtGui>
#include <QTimer>
#include <QNetworkReply>

class QListWidgetItem;
class DownloadManager;

class DownloadItem : public QWidget
{
  Q_OBJECT
public:
  explicit DownloadItem(QListWidgetItem *item, QNetworkReply *reply,
                        const QString &fileName, bool openAfterDownload,
                        DownloadManager *manager);
  ~DownloadItem();

  void startDownloading();
  bool isDownloading() { return downloading_; }
  static QString remaingTimeToString(QTime time);
  static QString currentSpeedToString(double speed);

signals:
  void deleteItem(DownloadItem*);
  void downloadFinished(bool success);

protected:
  virtual void mouseDoubleClickEvent(QMouseEvent*);

private slots:
  void finished();
  void metaDataChanged();
  void updateInfo();
  void downloadProgress(qint64 received, qint64 total);
  void stop(bool askForDeleteFile = true);
  void openFile();
  void openFolder();
  void readyRead();
  void error();
  void updateDownload();
  void customContextMenuRequested(const QPoint &pos);
  void clear();

  void copyDownloadLink();

private:
  QString fileSizeToString(qint64 size);

  QListWidgetItem* item_;
  QNetworkReply* reply_;
  QString fileName_;
  QTime downloadTimer_;
  QTimer updateInfoTimer_;
  QFile outputFile_;
  QUrl downloadUrl_;
  DownloadManager *manager_;

  bool downloading_;
  bool openAfterFinish_;
  bool downloadStopped_;
  double curSpeed_;
  qint64 received_;
  qint64 total_;

  QLabel *fileNameLabel_;
  QProgressBar *progressBar_;
  QFrame *progressFrame_;
  QLabel *downloadInfo_;
};

#endif // DOWNLOADITEM_H
