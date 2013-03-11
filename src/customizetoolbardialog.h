#ifndef CUSTOMIZETOOLBARDIALOG_H
#define CUSTOMIZETOOLBARDIALOG_H

#include "dialog.h"

class CustomizeToolbarDialog : public Dialog
{
  Q_OBJECT
public:
  explicit CustomizeToolbarDialog(QWidget *parent, QToolBar *toolbar);

private slots:
  void acceptDialog();
  void closeDialog();

  void showMenuAddButton();
  void addShortcut(QAction *action);
  void removeShortcut();
  void moveUpShortcut();
  void moveDownShortcut();
  void defaultShortcut();

  void slotCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *);

private:
  QToolBar *toolbar_;
  QSettings *settings_;
  QTreeWidget *shortcutTree_;

  QComboBox *styleBox_;
  QComboBox *iconBox_;

  QMenu *addButtonMenu_;

  QPushButton *addButton_;
  QPushButton *removeButton_;
  QPushButton *moveUpButton_;
  QPushButton *moveDownButton_;

};

#endif // CUSTOMIZETOOLBARDIALOG_H
