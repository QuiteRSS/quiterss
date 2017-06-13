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
#include "itemcondition.h"

ItemCondition::ItemCondition(QWidget * parent)
  : QWidget(parent)
{
  comboBox1_ = new QComboBox(this);
  comboBox2_ = new QComboBox(this);
  comboBox3_ = new QComboBox(this);
  lineEdit_ = new LineEdit(this);

  QStringList itemList;
  itemList << tr("Title")  << tr("Description")
           << tr("Author") << tr("Category") << tr("State")
           << tr("Link") << tr("News")
              /*<< tr("Published") << tr("Received")*/;
  comboBox1_->addItems(itemList);

  itemList.clear();
  itemList << tr("New") << tr("Read") << tr("Starred");
  comboBox3_->addItems(itemList);

  currentIndexChanged(tr("Title"));

  addButton_ = new ToolButton(this);
  addButton_->setIcon(QIcon(":/images/addT"));
  addButton_->setToolTip(tr("Add Condition"));
  addButton_->setAutoRaise(true);
  deleteButton_ = new ToolButton(this);
  deleteButton_->setIcon(QIcon(":/images/deleteT"));
  deleteButton_->setToolTip(tr("Delete Condition"));
  deleteButton_->setAutoRaise(true);

  QHBoxLayout *buttonsLayout = new QHBoxLayout();
  buttonsLayout->setAlignment(Qt::AlignCenter);
  buttonsLayout->setMargin(0);
  buttonsLayout->setSpacing(5);
  buttonsLayout->addWidget(comboBox1_);
  buttonsLayout->addWidget(comboBox2_);
  buttonsLayout->addWidget(comboBox3_, 1);
  buttonsLayout->addWidget(lineEdit_, 1);
  buttonsLayout->addWidget(addButton_);
  buttonsLayout->addWidget(deleteButton_);

  setLayout(buttonsLayout);

  connect(deleteButton_, SIGNAL(clicked()), this, SLOT(deleteFilterRules()));
  connect(comboBox1_, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(currentIndexChanged(QString)));
}

void ItemCondition::deleteFilterRules()
{
  emit signalDeleteCondition(this);
}

void ItemCondition::currentIndexChanged(const QString &str)
{
  QStringList itemList;

  comboBox2_->clear();
  comboBox3_->setVisible(false);
  lineEdit_->setVisible(true);
  if (str == tr("Title")) {
    itemList << tr("contains") << tr("doesn't contains")
             << tr("is") << tr("isn't")
             << tr("begins with") << tr("ends with")
             << tr("Regular expressions");
    comboBox2_->addItems(itemList);
  } else if (str == tr("Description")) {
    itemList << tr("contains") << tr("doesn't contains")
             << tr("Regular expressions");
    comboBox2_->addItems(itemList);
  } else if (str == tr("Author")) {
    itemList << tr("contains") << tr("doesn't contains")
             << tr("is") << tr("isn't")
             << tr("Regular expressions");
    comboBox2_->addItems(itemList);
  } else if (str == tr("Category")) {
    itemList << tr("contains") << tr("doesn't contains")
             << tr("is") << tr("isn't")
             << tr("begins with") << tr("ends with")
             << tr("Regular expressions");
    comboBox2_->addItems(itemList);
  } else if (str == tr("State")) {
    itemList << tr("is") << tr("isn't");
    comboBox2_->addItems(itemList);
    comboBox3_->setVisible(true);
    lineEdit_->setVisible(false);
  } else if (str == tr("Link")) {
    itemList << tr("contains") << tr("doesn't contains")
             << tr("is") << tr("isn't")
             << tr("begins with") << tr("ends with")
             << tr("Regular expressions");
    comboBox2_->addItems(itemList);
  } else if (str == tr("News")) {
    itemList << tr("contains") << tr("doesn't contains")
             << tr("Regular expressions");
    comboBox2_->addItems(itemList);
  } /*else if (str == "Published") {
    itemList << tr("is") << tr("isn't")
             << tr("is before") << tr("is after");
    comboBox2->addItems(itemList);
  } else if (str == "Received") {
    itemList << tr("is") << tr("isn't")
             << tr("is before") << tr("is after");
    comboBox2->addItems(itemList);
  }*/
}
