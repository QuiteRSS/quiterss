#include <QtGui>

#include <qtsingleapplication.h>

#include "db_func.h"
#include "logfile.h"
#include "VersionNo.h"
#include "rsslisting.h"
#include "splashscreen.h"

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
  app.setWindowIcon(QIcon(":/images/QuiteRSS.ico"));
#if defined(QT_NO_DEBUG_OUTPUT)
  app.setWindowIcon(QIcon(":/images/QuiteRSS.ico"));
#else
  app.setWindowIcon(QIcon(":/images/images/quiterss_debug.ico"));
#endif
  app.setQuitOnLastWindowClosed(false);

QString dataDirPath_;
QSettings *settings_;

#if defined(PORTABLE)
  if (PORTABLE) {
    dataDirPath_ = QCoreApplication::applicationDirPath();
    settings_ = new QSettings(
          dataDirPath_ + QDir::separator() + QCoreApplication::applicationName() + ".ini",
          QSettings::IniFormat);
  } else {
    settings_ = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                              QCoreApplication::organizationName(), QCoreApplication::applicationName());
    dataDirPath_ = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    QDir d(dataDirPath_);
    d.mkpath(dataDirPath_);
  }
#else
  settings_ = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                            QCoreApplication::organizationName(), QCoreApplication::applicationName());
  dataDirPath_ = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
  QDir d(dataDirPath_);
  d.mkpath(dataDirPath_);
#endif

  bool  showSplashScreen_ = settings_->value("Settings/showSplashScreen", true).toBool();

  QString styleActionStr = settings_->value(
        "Settings/styleApplication", "greenStyle_").toString();
  QString fileString;
  if (styleActionStr == "systemStyle_") {
    fileString = ":/style/systemStyle";
  } else if (styleActionStr == "system2Style_") {
    fileString = ":/style/system2Style";
  } else if (styleActionStr == "orangeStyle_") {
    fileString = ":/style/orangeStyle";
  } else if (styleActionStr == "purpleStyle_") {
    fileString = ":/style/purpleStyle";
  } else if (styleActionStr == "pinkStyle_") {
    fileString = ":/style/pinkStyle";
  } else if (styleActionStr == "grayStyle_") {
    fileString = ":/style/grayStyle";
  } else {
    fileString = ":/style/greenStyle";
  }
  QFile file(fileString);
  file.open(QFile::ReadOnly);
  app.setStyleSheet(QLatin1String(file.readAll()));
  file.close();

//#if defined(QT_NO_DEBUG_OUTPUT)
//  qInstallMsgHandler(logMessageOutput);
//#endif

  QString versionDB = settings_->value("versionDB", "1.0").toString();
  if (versionDB != kDbVersion) showSplashScreen_ = true;

  SplashScreen *splashScreen = new SplashScreen(QPixmap(":/images/images/splashScreen.png"));
  if (showSplashScreen_) {
    splashScreen->show();
    if (versionDB != kDbVersion)
      splashScreen->showMessage(QString("Converting database to version %1...").
                          arg(kDbVersion),
                          Qt::AlignRight | Qt::AlignTop, Qt::darkGray);
  }

  RSSListing rsslisting(settings_, dataDirPath_);

  app.setActivationWindow(&rsslisting, true);
  QObject::connect(&app, SIGNAL(messageReceived(const QString&)),
                   &rsslisting, SLOT(receiveMessage(const QString&)));

  if (showSplashScreen_)
    splashScreen->loadModules();

  if (!rsslisting.startingTray_ || !rsslisting.showTrayIcon_)
    rsslisting.show();

  splashScreen->finish(&rsslisting);

  rsslisting.restoreFeedsOnStartUp();

  if (rsslisting.showTrayIcon_) {
    qApp->processEvents();
    rsslisting.traySystem->show();
  }

  if (message.contains("feed://", Qt::CaseInsensitive))
    rsslisting.receiveMessage(message);

  return app.exec();
}
