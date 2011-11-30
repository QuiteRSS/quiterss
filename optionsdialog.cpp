#include "optionsdialog.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent)
{
  QTreeWidget *categoriesTree = new QTreeWidget();
  categoriesTree->setHeaderHidden(true);
  categoriesTree->setColumnCount(2);
  categoriesTree->setColumnHidden(0, true);
  QStringList treeItem;
  treeItem << "0" << tr("Zero");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "1" << tr("First");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "2" << tr("Second");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));

  widgetZero_ = new QWidget();
  widgetZero_->setToolTip(tr("widgetZero"));
  widgetFirst_ = new QWidget();
  widgetFirst_->setToolTip(tr("widgetFirst"));
  widgetSecond_ = new QWidget();
  widgetSecond_->setToolTip(tr("widgetSecond"));

  contentLabel_ = new QLabel(tr("ContentLabel"));
  contentStack_ = new QStackedWidget();
  contentStack_->addWidget(widgetZero_);
  contentStack_->addWidget(widgetFirst_);
  contentStack_->addWidget(widgetSecond_);

  QVBoxLayout *contentLayout = new QVBoxLayout();
  contentLayout->setMargin(4);
  contentLayout->addWidget(contentLabel_);
  contentLayout->addWidget(contentStack_);

  QWidget *contentWidget = new QWidget();
  contentWidget->setLayout(contentLayout);

  QSplitter *splitter = new QSplitter();
  splitter->setChildrenCollapsible(false);
  splitter->addWidget(categoriesTree);
  splitter->addWidget(contentWidget);
  QList<int> sizes;
  sizes << 150 << 600;
  splitter->setSizes(sizes);

  connect(categoriesTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
          this, SLOT(slotCategoriesItemCLicked(QTreeWidgetItem*,int)));

  buttonBox_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox_, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox_, SIGNAL(rejected()), this, SLOT(reject()));

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setMargin(4);
  mainLayout->addWidget(splitter);
  mainLayout->addWidget(buttonBox_);

  setLayout(mainLayout);

  slotCategoriesItemCLicked(categoriesTree->topLevelItem(0), 0);
}

void OptionsDialog::slotCategoriesItemCLicked(QTreeWidgetItem* item, int column)
{
  contentLabel_->setText(item->data(1, Qt::DisplayRole).toString());
  contentStack_->setCurrentIndex(item->data(0, Qt::DisplayRole).toInt());
}
