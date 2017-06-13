/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
* Copyright (C) 2010-2013  David Rosca <nowrep@gmail.com>
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
#include "rssdetectionwidget.h"

#include "mainapplication.h"
#include "webview.h"

#include <QToolTip>
#include <QPushButton>
#include <QWebFrame>

RSSDetectionWidget::RSSDetectionWidget(WebView* view, QWidget* parent)
  : QFrame(parent, Qt::Popup)
  , view_(view)
{
  setAttribute(Qt::WA_DeleteOnClose);
  setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  setLineWidth(1);
  setMidLineWidth(2);

  gridLayout_ = new QGridLayout(this);
  gridLayout_->setMargin(5);
  gridLayout_->setSpacing(5);

  QWebFrame* frame = view_->page()->mainFrame();
  QWebElementCollection links = frame->findAllElements("link[type=\"application/rss+xml\"]");

  int cols = links.count() / 10 == 0 ? 1 : links.count() / 10;
  int row = 0;

  for (int i = 0; i < links.count(); i++) {
    QWebElement element = links.at(i);
    QString title = element.attribute("title");
    const QUrl url = QUrl::fromEncoded(element.attribute("href").toUtf8());
    if (url.isEmpty()) {
      continue;
    }

    if (title.isEmpty()) {
      title = tr("Untitled feed");
    }

    QPushButton* button = new QPushButton(this);
    button->setStyleSheet("QPushButton {text-align:left; border: none; padding: 0px;}"
                          "QPushButton:hover {color: #1155CC;}");
    button->setCursor(Qt::PointingHandCursor);
    button->setText(title);
    button->setToolTip(url.toString());
    button->setProperty("rss-url", url);
    button->setProperty("rss-title", title);

    int pos = i % cols > 0 ? (i % cols) * 2 : 0;

    gridLayout_->addWidget(button, row, pos);
    connect(button, SIGNAL(clicked()), this, SLOT(addRss()));

    if (i % cols == cols - 1) {
      row++;
    }
  }
}

RSSDetectionWidget::~RSSDetectionWidget()
{
}

void RSSDetectionWidget::showAt(QWidget* parent)
{
  // Calculate sizes before showing
  layout()->invalidate();
  layout()->activate();

  QPoint p = parent->mapToGlobal(QPoint(0, 0));

  p.setX(p.x() + parent->width() - width());
  p.setY(p.y() + parent->height());
  move(p);

  QFrame::show();
}

void RSSDetectionWidget::addRss()
{
  if (QPushButton *button = qobject_cast<QPushButton*>(sender())) {
    QUrl url = button->property("rss-url").toUrl();

    if (url.isRelative()) {
      url = view_->page()->mainFrame()->baseUrl().resolved(url);
    }

    if (!url.isValid()) {
      return;
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(url.toString());

    mainApp->mainWindow()->addFeed();

    close();
  }
}
