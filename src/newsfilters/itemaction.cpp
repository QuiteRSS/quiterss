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
#include "itemaction.h"

ItemAction::ItemAction(QWidget * parent)
  : QWidget(parent)
{
  QStringList itemList;
  comboBox1_ = new QComboBox(this);
  itemList /*<< tr("Move News to")  << tr("Copy News to")*/
      << tr("Mark News as Read") << tr("Add Star")
      << tr("Delete") << tr("Add Label")
      << tr("Play a Sound") << tr("Show News in Notifier");
  comboBox1_->addItems(itemList);

  comboBox2_ = new QComboBox(this);
  comboBox2_->setVisible(false);

  soundPathEdit_ = new QLineEdit(this);
  selectionSound_ = new ToolButton(this);
  selectionSound_->setText("...");
  selectionSound_->setToolTip(tr("Browse"));
  selectionSound_->setAutoRaise(true);
  playSound_ = new ToolButton(this);
  playSound_->setIcon(QIcon(":/images/play"));
  playSound_->setToolTip(tr("Play"));
  playSound_->setAutoRaise(true);
  QHBoxLayout *soundLayout = new QHBoxLayout();
  soundLayout->setMargin(0);
  soundLayout->setSpacing(1);
  soundLayout->addWidget(soundPathEdit_, 1);
  soundLayout->addWidget(selectionSound_);
  soundLayout->addWidget(playSound_);
  soundWidget_ = new QWidget(this);
  soundWidget_->setLayout(soundLayout);
  soundWidget_->setVisible(false);

  colorButton_ = new ToolButton(this);
  colorButton_->setIconSize(QSize(16, 16));
  colorButton_->setToolTip("#000000");
  QPixmap pixmap(14, 14);
  pixmap.fill(QColor("#000000"));
  colorButton_->setIcon(pixmap);
  colorButton_->setVisible(false);

  addButton_ = new ToolButton(this);
  addButton_->setIcon(QIcon(":/images/addT"));
  addButton_->setToolTip(tr("Add Action"));
  addButton_->setAutoRaise(true);
  deleteButton_ = new ToolButton(this);
  deleteButton_->setIcon(QIcon(":/images/deleteT"));
  deleteButton_->setToolTip(tr("Delete Action"));
  deleteButton_->setAutoRaise(true);

  QHBoxLayout *buttonsLayout = new QHBoxLayout();
  buttonsLayout->setAlignment(Qt::AlignCenter);
  buttonsLayout->setMargin(0);
  buttonsLayout->setSpacing(5);
  buttonsLayout->addWidget(comboBox1_);
  buttonsLayout->addWidget(comboBox2_);
  buttonsLayout->addWidget(soundWidget_, 1);
  buttonsLayout->addWidget(colorButton_);
  buttonsLayout->addStretch();
  buttonsLayout->addWidget(addButton_);
  buttonsLayout->addWidget(deleteButton_);

  setLayout(buttonsLayout);

  connect(playSound_, SIGNAL(clicked()), this, SLOT(slotPlaySound()));
  connect(deleteButton_, SIGNAL(clicked()), this, SLOT(deleteFilterAction()));
  connect(comboBox1_, SIGNAL(currentIndexChanged(int)),
          this, SLOT(currentIndexChanged(int)));
  connect(selectionSound_, SIGNAL(clicked()), this, SLOT(selectionSound()));
  connect(colorButton_, SIGNAL(clicked()), this, SLOT(selectColorText()));
}

void ItemAction::deleteFilterAction()
{
  emit signalDeleteAction(this);
}

void ItemAction::currentIndexChanged(int index)
{
  if (index == 3) comboBox2_->setVisible(true);
  else comboBox2_->setVisible(false);
  if (index == 4) soundWidget_->setVisible(true);
  else soundWidget_->setVisible(false);
  if (index == 5) colorButton_->setVisible(true);
  else colorButton_->setVisible(false);
}

void ItemAction::selectionSound()
{
  QString path;

  QFileInfo file(soundPathEdit_->text());
  if (file.isFile()) path = soundPathEdit_->text();
  else path = file.path();

  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Open File..."),
                                                  path, "*.wav");
  if (!fileName.isEmpty())
    soundPathEdit_->setText(fileName);
}

void ItemAction::slotPlaySound()
{
  if (!soundPathEdit_->text().isEmpty())
    emit signalPlaySound(soundPathEdit_->text());
}

void ItemAction::selectColorText()
{
  QColorDialog *colorDialog = new QColorDialog(this);

  if (colorDialog->exec() == QDialog::Rejected) {
    delete colorDialog;
    return;
  }

  QColor color = colorDialog->selectedColor();
  delete colorDialog;

  colorButton_->setToolTip(color.name());
  QPixmap pixmap(14, 14);
  pixmap.fill(color);
  colorButton_->setIcon(pixmap);
}
