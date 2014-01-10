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
#include "webview.h"
#include "webpage.h"

#include <QApplication>
#include <QInputEvent>

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
    posX_ = event->pos().x();

  QWebView::mousePressEvent(event);
}

/*virtual*/ void WebView::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() & Qt::RightButton) {
    int posX2 = event->pos().x();
    if (posX_ > posX2+5) {
      if (history()->canGoBack())
        back();
      else
        emit signalGoHome();
    } else if (posX_+5 < posX2) {
      forward();
    } else {
      emit showContextMenu(event->pos());
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
    if (event->delta() > 0) {
      if (zoomFactor() < 5.0)
        setZoomFactor(zoomFactor()+0.1);
    }
    else {
      if (zoomFactor() > 0.3)
        setZoomFactor(zoomFactor()-0.1);
    }
    event->accept();
    return;
  }
  QWebView::wheelEvent(event);
}
