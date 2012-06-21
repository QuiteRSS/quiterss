#include "treeviewdb.h"

TreeViewDB::TreeViewDB(QWidget *parent) :
  QTreeView(parent)
{
}

void TreeViewDB::handleDrop(QDropEvent *e)
{
  QModelIndex indexWhat = currentIndex();
  QModelIndex indexWhere = indexAt(e->pos());
  emit signalDropped(indexWhat, indexWhere);
}

void TreeViewDB::dropEvent(QDropEvent *e)
{
  handleDrop(e);
}

void TreeViewDB::dragEnterEvent(QDragEnterEvent *e)
{
  QTreeView::dragEnterEvent(e);
}

void TreeViewDB::dragMoveEvent(QDragMoveEvent *e)
{
  QTreeView::dragMoveEvent(e);
  if (currentIndex().parent() == indexAt(e->pos())) e->ignore();
}
