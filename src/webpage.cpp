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
#include "webpage.h"

#include "mainapplication.h"
#include "webpluginfactory.h"

#include <QAction>
#include <QDesktopServices>
#include <QNetworkRequest>

WebPage::WebPage(QObject *parent)
  : QWebPage(parent)
{
  setNetworkAccessManager(mainApp->networkManager());

  setPluginFactory(new WebPluginFactory(this));
  setForwardUnsupportedContent(true);

  action(QWebPage::OpenFrameInNewWindow)->setVisible(false);
  action(QWebPage::OpenImageInNewWindow)->setVisible(false);

  connect(this, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished()));

  connect(this, SIGNAL(unsupportedContent(QNetworkReply*)),
          this, SLOT(handleUnsupportedContent(QNetworkReply*)));
  connect(this, SIGNAL(downloadRequested(QNetworkRequest)),
          this, SLOT(downloadRequested(QNetworkRequest)));
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame,
                                      const QNetworkRequest &request,
                                      NavigationType type)
{
  return QWebPage::acceptNavigationRequest(frame,request,type);
}

QWebPage *WebPage::createWindow(WebWindowType type)
{
  Q_UNUSED(type)

  return mainApp->mainWindow()->createWebTab();
}

void WebPage::scheduleAdjustPage()
{
  WebView* webView = qobject_cast<WebView*>(view());
  if (!webView) {
    return;
  }

  if (webView->isLoading()) {
    adjustingScheduled_ = true;
  } else {
    const QSize &originalSize = webView->size();
    QSize newSize(originalSize.width() - 1, originalSize.height() - 1);

    webView->resize(newSize);
    webView->resize(originalSize);
  }
}

void WebPage::slotLoadFinished()
{
  if (adjustingScheduled_) {
    adjustingScheduled_ = false;

    WebView* webView = qobject_cast<WebView*>(view());
    const QSize &originalSize = webView->size();
    QSize newSize(originalSize.width() - 1, originalSize.height() - 1);

    webView->resize(newSize);
    webView->resize(originalSize);
  }
}

void WebPage::downloadRequested(const QNetworkRequest &request)
{
  mainApp->downloadManager()->download(request);
}

void WebPage::handleUnsupportedContent(QNetworkReply* reply)
{
  if (!reply)
    return;

  const QUrl &url = reply->url();

  switch (reply->error()) {
  case QNetworkReply::NoError:
    if (reply->header(QNetworkRequest::ContentTypeHeader).isValid()) {
      QString requestUrl = reply->request().url().toString(QUrl::RemoveFragment | QUrl::RemoveQuery);
      if (requestUrl.endsWith(QLatin1String(".swf"))) {
        const QWebElement &docElement = mainFrame()->documentElement();
        const QWebElement &object = docElement.findFirst(QString("object[src=\"%1\"]").arg(requestUrl));
        const QWebElement &embed = docElement.findFirst(QString("embed[src=\"%1\"]").arg(requestUrl));

        if (!object.isNull() || !embed.isNull()) {
          qDebug() << "WebPage::UnsupportedContent" << url << "Attempt to download flash object on site!";
          reply->deleteLater();
          return;
        }
      }
      mainApp->downloadManager()->handleUnsupportedContent(reply, mainApp->mainWindow()->askDownloadLocation_);
      return;
    }

  case QNetworkReply::ProtocolUnknownError: {
    qDebug() << "WebPage::UnsupportedContent" << url << "ProtocolUnknowError";
    QDesktopServices::openUrl(url);

    reply->deleteLater();
    return;
  }
  default:
    break;
  }

  qDebug() << "WebPage::UnsupportedContent error" << url << reply->errorString();
  reply->deleteLater();
}
