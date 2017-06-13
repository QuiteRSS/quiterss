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
#include "webview.h"
#include "webpage.h"

#include <QApplication>
#include <QInputEvent>
#include <QDebug>
#include <QDrag>
#include <QMimeData>

WebView::WebView(QWidget *parent)
  : QWebView(parent)
  , buttonClick_(0)
  , isLoading_(false)
  , rssChecked_(false)
  , hasRss_(false)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  setPage(new WebPage(this));
  QPalette pal(qApp->palette());
  pal.setColor(QPalette::Base, Qt::white);
  setPalette(pal);

  connect(this, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
  connect(this, SIGNAL(loadProgress(int)), this, SLOT(slotLoadProgress(int)));
  connect(this, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished()));
}

void WebView::disconnectObjects()
{
  disconnect(this);
}

/*virtual*/ void WebView::mousePressEvent(QMouseEvent *event)
{
  buttonClick_ = 0;

  if (event->buttons() == Qt::RightButton) {
    posX_ = event->pos().x();
  } else if (event->buttons() == Qt::LeftButton) {
    dragStartPos_ = event->pos();
  }

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

void WebView::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons() != Qt::LeftButton) {
    QWebView::mouseMoveEvent(event);
    return;
  }

  QSize viewSize;
  viewSize.setWidth(page()->viewportSize().width() -
                    page()->mainFrame()->scrollBarGeometry(Qt::Vertical).width());
  viewSize.setHeight(page()->viewportSize().height() -
                     page()->mainFrame()->scrollBarGeometry(Qt::Horizontal).height());
  if ((dragStartPos_.x() > viewSize.width()) || (dragStartPos_.y() > viewSize.height())) {
    QWebView::mouseMoveEvent(event);
    return;
  }

  int manhattanLength = (event->pos() - dragStartPos_).manhattanLength();
  if (manhattanLength <= QApplication::startDragDistance()) {
    QWebView::mouseMoveEvent(event);
    return;
  }

  const QWebHitTestResult &hitTest = page()->mainFrame()->hitTestContent(dragStartPos_);
  if (hitTest.linkUrl().isEmpty()) {
    QWebView::mouseMoveEvent(event);
    return;
  }

  QDrag *drag = new QDrag(this);
  QMimeData *mime = new QMimeData;
  mime->setUrls(QList<QUrl>() << hitTest.linkUrl());
  mime->setText(hitTest.linkUrl().toString());

  drag->setMimeData(mime);
  drag->exec();
}

void WebView::slotLoadStarted()
{
  isLoading_ = true;

  rssChecked_ = false;
  emit rssChanged(false);
}

void WebView::slotLoadProgress(int value)
{
  if (value > 60) {
    checkRss();
  }
}

void WebView::slotLoadFinished()
{
  isLoading_ = false;
}

void WebView::checkRss()
{
  if (rssChecked_) {
    return;
  }

  rssChecked_ = true;
  QWebFrame* frame = page()->mainFrame();
  const QWebElementCollection links = frame->findAllElements("link[type=\"application/rss+xml\"]");

  hasRss_ = links.count() != 0;
  emit rssChanged(hasRss_);
}
