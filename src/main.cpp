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
#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <qtsingleapplication.h>

#include "db_func.h"
#include "VersionNo.h"
#include "rsslisting.h"
#include "splashscreen.h"
#include "logfile.h"
#include "settings.h"

int main(int argc, char **argv)
{
  QtSingleApplication app(argc, argv);

  QString message = app.arguments().value(1);
  if (app.isRunning()) {
    if (1 == argc) {
      app.sendMessage("--show");
    }
    else {
      for (int i = 2; i < argc; ++i)
        message += '\n' + app.arguments().value(i);
      app.sendMessage(message);
    }
    return 0;
  } else {
    if (message.contains("--exit", Qt::CaseInsensitive))
      return 0;
  }

  app.setApplicationName("QuiteRss");
  app.setOrganizationName("QuiteRss");
  app.setApplicationVersion(STRPRODUCTVER);
#if defined(QT_NO_DEBUG_OUTPUT)
  app.setWindowIcon(QIcon(":/images/quiterss32"));
#else
  app.setWindowIcon(QIcon(":/images/images/quiterss_debug.ico"));
#endif
  app.setQuitOnLastWindowClosed(false);

  QString dataDirPath;

#if defined(PORTABLE)
  bool portable = true;
  dataDirPath = QCoreApplication::applicationDirPath();

  QString fileName;
  fileName = dataDirPath + QDir::separator() + "portable.dat";
  if (!QFile::exists(fileName)) {
    fileName = dataDirPath + QDir::separator() + QCoreApplication::applicationName() + ".ini";
    if (!QFile::exists(fileName))
      portable = false;
  }

  if (portable) {
    Settings::createSettings(dataDirPath + QDir::separator() + QCoreApplication::applicationName() + ".ini");
  } else {
    Settings::createSettings();
#ifdef HAVE_QT5
    dataDirPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
    dataDirPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
    QDir d(dataDirPath);
    d.mkpath(dataDirPath);
  }
#else
  Settings::createSettings();
#ifdef HAVE_QT5
  dataDirPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
  dataDirPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
  QDir d(dataDirPath);
  d.mkpath(dataDirPath);
#endif

  QString appDataDirPath;
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
  appDataDirPath = QCoreApplication::applicationDirPath();
#else
  appDataDirPath = DATA_DIR_PATH;
#endif

#if defined(QT_NO_DEBUG_OUTPUT)
#ifdef HAVE_QT5
  qInstallMessageHandler(LogFile::msgHandler);
#else
  qInstallMsgHandler(LogFile::msgHandler);
#endif
  qWarning() << "Start application!";
#endif
  qWarning() << "Library paths: " << QApplication::libraryPaths();

  Settings settings;
  QString styleActionStr = settings.value(
        "Settings/styleApplication", "greenStyle_").toString();
  QString fileString(appDataDirPath);
  if (styleActionStr == "systemStyle_") {
    fileString.append("/style/system.qss");
  } else if (styleActionStr == "system2Style_") {
    fileString.append("/style/system2.qss");
  } else if (styleActionStr == "orangeStyle_") {
    fileString.append("/style/orange.qss");
  } else if (styleActionStr == "purpleStyle_") {
    fileString.append("/style/purple.qss");
  } else if (styleActionStr == "pinkStyle_") {
    fileString.append("/style/pink.qss");
  } else if (styleActionStr == "grayStyle_") {
    fileString.append("/style/gray.qss");
  } else {
    fileString.append("/style/green.qss");
  }
  QFile file(fileString);
  if (!file.open(QFile::ReadOnly)) {
    file.setFileName(":/style/systemStyle");
    file.open(QFile::ReadOnly);
  }
  app.setStyleSheet(QLatin1String(file.readAll()));
  file.close();

  bool  showSplashScreen_ = settings.value("Settings/showSplashScreen", true).toBool();
  QString versionDB = settings.value("versionDB", "1.0").toString();
  if ((versionDB != kDbVersion) && QFile(settings.fileName()).exists())
    showSplashScreen_ = true;

  SplashScreen *splashScreen = new SplashScreen(QPixmap(":/images/images/splashScreen.png"));
  if (showSplashScreen_) {
    splashScreen->show();
    if ((versionDB != kDbVersion) && QFile(settings.fileName()).exists())
      splashScreen->showMessage(QString("Converting database to version %1...").
                          arg(kDbVersion),
                          Qt::AlignRight | Qt::AlignTop, Qt::darkGray);
  }

  RSSListing rsslisting(appDataDirPath, dataDirPath);

  app.setActivationWindow(&rsslisting, true);
  QObject::connect(&app, SIGNAL(messageReceived(const QString&)),
                   &rsslisting, SLOT(receiveMessage(const QString&)));

  if (showSplashScreen_)
    splashScreen->loadModules();

  if (!rsslisting.startingTray_ || !rsslisting.showTrayIcon_)
    rsslisting.show();
  rsslisting.minimizeToTray_ = false;

  if (rsslisting.showTrayIcon_) {
    qApp->processEvents();
    rsslisting.traySystem->show();
  }

  if (showSplashScreen_)
    splashScreen->finish(&rsslisting);

  rsslisting.restoreFeedsOnStartUp();

  if (message.contains("feed:", Qt::CaseInsensitive))
    rsslisting.receiveMessage(message);

  return app.exec();
}
