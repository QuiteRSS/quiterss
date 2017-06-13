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
