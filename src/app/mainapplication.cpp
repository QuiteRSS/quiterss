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

#include "cookiejar.h"
#include "db_func.h"
#include "networkmanager.h"
#include "settings.h"
#include "splashscreen.h"
#include "updatefeeds.h"
#include "VersionNo.h"

MainApplication::MainApplication(int &argc, char **argv)
  : QtSingleApplication(argc, argv)
  , isPortable_(false)
  , isClosing_(false)
  , mainWindow_(0)
  , networkManager_(0)
  , cookieJar_(0)
  , diskCache_(0)
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

  createSettings();

  qWarning() << "Run application!";

  setStyleApplication();

  showSplashScreen();

  mainWindow_ = new MainWindow();

  loadSettings();

  updateFeeds_ = new UpdateFeeds(mainWindow_);

  if (showSplashScreen_)
    splashScreen_->loadModules();

  if (!mainWindow_->startingTray_ || !mainWindow_->showTrayIcon_)
    mainWindow_->show();
  mainWindow_->minimizeToTray_ = false;

  if (mainWindow_->showTrayIcon_) {
    processEvents();
    mainWindow_->traySystem->show();
  }

  if (showSplashScreen_) {
    splashScreen_->finish(mainWindow_);
    splashScreen_->deleteLater();
  }

  mainWindow_->restoreFeedsOnStartUp();

  receiveMessage(message);
  connect(this, SIGNAL(messageReceived(QString)), SLOT(receiveMessage(QString)));
}

MainApplication::~MainApplication()
{

}

MainApplication *MainApplication::getInstance() {
  return static_cast<MainApplication*>(QCoreApplication::instance());
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
      if (param == "--exit") mainWindow_->quitApp();
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

  if (isPortable_) {
    cacheDir_ = dataDir_ + "/cache";
  } else {
#ifdef HAVE_QT5
    cacheDir_ = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
    cacheDir_ = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
#endif
  }
}

void MainApplication::createSettings()
{
  Settings::createSettings();

  Settings settings;
  settings.beginGroup("Settings");
  storeDBMemory_ = settings.value("storeDBMemory", true).toBool();
  isSaveDataLastFeed_ = settings.value("createLastFeed", false).toBool();
  styleApplication_ = settings.value("styleApplication", "greenStyle_").toString();
  showSplashScreen_ = settings.value("showSplashScreen", true).toBool();
  settings.endGroup();
}

void MainApplication::loadSettings()
{

  c2fLoadSettings();
}

void MainApplication::quitApplication()
{
  qWarning() << "Quit application";

  networkManager_->disconnect(networkManager_);

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

QString MainApplication::dbFileName() const
{
  return dataDir_ + "/" + kDbName;
}

bool MainApplication::isSaveDataLastFeed() const
{
  return isSaveDataLastFeed_;
}

bool MainApplication::storeDBMemory() const
{
  return storeDBMemory_;
}

bool MainApplication::removePath(const QString &path)
{
  bool result = true;
  QFileInfo info(path);
  if (info.isDir()) {
    QDir dir(path);
    foreach (const QString &entry, dir.entryList(QDir::AllDirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot)) {
      result &= removePath(dir.absoluteFilePath(entry));
    }
    if (!info.dir().rmdir(info.fileName()))
      return false;
  } else {
    result = QFile::remove(path);
  }
  return result;
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
  if ((versionDB != kDbVersion) && QFile::exists(settings.fileName()))
    showSplashScreen_ = true;

  if (showSplashScreen_) {
    splashScreen_ = new SplashScreen(QPixmap(":/images/images/splashScreen.png"));
    splashScreen_->show();
    if ((versionDB != kDbVersion) && QFile::exists(settings.fileName())) {
      splashScreen_->showMessage(QString("Converting database to version %1...").
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
        mainWindow_->quitApp();
      } else {
        manager.cancel();
      }
    }
  } else {
    manager.release();
    mainWindow_->quitApp();
  }
}

MainWindow *MainApplication::mainWindow()
{
  return mainWindow_;
}

NetworkManager *MainApplication::networkManager()
{
  if (!networkManager_) {
    networkManager_ = new NetworkManager(this);
    setDiskCache();
  }
  return networkManager_;
}

CookieJar *MainApplication::cookieJar()
{
  if (!cookieJar_) {
    cookieJar_ = new CookieJar(this);
  }
  return cookieJar_;
}

void MainApplication::setDiskCache()
{
  Settings settings;
  settings.beginGroup("Settings");

  bool useDiskCache = settings.value("useDiskCache", true).toBool();
  if (useDiskCache) {
    if (!diskCache_) {
      diskCache_ = new QNetworkDiskCache(this);
    }

    QString diskCacheDirPath = settings.value("dirDiskCache", cacheDir_).toString();
    if (diskCacheDirPath.isEmpty()) diskCacheDirPath = cacheDir_;

    bool cleanDiskCache = settings.value("cleanDiskCache", true).toBool();
    if (cleanDiskCache) {
      removePath(diskCacheDirPath);
      settings.setValue("cleanDiskCache", false);
    }

    diskCache_->setCacheDirectory(diskCacheDirPath);
    int maxDiskCache = settings.value("Settings/maxDiskCache", 50).toInt();
    diskCache_->setMaximumCacheSize(maxDiskCache*1024*1024);

    networkManager()->setCache(diskCache_);
  } else {
    if (diskCache_) {
      diskCache_->setMaximumCacheSize(0);
      diskCache_->clear();
    }
  }

  settings.endGroup();
}

QString MainApplication::cacheDefaultDir() const
{
  return cacheDir_;
}

UpdateFeeds *MainApplication::updateFeeds()
{
  return updateFeeds_;
}

void MainApplication::runUserFilter(int feedId, int filterId)
{
  updateFeeds_->parseObject_->runUserFilter(feedId, filterId);
}

void MainApplication::sqlQueryExec(const QString &query)
{
  emit signalSqlQueryExec(query);
}

/** @brief Click to Flash
 *---------------------------------------------------------------------------*/
void MainApplication::c2fLoadSettings()
{
  Settings settings;
  settings.beginGroup("ClickToFlash");
  c2fWhitelist_ = settings.value("whitelist", QStringList()).toStringList();
  c2fEnabled_ = settings.value("enabled", true).toBool();
  settings.endGroup();
}

void MainApplication::c2fSaveSettings()
{
  Settings settings;
  settings.beginGroup("ClickToFlash");
  settings.setValue("whitelist", c2fWhitelist_);
  settings.setValue("enabled", c2fEnabled_);
  settings.endGroup();
}

bool MainApplication::c2fIsEnabled() const
{
  return c2fEnabled_;
}

void MainApplication::c2fSetEnabled(bool enabled)
{
  c2fEnabled_ = enabled;
}

QStringList MainApplication::c2fGetWhitelist()
{
  return c2fWhitelist_;
}

void MainApplication::c2fSetWhitelist(QStringList whitelist)
{
  c2fWhitelist_ = whitelist;
}

void MainApplication::c2fAddWhitelist(const QString &site)
{
  c2fWhitelist_.append(site);
}
