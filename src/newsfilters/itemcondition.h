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
#ifndef ITEMCONDITION_H
#define ITEMCONDITION_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include "lineedit.h"
#include "toolbutton.h"

class ItemCondition : public QWidget
{
  Q_OBJECT
public:
  explicit ItemCondition(QWidget * parent = 0);

  QComboBox *comboBox1_;
  QComboBox *comboBox2_;
  QComboBox *comboBox3_;
  LineEdit *lineEdit_;

  ToolButton *addButton_;

signals:
  void signalDeleteCondition(ItemCondition *item);

private slots:
  void deleteFilterRules();
  void currentIndexChanged(const QString &str);

private:
  ToolButton *deleteButton_;

};

#endif // ITEMCONDITION_H
