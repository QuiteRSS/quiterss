/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#ifndef FILTERRULES_H
#define FILTERRULES_H

#include "dialog.h"
#include "lineedit.h"

class ItemCondition : public QWidget
{
  Q_OBJECT
public:
  ItemCondition(QWidget * parent = 0)
    : QWidget(parent)
  {
    comboBox1_ = new QComboBox(this);
    comboBox2_ = new QComboBox(this);
    comboBox3_ = new QComboBox(this);
    lineEdit_ = new LineEdit(this);

    QStringList itemList;
    itemList << tr("Title")  << tr("Description")
             << tr("Author") << tr("Category") << tr("State")
                /*<< tr("Published") << tr("Received")*/;
    comboBox1_->addItems(itemList);

    itemList.clear();
    itemList << tr("New") << tr("Read") << tr("Starred");
    comboBox3_->addItems(itemList);

    currentIndexChanged(tr("Title"));

    addButton_ = new QToolButton(this);
    addButton_->setIcon(QIcon(":/images/addT"));
    addButton_->setToolTip(tr("Add Condition"));
    addButton_->setAutoRaise(true);
    deleteButton_ = new QToolButton(this);
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

  QComboBox *comboBox1_;
  QComboBox *comboBox2_;
  QComboBox *comboBox3_;
  LineEdit *lineEdit_;

  QToolButton *addButton_;

signals:
  void signalDeleteCondition(ItemCondition *item);

private slots:
  void deleteFilterRules()
  {
    emit signalDeleteCondition(this);
  }

  void currentIndexChanged(const QString &str)
  {
    QStringList itemList;

    comboBox2_->clear();
    comboBox3_->setVisible(false);
    lineEdit_->setVisible(true);
    if (str == tr("Title")) {
      itemList << tr("contains") << tr("doesn't contains")
               << tr("is") << tr("isn't")
               << tr("begins with") << tr("ends with");
      comboBox2_->addItems(itemList);
    } else if (str == tr("Description")) {
      itemList << tr("contains") << tr("doesn't contains");
      comboBox2_->addItems(itemList);
    } else if (str == tr("Author")) {
      itemList << tr("contains") << tr("doesn't contains")
               << tr("is") << tr("isn't");
      comboBox2_->addItems(itemList);
    } else if (str == tr("Category")) {
      itemList << tr("contains") << tr("doesn't contains")
               << tr("is") << tr("isn't")
               << tr("begins with") << tr("ends with");
      comboBox2_->addItems(itemList);
    } else if (str == tr("Status")) {
      itemList << tr("is") << tr("isn't");
      comboBox2_->addItems(itemList);
      comboBox3_->setVisible(true);
      lineEdit_->setVisible(false);
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

private:
  QToolButton *deleteButton_;

};

class ItemAction : public QWidget
{
  Q_OBJECT
public:
  ItemAction(QWidget * parent = 0)
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
    selectionSound_ = new QToolButton(this);
    selectionSound_->setText("...");
    selectionSound_->setToolTip(tr("Browse"));
    selectionSound_->setAutoRaise(true);
    playSound_ = new QToolButton(this);
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

    colorButton_ = new QToolButton(this);
    colorButton_->setIconSize(QSize(16, 16));
    colorButton_->setToolTip("#000000");
    QPixmap pixmap(14, 14);
    pixmap.fill(QColor("#000000"));
    colorButton_->setIcon(pixmap);
    colorButton_->setVisible(false);

    addButton_ = new QToolButton(this);
    addButton_->setIcon(QIcon(":/images/addT"));
    addButton_->setToolTip(tr("Add Action"));
    addButton_->setAutoRaise(true);
    deleteButton_ = new QToolButton(this);
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

  QComboBox *comboBox1_;
  QComboBox *comboBox2_;
  QWidget *soundWidget_;
  QLineEdit *soundPathEdit_;
  QToolButton *selectionSound_;
  QToolButton *playSound_;
  QToolButton *colorButton_;

  QToolButton *addButton_;

signals:
  void signalDeleteAction(ItemAction *item);
  void signalPlaySound(const QString &soundPath);

private slots:
  void deleteFilterAction()
  {
    emit signalDeleteAction(this);
  }

  void currentIndexChanged(int index)
  {
    if (index == 3) comboBox2_->setVisible(true);
    else comboBox2_->setVisible(false);
    if (index == 4) soundWidget_->setVisible(true);
    else soundWidget_->setVisible(false);
    if (index == 5) colorButton_->setVisible(true);
    else colorButton_->setVisible(false);
  }

  void selectionSound()
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

  void slotPlaySound()
  {
    if (!soundPathEdit_->text().isEmpty())
      emit signalPlaySound(soundPathEdit_->text());
  }

  void selectColorText()
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

private:
  QToolButton *deleteButton_;

};

class FilterRulesDialog : public Dialog
{
  Q_OBJECT
public:
  explicit FilterRulesDialog(QWidget *parent, int filterId, int feedId = -1);

  int filterId_;
  bool itemNotChecked_;

signals:
  void signalPlaySound(const QString &soundPath);

private slots:
  void feedItemChanged(QTreeWidgetItem *item, int column);
  void setCheckStateItem(QTreeWidgetItem *item, Qt::CheckState state);
  void closeDialog();
  void acceptDialog();

  void filterNameChanged(const QString &text);

  ItemCondition *addCondition();
  void deleteCondition(ItemCondition *item);

  ItemAction *addAction();
  void deleteAction(ItemAction *item);

  void selectMatch(int index);

private:
  void setData();

  QSettings *settings_;
  LineEdit *filterName_;
  QTreeWidget *feedsTree_;

  QComboBox *matchComboBox_;

  QScrollArea *conditionScrollArea_;
  QVBoxLayout *conditionLayout_;
  QWidget *conditionWidget_;

  QScrollArea *actionsScrollArea_;
  QVBoxLayout *actionsLayout_;
  QWidget *actionsWidget_;

  QLabel *textWarning_;
  QWidget *warningWidget_;

};

#endif // FILTERRULES_H
