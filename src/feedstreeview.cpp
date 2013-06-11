/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
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
#include "feedstreeview.h"
#include "feedstreemodel.h"
#include "delegatewithoutfocus.h"

#include <QSqlTableModel>
#include <QSqlQuery>

// ----------------------------------------------------------------------------
FeedsTreeView::FeedsTreeView(QWidget * parent)
  : QyurSqlTreeView(parent)
  , selectIdEn_(true)
  , autocollapseFolder_(false)
  , dragPos_(QPoint())
  , dragStartPos_(QPoint())
  , selectOldId_(-1)
  , selectOldParentId_(-1)
{
  setObjectName("feedsTreeView_");
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setEditTriggers(QAbstractItemView::NoEditTriggers);

  setSelectionBehavior(QAbstractItemView::SelectRows);
//  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setSelectionMode(QAbstractItemView::SingleSelection);

  setUniformRowHeights(true);

  header()->setStretchLastSection(false);
//  header()->setVisible(false);

  DelegateWithoutFocus *itemDelegate = new DelegateWithoutFocus(this);
  setItemDelegate(itemDelegate);

  setContextMenuPolicy(Qt::CustomContextMenu);

  setDragDropMode(QAbstractItemView::InternalMove);
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);

  connect(this, SIGNAL(expanded(const QModelIndex&)), SLOT(slotExpanded(const QModelIndex&)));
  connect(this, SIGNAL(collapsed(const QModelIndex&)), SLOT(slotCollapsed(const QModelIndex&)));
}

/** @brief Find index of next unread (by meaning) feed
 * @details Find unread feed with condition.
 * @param indexCur Index to start search from
 * @param nextCondition Find condition:
 *  1 - find previous,
 *  2 - find next,
 *  0 - find next. If fails, find previous (default)
 * @return finded index or QModelIndex()
 *---------------------------------------------------------------------------*/
QModelIndex FeedsTreeView::indexNextUnread(const QModelIndex &indexCur, int nextCondition)
{
  if (nextCondition != 2) {
    // find next
    QModelIndex index = indexNext(indexCur);
    while (index.isValid()) {
      int feedUnreadCount = ((FeedsTreeModel*)model())->dataField(index, "unread").toInt();
      if (0 < feedUnreadCount)
        return index;  // ok

      index = indexNext(index);
    }
  }

  if (nextCondition != 1) {
    // find previous
    QModelIndex index = indexPrevious(indexCur);
    while (index.isValid()) {
      int feedUnreadCount = ((FeedsTreeModel*)model())->dataField(index, "unread").toInt();
      if (0 < feedUnreadCount)
        return index;  // ok

      index = indexPrevious(index);
    }
  }

  // find next. If fails, find previous (default)
  return QModelIndex();
}

// ----------------------------------------------------------------------------
QModelIndex FeedsTreeView::lastFeedInFolder(const QModelIndex &indexFolder)
{
  QModelIndex index = QModelIndex();

  for(int i = model()->rowCount(indexFolder)-1; i >= 0; --i) {
    index = indexFolder.child(i, indexFolder.column());
    if (((FeedsTreeModel*)model())->isFolder(index))
      index = lastFeedInFolder(index);
    if (index.isValid())
      break;
  }

  return index;
}

/** @brief Find index of previous feed
 * @param indexCur Index to start search from
 * @param isParent Start search from parent excluding him
 * @return finded index or QModelIndex()
 *---------------------------------------------------------------------------*/
QModelIndex FeedsTreeView::indexPrevious(const QModelIndex &indexCur, bool isParent)
{
  QModelIndex index = QModelIndex();
  if (((FeedsTreeModel*)model())->isFolder(indexCur) && !isParent) {
    index = lastFeedInFolder(indexCur);
    if (index.isValid())
      return index;
  }

  for(int i = indexCur.row()-1; i >= 0; --i) {
    index = indexCur.sibling(i, indexCur.column());
    if (((FeedsTreeModel*)model())->isFolder(index))
      index = lastFeedInFolder(index);
    if (index.isValid())
      return index;
  }

  index = indexCur.parent();
  if (index.isValid())
    return indexPrevious(index, true);

  return QModelIndex();
}

// ----------------------------------------------------------------------------
QModelIndex FeedsTreeView::firstFeedInFolder(const QModelIndex &indexFolder)
{
  QModelIndex index = QModelIndex();

  for(int i = 0; i < model()->rowCount(indexFolder); i++) {
    index = indexFolder.child(i, indexFolder.column());
    if (((FeedsTreeModel*)model())->isFolder(index))
      index = firstFeedInFolder(index);
    if (index.isValid())
      break;
  }

  return index;
}

/** @brief Find index of next feed
 * @param indexCur Index to start search from
 * @param isParent Start search from parent excluding him
 * @return finded index or QModelIndex()
 ******************************************************************************/
QModelIndex FeedsTreeView::indexNext(const QModelIndex &indexCur, bool isParent)
{
  QModelIndex index = QModelIndex();
  if (((FeedsTreeModel*)model())->isFolder(indexCur) && !isParent) {
    index = firstFeedInFolder(indexCur);
    if (index.isValid())
      return index;
  }

  int rowCount = model()->rowCount(indexCur.parent());

  for(int i = indexCur.row()+1; i < rowCount; i++) {
    index = indexCur.sibling(i, indexCur.column());
    if (((FeedsTreeModel*)model())->isFolder(index))
      index = firstFeedInFolder(index);
    if (index.isValid())
      return index;
  }

  index = indexCur.parent();
  if (index.isValid())
    return indexNext(index, true);

  return QModelIndex();
}

// ----------------------------------------------------------------------------
QModelIndex FeedsTreeView::lastFolderInFolder(const QModelIndex &indexFolder)
{
  if (indexFolder.isValid()) {
    for(int i = model()->rowCount(indexFolder)-1; i >= 0; --i) {
      QModelIndex index = indexFolder.child(i, indexFolder.column());
      if (((FeedsTreeModel*)model())->isFolder(index)) {
        return index;
      }
    }
  } else {
    for(int i = model()->rowCount(indexFolder)-1; i >= 0; --i) {
      QModelIndex index = model()->index(i, columnIndex("text"));
      if (((FeedsTreeModel*)model())->isFolder(index))
        return index;
    }
  }

  return QModelIndex();
}

// -----------------------------------------------------------------------------
QModelIndex FeedsTreeView::indexPreviousFolder(const QModelIndex &indexCur)
{
  QModelIndex index = QModelIndex();

  for(int i = indexCur.row()-1; i >= 0; --i) {
    index = indexCur.sibling(i, indexCur.column());
    if (((FeedsTreeModel*)model())->isFolder(index))
      return index;
  }

  index = indexCur.parent();
  if (index.isValid())
    return index;

  return QModelIndex();
}

// -----------------------------------------------------------------------------
QModelIndex FeedsTreeView::firstFolderInFolder(const QModelIndex &indexFolder)
{
  if (indexFolder.isValid()) {
    for(int i = 0; i < model()->rowCount(indexFolder); i++) {
      QModelIndex index = indexFolder.child(i, indexFolder.column());
      if (((FeedsTreeModel*)model())->isFolder(index)) {
        return index;
      }
    }
  } else {
    for(int i = 0; i < model()->rowCount(indexFolder); i++) {
      QModelIndex index = model()->index(i, columnIndex("text"));
      if (((FeedsTreeModel*)model())->isFolder(index))
        return index;
    }
  }

  return QModelIndex();
}

// -----------------------------------------------------------------------------
QModelIndex FeedsTreeView::indexNextFolder(const QModelIndex &indexCur, bool isParent)
{
  QModelIndex index = QModelIndex();
  if ((((FeedsTreeModel*)model())->isFolder(indexCur) && !isParent)) {
    index = firstFolderInFolder(indexCur);
    if (index.isValid()) {
      return index;
    }
  }

  int rowCount = model()->rowCount(indexCur.parent());
  for(int i = indexCur.row()+1; i < rowCount; i++) {
    index = indexCur.sibling(i, indexCur.column());
    if (((FeedsTreeModel*)model())->isFolder(index))
      return index;
  }

  index = indexCur.parent();
  if (index.isValid()) {
    return indexNextFolder(index, true);
  }

  return QModelIndex();
}

/** @brief Own process mouse press event
 * @details Remember pressed index to selectedIndex_,
 *    process middle mouse button clicks,
 *    ignore right mouse button clicks,
 *    call standart handler for left mouse button clicks
 * @param event Event data structure
 * @sa selectedIndex_
 *---------------------------------------------------------------------------*/
void FeedsTreeView::mousePressEvent(QMouseEvent *event)
{
  QModelIndex index = indexAt(event->pos());
  QRect rectText = visualRect(index);

  if (event->buttons() & Qt::RightButton) {
    if (event->pos().x() >= rectText.x()) {
      selectId_ = ((FeedsTreeModel*)model())->getIdByIndex(index);
      selectParentId_ = ((FeedsTreeModel*)model())->getParidByIndex(index);
    }
    return;
  }

  if (!index.isValid()) return;
  if (!(event->pos().x() >= rectText.x())) {
    QyurSqlTreeView::mousePressEvent(event);
    return;
  }

  indexClicked_ = index;
  selectId_ = ((FeedsTreeModel*)model())->getIdByIndex(index);
  selectParentId_ = ((FeedsTreeModel*)model())->getParidByIndex(index);

  if ((event->buttons() & Qt::MiddleButton)) {
    emit signalMiddleClicked();
  } else if (event->buttons() & Qt::LeftButton) {
    dragStartPos_ = event->pos();
    QyurSqlTreeView::mousePressEvent(event);
  }
}

// ----------------------------------------------------------------------------
void FeedsTreeView::mouseReleaseEvent(QMouseEvent *event)
{
  dragStartPos_ = QPoint();
  QyurSqlTreeView::mouseReleaseEvent(event);
}

// ----------------------------------------------------------------------------
/*virtual*/ void FeedsTreeView::mouseMoveEvent(QMouseEvent *event)
{
  if (dragStartPos_.isNull()) return;

  if ((event->pos() - dragStartPos_).manhattanLength() < qApp->startDragDistance())
    return;

  event->accept();

  dragPos_ = event->pos();

  QMimeData *mimeData = new QMimeData;
//  mimeData->setText("MovingItem");

  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setHotSpot(event->pos() + QPoint(10,10));

  drag->exec();
}

// ----------------------------------------------------------------------------
/*virtual*/ void FeedsTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
  QModelIndex index = indexAt(event->pos());
  QRect rectText = visualRect(index);

  if (!index.isValid()) return;
  if (!(event->pos().x() >= rectText.x()) ||
      (((FeedsTreeModel*)model())->isFolder(index))) {
    QyurSqlTreeView::mouseDoubleClickEvent(event);
    return;
  }

  if (indexClicked_ == indexAt(event->pos()))
    emit signalDoubleClicked();
  else
    mousePressEvent(event);
}

// ----------------------------------------------------------------------------
/*virtual*/ void FeedsTreeView::keyPressEvent(QKeyEvent *event)
{
  if (!event->modifiers()) {
    if (event->key() == Qt::Key_Up)         emit pressKeyUp();
    else if (event->key() == Qt::Key_Down)  emit pressKeyDown();
    else if (event->key() == Qt::Key_Home)  emit pressKeyHome();
    else if (event->key() == Qt::Key_End)   emit pressKeyEnd();
  }
}

// ----------------------------------------------------------------------------
/*virtual*/ void FeedsTreeView::currentChanged(const QModelIndex &current,
                                           const QModelIndex &previous)
{
  if (selectIdEn_) {
    QModelIndex index = current;
    selectId_ = ((FeedsTreeModel*)model())->getIdByIndex(index);
    selectParentId_ = ((FeedsTreeModel*)model())->getParidByIndex(index);
  }
  selectIdEn_ = true;

  QyurSqlTreeView::currentChanged(current, previous);
}

// ----------------------------------------------------------------------------
void FeedsTreeView::dragEnterEvent(QDragEnterEvent *event)
{
  event->accept();
  dragPos_ = event->pos();
  viewport()->update();
}

// ----------------------------------------------------------------------------
void FeedsTreeView::dragLeaveEvent(QDragLeaveEvent *event)
{
  event->accept();
  dragPos_ = QPoint();
  viewport()->update();
}

// ----------------------------------------------------------------------------
void FeedsTreeView::dragMoveEvent(QDragMoveEvent *event)
{
  if (dragPos_.isNull()) {
    event->ignore();
    viewport()->update();
    return;
  }

  dragPos_ = event->pos();
  QModelIndex dragIndex = indexAt(dragPos_);

  // Process categories
  if (((FeedsTreeModel*)model())->isFolder(dragIndex)) {
    if (dragIndex == currentIndex().parent())
      event->ignore();  // drag-to-category is parent already dragged one
    else if (dragIndex == currentIndex())
      event->ignore();  // don't move category to itself
    else {
      bool ignore = false;
      QModelIndex child = dragIndex.parent();
      while (child.isValid()) {
        if (child == currentIndex()) {
          event->ignore();  // don't move category inside itself
          ignore = true;
          break;
        }
        child = child.parent();
      }
      if (!ignore) event->accept();
    }
  }
  // Process feeds
  else {
    if (dragIndex == currentIndex())
      event->ignore();  // don't move feed to itseelf
    else if (dragIndex.parent() == currentIndex())
      event->ignore();  // don't move feed to same parent
    else {
      bool ignore = false;
      QModelIndex child = dragIndex.parent();
      while (child.isValid()) {
        if (child == currentIndex()) {
          event->ignore();  // don't move feed inside itself
          ignore = true;
          break;
        }
        child = child.parent();
      }
      if (!ignore) event->accept();
    }
  }

  viewport()->update();

  if (shouldAutoScroll(event->pos()))
    startAutoScroll();
}

// ----------------------------------------------------------------------------
bool FeedsTreeView::shouldAutoScroll(const QPoint &pos) const
{
    if (!hasAutoScroll())
        return false;
    QRect area = viewport()->rect();
    return (pos.y() - area.top() < autoScrollMargin())
        || (area.bottom() - pos.y() < autoScrollMargin())
        || (pos.x() - area.left() < autoScrollMargin())
        || (area.right() - pos.x() < autoScrollMargin());
}

/** @brief Process item expanding
 *----------------------------------------------------------------------------*/
void FeedsTreeView::slotExpanded(const QModelIndex &index)
{
  QModelIndex indexExpanded = index.sibling(index.row(), columnIndex("f_Expanded"));
  model()->setData(indexExpanded, 1);

  int feedId = ((FeedsTreeModel*)model())->getIdByIndex(indexExpanded);
  QSqlQuery q;
  q.exec(QString("UPDATE feeds SET f_Expanded=1 WHERE id=='%2'").arg(feedId));

  if (feedId == selectOldId_) return;

  QModelIndex indexCollapsed =
      ((FeedsTreeModel*)model())->getIndexById(selectOldId_, selectOldParentId_);
  selectOldId_ = feedId;
  selectOldParentId_ = ((FeedsTreeModel*)model())->getParidByIndex(index);

  if (!autocollapseFolder_) return;

  if (indexExpanded.parent() != indexCollapsed)
    collapse(indexCollapsed);

  int value = index.row();
  QModelIndex parent = index.parent();
  if (parent.isValid()) value = value + 1;
  while (parent.isValid()) {
    value = value + parent.row();
    parent = parent.parent();
  }
  verticalScrollBar()->setValue(value);
}

/** @brief Process item collapsing
 *----------------------------------------------------------------------------*/
void FeedsTreeView::slotCollapsed(const QModelIndex &index)
{
  QModelIndex indexExpanded = index.sibling(index.row(), columnIndex("f_Expanded"));
  model()->setData(indexExpanded, 0);

  int feedId = ((FeedsTreeModel*)model())->getIdByIndex(indexExpanded);
  QSqlQuery q;
  q.exec(QString("UPDATE feeds SET f_Expanded=0 WHERE id=='%2'").arg(feedId));
}

// ----------------------------------------------------------------------------
void FeedsTreeView::dropEvent(QDropEvent *event)
{
  dragPos_ = QPoint();
  viewport()->update();

  event->setDropAction(Qt::MoveAction);
  event->accept();
  handleDrop(event);
}

// ----------------------------------------------------------------------------
void FeedsTreeView::paintEvent(QPaintEvent *event)
{
  QyurSqlTreeView::paintEvent(event);

  if (dragPos_.isNull()) return;

  QModelIndex dragIndex = indexAt(dragPos_);

  // Process folders
  if (((FeedsTreeModel*)model())->isFolder(dragIndex)) {
    if ((dragIndex == currentIndex().parent()) ||
        (dragIndex == currentIndex()))
      return;

    QModelIndex child = dragIndex.parent();
    while (child.isValid()) {
      if (child == currentIndex()) return;
      child = child.parent();
    }
  }
  // Process feeds
  else {
    if ((dragIndex == currentIndex()) ||
        (dragIndex.parent() == currentIndex()))
      return;
    QModelIndex child = dragIndex.parent();
    while (child.isValid()) {
      if (child == currentIndex()) return;
      child = child.parent();
    }
  }

  QModelIndex indexText = model()->index(dragIndex.row(),
                                         ((QyurSqlTreeModel*)model())->proxyColumnByOriginal("text"),
                                         dragIndex.parent());

  QRect rectText = visualRect(indexText);
  rectText.setWidth(rectText.width()-1);
  QBrush brush = qApp->palette().brush(QPalette::Highlight);

  QPainter painter;

  if (qAbs(rectText.top() - dragPos_.y()) < 3) {
    painter.begin(this->viewport());
    painter.setPen(QPen(brush, 2));
    painter.drawLine(rectText.topLeft().x()-2, rectText.top(),
                     viewport()->width()-2, rectText.top());
    painter.drawLine(rectText.topLeft().x()-2, rectText.top()-2,
                     rectText.topLeft().x()-2, rectText.top()+2);
    painter.drawLine(viewport()->width()-2, rectText.top()-2,
                     viewport()->width()-2, rectText.top()+2);
  }
  else if (qAbs(rectText.bottom() - dragPos_.y()) < 3) {
    painter.begin(this->viewport());
    painter.setPen(QPen(brush, 2));
    painter.drawLine(rectText.bottomLeft().x()-2, rectText.bottom(),
                     viewport()->width()-2, rectText.bottom());
    painter.drawLine(rectText.topLeft().x()-2, rectText.bottom()-2,
                     rectText.topLeft().x()-2, rectText.bottom()+2);
    painter.drawLine(viewport()->width()-2, rectText.bottom()-2,
                     viewport()->width()-2, rectText.bottom()+2);
  }
  else {
    if (!((FeedsTreeModel*)model())->isFolder(dragIndex))
      return;

    painter.begin(this->viewport());
    painter.setPen(QPen(brush, 1, Qt::DashLine));
    painter.setOpacity(0.5);
    painter.drawRect(rectText);

    painter.setPen(QPen());
    painter.setBrush(brush);
    painter.setOpacity(0.1);
    painter.drawRect(rectText);
  }

  painter.end();
}

// ----------------------------------------------------------------------------
QPersistentModelIndex FeedsTreeView::selectIndex()
{
  return ((FeedsTreeModel*)model())->getIndexById(selectId_, selectParentId_);
}

/** @brief Update cursor without list scrolling
 *---------------------------------------------------------------------------*/
void FeedsTreeView::updateCurrentIndex(const QModelIndex &index)
{
  setUpdatesEnabled(false);
  int topRow = verticalScrollBar()->value();
  setCurrentIndex(index);
  verticalScrollBar()->setValue(topRow);
  setUpdatesEnabled(true);
}

// ----------------------------------------------------------------------------
void FeedsTreeView::handleDrop(QDropEvent *e)
{
  QModelIndex dropIndex = indexAt(e->pos());

  QModelIndex indexWhat = currentIndex();
  QModelIndex indexWhere = dropIndex;

  int how = 0;
  QRect rectText = visualRect(dropIndex);
  if (qAbs(rectText.top() - e->pos().y()) < 3) {
    how = 0;
  } else if (qAbs(rectText.bottom() - e->pos().y()) < 3) {
    how = 1;
  } else {
    if (((FeedsTreeModel*)model())->isFolder(dropIndex)) {
      how = 2;
    } else {
      dropIndex = model()->index(dropIndex.row()+1,
                                  ((QyurSqlTreeModel*)model())->proxyColumnByOriginal("text"),
                                  dropIndex.parent());
      if (!dropIndex.isValid()) how = 1;
    }
  }

  emit signalDropped(indexWhat, indexWhere, how);
}
