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
#ifndef ADDFOLDERDIALOG_H
#define ADDFOLDERDIALOG_H

#include <QtSql>

#include "dialog.h"
#include "lineedit.h"

class AddFolderDialog : public Dialog
{
  Q_OBJECT

public:
  explicit AddFolderDialog(QWidget *parent, int curFolderId = 0);

  LineEdit *nameFeedEdit_;
  QTreeWidget *foldersTree_;

private slots:
  void nameFeedEditChanged(const QString&);

};

#endif // ADDFOLDERDIALOG_H
