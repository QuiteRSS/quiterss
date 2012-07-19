#ifndef TREEVIEWDB_H
#define TREEVIEWDB_H

#include <QtGui>

class TreeViewDB : public QTreeView
{
  Q_OBJECT
public:
  QModelIndex selectIndex;

  TreeViewDB(QWidget *parent = NULL);

public slots:
  void setSelectIndex();
  void updateCurrentIndex(const QModelIndex &index);

protected:
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mouseDoubleClickEvent(QMouseEvent *event);
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void currentChanged(const QModelIndex &current,
                              const QModelIndex &previous);
  virtual void dragEnterEvent(QDragEnterEvent *e);
  virtual void dragMoveEvent(QDragMoveEvent *e);
  virtual void dropEvent(QDropEvent *e);

private:
  void handleDrop(QDropEvent *e);

signals:
  void signalDropped(QModelIndex what, QModelIndex where);
  void signalDoubleClicked(QModelIndex index);
  void signalMiddleClicked();
  void pressKeyUp();
  void pressKeyDown();
  void pressKeyHome();
  void pressKeyEnd();

};

#endif // TREEVIEWDB_H
