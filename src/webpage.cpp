#include <QAction>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QSslError>
#include "rsslisting.h"
#include "webpage.h"
#include "webpluginfactory.h"

WebPage::WebPage(QObject *parent, QNetworkAccessManager *networkManager)
  : QWebPage(parent)
  , isLoading_(false)
{
  setNetworkAccessManager(networkManager);

  setPluginFactory(new WebPluginFactory(this, parent));

  action(QWebPage::OpenFrameInNewWindow)->setVisible(false);
  action(QWebPage::DownloadLinkToDisk)->setVisible(false);
  action(QWebPage::OpenImageInNewWindow)->setVisible(false);
  action(QWebPage::DownloadImageToDisk)->setVisible(false);

  connect(this, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
  connect(this, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished()));
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
  return QWebPage::acceptNavigationRequest(frame,request,type);
}

QWebPage *WebPage::createWindow(WebWindowType type)
{
  Q_UNUSED(type)

  QObject *parent_ = parent();
  while(parent_->parent()) {
    parent_ = parent_->parent();
  }
  RSSListing *rsslisting_ = qobject_cast<RSSListing*>(parent_);
  if (rsslisting_->currentNewsTab->webView_->midButtonClick)
    return rsslisting_->createWebTab();
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
