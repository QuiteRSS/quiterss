#include "rsslisting.h"
#include "filterrulesdialog.h"

FilterRulesDialog::FilterRulesDialog(QWidget *parent, int filterId, int feedId)
  : Dialog(parent),
    filterId_(filterId)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Filter Rules"));
  setMinimumHeight(300);

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  settings_ = rssl_->settings_;

  feedsTree = new QTreeWidget(this);
  feedsTree->setObjectName("feedsTreeFR");
  feedsTree->setColumnCount(2);
  feedsTree->setColumnHidden(1, true);
  feedsTree->header()->setMovable(false);

  itemNotChecked_ = false;

  QStringList treeItem;
  treeItem << tr("Feeds") << "Id";
  feedsTree->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << tr("All Feeds") << "0";
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setCheckState(0, Qt::Checked);
  feedsTree->addTopLevelItem(treeWidgetItem);

  QSqlQuery q;
  QQueue<int> parentIds;
  parentIds.enqueue(0);
  while (!parentIds.empty()) {
    int parentId = parentIds.dequeue();
    QString qStr = QString("SELECT text, id, image, xmlUrl FROM feeds WHERE parentId='%1' ORDER BY text").
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
      if (!byteArray.isNull()) {
        iconItem.loadFromData(QByteArray::fromBase64(byteArray));
      } else if (!xmlUrl.isEmpty()) {
        iconItem.load(":/images/feed");
      } else {
        iconItem.load(":/images/folder");
      }
      treeWidgetItem->setIcon(0, iconItem);

      QList<QTreeWidgetItem *> treeItems =
          feedsTree->findItems(QString::number(parentId),
                               Qt::MatchFixedString | Qt::MatchRecursive,
                               1);
      treeItems.at(0)->addChild(treeWidgetItem);
      parentIds.enqueue(feedIdT.toInt());
    }
  }
  feedsTree->expandAll();

  if (feedId != -1) {
    int rowCount = 0;
    QTreeWidgetItem *childItem = feedsTree->topLevelItem(0);
    while (childItem) {
      if (childItem->text(1).toInt() == feedId) break;
      rowCount++;
      childItem = feedsTree->itemBelow(childItem);
    }
    feedsTree->verticalScrollBar()->setValue(rowCount);
    feedsTree->topLevelItem(0)->setCheckState(0, Qt::Unchecked);
  }
  connect(feedsTree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
          this, SLOT(feedItemChanged(QTreeWidgetItem*,int)));

  filterName = new LineEdit(this);
  QHBoxLayout *filterNamelayout = new QHBoxLayout();
  filterNamelayout->addWidget(new QLabel(tr("Filter name:")));
  filterNamelayout->addWidget(filterName);

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

  conditionScrollArea = new QScrollArea(this);
  conditionScrollArea->setFocusPolicy(Qt::NoFocus);
  conditionScrollArea->setWidgetResizable(true);

  conditionLayout = new QVBoxLayout();
  conditionLayout->setMargin(1);
  conditionLayout->setSpacing(1);
  if (filterId_ == -1)
    addCondition();

  conditionWidget = new QWidget(this);
  conditionWidget->setObjectName("infoWidgetFR");
  conditionWidget->setLayout(conditionLayout);
  conditionScrollArea->setWidget(conditionWidget);

  QVBoxLayout *splitterLayoutV1 = new QVBoxLayout();
  splitterLayoutV1->setMargin(0);
  splitterLayoutV1->addLayout(matchLayout);
  splitterLayoutV1->addWidget(conditionScrollArea, 1);

  QWidget *splitterWidget1 = new QWidget(this);
  splitterWidget1->setMinimumWidth(400);
  splitterWidget1->setLayout(splitterLayoutV1);

  actionsScrollArea = new QScrollArea(this);
  actionsScrollArea->setWidgetResizable(true);
  actionsScrollArea->setFocusPolicy(Qt::NoFocus);

  actionsLayout = new QVBoxLayout();
  actionsLayout->setMargin(1);
  actionsLayout->setSpacing(1);
  if (filterId_ == -1)
    addAction();

  actionsWidget = new QWidget(this);
  actionsWidget->setObjectName("actionsWidgetFR");
  actionsWidget->setLayout(actionsLayout);
  actionsScrollArea->setWidget(actionsWidget);

  QVBoxLayout *splitterLayoutV2 = new QVBoxLayout();
  splitterLayoutV2->setMargin(0);
  splitterLayoutV2->addWidget(new QLabel(tr("Perform these actions:")));
  splitterLayoutV2->addWidget(actionsScrollArea, 1);

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
  mainSpliter->addWidget(feedsTree);

  QLabel *iconWarning = new QLabel(this);
  iconWarning->setPixmap(QPixmap(":/images/warning"));
  textWarning = new QLabel(this);
  QFont font = textWarning->font();
  font.setBold(true);
  textWarning->setFont(font);

  QHBoxLayout *warningLayout = new QHBoxLayout();
  warningLayout->setMargin(0);
  warningLayout->addWidget(iconWarning);
  warningLayout->addWidget(textWarning, 1);

  warningWidget_ = new QWidget(this);
  warningWidget_->setLayout(warningLayout);
  warningWidget_->setVisible(false);

  buttonsLayout->insertWidget(0, warningWidget_, 1);
  buttonBox->addButton(QDialogButtonBox::Ok);
  buttonBox->addButton(QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptDialog()));

  pageLayout->addWidget(mainSpliter);

  setData();

  filterName->setFocus();
  filterName->selectAll();

  restoreGeometry(settings_->value("filterRulesDlg/geometry").toByteArray());

  connect(filterName, SIGNAL(textChanged(QString)),
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
    filterName->setText(q.value(0).toString());

    matchComboBox_->setCurrentIndex(q.value(1).toInt());

    itemNotChecked_ = true;
    QStringList strIdFeeds = q.value(2).toString().split(",", QString::SkipEmptyParts);
    foreach (QString strIdFeed, strIdFeeds) {
      QList<QTreeWidgetItem *> treeItems =
          feedsTree->findItems(strIdFeed,
                               Qt::MatchFixedString | Qt::MatchRecursive,
                               1);
      if (treeItems.count())
        treeItems.at(0)->setCheckState(0, Qt::Checked);
    }
    QTreeWidgetItem *treeItem = feedsTree->itemBelow(feedsTree->topLevelItem(0));
    while (treeItem) {
      if (treeItem->checkState(0) == Qt::Unchecked) {
        feedsTree->topLevelItem(0)->setCheckState(0, Qt::Unchecked);
        break;
      }
      treeItem = feedsTree->itemBelow(treeItem);
    }
    itemNotChecked_ = false;

    qStr = QString("SELECT field, condition, content "
        "FROM filterConditions WHERE idFilter=='%1' ORDER BY content").
        arg(filterId_);
    q.exec(qStr);
    while (q.next()) {
      ItemCondition *itemCondition = addCondition();
      itemCondition->comboBox1->setCurrentIndex(q.value(0).toInt());
      itemCondition->comboBox2->setCurrentIndex(q.value(1).toInt());
      if (q.value(0).toInt() == 4)
        itemCondition->comboBox3->setCurrentIndex(q.value(2).toInt());
      else
        itemCondition->lineEdit->setText(q.value(2).toString());
    }

    qStr = QString("SELECT action, params "
        "FROM filterActions WHERE idFilter=='%1'").
        arg(filterId_);
    q.exec(qStr);
    while (q.next()) {
      ItemAction *itemAction = addAction();
      int action = q.value(0).toInt();
      itemAction->comboBox1->setCurrentIndex(action);
      if (action == 3) {
        int index = itemAction->comboBox2->findData(q.value(1).toInt());
        itemAction->comboBox2->setCurrentIndex(index);
      }

    }
  }
}

void FilterRulesDialog::closeDialog()
{
  settings_->setValue("filterRulesDlg/geometry", saveGeometry());
}

void FilterRulesDialog::acceptDialog()
{
  if (filterName->text().isEmpty()) {
    filterName->setFocus();
    textWarning->setText(tr("Please enter name for the filter."));
    warningWidget_->setVisible(true);
    return;
  }

  if (matchComboBox_->currentIndex() != 0) {
    for (int i = 0; i < conditionLayout->count()-2; i++) {
      ItemCondition *itemCondition =
          qobject_cast<ItemCondition*>(conditionLayout->itemAt(i)->widget());
      if ((itemCondition->comboBox1->currentIndex() != 4) &&
          itemCondition->lineEdit->text().isEmpty()) {
        itemCondition->lineEdit->setFocus();
        textWarning->setText(tr("Please enter search condition for the news filter."));
        warningWidget_->setVisible(true);
        return;
      }
    }
  }

  QString strIdFeeds;
  QTreeWidgetItem *treeItem = feedsTree->itemBelow(feedsTree->topLevelItem(0));
  while (treeItem) {
    if (treeItem->checkState(0) == Qt::Checked) {
      strIdFeeds.append(",");
      strIdFeeds.append(treeItem->text(1));
    }
    treeItem = feedsTree->itemBelow(treeItem);
  }
  strIdFeeds.append(",");

  QSqlQuery q;
  if (filterId_ == -1) {
    QString qStr = QString("INSERT INTO filters (name, type, feeds) "
                           "VALUES (?, ?, ?)");
    q.prepare(qStr);
    q.addBindValue(filterName->text());
    q.addBindValue(matchComboBox_->currentIndex());
    q.addBindValue(strIdFeeds);
    q.exec();

    filterId_ = q.lastInsertId().toInt();
    qStr = QString("UPDATE filters SET num='%1' WHERE id=='%1'").
        arg(filterId_);
    q.exec(qStr);

    for (int i = 0; i < conditionLayout->count()-2; i++) {
      ItemCondition *itemCondition =
          qobject_cast<ItemCondition*>(conditionLayout->itemAt(i)->widget());
      qStr = QString("INSERT INTO filterConditions "
                     "(idFilter, field, condition, content) "
                     "VALUES (?, ?, ?, ?)");
      q.prepare(qStr);
      q.addBindValue(filterId_);
      q.addBindValue(itemCondition->comboBox1->currentIndex());
      q.addBindValue(itemCondition->comboBox2->currentIndex());
      if (itemCondition->comboBox1->currentIndex() == 4)
        q.addBindValue(itemCondition->comboBox3->currentIndex());
      else
        q.addBindValue(itemCondition->lineEdit->text());
      q.exec();
    }

    for (int i = 0; i < actionsLayout->count()-2; i++) {
      ItemAction *itemAction =
          qobject_cast<ItemAction*>(actionsLayout->itemAt(i)->widget());
      qStr = QString("INSERT INTO filterActions "
                     "(idFilter, action, params) "
                     "VALUES (?, ?, ?)");
      q.prepare(qStr);
      q.addBindValue(filterId_);
      q.addBindValue(itemAction->comboBox1->currentIndex());
      q.addBindValue(itemAction->comboBox2->itemData(itemAction->comboBox2->currentIndex()));
      q.exec();
    }
  } else {
    q.prepare("UPDATE filters SET name=?, type=?, feeds=? WHERE id=?");
    q.addBindValue(filterName->text());
    q.addBindValue(matchComboBox_->currentIndex());
    q.addBindValue(strIdFeeds);
    q.addBindValue(filterId_);
    q.exec();

    q.exec(QString("DELETE FROM filterConditions WHERE idFilter='%1'").arg(filterId_));
    q.exec(QString("DELETE FROM filterActions WHERE idFilter='%1'").arg(filterId_));

    for (int i = 0; i < conditionLayout->count()-2; i++) {
      ItemCondition *itemCondition =
          qobject_cast<ItemCondition*>(conditionLayout->itemAt(i)->widget());
      QString qStr = QString("INSERT INTO filterConditions "
                     "(idFilter, field, condition, content) "
                     "VALUES (?, ?, ?, ?)");
      q.prepare(qStr);
      q.addBindValue(filterId_);
      q.addBindValue(itemCondition->comboBox1->currentIndex());
      q.addBindValue(itemCondition->comboBox2->currentIndex());
      if (itemCondition->comboBox1->currentIndex() == 4)
        q.addBindValue(itemCondition->comboBox3->currentIndex());
      else
        q.addBindValue(itemCondition->lineEdit->text());
      q.exec();
    }

    for (int i = 0; i < actionsLayout->count()-2; i++) {
      ItemAction *itemAction =
          qobject_cast<ItemAction*>(actionsLayout->itemAt(i)->widget());
      QString qStr = QString("INSERT INTO filterActions "
                     "(idFilter, action, params) "
                     "VALUES (?, ?, ?)");
      q.prepare(qStr);
      q.addBindValue(filterId_);
      q.addBindValue(itemAction->comboBox1->currentIndex());
      q.addBindValue(itemAction->comboBox2->itemData(itemAction->comboBox2->currentIndex()));
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
    conditionWidget->setEnabled(false);
  } else {
    conditionWidget->setEnabled(true);
  }
}

void FilterRulesDialog::feedItemChanged(QTreeWidgetItem *item, int column)
{
  if ((column != 0) || itemNotChecked_) return;

  itemNotChecked_ = true;
  if (item->checkState(0) == Qt::Unchecked) {
    if (item->childCount()) {
      QTreeWidgetItem *childItem = feedsTree->itemBelow(item);
      while (childItem) {
        childItem->setCheckState(0, Qt::Unchecked);
        childItem = feedsTree->itemBelow(childItem);
        if (childItem) {
          if (item->parent() == childItem->parent()) break;
        }
      }
    }
    QTreeWidgetItem *parentItem = item->parent();
    while (parentItem) {
      parentItem->setCheckState(0, Qt::Unchecked);
      parentItem = parentItem->parent();
    }
  } else if (item->childCount()) {
    QTreeWidgetItem *childItem = feedsTree->itemBelow(item);
    while (childItem) {
      childItem->setCheckState(0, Qt::Checked);
      childItem = feedsTree->itemBelow(childItem);
      if (childItem) {
        if (item->parent() == childItem->parent()) break;
      }
    }
  }
  itemNotChecked_ = false;
}

ItemCondition *FilterRulesDialog::addCondition()
{
  conditionLayout->removeItem(conditionLayout->itemAt(conditionLayout->count()-1));
  conditionLayout->removeItem(conditionLayout->itemAt(conditionLayout->count()-1));
  ItemCondition *itemCondition = new ItemCondition(this);
  conditionLayout->addWidget(itemCondition);
  conditionLayout->addStretch();
  conditionLayout->addSpacing(25);
  connect(itemCondition->addButton, SIGNAL(clicked()), this, SLOT(addCondition()));
  connect(itemCondition, SIGNAL(signalDeleteCondition(ItemCondition*)),
          this, SLOT(deleteCondition(ItemCondition*)));

  QScrollBar *scrollBar = conditionScrollArea->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum() -
                      scrollBar->minimum() +
                      scrollBar->pageStep());
  itemCondition->lineEdit->setFocus();
  connect(itemCondition->lineEdit, SIGNAL(textChanged(QString)),
          this, SLOT(filterNameChanged(QString)));
  connect(itemCondition->comboBox1, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(filterNameChanged(QString)));
  return itemCondition;
}

void FilterRulesDialog::deleteCondition(ItemCondition *item)
{
  delete item;
  if (conditionLayout->count() == 2) {
    addCondition();
  }
}

ItemAction *FilterRulesDialog::addAction()
{
  actionsLayout->removeItem(actionsLayout->itemAt(actionsLayout->count()-1));
  actionsLayout->removeItem(actionsLayout->itemAt(actionsLayout->count()-1));
  ItemAction *itemAction = new ItemAction(this);

  QSqlQuery q;
  q.exec("SELECT id, name, image FROM labels ORDER BY num");
  while (q.next()) {
    int idLabel = q.value(0).toInt();
    QString nameLabel = q.value(1).toString();
    QByteArray byteArray = q.value(2).toByteArray();
    QPixmap imageLabel;
    if (!byteArray.isNull())
      imageLabel.loadFromData(byteArray);

    itemAction->comboBox2->addItem(QIcon(imageLabel), nameLabel, idLabel);
  }

  actionsLayout->addWidget(itemAction);
  actionsLayout->addStretch();
  actionsLayout->addSpacing(25);
  connect(itemAction->addButton, SIGNAL(clicked()), this, SLOT(addAction()));
  connect(itemAction, SIGNAL(signalDeleteAction(ItemAction*)),
          this, SLOT(deleteAction(ItemAction*)));
  return itemAction;
}

void FilterRulesDialog::deleteAction(ItemAction *item)
{
  delete item;
  if (actionsLayout->count() == 2) {
    addAction();
  }
}
