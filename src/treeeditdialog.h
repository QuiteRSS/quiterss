#ifndef TREEEDITDIALOG_H
#define TREEEDITDIALOG_H

#include <QtGui>
#include "treemodeldb.h"

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
  QTreeView *view_;

  QDialogButtonBox *buttonBox_;

  TreeModel *model_;
  QSqlDatabase *db_;

  void renewModel(void);

private slots:
  void slotUpdateActions(QModelIndex &index);
  void slotCreateFolder();
  void slotDeleteNode();
  void slotMoveUp();
  void slotMoveDown();
  void slotMoveLeft();
  void slotMoveRight();

public:
  explicit TreeEditDialog(QWidget *parent = NULL, QSqlDatabase *db = NULL);
  
signals:
  
public slots:
  
};

#endif // TREEEDITDIALOG_H
