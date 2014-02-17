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
#include "mainapplication.h"

#include "db_func.h"
#include "VersionNo.h"
#include "rsslisting.h"
#include "splashscreen.h"
#include "settings.h"

MainApplication::MainApplication(int &argc, char **argv)
  : QtSingleApplication(argc, argv)
  , isClosing_(false)
{
  QString message = arguments().value(1);
  if (isRunning()) {
    if (argc == 1) {
      sendMessage("--show");
    } else {
      for (int i = 2; i < argc; ++i)
        message += '\n' + arguments().value(i);
      sendMessage(message);
    }
    isClosing_ = true;
    return;
  } else {
    if (message.contains("--exit", Qt::CaseInsensitive)) {
      isClosing_ = true;
      return;
    }
  }

  setApplicationName("QuiteRss");
  setOrganizationName("QuiteRss");
  setApplicationVersion(STRPRODUCTVER);
  setWindowIcon(QIcon(":/images/quiterss32"));
  setQuitOnLastWindowClosed(false);
  QSettings::setDefaultFormat(QSettings::IniFormat);

  checkPortable();

  checkDir();

  Settings::createSettings();

  qWarning() << "Run application!";

  loadSettings();

  setStyleApplication();

  showSplashScreen();

  mainWindow_ = new RSSListing(resourcesDir_, dataDir_);

  if (showSplashScreen_)
    splashScreen->loadModules();

  if (!mainWindow_->startingTray_ || !mainWindow_->showTrayIcon_)
    mainWindow_->show();
  mainWindow_->minimizeToTray_ = false;

  if (mainWindow_->showTrayIcon_) {
    processEvents();
    mainWindow_->traySystem->show();
  }

  if (showSplashScreen_) {
    splashScreen->finish(mainWindow_);
    splashScreen->deleteLater();
  }

  mainWindow_->restoreFeedsOnStartUp();

  receiveMessage(message);
  connect(this, SIGNAL(messageReceived(QString)), SLOT(receiveMessage(QString)));
}

MainApplication::~MainApplication()
{

}

void MainApplication::receiveMessage(const QString &message)
{
  if (!message.isEmpty()) {
    qWarning() << QString("Received message: %1").arg(message);

    QStringList params = message.split('\n');
    foreach (QString param, params) {
      if (param == "--show") {
        if (isClosing_)
          return;
        mainWindow_->showWindows();
      }
      if (param == "--exit") mainWindow_->slotClose();
      if (param.contains("feed:", Qt::CaseInsensitive)) {
        QClipboard *clipboard = QApplication::clipboard();
        if (param.contains("https://", Qt::CaseInsensitive)) {
          param.remove(0, 5);
          clipboard->setText(param);
        } else {
          param.remove(0, 7);
          clipboard->setText("http://" + param);
        }
        mainWindow_->showWindows();
        mainWindow_->addFeed();
      }
    }
  }
}

void MainApplication::checkPortable()
{
#if defined(Q_OS_WIN)
  isPortable_ = true;
  QString fileName(QCoreApplication::applicationDirPath() + "/portable.dat");
  if (!QFile::exists(fileName)) {
    fileName = QCoreApplication::applicationDirPath() + "/" + QCoreApplication::applicationName() + ".ini";
    if (!QFile::exists(fileName))
      isPortable_ = false;
  }
#else
  isPortable_ = false;
#endif
}

void MainApplication::checkDir()
{
  if (isPortable_) {
    dataDir_ = QCoreApplication::applicationDirPath();
  } else {
#ifdef HAVE_QT5
    dataDir_ = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
    dataDir_ = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
    QDir dir(dataDir_);
    dir.mkpath(dataDir_);
  }

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
  resourcesDir_ = ".";
#else
#if defined(Q_OS_MAC)
  resourcesDir_ = QCoreApplication::applicationDirPath() + "/../Resources";
#else
  resourcesDir_ = RESOURCES_DIR;
#endif
#endif
}

void MainApplication::loadSettings()
{
  Settings settings;
  settings.beginGroup("Settings");
  storeDBMemory_ = settings.value("storeDBMemory", true).toBool();
  isSaveDataLastFeed_ = settings.value("createLastFeed", false).toBool();
  styleApplication_ = settings.value("styleApplication", "greenStyle_").toString();
  showSplashScreen_ = settings.value("showSplashScreen", true).toBool();
  settings.endGroup();
}

void MainApplication::quitApplication()
{
  qWarning() << "Quit application";

  quit();
}

bool MainApplication::isPoratble() const
{
  return isPortable_;
}

void MainApplication::setClosing()
{
  isClosing_ = true;
}

bool MainApplication::isClosing() const
{
  return isClosing_;
}

QString MainApplication::resourcesDir() const
{
  return resourcesDir_;
}

QString MainApplication::dataDir() const
{
  return dataDir_;
}

bool MainApplication::isSaveDataLastFeed() const
{
  return isSaveDataLastFeed_;
}

bool MainApplication::storeDBMemory() const
{
  return storeDBMemory_;
}

void MainApplication::setStyleApplication()
{
  QString fileName(resourcesDir_);
  if (styleApplication_ == "systemStyle_") {
    fileName.append("/style/system.qss");
  } else if (styleApplication_ == "system2Style_") {
    fileName.append("/style/system2.qss");
  } else if (styleApplication_ == "orangeStyle_") {
    fileName.append("/style/orange.qss");
  } else if (styleApplication_ == "purpleStyle_") {
    fileName.append("/style/purple.qss");
  } else if (styleApplication_ == "pinkStyle_") {
    fileName.append("/style/pink.qss");
  } else if (styleApplication_ == "grayStyle_") {
    fileName.append("/style/gray.qss");
  } else {
    fileName.append("/style/green.qss");
  }
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly)) {
    file.setFileName(":/style/systemStyle");
    file.open(QFile::ReadOnly);
  }
  setStyleSheet(QLatin1String(file.readAll()));
  file.close();

  setStyle(new QProxyStyle);
}

void MainApplication::showSplashScreen()
{
  Settings settings;
  QString versionDB = settings.value("versionDB", "1").toString();
  if ((versionDB != kDbVersion) && QFile(settings.fileName()).exists())
    showSplashScreen_ = true;

  if (showSplashScreen_) {
    splashScreen = new SplashScreen(QPixmap(":/images/images/splashScreen.png"));
    splashScreen->show();
    if ((versionDB != kDbVersion) && QFile(settings.fileName()).exists()) {
      splashScreen->showMessage(QString("Converting database to version %1...").
                                arg(kDbVersion),
                                Qt::AlignRight | Qt::AlignTop, Qt::darkGray);
    }
  }
}

void MainApplication::commitData(QSessionManager &manager)
{
  if (storeDBMemory_) {
    if (manager.allowsInteraction()) {
      int ret = QMessageBox::warning(0, tr("Save data"),
                                     tr("Attention! Need to save data.\nSave?"),
                                     QMessageBox::Yes, QMessageBox::No);
      if (ret == QMessageBox::Yes) {
        manager.release();
        mainWindow_->slotClose();
      } else {
        manager.cancel();
      }
    }
  } else {
    manager.release();
    mainWindow_->slotClose();
  }
}
