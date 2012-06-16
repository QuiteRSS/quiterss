#include <QtGui>

#include <qtsingleapplication.h>

#include "logfile.h"
#include "VersionNo.h"
#include "rsslisting.h"

QSplashScreen *splash;

void loadModules(QSplashScreen* psplash)
{
  QElapsedTimer time;
  time.start();

  QProgressBar splashProgress;
  splashProgress.setObjectName("splashProgress");
  splashProgress.setTextVisible(false);
  splashProgress.setFixedHeight(10);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addStretch(1);
  layout->addWidget(&splashProgress);
  splash->setLayout(layout);
  for (int i = 0; i < 100; ) {
    if (time.elapsed() >= 1) {
      time.start();
      ++i;
      qApp->processEvents();
      splashProgress.setValue(i);
      psplash->showMessage("Loading: " + QString::number(i) + "%",
                           Qt::AlignRight | Qt::AlignTop, Qt::white);
    }
  }
}

void createSplashScreen()
{
  splash = 0;
  splash = new QSplashScreen(QPixmap(":/images/images/splashScreen.png"));
  splash->setFixedSize(splash->pixmap().width(), splash->pixmap().height());
  splash->setContentsMargins(15, 0, 15, 0);
  splash->setEnabled(false);
  splash->showMessage("Prepare loading...",
                      Qt::AlignRight | Qt::AlignTop, Qt::white);
  splash->setAttribute(Qt::WA_DeleteOnClose);
  splash->show();
}

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
  }

  app.setApplicationName("QuiteRss");
  app.setOrganizationName("QuiteRss");
  app.setWindowIcon(QIcon(":/images/QuiteRSS.ico"));
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

//#if defined(QT_NO_DEBUG_OUTPUT)
//  qInstallMsgHandler(logMessageOutput);
//#endif

  QString versionDB = settings_->value("versionDB", "1.0").toString();
  if (versionDB != "0.9.0") showSplashScreen_ = true;

  if (showSplashScreen_)
    createSplashScreen();
  if (versionDB != "0.9.0")
    splash->showMessage("Converting database to version 0.9.0...",
                        Qt::AlignRight | Qt::AlignTop, Qt::white);

  RSSListing rsslisting(settings_, dataDirPath_);

  app.setActivationWindow(&rsslisting, true);
  QObject::connect(&app, SIGNAL(messageReceived(const QString&)),
                   &rsslisting, SLOT(receiveMessage(const QString&)));

  if (showSplashScreen_)
    loadModules(splash);

  if (!rsslisting.startingTray_ || !rsslisting.showTrayIcon_)
    rsslisting.show();

  if (showSplashScreen_)
    splash->finish(&rsslisting);

  rsslisting.setCurrentFeed();

  if (rsslisting.showTrayIcon_) {
    qApp->processEvents();
    rsslisting.traySystem->show();
  }

  if (message.contains("feed://", Qt::CaseInsensitive))
    rsslisting.receiveMessage(message);

  return app.exec();
}
