#include <QAction>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QSslError>
#include "rsslisting.h"
#include "webpage.h"

WebPage::WebPage(QWidget *parent, QNetworkAccessManager *networkManager) :
  QWebPage(parent)
{
  setNetworkAccessManager(networkManager);

  action(QWebPage::OpenFrameInNewWindow)->setVisible(false);
  action(QWebPage::DownloadLinkToDisk)->setVisible(false);
  action(QWebPage::OpenImageInNewWindow)->setVisible(false);
  action(QWebPage::DownloadImageToDisk)->setVisible(false);
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
