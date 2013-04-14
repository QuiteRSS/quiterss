#include <QApplication>
#include <QInputEvent>
#include "webpage.h"
#include "webview.h"

WebView::WebView(QWidget *parent, QNetworkAccessManager *networkManager)
  : QWebView(parent)
  , buttonClick_(0)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  setPage(new WebPage(this, networkManager));
  QPalette pal(qApp->palette());
  pal.setColor(QPalette::Base, Qt::white);
  setPalette(pal);
}

/*virtual*/ void WebView::mousePressEvent(QMouseEvent *event)
{
  buttonClick_ = 0;
  if (event->buttons() & Qt::RightButton)
    posX1 = event->pos().x();

  QWebView::mousePressEvent(event);
}

/*virtual*/ void WebView::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() & Qt::RightButton) {
    rightButtonClick_ = false;
    int posX2 = event->pos().x();
    if (posX1 > posX2+5) {
      rightButtonClick_ = true;
      back();
    } else if (posX1+5 < posX2) {
      rightButtonClick_ = true;
      forward();
    }
  } else if (event->button() & Qt::MiddleButton) {
    if (event->modifiers() == Qt::NoModifier) {
      buttonClick_ = MIDDLE_BUTTON;
    } else {
      buttonClick_ = MIDDLE_BUTTON_MOD;
    }
  } else if (event->button() & Qt::LeftButton) {
    if (event->modifiers() == Qt::ControlModifier) {
      buttonClick_ = LEFT_BUTTON_CTRL;
    } else if ((event->modifiers() == Qt::ShiftModifier) ||
               (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))) {
      buttonClick_ = LEFT_BUTTON_SHIFT;
    } else if (event->modifiers() == Qt::AltModifier) {
      buttonClick_ = LEFT_BUTTON_ALT;
    } else {
      buttonClick_ = LEFT_BUTTON;
    }
  }

  QWebView::mouseReleaseEvent(event);
}

/*virtual*/ void WebView::wheelEvent(QWheelEvent *event)
{
  if (event->modifiers() == Qt::ControlModifier) {
    if (event->delta() > 0)
      setZoomFactor(zoomFactor()+0.1);
    else if (zoomFactor() > 0.1)
      setZoomFactor(zoomFactor()-0.1);
    return;
  }
  QWebView::wheelEvent(event);
}
