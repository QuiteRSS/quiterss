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

  systemProxyButton_ = new QRadioButton(tr("System proxy configuration (if available)"));
  systemProxyButton_->setEnabled(false);
  directConnectionButton_ = new QRadioButton(tr("Direct connection to the Internet"));
  manualProxyButton_ = new QRadioButton(tr("Manual proxy configuration:"));

  QVBoxLayout *networkConnectionsLayout = new QVBoxLayout();
  networkConnectionsLayout->setMargin(0);
  networkConnectionsLayout->addWidget(systemProxyButton_);
  networkConnectionsLayout->addWidget(directConnectionButton_);
  networkConnectionsLayout->addWidget(manualProxyButton_);

  editHost_ = new QLineEdit();
  editPort_ = new QLineEdit();
  editPort_->setFixedWidth(60);
  editUser_ = new QLineEdit();
  editPassword_ = new QLineEdit();

  QHBoxLayout *addrPortLayout = new QHBoxLayout();
  addrPortLayout->setMargin(0);
  addrPortLayout->addWidget(new QLabel(tr("Proxy server:")));
  addrPortLayout->addWidget(editHost_);
  addrPortLayout->addWidget(new QLabel(tr("Port:")));
  addrPortLayout->addWidget(editPort_);

  QWidget *addrPortWidget = new QWidget();
  addrPortWidget->setLayout(addrPortLayout);

  QGridLayout *userPasswordLayout = new QGridLayout();
  userPasswordLayout->setMargin(0);
  userPasswordLayout->addWidget(new QLabel(tr("Username:")), 0, 0);
  userPasswordLayout->addWidget(editUser_,                   0, 1);
  userPasswordLayout->addWidget(new QLabel(tr("Password:")), 1, 0);
  userPasswordLayout->addWidget(editPassword_,               1, 1);

  QWidget *userPasswordWidget = new QWidget();
  userPasswordWidget->setLayout(userPasswordLayout);

  QVBoxLayout *manualLayout = new QVBoxLayout();
  manualLayout->setMargin(0);
  manualLayout->addWidget(addrPortWidget);
  manualLayout->addWidget(userPasswordWidget);
  manualLayout->addStretch();

  manualWidget_ = new QWidget();
  manualWidget_->setEnabled(false);  // т.к. при создании соответствующая радио-кнока не выбрана
  // @TODO(arhohryakov:2011.12.08): убрать границу и заголовок группы
  manualWidget_->setLayout(manualLayout);

  networkConnectionsLayout->addWidget(manualWidget_);

  networkConnectionsWidget_ = new QWidget();
  networkConnectionsWidget_->setToolTip(tr("networkConnectionsWidget_"));
  networkConnectionsWidget_->setLayout(networkConnectionsLayout);

  widgetFirst_ = new QWidget();
  widgetFirst_->setToolTip(tr("widgetFirst"));
  widgetSecond_ = new QWidget();
  widgetSecond_->setToolTip(tr("widgetSecond"));

  contentLabel_ = new QLabel(tr("ContentLabel"));

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

  buttonBox_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setMargin(4);
  mainLayout->addWidget(splitter);
  mainLayout->addWidget(buttonBox_);

  setLayout(mainLayout);

  connect(categoriesTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
          this, SLOT(slotCategoriesItemCLicked(QTreeWidgetItem*,int)));
  connect(buttonBox_, SIGNAL(accepted()), this, SLOT(acceptSlot()));
  connect(buttonBox_, SIGNAL(rejected()), this, SLOT(reject()));
  connect(manualProxyButton_, SIGNAL(toggled(bool)),
      this, SLOT(manualProxyToggle(bool)));

  slotCategoriesItemCLicked(categoriesTree->topLevelItem(0), 0);

  // не нужно, т.к. после создания окна из главного окна передаются настройки
//  manualProxyToggle(manualProxyButton_->isChecked());
}

void OptionsDialog::slotCategoriesItemCLicked(QTreeWidgetItem* item, int column)
{
  contentLabel_->setText(item->data(1, Qt::DisplayRole).toString());
  contentStack_->setCurrentIndex(item->data(0, Qt::DisplayRole).toInt());
}

void OptionsDialog::manualProxyToggle(bool checked)
{
  manualWidget_->setEnabled(checked);
}

QNetworkProxy OptionsDialog::proxy()
{
  return networkProxy_;
}

void OptionsDialog::setProxy(const QNetworkProxy proxy)
{
  networkProxy_ = proxy;
  updateProxy();
}

void OptionsDialog::updateProxy()
{
  switch (networkProxy_.type()) {
    case QNetworkProxy::HttpProxy:
      manualProxyButton_->setChecked(true);
      break;
    case QNetworkProxy::NoProxy:
    default:
      directConnectionButton_->setChecked(true);
  }
  editHost_->setText(networkProxy_.hostName());
  editPort_->setText(QVariant(networkProxy_.port()).toString());
  editUser_->setText(networkProxy_.user());
  editPassword_->setText(networkProxy_.password());
}

void OptionsDialog::applyProxy()
{
  if (manualProxyButton_->isChecked())
    networkProxy_.setType(QNetworkProxy::HttpProxy);
  else
    networkProxy_.setType(QNetworkProxy::NoProxy);
  networkProxy_.setHostName(editHost_->text());
  networkProxy_.setPort(    editPort_->text().toInt());
  networkProxy_.setUser(    editUser_->text());
  networkProxy_.setPassword(editPassword_->text());
}

void OptionsDialog::acceptSlot()
{
  applyProxy();
  accept();
}
