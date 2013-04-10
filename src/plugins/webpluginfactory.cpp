#include "webpluginfactory.h"
#include "clicktoflash.h"
#include "webpage.h"

#include <QDebug>
#include <QNetworkRequest>

WebPluginFactory::WebPluginFactory(WebPage* page)
  : QWebPluginFactory(page)
  , page_(page)
{
}

QObject* WebPluginFactory::create(const QString &mimeType, const QUrl &url,
                                  const QStringList &argumentNames,
                                  const QStringList &argumentValues) const
{
  if (url.isEmpty()) {
    return new QObject();
  }

  QString mime = mimeType.trimmed();
  if (mime.isEmpty()) {
    if (url.toString().endsWith(QLatin1String(".swf"))) {
      mime = "application/x-shockwave-flash";
    } else {
      return 0;
    }
  }

  if (mime != QLatin1String("application/x-shockwave-flash")) {
    if ((mime != QLatin1String("application/futuresplash")) &&
        (mime != QLatin1String("application/x-java-applet"))) {
      qDebug()  << "WebPluginFactory::create creating object of mimeType : "
                << mime;
    }
    return 0;
  }

  // Click2Flash already accepted
  if (ClickToFlash::isAlreadyAccepted(url, argumentNames, argumentValues)) {
    return 0;
  }

  int ctfWidth = 0;
  int ctfHeight = 0;
  for (int i = 0; i < argumentNames.count(); i++) {
    if (argumentNames[i] == "width") ctfWidth = argumentValues[i].toInt();
    if (argumentNames[i] == "height") ctfHeight = argumentValues[i].toInt();
  }
  if ((ctfWidth < 5) && (ctfHeight < 5)) {
    return 0;
  }

  ClickToFlash* ctf = new ClickToFlash(url, argumentNames, argumentValues, page_);
  return ctf;
}

QList<QWebPluginFactory::Plugin> WebPluginFactory::plugins() const
{
  QList<Plugin> plugins;
  return plugins;
}
