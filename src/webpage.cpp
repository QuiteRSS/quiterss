/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
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
#include "rsslisting.h"
#include "webpluginfactory.h"

#include <QAction>
#include <QDesktopServices>
#include <QNetworkRequest>

WebPage::WebPage(QObject *parent, QNetworkAccessManager *networkManager)
  : QWebPage(parent)
  , isLoading_(false)
{
  QObject *parent_ = parent;
  while(parent_->parent()) {
    parent_ = parent_->parent();
  }
  rssl_ = qobject_cast<RSSListing*>(parent_);

  setNetworkAccessManager(networkManager);

  setPluginFactory(new WebPluginFactory(this, rssl_));
  setForwardUnsupportedContent(true);

  action(QWebPage::OpenFrameInNewWindow)->setVisible(false);
  action(QWebPage::OpenImageInNewWindow)->setVisible(false);

  connect(this, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
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

  if (rssl_->currentNewsTab->webView_->buttonClick_)
    return rssl_->createWebTab();
  else return this;
}

void WebPage::scheduleAdjustPage()
{
  WebView* webView = qobject_cast<WebView*>(view());
  if (!webView) {
    return;
  }

  if (isLoading_) {
    adjustingScheduled_ = true;
  } else {
    const QSize &originalSize = webView->size();
    QSize newSize(originalSize.width() - 1, originalSize.height() - 1);

    webView->resize(newSize);
    webView->resize(originalSize);
  }
}

void WebPage::slotLoadStarted()
{
  isLoading_ = true;
}

void WebPage::slotLoadFinished()
{
  isLoading_ = false;

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
  rssl_->downloadManager_->download(request);
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
      rssl_->downloadManager_->handleUnsupportedContent(reply, rssl_->askDownloadLocation_);
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
