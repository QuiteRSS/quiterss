#include <QtGui>

#include "qtsingleapplication/qtsingleapplication.h"
#include "rsslisting.h"

void LoadLang (QString &lang){
    QString AppFileName = qApp->applicationFilePath();
    AppFileName.replace(".exe", ".ini");
    QSettings *m_settings = new QSettings(AppFileName, QSettings::IniFormat);
    QString strLocalLang = QLocale::system().name();

    lang = m_settings->value("Settings/Lang", strLocalLang).toString();
}

int main(int argc, char **argv)
{
    QtSingleApplication app(argc, argv);
    if (app.isRunning()) {
      if (1 == argc) {
        app.sendMessage("--show");
      }
      else {
        QString message = app.arguments().value(1);
        for (int i = 2; i < argc; ++i)
          message += '\n' + app.arguments().value(i);
        app.sendMessage(message);
      }
      return 0;
    }
    app.setApplicationName("QuiteRss");
    app.setWindowIcon(QIcon(":/images/QtRSS.ico"));
    app.setQuitOnLastWindowClosed(false);

    QString fileString = ":/style/qstyle";
//    QString fileString = app.applicationDirPath() + "/Style/QtRSS.qss";
    QFile file(fileString);
    file.open(QFile::ReadOnly);
    app.setStyleSheet(QLatin1String(file.readAll()));

    QString lang;
    LoadLang(lang);
    QTranslator translator;
    translator.load(lang, app.applicationDirPath() + QString("/lang"));
    app.installTranslator(&translator);

    RSSListing rsslisting;

    app.setActivationWindow(&rsslisting, true);
    QObject::connect(&app, SIGNAL(messageReceived(const QString&)), &rsslisting, SLOT(receiveMessage(const QString&)));

    rsslisting.show();

    return app.exec();
}
