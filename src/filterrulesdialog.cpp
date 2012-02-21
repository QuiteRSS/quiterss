#include "filterrulesdialog.h"

FilterRulesDialog::FilterRulesDialog(QWidget *parent, QSettings *settings)
  : QDialog(parent),
    settings_(settings)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Filter rules"));
  setMinimumHeight(300);

  QTreeWidget *feedsTree = new QTreeWidget();
  feedsTree->setObjectName("feedsTreeFR");
  feedsTree->setColumnCount(2);
  feedsTree->setColumnHidden(1, true);
  feedsTree->header()->setMovable(false);

  QStringList treeItem;
  treeItem << tr("Locations") << tr("Id") ;
  feedsTree->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << "All feeds" << "0";
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setCheckState(0, Qt::Unchecked);
  feedsTree->addTopLevelItem(treeWidgetItem);
  treeItem.clear();
  treeItem << "Feed 1" << "1";
  QTreeWidgetItem *treeWidgetItem1 = new QTreeWidgetItem(treeItem);
  treeWidgetItem1->setCheckState(0, Qt::Unchecked);
  treeWidgetItem->addChild(treeWidgetItem1);
  feedsTree->expandAll();

  filterName = new QLineEdit();

  QHBoxLayout *filterNamelayout = new QHBoxLayout();
  filterNamelayout->addWidget(new QLabel(tr("Filter name:")));
  filterNamelayout->addWidget(filterName);

  QRadioButton *matchAllFollowing_ =
      new QRadioButton(tr("Match all of the following"));
  matchAllFollowing_->setChecked(true);
  QRadioButton *matchAnyFollowing_ =
      new QRadioButton(tr("Match any of the following"));
  QRadioButton *matchAllNews_ =
      new QRadioButton(tr("Match all news"));

  QHBoxLayout *matchLayout = new QHBoxLayout();
  matchLayout->setSpacing(10);
  matchLayout->addWidget(matchAllFollowing_);
  matchLayout->addWidget(matchAnyFollowing_);
  matchLayout->addWidget(matchAllNews_);
  matchLayout->addStretch();

  infoScrollArea = new QScrollArea();
  infoScrollArea->setWidgetResizable(true);

  infoLayout = new QVBoxLayout();
  infoLayout->setMargin(5);
  addFilterRules();

  infoWidget = new QWidget();
  infoWidget->setObjectName("infoWidgetFR");
  infoWidget->setLayout(infoLayout);
  infoScrollArea->setWidget(infoWidget);

  QVBoxLayout *splitterLayoutV1 = new QVBoxLayout();
  splitterLayoutV1->setMargin(0);
  splitterLayoutV1->addLayout(matchLayout);
  splitterLayoutV1->addWidget(infoScrollArea, 1);

  QWidget *splitterWidget1 = new QWidget();
  splitterWidget1->setLayout(splitterLayoutV1);

  QScrollArea *actionsScrollArea = new QScrollArea();
  actionsScrollArea->setWidgetResizable(true);

  QVBoxLayout *actionsLayout = new QVBoxLayout();
  actionsLayout->setMargin(5);
  actionsLayout->addWidget(new ItemRules(), 0, Qt::AlignTop);

  QWidget *actionsWidget = new QWidget();
  actionsWidget->setObjectName("actionsWidgetFR");
  actionsWidget->setLayout(actionsLayout);
  actionsScrollArea->setWidget(actionsWidget);

  QVBoxLayout *splitterLayoutV2 = new QVBoxLayout();
  splitterLayoutV2->setMargin(0);
  splitterLayoutV2->addWidget(new QLabel(tr("Perform these actions:")));
  splitterLayoutV2->addWidget(actionsScrollArea, 1);

  QWidget *splitterWidget2 = new QWidget();
  splitterWidget2->setLayout(splitterLayoutV2);

  QSplitter *spliter = new QSplitter(Qt::Vertical);
  spliter->setChildrenCollapsible(false);
  spliter->setMinimumWidth(420);
  spliter->addWidget(splitterWidget1);
  spliter->addWidget(splitterWidget2);

  QVBoxLayout *rulesLayout = new QVBoxLayout();
  rulesLayout->setMargin(0);
  rulesLayout->addLayout(filterNamelayout);
  rulesLayout->addWidget(spliter);

  QWidget *rulesWidget = new QWidget();
  rulesWidget->setLayout(rulesLayout);

  QSplitter *mainSpliter = new QSplitter();
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

  connect(this, SIGNAL(finished(int)), this, SLOT(closeDialog()));

  resize(600, 350);
  restoreGeometry(settings_->value("filterRulesDlg/geometry").toByteArray());
}

void FilterRulesDialog::closeDialog()
{
  settings_->setValue("filterRulesDlg/geometry", saveGeometry());
}

void FilterRulesDialog::addFilterRules()
{
  infoLayout->removeItem(infoLayout->itemAt(infoLayout->count()-1));
  infoLayout->removeItem(infoLayout->itemAt(infoLayout->count()-1));
  ItemRules *itemRules = new ItemRules();
  infoLayout->addWidget(itemRules);
  infoLayout->addStretch();
  infoLayout->addSpacing(25);
  connect(itemRules->addButton, SIGNAL(clicked()), this, SLOT(addFilterRules()));
  connect(itemRules, SIGNAL(siganlDeleteFilterRules(ItemRules*)),
          this, SLOT(deleteFilterRules(ItemRules*)));

  QScrollBar *scrollBar = infoScrollArea->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum() -
                      scrollBar->minimum() +
                      scrollBar->pageStep());
  itemRules->lineEdit->setText(QString::number(infoLayout->count()-2));
  itemRules->lineEdit->setFocus();
}

void FilterRulesDialog::deleteFilterRules(ItemRules *item)
{
  delete item;
  if (infoLayout->count() == 2) {
    addFilterRules();
  }
}
