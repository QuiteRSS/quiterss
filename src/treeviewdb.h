#ifndef TREEVIEWDB_H
#define TREEVIEWDB_H

#include <QtGui>

class TreeViewDB : public QTreeView
{
  Q_OBJECT
public:
  TreeViewDB(QWidget *parent = NULL);

  void dragEnterEvent(QDragEnterEvent *e);
  void dragMoveEvent(QDragMoveEvent *e);
  void dropEvent(QDropEvent *e);
  void handleDrop(QDropEvent *e);

};

#endif // TREEVIEWDB_H
