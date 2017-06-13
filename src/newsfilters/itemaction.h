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
#ifndef ITEMACTION_H
#define ITEMACTION_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include "lineedit.h"
#include "toolbutton.h"

class ItemAction : public QWidget
{
  Q_OBJECT
public:
  explicit ItemAction(QWidget * parent = 0);

  QComboBox *comboBox1_;
  QComboBox *comboBox2_;
  QWidget *soundWidget_;
  QLineEdit *soundPathEdit_;
  ToolButton *selectionSound_;
  ToolButton *playSound_;
  ToolButton *colorButton_;

  ToolButton *addButton_;

signals:
  void signalDeleteAction(ItemAction *item);
  void signalPlaySound(const QString &soundPath);

private slots:
  void deleteFilterAction();
  void currentIndexChanged(int index);
  void selectionSound();
  void slotPlaySound();
  void selectColorText();

private:
  ToolButton *deleteButton_;

};

#endif // ITEMACTION_H
