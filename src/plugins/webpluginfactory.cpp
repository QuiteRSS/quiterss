#include "webpluginfactory.h"
#include "clicktoflash.h"
#include "rsslisting.h"
#include "webpage.h"

#include <QDebug>
#include <QNetworkRequest>

WebPluginFactory::WebPluginFactory(WebPage *page, QObject *parent)
  : QWebPluginFactory(page)
  , page_(page)
{
  parent_ = parent;
  while(parent_->parent()) {
    parent_ = parent_->parent();
  }
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

  RSSListing *rss_ = qobject_cast<RSSListing*>(parent_);
  if (!rss_->c2fEnabled_) {
    return 0;
  }

  // Click2Flash whitelist
  QStringList whitelist = rss_->c2fWhitelist_;
  if (whitelist.contains(url.host()) || whitelist.contains("www." + url.host()) || whitelist.contains(url.host().remove(QLatin1String("www.")))) {
    return 0;
  }

  // Click2Flash already accepted
  if (ClickToFlash::isAlreadyAccepted(url, argumentNames, argumentValues)) {
    return 0;
  }

  int ctfWidth = 10;
  int ctfHeight = 10;
  for (int i = 0; i < argumentNames.count(); i++) {
    if (argumentNames[i] == "width") {
      if (!argumentValues[i].contains("%"))
        ctfWidth = argumentValues[i].toInt();
    }
    if (argumentNames[i] == "height") {
      if (!argumentValues[i].contains("%"))
        ctfHeight = argumentValues[i].toInt();
    }
  }
  if ((ctfWidth < 5) && (ctfHeight < 5)) {
    return 0;
  }

  ClickToFlash* ctf = new ClickToFlash(url, argumentNames, argumentValues, page_, parent_);
  return ctf;
}

QList<QWebPluginFactory::Plugin> WebPluginFactory::plugins() const
{
  QList<Plugin> plugins;
  return plugins;
}
