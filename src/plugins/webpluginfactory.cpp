/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "webpluginfactory.h"

#include "clicktoflash.h"
#include "mainapplication.h"
#include "adblockmanager.h"
#include "webpage.h"

#include <QDebug>
#include <QNetworkRequest>

WebPluginFactory::WebPluginFactory(WebPage *page)
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

  // AdBlock
  AdBlockManager* manager = AdBlockManager::instance();
  QNetworkRequest request(url);
  request.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 150), QString("object"));
  if (manager->isEnabled() && manager->block(request)) {
    return new QObject();
  }

  QString mime = mimeType.trimmed();
  if (mime.isEmpty()) {
    if (url.toString().contains(QLatin1String(".swf"))) {
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

  if (!mainApp->c2fIsEnabled()) {
    return 0;
  }

  // Click2Flash whitelist
  QStringList whitelist = mainApp->c2fGetWhitelist();
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

  ClickToFlash* ctf = new ClickToFlash(url, argumentNames, argumentValues, page_);
  return ctf;
}

QList<QWebPluginFactory::Plugin> WebPluginFactory::plugins() const
{
  QList<Plugin> plugins;
  return plugins;
}
