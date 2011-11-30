#include "optionsdialog.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent)
{
  QTreeWidget *categoriesTree = new QTreeWidget();
  categoriesTree->setHeaderHidden(true);
  QStringList treeItem;
  treeItem << tr("First");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Last");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));

  QWidget *widget1 = new QWidget();
  widget1->setToolTip(tr("widget1"));
  QWidget *widget2 = new QWidget();
  widget1->setToolTip(tr("widget2"));

  QLabel *contentLabel = new QLabel(tr("ContentLabel"));
  QStackedWidget *contentStack = new QStackedWidget();
  contentStack->addWidget(widget1);
  contentStack->addWidget(widget2);

  QVBoxLayout *contentLayout = new QVBoxLayout();
  contentLayout->setMargin(4);
  contentLayout->addWidget(contentLabel);
  contentLayout->addWidget(contentStack);

  QWidget *contentWidget = new QWidget();
  contentWidget->setLayout(contentLayout);

  QSplitter *splitter = new QSplitter();
  splitter->setChildrenCollapsible(false);
  splitter->addWidget(categoriesTree);
  splitter->addWidget(contentWidget);

  connect(categoriesTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
          contentStack, SLOT(slotcategoriesItemCLicked(QTreeWidgetItem*,int)));

  buttonBox_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox_, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox_, SIGNAL(rejected()), this, SLOT(reject()));

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setMargin(4);
  mainLayout->addWidget(splitter);
  mainLayout->addWidget(buttonBox_);

  setLayout(mainLayout);
}

void OptionsDialog::slotCategoriesItemCLicked(QTreeWidgetItem* item, int column)
{
  qDebug() << "clicked" << item << column;
}
