#ifndef TREEEDITDIALOG_H
#define TREEEDITDIALOG_H

#include <QtGui>
#include "treemodeldb.h"
#include "treeviewdb.h"

class TreeEditDialog : public QDialog
{
  Q_OBJECT

  QAction *createFolder_;
  QAction *deleteNode_;
  QAction *moveUp_;
  QAction *moveDown_;
  QAction *moveRight_;
  QAction *moveLeft_;

  QToolBar *toolBar_;
  TreeViewDB *view_;

  QDialogButtonBox *buttonBox_;

  TreeModel *model_;
  QSqlDatabase *db_;

  void renewModel(void);

private slots:
  void slotUpdateActions(QModelIndex index);
  void slotCreateFolder();
  void slotDeleteNode();
  void slotMoveUp();
  void slotMoveDown();
  void slotMoveLeft();
  void slotMoveRight();
  void slotMoveIndex(QModelIndex indexWhat,QModelIndex indexWhere);

public:
  explicit TreeEditDialog(QWidget *parent = NULL, QSqlDatabase *db = NULL);
  
signals:
  
public slots:
  
};

#endif // TREEEDITDIALOG_H
