#include "optionsdialog.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent)
{
  QTreeWidget *categoriesTree = new QTreeWidget();
  categoriesTree->setHeaderHidden(true);
  categoriesTree->setColumnCount(2);
  categoriesTree->setColumnHidden(0, true);
  categoriesTree->setFixedWidth(130);
  QStringList treeItem;
  treeItem << "0" << tr("Network Connections");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "1" << tr("First");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "2" << tr("Second");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));

  QVBoxLayout *networkConnectionsLayout = new QVBoxLayout();
  networkConnectionsLayout->setMargin(0);
  networkConnectionsLayout->addWidget(new QRadioButton(tr("System proxy configuration (if available)")));
  networkConnectionsLayout->addWidget(new QRadioButton(tr("Direct connection to the Internet")));
  networkConnectionsLayout->addWidget(new QRadioButton(tr("Manual proxy configuration:")));

  QHBoxLayout *addrPortLayout = new QHBoxLayout();
  addrPortLayout->setMargin(0);
  addrPortLayout->addWidget(new QLabel(tr("Host:")));
  addrPortLayout->addWidget(new QLineEdit(tr("host")));
  addrPortLayout->addWidget(new QLabel(tr("Port:")));
  QLineEdit *portEdit = new QLineEdit(tr("port"));
  portEdit->setFixedWidth(60);
  addrPortLayout->addWidget(portEdit);

  QWidget *addrPortWidget = new QWidget();
  addrPortWidget->setLayout(addrPortLayout);

  QGridLayout *userPasswordLayout = new QGridLayout();
  userPasswordLayout->setMargin(0);
  userPasswordLayout->addWidget(new QLabel(tr("User:")),     0, 0);
  userPasswordLayout->addWidget(new QLineEdit(tr("")),       0, 1);
  userPasswordLayout->addWidget(new QLabel(tr("Password:")), 1, 0);
  userPasswordLayout->addWidget(new QLineEdit(tr("")),       1, 1);

  QWidget *userPasswordWidget = new QWidget();
  userPasswordWidget->setLayout(userPasswordLayout);

  QVBoxLayout *manualLayout = new QVBoxLayout();
  manualLayout->setMargin(0);
  manualLayout->addWidget(addrPortWidget);
  manualLayout->addWidget(userPasswordWidget);
  manualLayout->addStretch();

  QWidget *manualWidget = new QGroupBox(tr("manual proxy settings"));
  // @TODO(arhohryakov:2011.12.08): убрать границу и заголовок группы
  manualWidget->setLayout(manualLayout);

  networkConnectionsLayout->addWidget(manualWidget);

  QHBoxLayout *networkConnectionsButtonsLayout = new QHBoxLayout();
  networkConnectionsButtonsLayout->setMargin(0);
  networkConnectionsButtonsLayout->addStretch();
  networkConnectionsButtonsLayout->addWidget(new QPushButton(tr("Restore defaults")));
  networkConnectionsButtonsLayout->addWidget(new QPushButton(tr("Apply")));
  networkConnectionsLayout->addLayout(networkConnectionsButtonsLayout);

  networkConnectionsWidget_ = new QWidget();
  networkConnectionsWidget_->setToolTip(tr("networkConnectionsWidget_"));
  networkConnectionsWidget_->setLayout(networkConnectionsLayout);

  widgetFirst_ = new QWidget();
  widgetFirst_->setToolTip(tr("widgetFirst"));
  widgetSecond_ = new QWidget();
  widgetSecond_->setToolTip(tr("widgetSecond"));

  contentLabel_ = new QLabel(tr("ContentLabel"));
  contentLabel_->setStyleSheet("border-bottom: 2px solid black;"
      " font-size: 12px ;font-weight: bold");

  contentStack_ = new QStackedWidget();
  contentStack_->addWidget(networkConnectionsWidget_);
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
