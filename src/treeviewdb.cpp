#include "treeviewdb.h"

TreeViewDB::TreeViewDB(QWidget *parent) :
  QTreeView(parent)
{
}

void TreeViewDB::handleDrop(QDropEvent *e)
{
  qDebug() << __FUNCTION__;
  QModelIndex indexFrom = currentIndex();
  QModelIndex indexTo = indexAt(e->pos());
  qDebug() << "move" << indexFrom << "to" << indexTo;
}

void TreeViewDB::dropEvent(QDropEvent *e)
{
  qDebug() << __FUNCTION__ << e->pos() << indexAt(e->pos());
  handleDrop(e);
}

void TreeViewDB::dragEnterEvent(QDragEnterEvent *e)
{
  QTreeView::dragEnterEvent(e);
}

void TreeViewDB::dragMoveEvent(QDragMoveEvent *e)
{
  QTreeView::dragMoveEvent(e);
}
