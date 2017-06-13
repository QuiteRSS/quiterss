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
#ifndef DIALOG_H
#define DIALOG_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif

class Dialog : public QDialog
{
  Q_OBJECT
public:
  explicit Dialog(QWidget *parent, Qt::WindowFlags flag = 0);

  QVBoxLayout *mainLayout;
  QVBoxLayout *pageLayout;
  QHBoxLayout *buttonsLayout;

  QDialogButtonBox *buttonBox;

};

#endif // DIALOG_H
