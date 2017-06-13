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
#include "tabbar.h"

#include "mainapplication.h"

TabBar::TabBar(QWidget *parent)
  : QTabBar(parent)
  , closingTabState_(CloseTabIdle)
  , indexClickedTab_(-1)
  , tabFixed_(false)
{
  setObjectName("tabBar_");
  setFocusPolicy(Qt::NoFocus);
  setDocumentMode(true);
  setMouseTracking(true);
  setExpanding(false);
  setMovable(true);
  setElideMode(Qt::ElideNone);
  setIconSize(QSize(0, 0));
  setContextMenuPolicy(Qt::CustomContextMenu);

  setStyleSheet(QString("#tabBar_ QToolButton {border: 1px solid %1; border-radius: 2px; background: %2;}").
                arg(qApp->palette().color(QPalette::Dark).name()).
                arg(palette().background().color().name()));

  addTab("");

  connect(this, SIGNAL(customContextMenuRequested(QPoint)),
          this, SLOT(showContextMenuTabBar(const QPoint &)));

  installEventFilter(this);
}

bool TabBar::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::MouseButtonPress) {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    if (tabAt(mouseEvent->pos()) < 0)
      return false;
    if (mouseEvent->button() & Qt::MiddleButton) {
      emit closeTab(tabAt(mouseEvent->pos()));
    } else if (mouseEvent->button() & Qt::LeftButton) {
      if (tabAt(QPoint(mouseEvent->pos().x(), 0)) == 0)
        tabFixed_ = true;
      else
        tabFixed_ = false;
    }
  } else if (event->type() == QEvent::MouseMove) {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    if (mouseEvent->buttons() & Qt::LeftButton) {
      if ((tabAt(QPoint(mouseEvent->pos().x()-78, 0)) <= 0) || tabFixed_)
        return true;
    }
  }

  return QTabBar::eventFilter(obj, event);
}

void TabBar::showContextMenuTabBar(const QPoint &pos)
{
  indexClickedTab_ = tabAt(pos);

  if (indexClickedTab_ == -1) return;

  QMenu menu;
  menu.addAction(mainApp->mainWindow()->closeTabAct_);
  menu.addAction(mainApp->mainWindow()->closeOtherTabsAct_);
  menu.addAction(mainApp->mainWindow()->closeAllTabsAct_);

  menu.exec(mapToGlobal(pos));

  indexClickedTab_ = -1;
}

void TabBar::slotCloseTab()
{
  int index = indexClickedTab_;
  if (index == -1) index = currentIndex();

  emit closeTab(index);
}

void TabBar::slotCloseOtherTabs()
{
  int index = indexClickedTab_;
  int curIndex = -1;
  if (index == -1) index = currentIndex();
  else {
    if (indexClickedTab_ > currentIndex()) {
      curIndex = currentIndex();
    } else if (indexClickedTab_ < currentIndex()){
      curIndex = indexClickedTab_ + 1;
    }
  }

  closingTabState_ = CloseTabOtherIndex;
  for (int i = count()-1; i > 0; i--) {
    if (i == index) continue;
    if (i == curIndex) closingTabState_ = CloseTabCurrentIndex;
    else closingTabState_ = CloseTabOtherIndex;

    emit closeTab(i);
  }
  closingTabState_ = CloseTabIdle;
}

void TabBar::slotCloseAllTab()
{
  closingTabState_ = CloseTabOtherIndex;
  for (int i = count()-1; i > 0; i--) {
    if (i == 1) closingTabState_ = CloseTabCurrentIndex;

    emit closeTab(i);
  }
  closingTabState_ = CloseTabIdle;
}

void TabBar::slotNextTab()
{
  setCurrentIndex(currentIndex()+1);
}

void TabBar::slotPrevTab()
{
  setCurrentIndex(currentIndex()-1);
}
