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
#include "dialog.h"

Dialog::Dialog(QWidget *parent, Qt::WindowFlags flag)
  : QDialog(parent, flag)
{
  pageLayout = new QVBoxLayout();
  pageLayout->setContentsMargins(10, 10, 10, 5);
  pageLayout->setSpacing(5);

  buttonBox = new QDialogButtonBox();

  buttonsLayout = new QHBoxLayout();
  buttonsLayout->setMargin(10);
  buttonsLayout->addWidget(buttonBox);

  QFrame *line = new QFrame();
  line->setFrameStyle(QFrame::HLine | QFrame::Sunken);

  mainLayout = new QVBoxLayout();
  mainLayout->setMargin(0);
  mainLayout->setSpacing(0);
  mainLayout->addLayout(pageLayout, 1);
  mainLayout->addWidget(line);
  mainLayout->addLayout(buttonsLayout);
  setLayout(mainLayout);

  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}
