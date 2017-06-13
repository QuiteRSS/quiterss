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
#include "webpage.h"

#include "mainapplication.h"
#include "networkmanagerproxy.h"
#include "webpluginfactory.h"
#include "adblockicon.h"
#include "adblockmanager.h"

#include <QAction>
#include <QDesktopServices>
#include <QNetworkRequest>

QList<WebPage*> WebPage::livingPages_;

WebPage::WebPage(QObject *parent)
  : QWebPage(parent)
  , loadProgress_(-1)
{
  networkManagerProxy_ = new NetworkManagerProxy(this, this);
  setNetworkAccessManager(networkManagerProxy_);

  setPluginFactory(new WebPluginFactory(this));
  setForwardUnsupportedContent(true);

  action(QWebPage::OpenFrameInNewWindow)->setVisible(false);
  action(QWebPage::OpenImageInNewWindow)->setVisible(false);

  connect(this, SIGNAL(loadProgress(int)), this, SLOT(progress(int)));
  connect(this, SIGNAL(loadFinished(bool)), this, SLOT(finished()));

  connect(this, SIGNAL(unsupportedContent(QNetworkReply*)),
          this, SLOT(handleUnsupportedContent(QNetworkReply*)));
  connect(this, SIGNAL(downloadRequested(QNetworkRequest)),
          this, SLOT(downloadRequested(QNetworkRequest)));
  connect(this, SIGNAL(printRequested(QWebFrame*)),
          mainApp->mainWindow(), SLOT(slotPrint(QWebFrame*)));

  livingPages_.append(this);
}

WebPage::~WebPage()
{
  livingPages_.removeOne(this);
}

void WebPage::disconnectObjects()
{
  livingPages_.removeOne(this);

  disconnect(this);
  networkManagerProxy_->disconnectObjects();
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame,
                                      const QNetworkRequest &request,
                                      NavigationType type)
{
  lastRequestType_ = type;
  lastRequestUrl_ = request.url();

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

bool WebPage::isLoading() const
{
  return loadProgress_ < 100;
}

void WebPage::urlChanged(const QUrl &url)
{
  Q_UNUSED(url)

  if (isLoading()) {
    adBlockedEntries_.clear();
  }
}

void WebPage::progress(int prog)
{
  loadProgress_ = prog;
}

void WebPage::finished()
{
  progress(100);

  if (adjustingScheduled_) {
    adjustingScheduled_ = false;

    WebView* webView = qobject_cast<WebView*>(view());
    const QSize &originalSize = webView->size();
    QSize newSize(originalSize.width() - 1, originalSize.height() - 1);

    webView->resize(newSize);
    webView->resize(originalSize);
  }

  // AdBlock
  cleanBlockedObjects();
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

bool WebPage::isPointerSafeToUse(WebPage* page)
{
  // Pointer to WebPage is passed with every QNetworkRequest casted to void*
  // So there is no way to test whether pointer is still valid or not, except
  // this hack.

  return page == 0 ? false : livingPages_.contains(page);
}

void WebPage::populateNetworkRequest(QNetworkRequest &request)
{
  WebPage* pagePointer = this;

  QVariant variant = QVariant::fromValue((void*) pagePointer);
  request.setAttribute((QNetworkRequest::Attribute)(QNetworkRequest::User + 100), variant);

  if (lastRequestUrl_ == request.url()) {
    request.setAttribute((QNetworkRequest::Attribute)(QNetworkRequest::User + 101), lastRequestType_);
    if (lastRequestType_ == NavigationTypeLinkClicked) {
      request.setRawHeader("X-QuiteRSS-UserLoadAction", QByteArray("1"));
    }
  }
}

void WebPage::addAdBlockRule(const AdBlockRule* rule, const QUrl &url)
{
  AdBlockedEntry entry;
  entry.rule = rule;
  entry.url = url;

  if (!adBlockedEntries_.contains(entry)) {
    adBlockedEntries_.append(entry);
  }
}

QVector<WebPage::AdBlockedEntry> WebPage::adBlockedEntries() const
{
  return adBlockedEntries_;
}

void WebPage::cleanBlockedObjects()
{
  AdBlockManager* manager = AdBlockManager::instance();
  if (!manager->isEnabled()) {
    return;
  }

  const QWebElement docElement = mainFrame()->documentElement();

  foreach (const AdBlockedEntry &entry, adBlockedEntries_) {
    const QString urlString = entry.url.toString();
    if (urlString.endsWith(QLatin1String(".js")) || urlString.endsWith(QLatin1String(".css"))) {
      continue;
    }

    QString urlEnd;

    int pos = urlString.lastIndexOf(QLatin1Char('/'));
    if (pos > 8) {
      urlEnd = urlString.mid(pos + 1);
    }

    if (urlString.endsWith(QLatin1Char('/'))) {
      urlEnd = urlString.left(urlString.size() - 1);
    }

    QString selector("img[src$=\"%1\"], iframe[src$=\"%1\"],embed[src$=\"%1\"]");
    QWebElementCollection elements = docElement.findAll(selector.arg(urlEnd));

    foreach (QWebElement element, elements) {
      QString src = element.attribute("src");
      src.remove(QLatin1String("../"));

      if (urlString.contains(src)) {
        element.setStyleProperty("display", "none");
      }
    }
  }

  // Apply domain-specific element hiding rules
  QString elementHiding = manager->elementHidingRulesForDomain(mainFrame()->url());
  if (elementHiding.isEmpty()) {
    return;
  }

  elementHiding.append(QLatin1String("\n</style>"));

  QWebElement bodyElement = docElement.findFirst("body");
  bodyElement.appendInside("<style type=\"text/css\">\n/* AdBlock */\n" + elementHiding);

  // When hiding some elements, scroll position of page will change
  // If user loaded anchor link in background tab (and didn't show it yet), fix the scroll position
  if (view() && !view()->isVisible() && !mainFrame()->url().fragment().isEmpty()) {
    mainFrame()->scrollToAnchor(mainFrame()->url().fragment());
  }
}
