#include <QNetworkRequest>
#include <QDesktopServices>
#include "webpage.h"

WebPage::WebPage(QWidget *parent) :
  QWebPage(parent)
{
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type) {
  return QWebPage::acceptNavigationRequest(frame,request,type);
}

QWebPage *WebPage::createWindow(WebWindowType type) {
  return this;
}
