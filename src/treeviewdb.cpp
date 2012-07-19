#include "treeviewdb.h"

TreeViewDB::TreeViewDB(QWidget *parent) :
  QTreeView(parent)
{
}

void TreeViewDB::setSelectIndex()
{
  selectIndex = currentIndex();
}

void TreeViewDB::updateCurrentIndex(const QModelIndex &index)
{
  int topRow = verticalScrollBar()->value();
  setCurrentIndex(index);
  verticalScrollBar()->setValue(topRow);
}

/*virtual*/ void TreeViewDB::mousePressEvent(QMouseEvent *event)
{
  if (!indexAt(event->pos()).isValid()) return;

  selectIndex = indexAt(event->pos());
  if ((event->buttons() & Qt::MiddleButton)) {
    if (selectIndex.isValid())
      emit signalMiddleClicked();
  } else if (event->buttons() & Qt::RightButton) {

  } else QTreeView::mousePressEvent(event);
}

/*virtual*/ void TreeViewDB::mouseMoveEvent(QMouseEvent *event)
{
  Q_UNUSED(event)
}

/*virtual*/ void TreeViewDB::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (!indexAt(event->pos()).isValid()) return;

  emit signalDoubleClicked(indexAt(event->pos()));
}

/*virtual*/ void TreeViewDB::keyPressEvent(QKeyEvent *event)
{
  if (!event->modifiers()) {
    if (event->key() == Qt::Key_Up)         emit pressKeyUp();
    else if (event->key() == Qt::Key_Down)  emit pressKeyDown();
    else if (event->key() == Qt::Key_Home)  emit pressKeyHome();
    else if (event->key() == Qt::Key_End)   emit pressKeyEnd();
  }
}

/*virtual*/ void TreeViewDB::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
  selectIndex = current;
  QTreeView::currentChanged(current, previous);
}

void TreeViewDB::handleDrop(QDropEvent *e)
{
  QModelIndex indexWhat = currentIndex();
  QModelIndex indexWhere = indexAt(e->pos());
  emit signalDropped(indexWhat, indexWhere);
}

/*virtual*/ void TreeViewDB::dropEvent(QDropEvent *e)
{
  handleDrop(e);
}

/*virtual*/ void TreeViewDB::dragEnterEvent(QDragEnterEvent *e)
{
  QTreeView::dragEnterEvent(e);
}

/*virtual*/ void TreeViewDB::dragMoveEvent(QDragMoveEvent *e)
{
  QTreeView::dragMoveEvent(e);
}

