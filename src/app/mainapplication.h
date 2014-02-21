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
#ifndef MAINAPPLICATION_H
#define MAINAPPLICATION_H

#define mainApp MainApplication::getInstance()

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <qtsingleapplication.h>
#include <QNetworkDiskCache>

#include "cookiejar.h"
#include "dbmemfilethread.h"
#include "downloadmanager.h"
#include "mainwindow.h"

class NetworkManager;
class SplashScreen;
class UpdateFeeds;

class MainApplication : public QtSingleApplication
{
  Q_OBJECT
public:
  explicit MainApplication(int &argc, char** argv);
  ~MainApplication();

  static MainApplication *getInstance();

  bool isPoratble() const;
  void setClosing();
  bool isClosing() const;

  QString resourcesDir() const;
  QString dataDir() const;
  QString dbFileName() const;
  QString cacheDefaultDir() const;

  bool storeDBMemory() const;
  bool isSaveDataLastFeed() const;
  bool removePath(const QString &path);
  void sqlQueryExec(const QString &query);

  MainWindow *mainWindow();
  NetworkManager *networkManager();
  CookieJar *cookieJar();
  void setDiskCache();
  UpdateFeeds *updateFeeds();
  void runUserFilter(int feedId, int filterId);
  DBMemFileThread *dbMemFileThread();
  DownloadManager *downloadManager();

  void c2fLoadSettings();
  void c2fSaveSettings();
  bool c2fIsEnabled() const;
  void c2fSetEnabled(bool enabled);
  QStringList c2fGetWhitelist();
  void c2fSetWhitelist(QStringList whitelist);
  void c2fAddWhitelist(const QString &site);

public slots:
  void receiveMessage(const QString &message);
  void quitApplication();

signals:
  void signalSqlQueryExec(const QString &query);

private slots:
  void commitData(QSessionManager &manager);

private:
  void checkPortable();
  void checkDir();
  void createSettings();
  void connectDatabase();
  void loadSettings();
  void setStyleApplication();
  void showSplashScreen();

  bool isPortable_;
  bool isClosing_;

  QString resourcesDir_;
  QString dataDir_;
  QString cacheDir_;

  bool storeDBMemory_;
  bool isSaveDataLastFeed_;
  QString styleApplication_;
  bool showSplashScreen_;

  SplashScreen *splashScreen_;
  MainWindow *mainWindow_;
  NetworkManager *networkManager_;
  CookieJar *cookieJar_;
  QNetworkDiskCache *diskCache_;
  DBMemFileThread *dbMemFileThread_;
  UpdateFeeds *updateFeeds_;
  DownloadManager *downloadManager_;

  QStringList c2fWhitelist_;
  bool c2fEnabled_;

};

#endif // MAINAPPLICATION_H
