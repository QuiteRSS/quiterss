#include "rsslisting.h"
#include "filterrulesdialog.h"

FilterRulesDialog::FilterRulesDialog(QWidget *parent, bool newFilter, int feedId)
  : QDialog(parent),
    newFilter_(newFilter),
    feedId_(feedId)
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
  QString qStr = QString("select text, id, image from feeds");
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
      treeWidgetItem1->setCheckState(0, Qt::Checked);
      rowFeed = feedsTree->topLevelItem(0)->childCount();
    }
    else
      treeWidgetItem1->setCheckState(0, Qt::Unchecked);
  }

  feedsTree->expandAll();

  feedsTree->topLevelItem(0)->setCheckState(0, Qt::PartiallyChecked);
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

  buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setMargin(5);
  mainLayout->addWidget(mainSpliter);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  connect(this, SIGNAL(finished(int)), this, SLOT(closeDialog(int)));

  restoreGeometry(settings_->value("filterRulesDlg/geometry").toByteArray());
  filterName->setFocus();
}

void FilterRulesDialog::closeDialog(int result)
{
  settings_->setValue("filterRulesDlg/geometry", saveGeometry());

  if (result == QDialog::Accepted) {
    RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
    QSqlQuery q(rssl_->db_);
    if (newFilter_) {
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
          strIdFeeds.append(treeWidgetItem->child(i)->text(1));
        }
      }

//      QString qStr = QString("INSERT INTO filters (name, type, feeds)"
//                             "VALUES (?, ?, ?)");
//      q.prepare(qStr);
//      q.addBindValue(filterName->text());
//      q.addBindValue(type);
//      q.addBindValue(strIdFeeds);
//      q.exec();

//      int filterId = -1;
//      q.exec(QString("SELECT id FROM filters WHERE name LIKE '%1'").
//             arg(filterName->text()));
//      if (q.next()) filterId = q.value(0).toInt();

      for (int i = 0; i < conditionLayout->count()-2; i++) {
        ItemCondition *itemCondition =
            qobject_cast<ItemCondition*>(conditionLayout->itemAt(i)->widget());
        qCritical() << itemCondition->comboBox1->currentIndex()
                    << itemCondition->comboBox2->currentIndex()
                    << itemCondition->comboBox3->currentIndex()
                    << itemCondition->lineEdit->text();
      }

    } else {

    }
  }
}

void FilterRulesDialog::setData()
{

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
      rootChecked = true;
      feedsTree->topLevelItem(0)->setCheckState(0, Qt::PartiallyChecked);
      rootChecked = false;
    }
  }
}

void FilterRulesDialog::addCondition()
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
}

void FilterRulesDialog::deleteCondition(ItemCondition *item)
{
  delete item;
  if (conditionLayout->count() == 2) {
    addCondition();
  }
}

void FilterRulesDialog::addAction()
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
}

void FilterRulesDialog::deleteAction(ItemAction *item)
{
  delete item;
  if (actionsLayout->count() == 2) {
    addAction();
  }
}
