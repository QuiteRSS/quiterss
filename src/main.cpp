#include <QtGui>

#include "qtsingleapplication/qtsingleapplication.h"
#include "rsslisting.h"

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
    app.setWindowIcon(QIcon(":/images/QuiteRSS.ico"));
    app.setQuitOnLastWindowClosed(false);

    QString fileString = ":/style/qstyle";
//    QString fileString = app.applicationDirPath() + "/Style/QuiteRSS.qss";
    QFile file(fileString);
    file.open(QFile::ReadOnly);
    app.setStyleSheet(QLatin1String(file.readAll()));

    RSSListing rsslisting;

    app.setActivationWindow(&rsslisting, true);
    QObject::connect(&app, SIGNAL(messageReceived(const QString&)), &rsslisting, SLOT(receiveMessage(const QString&)));

    if (!rsslisting.startingTray_)
      rsslisting.show();

    return app.exec();
}
