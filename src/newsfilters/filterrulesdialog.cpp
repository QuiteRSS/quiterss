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
#include "filterrulesdialog.h"

#include "mainapplication.h"
#include "settings.h"

FilterRulesDialog::FilterRulesDialog(QWidget *parent, int filterId, int feedId)
  : Dialog(parent, Qt::WindowMinMaxButtonsHint)
  , filterId_(filterId)
{
  setWindowTitle(tr("Filter Rules"));
  setMinimumHeight(300);

  feedsTree_ = new QTreeWidget(this);
  feedsTree_->setObjectName("feedsTreeFR");
  feedsTree_->setColumnCount(2);
  feedsTree_->setColumnHidden(1, true);
#ifdef HAVE_QT5
  feedsTree_->header()->setSectionsMovable(false);
#else
  feedsTree_->header()->setMovable(false);
#endif

  itemNotChecked_ = false;

  QStringList treeItem;
  treeItem << tr("Feeds") << "Id";
  feedsTree_->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << tr("All Feeds") << "0";
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setCheckState(0, Qt::Unchecked);
  feedsTree_->addTopLevelItem(treeWidgetItem);

  QSqlQuery q;
  QQueue<int> parentIds;
  parentIds.enqueue(0);
  while (!parentIds.empty()) {
    int parentId = parentIds.dequeue();
    QString qStr = QString("SELECT text, id, image, xmlUrl FROM feeds WHERE parentId='%1' ORDER BY rowToParent").
        arg(parentId);
    q.exec(qStr);
    while (q.next()) {
      QString feedText = q.value(0).toString();
      QString feedIdT = q.value(1).toString();
      QByteArray byteArray = q.value(2).toByteArray();
      QString xmlUrl = q.value(3).toString();

      treeItem.clear();
      treeItem << feedText << feedIdT;
      treeWidgetItem = new QTreeWidgetItem(treeItem);

      if ((feedId == feedIdT.toInt()) || (feedId == parentId))
        treeWidgetItem->setCheckState(0, Qt::Checked);
      else
        treeWidgetItem->setCheckState(0, Qt::Unchecked);

      QPixmap iconItem;
      if (xmlUrl.isEmpty()) {
        iconItem.load(":/images/folder");
      } else {
        if (byteArray.isNull() || mainApp->mainWindow()->defaultIconFeeds_) {
          iconItem.load(":/images/feed");
        } else {
          iconItem.loadFromData(QByteArray::fromBase64(byteArray));
        }
      }
      treeWidgetItem->setIcon(0, iconItem);

      QList<QTreeWidgetItem *> treeItems =
          feedsTree_->findItems(QString::number(parentId),
                                Qt::MatchFixedString | Qt::MatchRecursive,
                                1);
      treeItems.at(0)->addChild(treeWidgetItem);
      if (xmlUrl.isEmpty())
        parentIds.enqueue(feedIdT.toInt());
    }
  }
  feedsTree_->expandAll();

  if (feedId != -1) {
    int rowCount = 0;
    QTreeWidgetItem *childItem = feedsTree_->topLevelItem(0);
    while (childItem) {
      if (childItem->text(1).toInt() == feedId) break;
      rowCount++;
      childItem = feedsTree_->itemBelow(childItem);
    }
    feedsTree_->verticalScrollBar()->setValue(rowCount);
    feedsTree_->topLevelItem(0)->setCheckState(0, Qt::Unchecked);
  }
  connect(feedsTree_, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
          this, SLOT(feedItemChanged(QTreeWidgetItem*,int)));

  filterName_ = new LineEdit(this);
  QHBoxLayout *filterNamelayout = new QHBoxLayout();
  filterNamelayout->addWidget(new QLabel(tr("Filter name:")));
  filterNamelayout->addWidget(filterName_);

  matchComboBox_ = new QComboBox(this);

  QStringList itemList;
  itemList << tr("Match all news") << tr("Match all conditions")
           << tr("Match any condition");
  matchComboBox_->addItems(itemList);
  matchComboBox_->setCurrentIndex(1);
  connect(matchComboBox_, SIGNAL(currentIndexChanged(int)),
          this, SLOT(selectMatch(int)));

  QHBoxLayout *matchLayout = new QHBoxLayout();
  matchLayout->addWidget(matchComboBox_);
  matchLayout->addStretch();

  conditionScrollArea_ = new QScrollArea(this);
  conditionScrollArea_->setFocusPolicy(Qt::NoFocus);
  conditionScrollArea_->setWidgetResizable(true);

  conditionLayout_ = new QVBoxLayout();
  conditionLayout_->setMargin(1);
  conditionLayout_->setSpacing(1);
  if (filterId_ == -1)
    addCondition();

  conditionWidget_ = new QWidget(this);
  conditionWidget_->setObjectName("infoWidgetFR");
  conditionWidget_->setLayout(conditionLayout_);
  conditionScrollArea_->setWidget(conditionWidget_);

  QVBoxLayout *splitterLayoutV1 = new QVBoxLayout();
  splitterLayoutV1->setMargin(0);
  splitterLayoutV1->addLayout(matchLayout);
  splitterLayoutV1->addWidget(conditionScrollArea_, 1);

  QWidget *splitterWidget1 = new QWidget(this);
  splitterWidget1->setMinimumWidth(400);
  splitterWidget1->setLayout(splitterLayoutV1);

  actionsScrollArea_ = new QScrollArea(this);
  actionsScrollArea_->setWidgetResizable(true);
  actionsScrollArea_->setFocusPolicy(Qt::NoFocus);

  actionsLayout_ = new QVBoxLayout();
  actionsLayout_->setMargin(1);
  actionsLayout_->setSpacing(1);
  if (filterId_ == -1)
    addAction();

  actionsWidget_ = new QWidget(this);
  actionsWidget_->setObjectName("actionsWidgetFR");
  actionsWidget_->setLayout(actionsLayout_);
  actionsScrollArea_->setWidget(actionsWidget_);

  QVBoxLayout *splitterLayoutV2 = new QVBoxLayout();
  splitterLayoutV2->setMargin(0);
  splitterLayoutV2->addWidget(new QLabel(tr("Perform these actions:")));
  splitterLayoutV2->addWidget(actionsScrollArea_, 1);

  QWidget *splitterWidget2 = new QWidget(this);
  splitterWidget2->setLayout(splitterLayoutV2);

  QSplitter *spliter = new QSplitter(Qt::Vertical, this);
  spliter->setChildrenCollapsible(false);
  spliter->addWidget(splitterWidget1);
  spliter->addWidget(splitterWidget2);

  QVBoxLayout *rulesLayout = new QVBoxLayout();
  rulesLayout->setMargin(0);
  rulesLayout->addLayout(filterNamelayout);
  rulesLayout->addWidget(spliter);

  QWidget *rulesWidget = new QWidget(this);
  rulesWidget->setLayout(rulesLayout);

  QSplitter *mainSpliter = new QSplitter(this);
  mainSpliter->setChildrenCollapsible(false);
  mainSpliter->addWidget(rulesWidget);
  mainSpliter->addWidget(feedsTree_);

  QLabel *iconWarning = new QLabel(this);
  iconWarning->setPixmap(QPixmap(":/images/warning"));
  textWarning_ = new QLabel(this);
  QFont font = textWarning_->font();
  font.setBold(true);
  textWarning_->setFont(font);

  QHBoxLayout *warningLayout = new QHBoxLayout();
  warningLayout->setMargin(0);
  warningLayout->addWidget(iconWarning);
  warningLayout->addWidget(textWarning_, 1);

  warningWidget_ = new QWidget(this);
  warningWidget_->setLayout(warningLayout);
  warningWidget_->setVisible(false);

  buttonsLayout->insertWidget(0, warningWidget_, 1);
  buttonBox->addButton(QDialogButtonBox::Ok);
  buttonBox->addButton(QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptDialog()));

  pageLayout->addWidget(mainSpliter);

  setData();

  filterName_->setFocus();
  filterName_->selectAll();

  Settings settings;
  restoreGeometry(settings.value("filterRulesDlg/geometry").toByteArray());

  connect(filterName_, SIGNAL(textChanged(QString)),
          this, SLOT(filterNameChanged(QString)));
  connect(this, SIGNAL(finished(int)), this, SLOT(closeDialog()));
}

void FilterRulesDialog::setData()
{
  if (filterId_ == -1) return;

  QSqlQuery q;
  QString qStr = QString("SELECT name, type, feeds FROM filters WHERE id=='%1'").
      arg(filterId_);
  q.exec(qStr);
  if (q.next()) {
    filterName_->setText(q.value(0).toString());

    matchComboBox_->setCurrentIndex(q.value(1).toInt());

    itemNotChecked_ = true;
    QStringList strIdFeeds = q.value(2).toString().split(",", QString::SkipEmptyParts);
    foreach (QString strIdFeed, strIdFeeds) {
      QList<QTreeWidgetItem *> treeItems =
          feedsTree_->findItems(strIdFeed,
                                Qt::MatchFixedString | Qt::MatchRecursive,
                                1);
      if (treeItems.count())
        treeItems.at(0)->setCheckState(0, Qt::Checked);
    }
    QTreeWidgetItem *treeItem = feedsTree_->itemBelow(feedsTree_->topLevelItem(0));
    while (treeItem) {
      if (treeItem->checkState(0) == Qt::Unchecked) {
        feedsTree_->topLevelItem(0)->setCheckState(0, Qt::Unchecked);
        break;
      }
      treeItem = feedsTree_->itemBelow(treeItem);
    }
    itemNotChecked_ = false;

    qStr = QString("SELECT field, condition, content "
                   "FROM filterConditions WHERE idFilter=='%1' ORDER BY content").
        arg(filterId_);
    q.exec(qStr);
    while (q.next()) {
      ItemCondition *itemCondition = addCondition();
      itemCondition->comboBox1_->setCurrentIndex(q.value(0).toInt());
      itemCondition->comboBox2_->setCurrentIndex(q.value(1).toInt());
      if (q.value(0).toInt() == 4)
        itemCondition->comboBox3_->setCurrentIndex(q.value(2).toInt());
      else
        itemCondition->lineEdit_->setText(q.value(2).toString());
    }
    if (conditionLayout_->count() == 0)
      addCondition();

    qStr = QString("SELECT action, params "
                   "FROM filterActions WHERE idFilter=='%1'").
        arg(filterId_);
    q.exec(qStr);
    while (q.next()) {
      ItemAction *itemAction = addAction();
      int action = q.value(0).toInt();
      itemAction->comboBox1_->setCurrentIndex(action);
      if (action == 3) {
        int index = itemAction->comboBox2_->findData(q.value(1).toInt());
        itemAction->comboBox2_->setCurrentIndex(index);
      } else if (action == 4) {
        itemAction->soundPathEdit_->setText(q.value(1).toString());
      } else if (action == 5) {
        itemAction->colorButton_->setToolTip(q.value(1).toString());
        QPixmap pixmap(14, 14);
        pixmap.fill(QColor(q.value(1).toString()));
        itemAction->colorButton_->setIcon(pixmap);
      }
    }
    if (actionsLayout_->count() == 0)
      addAction();
  }
}

void FilterRulesDialog::closeDialog()
{
  Settings settings;
  settings.setValue("filterRulesDlg/geometry", saveGeometry());
}

void FilterRulesDialog::acceptDialog()
{
  if (filterName_->text().isEmpty()) {
    filterName_->setFocus();
    textWarning_->setText(tr("Please enter name for the filter."));
    warningWidget_->setVisible(true);
    return;
  }

  if (matchComboBox_->currentIndex() != 0) {
    for (int i = 0; i < conditionLayout_->count()-2; i++) {
      ItemCondition *itemCondition =
          qobject_cast<ItemCondition*>(conditionLayout_->itemAt(i)->widget());
      if ((itemCondition->comboBox1_->currentIndex() != 4) &&
          itemCondition->lineEdit_->text().isEmpty()) {
        itemCondition->lineEdit_->setFocus();
        textWarning_->setText(tr("Please enter search condition for the news filter."));
        warningWidget_->setVisible(true);
        return;
      }
    }
  }

  feedsTree_->expandAll();
  QString strIdFeeds;
  QTreeWidgetItem *treeItem = feedsTree_->itemBelow(feedsTree_->topLevelItem(0));
  while (treeItem) {
    if (treeItem->checkState(0) == Qt::Checked) {
      strIdFeeds.append(",");
      strIdFeeds.append(treeItem->text(1));
    }
    treeItem = feedsTree_->itemBelow(treeItem);
  }
  strIdFeeds.append(",");

  QSqlQuery q;
  if (filterId_ == -1) {
    QString qStr = QString("INSERT INTO filters (name, type, feeds) "
                           "VALUES (?, ?, ?)");
    q.prepare(qStr);
    q.addBindValue(filterName_->text());
    q.addBindValue(matchComboBox_->currentIndex());
    q.addBindValue(strIdFeeds);
    q.exec();

    filterId_ = q.lastInsertId().toInt();
    qStr = QString("UPDATE filters SET num='%1' WHERE id=='%1'").
        arg(filterId_);
    q.exec(qStr);

    for (int i = 0; i < conditionLayout_->count()-2; i++) {
      ItemCondition *itemCondition =
          qobject_cast<ItemCondition*>(conditionLayout_->itemAt(i)->widget());
      qStr = QString("INSERT INTO filterConditions "
                     "(idFilter, field, condition, content) "
                     "VALUES (?, ?, ?, ?)");
      q.prepare(qStr);
      q.addBindValue(filterId_);
      q.addBindValue(itemCondition->comboBox1_->currentIndex());
      q.addBindValue(itemCondition->comboBox2_->currentIndex());
      if (itemCondition->comboBox1_->currentIndex() == 4)
        q.addBindValue(itemCondition->comboBox3_->currentIndex());
      else
        q.addBindValue(itemCondition->lineEdit_->text());
      q.exec();
    }

    for (int i = 0; i < actionsLayout_->count()-2; i++) {
      ItemAction *itemAction =
          qobject_cast<ItemAction*>(actionsLayout_->itemAt(i)->widget());
      qStr = QString("INSERT INTO filterActions "
                     "(idFilter, action, params) "
                     "VALUES (?, ?, ?)");
      q.prepare(qStr);
      q.addBindValue(filterId_);
      q.addBindValue(itemAction->comboBox1_->currentIndex());
      if (itemAction->comboBox1_->currentIndex() == 3)
        q.addBindValue(itemAction->comboBox2_->itemData(itemAction->comboBox2_->currentIndex()));
      else if (itemAction->comboBox1_->currentIndex() == 4)
        q.addBindValue(itemAction->soundPathEdit_->text());
      else if (itemAction->comboBox1_->currentIndex() == 5)
        q.addBindValue(itemAction->colorButton_->toolTip());
      else
        q.addBindValue(0);
      q.exec();
    }
  } else {
    q.prepare("UPDATE filters SET name=?, type=?, feeds=? WHERE id=?");
    q.addBindValue(filterName_->text());
    q.addBindValue(matchComboBox_->currentIndex());
    q.addBindValue(strIdFeeds);
    q.addBindValue(filterId_);
    q.exec();

    q.exec(QString("DELETE FROM filterConditions WHERE idFilter='%1'").arg(filterId_));
    q.exec(QString("DELETE FROM filterActions WHERE idFilter='%1'").arg(filterId_));

    for (int i = 0; i < conditionLayout_->count()-2; i++) {
      ItemCondition *itemCondition =
          qobject_cast<ItemCondition*>(conditionLayout_->itemAt(i)->widget());
      QString qStr = QString("INSERT INTO filterConditions "
                             "(idFilter, field, condition, content) "
                             "VALUES (?, ?, ?, ?)");
      q.prepare(qStr);
      q.addBindValue(filterId_);
      q.addBindValue(itemCondition->comboBox1_->currentIndex());
      q.addBindValue(itemCondition->comboBox2_->currentIndex());
      if (itemCondition->comboBox1_->currentIndex() == 4)
        q.addBindValue(itemCondition->comboBox3_->currentIndex());
      else
        q.addBindValue(itemCondition->lineEdit_->text());
      q.exec();
    }

    for (int i = 0; i < actionsLayout_->count()-2; i++) {
      ItemAction *itemAction =
          qobject_cast<ItemAction*>(actionsLayout_->itemAt(i)->widget());
      QString qStr = QString("INSERT INTO filterActions "
                             "(idFilter, action, params) "
                             "VALUES (?, ?, ?)");
      q.prepare(qStr);
      q.addBindValue(filterId_);
      q.addBindValue(itemAction->comboBox1_->currentIndex());
      if (itemAction->comboBox1_->currentIndex() == 3)
        q.addBindValue(itemAction->comboBox2_->itemData(itemAction->comboBox2_->currentIndex()));
      else if (itemAction->comboBox1_->currentIndex() == 4)
        q.addBindValue(itemAction->soundPathEdit_->text());
      else if (itemAction->comboBox1_->currentIndex() == 5)
        q.addBindValue(itemAction->colorButton_->toolTip());
      else
        q.addBindValue(0);
      q.exec();
    }
  }
  accept();
}

void FilterRulesDialog::filterNameChanged(const QString &)
{
  warningWidget_->setVisible(false);
}

void FilterRulesDialog::selectMatch(int index)
{
  if (index == 0) {
    conditionWidget_->setEnabled(false);
  } else {
    conditionWidget_->setEnabled(true);
  }
}

void FilterRulesDialog::feedItemChanged(QTreeWidgetItem *item, int column)
{
  if ((column != 0) || itemNotChecked_) return;

  itemNotChecked_ = true;
  if (item->checkState(0) == Qt::Unchecked) {
    setCheckStateItem(item, Qt::Unchecked);

    QTreeWidgetItem *parentItem = item->parent();
    while (parentItem) {
      parentItem->setCheckState(0, Qt::Unchecked);
      parentItem = parentItem->parent();
    }
  } else {
    setCheckStateItem(item, Qt::Checked);
  }
  itemNotChecked_ = false;
}

void FilterRulesDialog::setCheckStateItem(QTreeWidgetItem *item, Qt::CheckState state)
{
  for (int i = 0; i < item->childCount(); ++i) {
    QTreeWidgetItem *childItem = item->child(i);
    childItem->setCheckState(0, state);
    setCheckStateItem(childItem, state);
  }
}

ItemCondition *FilterRulesDialog::addCondition()
{
  conditionLayout_->removeItem(conditionLayout_->itemAt(conditionLayout_->count()-1));
  conditionLayout_->removeItem(conditionLayout_->itemAt(conditionLayout_->count()-1));
  ItemCondition *itemCondition = new ItemCondition(this);
  conditionLayout_->addWidget(itemCondition);
  conditionLayout_->addStretch();
  conditionLayout_->addSpacing(25);
  connect(itemCondition->addButton_, SIGNAL(clicked()), this, SLOT(addCondition()));
  connect(itemCondition, SIGNAL(signalDeleteCondition(ItemCondition*)),
          this, SLOT(deleteCondition(ItemCondition*)));

  QScrollBar *scrollBar = conditionScrollArea_->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum() -
                      scrollBar->minimum() +
                      scrollBar->pageStep());
  itemCondition->lineEdit_->setFocus();
  connect(itemCondition->lineEdit_, SIGNAL(textChanged(QString)),
          this, SLOT(filterNameChanged(QString)));
  connect(itemCondition->comboBox1_, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(filterNameChanged(QString)));
  return itemCondition;
}

void FilterRulesDialog::deleteCondition(ItemCondition *item)
{
  delete item;
  if (conditionLayout_->count() == 2) {
    addCondition();
  }
}

ItemAction *FilterRulesDialog::addAction()
{
  actionsLayout_->removeItem(actionsLayout_->itemAt(actionsLayout_->count()-1));
  actionsLayout_->removeItem(actionsLayout_->itemAt(actionsLayout_->count()-1));
  ItemAction *itemAction = new ItemAction(this);

  QSqlQuery q;
  q.exec("SELECT id, name, image FROM labels ORDER BY num");
  while (q.next()) {
    int idLabel = q.value(0).toInt();
    QString nameLabel = q.value(1).toString();
    if ((idLabel <= 6) && (MainWindow::nameLabels().at(idLabel-1) == nameLabel)) {
      nameLabel = MainWindow::trNameLabels().at(idLabel-1);
    }
    QByteArray byteArray = q.value(2).toByteArray();
    QPixmap imageLabel;
    if (!byteArray.isNull())
      imageLabel.loadFromData(byteArray);

    itemAction->comboBox2_->addItem(QIcon(imageLabel), nameLabel, idLabel);
  }

  actionsLayout_->addWidget(itemAction);
  actionsLayout_->addStretch();
  actionsLayout_->addSpacing(25);
  connect(itemAction, SIGNAL(signalPlaySound(QString)),
          parent(), SLOT(slotPlaySound(QString)));
  connect(itemAction->addButton_, SIGNAL(clicked()), this, SLOT(addAction()));
  connect(itemAction, SIGNAL(signalDeleteAction(ItemAction*)),
          this, SLOT(deleteAction(ItemAction*)));
  return itemAction;
}

void FilterRulesDialog::deleteAction(ItemAction *item)
{
  delete item;
  if (actionsLayout_->count() == 2) {
    addAction();
  }
}
