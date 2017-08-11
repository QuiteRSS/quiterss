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
#include "feedsview.h"

#include "mainapplication.h"
#include "feedsmodel.h"
#include "delegatewithoutfocus.h"

#include <QSqlTableModel>
#include <QSqlQuery>

// ----------------------------------------------------------------------------
FeedsView::FeedsView(QWidget * parent)
  : QTreeView(parent)
  , selectIdEn_(true)
  , autocollapseFolder_(false)
  , sourceModel_(0)
  , dragPos_(QPoint())
  , dragStartPos_(QPoint())
  , expandedOldId_(-1)
{
  setObjectName("feedsView_");
  setFrameStyle(QFrame::NoFrame);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setEditTriggers(QAbstractItemView::NoEditTriggers);

  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setUniformRowHeights(true);

  header()->setStretchLastSection(false);
  header()->setVisible(false);

  DelegateWithoutFocus *itemDelegate = new DelegateWithoutFocus(this);
  setItemDelegate(itemDelegate);

  setContextMenuPolicy(Qt::CustomContextMenu);

  setDragDropMode(QAbstractItemView::InternalMove);
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);

  connect(this, SIGNAL(expanded(QModelIndex)), SLOT(slotExpanded(QModelIndex)));
  connect(this, SIGNAL(collapsed(QModelIndex)), SLOT(slotCollapsed(QModelIndex)));
}

void FeedsView::setSourceModel(FeedsModel *sourceModel)
{
  sourceModel_ = sourceModel;

  QSqlQuery q;
  q.exec("SELECT id FROM feeds WHERE f_Expanded=1 AND (xmlUrl='' OR xmlUrl IS NULL)");
  while (q.next()) {
    int feedId = q.value(0).toInt();
    expandedList.append(feedId);
  }
}

void FeedsView::refresh()
{
  sourceModel_->refresh();
  ((FeedsProxyModel*)model())->reset();
  restoreExpanded();
}

void FeedsView::setColumnHidden(const QString& column, bool hide)
{
  QTreeView::setColumnHidden(columnIndex(column),hide);
}

int FeedsView::columnIndex(const QString& fieldName) const
{
  return sourceModel_->indexColumnOf(fieldName);
}

bool FeedsView::isFolder(const QModelIndex &index) const
{
  if (!index.isValid())
    return false;
  return sourceModel_->isFolder(((FeedsProxyModel*)model())->mapToSource(index));
}

void FeedsView::restoreExpanded()
{
  foreach (int id, expandedList) {
    QModelIndex index = ((FeedsProxyModel*)model())->mapFromSource(id);
    if (!isExpanded(index))
      setExpanded(index, true);
  }
}

/** @brief Process item expanding
 *----------------------------------------------------------------------------*/
void FeedsView::slotExpanded(const QModelIndex &index)
{
  int feedId = sourceModel_->idByIndex(((FeedsProxyModel*)model())->mapToSource(index));
  if (!expandedList.contains(feedId)) {
    expandedList.append(feedId);
    mainApp->sqlQueryExec(QString("UPDATE feeds SET f_Expanded=1 WHERE id=='%1'").arg(feedId));
  }

  if (!autocollapseFolder_ || (feedId == expandedOldId_))
    return;

  QModelIndex indexCollapsed = ((FeedsProxyModel*)model())->mapFromSource(expandedOldId_);
  expandedOldId_ = feedId;

  if (index.parent() != indexCollapsed)
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
void FeedsView::slotCollapsed(const QModelIndex &index)
{
  int feedId = sourceModel_->idByIndex(((FeedsProxyModel*)model())->mapToSource(index));
  expandedList.removeOne(feedId);
  mainApp->sqlQueryExec(QString("UPDATE feeds SET f_Expanded=0 WHERE id=='%1'").arg(feedId));
}

void FeedsView::expandAll()
{
  expandedList.clear();
  QTreeView::expandAll();


  QSqlQuery q;
  q.exec("SELECT id FROM feeds WHERE (xmlUrl='' OR xmlUrl IS NULL)");
  while (q.next()) {
    int feedId = q.value(0).toInt();
    expandedList.append(feedId);
  }
  mainApp->sqlQueryExec("UPDATE feeds SET f_Expanded=1 WHERE (xmlUrl='' OR xmlUrl IS NULL)");
}

void FeedsView::collapseAll()
{
  expandedList.clear();
  QTreeView::collapseAll();

  mainApp->sqlQueryExec("UPDATE feeds SET f_Expanded=0 WHERE (xmlUrl='' OR xmlUrl IS NULL)");
}

/** @brief Find index of next unread (by meaning) feed
 * @details Find unread feed with condition.
 * @param indexCur Index to start search from
 * @param nextCondition Find condition:
 *  2 - find previous,
 *  1 - find next,
 *  0 - find next. If fails, find previous (default)
 * @return finded index or QModelIndex()
 *---------------------------------------------------------------------------*/
QModelIndex FeedsView::indexNextUnread(const QModelIndex &indexCur, int nextCondition)
{
  if (nextCondition != 2) {
    // find next
    QModelIndex index = indexNext(indexCur);
    while (index.isValid()) {
      int feedUnreadCount = sourceModel_->dataField(((FeedsProxyModel*)model())->mapToSource(index), "unread").toInt();
      if (0 < feedUnreadCount)
        return index;  // ok

      index = indexNext(index);
    }
  }

  if (nextCondition != 1) {
    // find previous
    QModelIndex index = indexPrevious(indexCur);
    while (index.isValid()) {
      int feedUnreadCount = sourceModel_->dataField(((FeedsProxyModel*)model())->mapToSource(index), "unread").toInt();
      if (0 < feedUnreadCount)
        return index;  // ok

      index = indexPrevious(index);
    }
  }

  // find next. If fails, find previous (default)
  return QModelIndex();
}

// ----------------------------------------------------------------------------
QModelIndex FeedsView::lastFeedInFolder(const QModelIndex &indexFolder)
{
  QModelIndex index = QModelIndex();

  for (int i = model()->rowCount(indexFolder)-1; i >= 0; --i) {
    index = indexFolder.child(i, columnIndex("text"));
    if (isFolder(index))
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
QModelIndex FeedsView::indexPrevious(const QModelIndex &indexCur, bool isParent)
{
  QModelIndex index = QModelIndex();
  if (isFolder(indexCur) && !isParent) {
    index = lastFeedInFolder(indexCur);
    if (index.isValid())
      return index;
  }

  for (int i = indexCur.row()-1; i >= 0; --i) {
    index = model()->index(i, columnIndex("text"), indexCur.parent());
    if (isFolder(index))
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
QModelIndex FeedsView::firstFeedInFolder(const QModelIndex &indexFolder)
{
  QModelIndex index = QModelIndex();

  for (int i = 0; i < model()->rowCount(indexFolder); i++) {
    index = indexFolder.child(i, columnIndex("text"));
    if (isFolder(index))
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
QModelIndex FeedsView::indexNext(const QModelIndex &indexCur, bool isParent)
{
  QModelIndex index = QModelIndex();
  if (isFolder(indexCur) && !isParent) {
    index = firstFeedInFolder(indexCur);
    if (index.isValid())
      return index;
  }

  int rowCount = model()->rowCount(indexCur.parent());

  for (int i = indexCur.row()+1; i < rowCount; i++) {
    index = model()->index(i, columnIndex("text"), indexCur.parent());
    if (isFolder(index))
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
QModelIndex FeedsView::lastFolderInFolder(const QModelIndex &indexFolder)
{
  if (indexFolder.isValid()) {
    for (int i = model()->rowCount(indexFolder)-1; i >= 0; --i) {
      QModelIndex index = indexFolder.child(i, columnIndex("text"));
      if (isFolder(index)) {
        return index;
      }
    }
  } else {
    for (int i = model()->rowCount(indexFolder)-1; i >= 0; --i) {
      QModelIndex index = model()->index(i, columnIndex("text"));
      if (isFolder(index))
        return index;
    }
  }

  return QModelIndex();
}

// -----------------------------------------------------------------------------
QModelIndex FeedsView::indexPreviousFolder(const QModelIndex &indexCur)
{
  QModelIndex index = QModelIndex();

  for (int i = indexCur.row()-1; i >= 0; --i) {
    index = model()->index(i, columnIndex("text"), indexCur.parent());
    if (isFolder(index))
      return index;
  }

  index = indexCur.parent();
  if (index.isValid())
    return index;

  return QModelIndex();
}

// -----------------------------------------------------------------------------
QModelIndex FeedsView::firstFolderInFolder(const QModelIndex &indexFolder)
{
  if (indexFolder.isValid()) {
    for (int i = 0; i < model()->rowCount(indexFolder); i++) {
      QModelIndex index = indexFolder.child(i, columnIndex("text"));
      if (isFolder(index)) {
        return index;
      }
    }
  } else {
    for (int i = 0; i < model()->rowCount(indexFolder); i++) {
      QModelIndex index = model()->index(i, columnIndex("text"));
      if (isFolder(index))
        return index;
    }
  }

  return QModelIndex();
}

// -----------------------------------------------------------------------------
QModelIndex FeedsView::indexNextFolder(const QModelIndex &indexCur, bool isParent)
{
  QModelIndex index = QModelIndex();
  if ((isFolder(indexCur) && !isParent)) {
    index = firstFolderInFolder(indexCur);
    if (index.isValid()) {
      return index;
    }
  }

  int rowCount = model()->rowCount(indexCur.parent());
  for (int i = indexCur.row()+1; i < rowCount; i++) {
    index = model()->index(i, columnIndex("text"), indexCur.parent());
    if (isFolder(index))
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
void FeedsView::mousePressEvent(QMouseEvent *event)
{
  QModelIndex index = indexAt(event->pos());
  QRect rectText = visualRect(index);

  if (event->buttons() & Qt::RightButton) {
    if (event->pos().x() >= rectText.x()) {
      index = ((FeedsProxyModel*)model())->mapToSource(index);
      selectId_ = sourceModel_->idByIndex(index);
    }
    return;
  }

  if (!index.isValid()) return;
  if (!(event->pos().x() >= rectText.x())) {
    QTreeView::mousePressEvent(event);
    return;
  }

  indexClicked_ = index;
  index = ((FeedsProxyModel*)model())->mapToSource(index);
  selectId_ = sourceModel_->idByIndex(index);

  if ((event->buttons() & Qt::MiddleButton)) {
    emit signalMiddleClicked();
  } else if (event->buttons() & Qt::LeftButton) {
    dragStartPos_ = event->pos();
    QTreeView::mousePressEvent(event);
  }
}

// ----------------------------------------------------------------------------
void FeedsView::mouseReleaseEvent(QMouseEvent *event)
{
  dragStartPos_ = QPoint();
  QTreeView::mouseReleaseEvent(event);
}

// ----------------------------------------------------------------------------
/*virtual*/ void FeedsView::mouseMoveEvent(QMouseEvent *event)
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
/*virtual*/ void FeedsView::mouseDoubleClickEvent(QMouseEvent *event)
{
  QModelIndex index = indexAt(event->pos());
  QRect rectText = visualRect(index);

  if (!index.isValid()) return;
  if (!(event->pos().x() >= rectText.x()) ||
      (isFolder(index))) {
    QTreeView::mouseDoubleClickEvent(event);
    return;
  }

  if (indexClicked_ == indexAt(event->pos()))
    emit signalDoubleClicked();
  else
    mousePressEvent(event);
}

// ----------------------------------------------------------------------------
/*virtual*/ void FeedsView::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Up)         emit pressKeyUp();
  else if (event->key() == Qt::Key_Down)  emit pressKeyDown();
  else if (event->key() == Qt::Key_Home)  emit pressKeyHome();
  else if (event->key() == Qt::Key_End)   emit pressKeyEnd();
}

// ----------------------------------------------------------------------------
/*virtual*/ void FeedsView::currentChanged(const QModelIndex &current,
                                           const QModelIndex &previous)
{
  if (selectIdEn_) {
    QModelIndex index = ((FeedsProxyModel*)model())->mapToSource(current);
    selectId_ = sourceModel_->idByIndex(index);
  }
  selectIdEn_ = true;

  QTreeView::currentChanged(current, previous);
}

// ----------------------------------------------------------------------------
void FeedsView::dragEnterEvent(QDragEnterEvent *event)
{
  event->accept();
  dragPos_ = event->pos();
  viewport()->update();
}

// ----------------------------------------------------------------------------
void FeedsView::dragLeaveEvent(QDragLeaveEvent *event)
{
  event->accept();
  dragPos_ = QPoint();
  viewport()->update();
}

// ----------------------------------------------------------------------------
void FeedsView::dragMoveEvent(QDragMoveEvent *event)
{
  if (dragPos_.isNull()) {
    event->ignore();
    viewport()->update();
    return;
  }

  dragPos_ = event->pos();
  QModelIndex dragIndex = indexAt(dragPos_);

  // Process categories
  if (isFolder(dragIndex)) {
    if (dragIndex == currentIndex().parent())
      event->ignore();  // drag-to-category is parent already dragged one
    else if ((dragIndex.row() == currentIndex().row()) &&
             (dragIndex.parent() == currentIndex().parent()))
      event->ignore();  // don't move category to itself
    else {
      bool ignore = false;
      QModelIndex child = dragIndex.parent();
      while (child.isValid()) {
        if ((child.row() == currentIndex().row()) &&
            (child.parent() == currentIndex().parent())) {
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
    if ((dragIndex.row() == currentIndex().row()) &&
        (dragIndex.parent() == currentIndex().parent()))
      event->ignore();  // don't move feed to itseelf
    else if (dragIndex.parent() == currentIndex())
      event->ignore();  // don't move feed to same parent
    else {
      bool ignore = false;
      QModelIndex child = dragIndex.parent();
      while (child.isValid()) {
        if ((child.row() == currentIndex().row()) &&
            (child.parent() == currentIndex().parent())) {
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
bool FeedsView::shouldAutoScroll(const QPoint &pos) const
{
    if (!hasAutoScroll())
        return false;
    QRect area = viewport()->rect();
    return (pos.y() - area.top() < autoScrollMargin())
        || (area.bottom() - pos.y() < autoScrollMargin())
        || (pos.x() - area.left() < autoScrollMargin())
        || (area.right() - pos.x() < autoScrollMargin());
}

// ----------------------------------------------------------------------------
void FeedsView::dropEvent(QDropEvent *event)
{
  dragPos_ = QPoint();
  viewport()->update();

  event->setDropAction(Qt::MoveAction);
  event->accept();
  handleDrop(event);
}

// ----------------------------------------------------------------------------
void FeedsView::paintEvent(QPaintEvent *event)
{
  QTreeView::paintEvent(event);

  if (dragPos_.isNull()) return;

  QModelIndex dragIndex = indexAt(dragPos_);

  // Process folders
  if (isFolder(dragIndex)) {
    if ((dragIndex == currentIndex().parent()) ||
        ((dragIndex.row() == currentIndex().row()) &&
         (dragIndex.parent() == currentIndex().parent())))
      return;

    QModelIndex child = dragIndex.parent();
    while (child.isValid()) {
      if ((child.row() == currentIndex().row()) &&
          (child.parent() == currentIndex().parent()))
        return;
      child = child.parent();
    }
  }
  // Process feeds
  else {
    if ((dragIndex.parent() == currentIndex()) ||
        ((dragIndex.row() == currentIndex().row()) &&
         (dragIndex.parent() == currentIndex().parent())))
      return;
    QModelIndex child = dragIndex.parent();
    while (child.isValid()) {
      if ((child.row() == currentIndex().row()) &&
          (child.parent() == currentIndex().parent()))
        return;
      child = child.parent();
    }
  }

  QModelIndex indexText = model()->index(dragIndex.row(),
                                         columnIndex("text"),
                                         dragIndex.parent());

  QRect rectText = visualRect(indexText);
  rectText.setRight(viewport()->width()-2);
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
    if (!isFolder(dragIndex))
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
QPersistentModelIndex FeedsView::selectIndex()
{
  return sourceModel_->indexById(selectId_);
}

/** @brief Update cursor without list scrolling
 *---------------------------------------------------------------------------*/
void FeedsView::updateCurrentIndex(const QModelIndex &index)
{
  setUpdatesEnabled(false);
  int topRow = verticalScrollBar()->value();
  setCurrentIndex(index);
  verticalScrollBar()->setValue(topRow);
  setUpdatesEnabled(true);
}

// ----------------------------------------------------------------------------
void FeedsView::handleDrop(QDropEvent *e)
{
  QModelIndex dropIndex = indexAt(e->pos());

  QModelIndex indexWhere = dropIndex;

  int how = 0;
  QRect rectText = visualRect(dropIndex);
  if (qAbs(rectText.top() - e->pos().y()) < 3) {
    how = 0;
  } else if (qAbs(rectText.bottom() - e->pos().y()) < 3) {
    how = 1;
  } else {
    if (isFolder(dropIndex)) {
      how = 2;
    } else {
      dropIndex = model()->index(dropIndex.row()+1,
                                  columnIndex("text"),
                                  dropIndex.parent());
      if (!dropIndex.isValid()) how = 1;
    }
  }
  indexWhere = ((FeedsProxyModel*)model())->mapToSource(indexWhere);
  emit signalDropped(indexWhere, how);
}
