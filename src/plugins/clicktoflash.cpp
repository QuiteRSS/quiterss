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
/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2014  David Rosca <nowrep@gmail.com>
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
/* ============================================================
*
* Copyright (C) 2009 by Benjamin C. Meyer <ben@meyerhome.net>
* Copyright (C) 2010 by Matthieu Gicquel <matgic78@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*
* ============================================================ */
#include "clicktoflash.h"

#include "mainapplication.h"
#include "webpage.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <QFormLayout>
#include <QFrame>
#include <QMenu>
#include <QTimer>
#include <QWebView>
#include <QNetworkRequest>
#include <QWebHitTestResult>

QUrl ClickToFlash::acceptedUrl;
QStringList ClickToFlash::acceptedArgNames;
QStringList ClickToFlash::acceptedArgValues;

ClickToFlash::ClickToFlash(const QUrl &pluginUrl,
                           const QStringList &argumentNames,
                           const QStringList &argumentValues,
                           WebPage *parentPage,
                           QWidget *parent)
  : QWidget(parent)
  , argumentNames_(argumentNames)
  , argumentValues_(argumentValues)
  , url_(pluginUrl)
  , page_(parentPage)
{
  frame_ = new QFrame(this);
  frame_->setObjectName("click2flash-frame");
  frame_->setContentsMargins(0, 0, 0, 0);

  loadButton_ = new QToolButton(this);
  loadButton_->setObjectName("click2flash-toolbutton");
  loadButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  loadButton_->setCursor(Qt::PointingHandCursor);

  QHBoxLayout *layout1 = new QHBoxLayout(this);
  layout1->setContentsMargins(0, 0, 0, 0);
  layout1->addWidget(frame_);

  QHBoxLayout *layout2 = new QHBoxLayout(frame_);
  layout2->setContentsMargins(0, 0, 0, 0);
  layout2->addWidget(loadButton_);

  connect(loadButton_, SIGNAL(clicked()), this, SLOT(load()));
  setMinimumSize(29, 29);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenuRequested(QPoint)));

  QTimer::singleShot(0, this, SLOT(ensurePluginVisible()));
}

bool ClickToFlash::isAlreadyAccepted(const QUrl &url,
                                     const QStringList &argumentNames,
                                     const QStringList &argumentValues)
{
  return (url == acceptedUrl &&
          argumentNames == acceptedArgNames &&
          argumentValues == acceptedArgValues);
}

void ClickToFlash::ensurePluginVisible()
{
  page_->scheduleAdjustPage();
}

void ClickToFlash::customContextMenuRequested(const QPoint &pos)
{
  QMenu menu;
  menu.addAction(tr("Object blocked by ClickToFlash"));
  menu.addSeparator();
  menu.addAction(tr("Hide object"), this, SLOT(hideObject()));
  menu.addAction(tr("Add '%1' to whitelist").arg(url_.host()), this, SLOT(toWhitelist()));
  menu.actions().at(0)->setEnabled(false);
  menu.exec(mapToGlobal(pos));
}

void ClickToFlash::toWhitelist()
{
  mainApp->c2fAddWhitelist(url_.host());
  load();
}

void ClickToFlash::hideObject()
{
  findElement();
  if (!element_.isNull()) {
    element_.setStyleProperty("visibility", "hidden");
  } else {
    hide();
  }
}

void ClickToFlash::findElement()
{
  if (!loadButton_)
    return;

  QPoint objectPos = page_->view()->mapFromGlobal(loadButton_->mapToGlobal(loadButton_->pos()));
  QWebFrame* objectFrame = page_->frameAt(objectPos);
  QWebHitTestResult hitResult;
  QWebElement hitElement;

  if (objectFrame) {
    hitResult = objectFrame->hitTestContent(objectPos);
    hitElement = hitResult.element();
  }

  if (!hitElement.isNull() && (hitElement.tagName().compare("embed", Qt::CaseInsensitive) == 0 ||
                               hitElement.tagName().compare("object", Qt::CaseInsensitive) == 0)) {
    element_ = hitElement;
    return;
  }

  // HitTestResult failed, trying to find element by src
  // attribute in elements at all frames on page (less accurate)

  QList<QWebFrame*> frames;
  frames.append(objectFrame);
  frames.append(page_->mainFrame());

  while (!frames.isEmpty()) {
    QWebFrame* frame = frames.takeFirst();
    if (!frame) {
      continue;
    }
    QWebElement docElement = frame->documentElement();

    QWebElementCollection elements;
    elements.append(docElement.findAll(QLatin1String("embed")));
    elements.append(docElement.findAll(QLatin1String("object")));

    foreach (const QWebElement &element, elements) {
      if (!checkElement(element) && !checkUrlOnElement(element)) {
        continue;
      }
      element_ = element;
      return;
    }
    frames += frame->childFrames();
  }
}

void ClickToFlash::load()
{
  findElement();
  if (element_.isNull()) {
    qWarning("Click2Flash: Cannot find Flash object.");
  } else {
    acceptedUrl = url_;
    acceptedArgNames = argumentNames_;
    acceptedArgValues = argumentValues_;

    QString js = "var c2f_clone=this.cloneNode(true);var c2f_parentNode=this.parentNode;"
        "var c2f_substituteElement=document.createElement(this.tagName);"
        "c2f_substituteElement.width=this.width;c2f_substituteElement.height=this.height;"
        "c2f_substituteElement.type=\"application/futuresplash\";"
        "this.parentNode.replaceChild(c2f_substituteElement,this);setTimeout(function(){"
        "c2f_parentNode.replaceChild(c2f_clone,c2f_substituteElement);},250);";

    element_.evaluateJavaScript(js);
  }
}

bool ClickToFlash::checkUrlOnElement(QWebElement el)
{
  QString checkString = el.attribute("src");
  if (checkString.isEmpty()) {
    checkString = el.attribute("data");
  }
  if (checkString.isEmpty()) {
    checkString = el.attribute("value");
  }

  checkString = page_->mainFrame()->url().resolved(QUrl(checkString)).toString(QUrl::RemoveQuery);

  return url_.toEncoded().contains(checkString.toUtf8());
}

bool ClickToFlash::checkElement(QWebElement el)
{
  if (argumentNames_ == el.attributeNames()) {
    foreach (const QString &name, argumentNames_) {
      if (argumentValues_.indexOf(el.attribute(name)) == -1) {
        return false;
      }
    }
    return true;
  }
  return false;
}
