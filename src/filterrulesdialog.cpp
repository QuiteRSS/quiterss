#include "rsslisting.h"
#include "filterrulesdialog.h"

FilterRulesDialog::FilterRulesDialog(QWidget *parent, int filterId, int feedId)
  : QDialog(parent),
    filterId_(filterId)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Filter rules"));
  setMinimumHeight(300);

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  settings_ = rssl_->settings_;

  feedsTree = new QTreeWidget(this);
  feedsTree->setObjectName("feedsTreeFR");
  feedsTree->setColumnCount(2);
  feedsTree->setColumnHidden(1, true);
  feedsTree->header()->setMovable(false);

  QStringList treeItem;
  treeItem << tr("Feeds") << "Id";
  feedsTree->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << "All feeds" << "-1";
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setCheckState(0, Qt::Unchecked);
  feedsTree->addTopLevelItem(treeWidgetItem);
  connect(feedsTree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
          this, SLOT(feedItemChanged(QTreeWidgetItem*,int)));

  int rowFeed = - 1;
  QSqlQuery q(rssl_->db_);
  QString qStr = QString("SELECT text, id, image FROM feeds");
  q.exec(qStr);
  while (q.next()) {
    treeItem.clear();
    treeItem << q.value(0).toString() << q.value(1).toString();
    QTreeWidgetItem *treeWidgetItem1 = new QTreeWidgetItem(treeItem);

    QPixmap iconTab;
    QByteArray byteArray = q.value(2).toByteArray();
    if (!byteArray.isNull()) {
      iconTab.loadFromData(QByteArray::fromBase64(byteArray));
    } else {
      iconTab.load(":/images/feed");
    }
    treeWidgetItem1->setIcon(0, iconTab);

    treeWidgetItem->addChild(treeWidgetItem1);
    if (feedId == q.value(1).toInt()) {
      rowFeed = feedsTree->topLevelItem(0)->childCount();
      treeWidgetItem1->setCheckState(0, Qt::Checked);
      feedsTree->topLevelItem(0)->setCheckState(0, Qt::PartiallyChecked);
    } else {
      treeWidgetItem1->setCheckState(0, Qt::Unchecked);
    }
  }

  feedsTree->expandAll();
  feedsTree->verticalScrollBar()->setValue(rowFeed);

  filterName = new LineEdit(this);

  QHBoxLayout *filterNamelayout = new QHBoxLayout();
  filterNamelayout->addWidget(new QLabel(tr("Filter name:")));
  filterNamelayout->addWidget(filterName);

  matchAllCondition_ = new QRadioButton(tr("Match all conditions"), this);
  matchAllCondition_->setChecked(true);
  matchAnyCondition_ = new QRadioButton(tr("Match any condition"), this);
  matchAllNews_ = new QRadioButton(tr("Match all news"), this);
  connect(matchAllCondition_, SIGNAL(clicked()), this, SLOT(selectMatch()));
  connect(matchAnyCondition_, SIGNAL(clicked()), this, SLOT(selectMatch()));
  connect(matchAllNews_, SIGNAL(clicked()), this, SLOT(selectMatch()));

  QHBoxLayout *matchLayout = new QHBoxLayout();
  matchLayout->setSpacing(10);
  matchLayout->addWidget(matchAllCondition_);
  matchLayout->addWidget(matchAnyCondition_);
  matchLayout->addWidget(matchAllNews_);
  matchLayout->addStretch();

  conditionScrollArea = new QScrollArea(this);
  conditionScrollArea->setFocusPolicy(Qt::NoFocus);
  conditionScrollArea->setWidgetResizable(true);

  conditionLayout = new QVBoxLayout();
  conditionLayout->setMargin(5);
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
  splitterWidget1->setLayout(splitterLayoutV1);

  actionsScrollArea = new QScrollArea(this);
  actionsScrollArea->setWidgetResizable(true);
  actionsScrollArea->setFocusPolicy(Qt::NoFocus);

  actionsLayout = new QVBoxLayout();
  actionsLayout->setMargin(5);
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

  buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptDialog()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  QHBoxLayout *statusLayout = new QHBoxLayout();
  statusLayout->setMargin(0);
  statusLayout->addWidget(warningWidget_, 1);
  statusLayout->addWidget(buttonBox);

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setMargin(5);
  mainLayout->addWidget(mainSpliter);
  mainLayout->addLayout(statusLayout);
  setLayout(mainLayout);

  setData();

  connect(filterName, SIGNAL(textChanged(QString)),
          this, SLOT(filterNameChanged(QString)));
  connect(this, SIGNAL(finished(int)), this, SLOT(closeDialog()));

  restoreGeometry(settings_->value("filterRulesDlg/geometry").toByteArray());

  filterName->setFocus();
  filterName->selectAll();
}

void FilterRulesDialog::setData()
{
  if (filterId_ == -1) return;

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  QSqlQuery q(rssl_->db_);
  QString qStr = QString("SELECT name, type, feeds FROM filters WHERE id=='%1'").
      arg(filterId_);
  q.exec(qStr);
  if (q.next()) {
    filterName->setText(q.value(0).toString());

    if (q.value(1).toInt() == 1) {
      matchAllCondition_->setChecked(true);
    } else if (q.value(1).toInt() == 2) {
      matchAnyCondition_->setChecked(true);
    } else {
      matchAllNews_->setChecked(true);
    }
    selectMatch();

    QStringList strIdFeeds = q.value(2).toString().split(",");
    foreach (QString strIdFeed, strIdFeeds) {
      for (int i = 0; i < feedsTree->topLevelItem(0)->childCount(); i++) {
        if (strIdFeed == feedsTree->topLevelItem(0)->child(i)->text(1)) {
          feedsTree->topLevelItem(0)->child(i)->setCheckState(0, Qt::Checked);
        }
      }
    }

    qStr = QString("SELECT field, condition, content "
        "FROM filterConditions WHERE idFilter=='%1'").
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
      itemAction->comboBox1->setCurrentIndex(q.value(0).toInt());
      itemAction->comboBox2->setCurrentIndex(q.value(1).toInt());
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

  if (!matchAllNews_->isChecked()) {
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

  int type;
  if (matchAllCondition_->isChecked()) {
    type = 1;
  } else if (matchAnyCondition_->isChecked()) {
    type = 2;
  } else {
    type = 0;
  }

  QString strIdFeeds;
  QTreeWidgetItem *treeWidgetItem =
      feedsTree->topLevelItem(0);

  for (int i = 0; i < treeWidgetItem->childCount(); i++) {
    if (treeWidgetItem->child(i)->checkState(0) == Qt::Checked) {
      if (!strIdFeeds.isNull()) strIdFeeds.append(",");
      strIdFeeds.append(treeWidgetItem->child(i)->text(1));
    }
  }

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  QSqlQuery q(rssl_->db_);

  if (filterId_ == -1) {
    QString qStr = QString("INSERT INTO filters (name, type, feeds) "
                           "VALUES (?, ?, ?)");
    q.prepare(qStr);
    q.addBindValue(filterName->text());
    q.addBindValue(type);
    q.addBindValue(strIdFeeds);
    q.exec();

    q.exec(QString("SELECT id FROM filters WHERE name LIKE '%1'").
           arg(filterName->text()));
    if (q.next()) filterId_ = q.value(0).toInt();

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
      q.addBindValue(itemAction->comboBox2->currentIndex());
      q.exec();
    }
  } else {
    q.prepare("UPDATE filters SET name=?, type=?, feeds=? WHERE id=?");
    q.addBindValue(filterName->text());
    q.addBindValue(type);
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
      q.addBindValue(itemAction->comboBox2->currentIndex());
      q.exec();
    }
  }
  accept();
}

void FilterRulesDialog::filterNameChanged(const QString &)
{
  warningWidget_->setVisible(false);
}

void FilterRulesDialog::selectMatch()
{
  if (matchAllNews_->isChecked()) {
    conditionWidget->setEnabled(false);
  } else {
    conditionWidget->setEnabled(true);
  }
}

void FilterRulesDialog::feedItemChanged(QTreeWidgetItem *item, int column)
{
  static bool rootChecked = false;
  static bool notRootChecked = false;

  if (column != 0) return;

  if (feedsTree->indexOfTopLevelItem(item) == 0) {
    if (!rootChecked) {
      notRootChecked = true;
      if (item->checkState(0) == Qt::Unchecked) {
        for (int i = 0; i < feedsTree->topLevelItem(0)->childCount(); i++) {
          feedsTree->topLevelItem(0)->child(i)->setCheckState(0, Qt::Unchecked);
        }
      } else {
        for (int i = 0; i < feedsTree->topLevelItem(0)->childCount(); i++) {
          feedsTree->topLevelItem(0)->child(i)->setCheckState(0, Qt::Checked);
        }
      }
      notRootChecked = false;
    }
  } else {
    if (!notRootChecked) {
      bool childChecked = false;
      for (int i = 0; i < feedsTree->topLevelItem(0)->childCount(); i++) {
        if (feedsTree->topLevelItem(0)->child(i)->checkState(0)) {
          childChecked = true;
          break;
        }
      }
      rootChecked = true;
      if (childChecked)
        feedsTree->topLevelItem(0)->setCheckState(0, Qt::PartiallyChecked);
      else
        feedsTree->topLevelItem(0)->setCheckState(0, Qt::Unchecked);
      rootChecked = false;
    }
  }
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
