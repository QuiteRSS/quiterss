#include <QInputEvent>
#include "webpage.h"
#include "webview.h"

WebView::WebView(QWidget *parent) :
  QWebView(parent)
{
  setPage(new WebPage(this));
  midButtonClick = false;
}

/*virtual*/ void WebView::mousePressEvent(QMouseEvent *event)
{
  midButtonClick = false;
  if (event->buttons() & Qt::RightButton)
    posX1 = event->pos().x();

  QWebView::mousePressEvent(event);
}

/*virtual*/ void WebView::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() & Qt::RightButton) {
    rightButtonClick = false;
    int posX2 = event->pos().x();
    if (posX1 > posX2+5) {
      rightButtonClick = true;
      back();
    } else if (posX1+5 < posX2) {
      rightButtonClick = true;
      forward();
    }
  } else if (event->button() & Qt::MiddleButton) {
    midButtonClick = true;
  }

  QWebView::mouseReleaseEvent(event);
}

/*virtual*/ void WebView::contextMenuEvent(QContextMenuEvent *event)
{
  if (rightButtonClick) {
    rightButtonClick = false;
    return;
  }

  QWebView::contextMenuEvent(event);
}
