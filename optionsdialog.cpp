#include "optionsdialog.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);

  QTreeWidget *categoriesTree = new QTreeWidget();
  categoriesTree->setObjectName("categoriesTree");
  categoriesTree->setHeaderHidden(true);
  categoriesTree->setColumnCount(3);
  categoriesTree->setColumnHidden(0, true);
  categoriesTree->header()->setStretchLastSection(false);
  categoriesTree->header()->resizeSection(2, 5);
  categoriesTree->header()->setResizeMode(1, QHeaderView::Stretch);
  categoriesTree->setFixedWidth(150);
  QStringList treeItem;
  treeItem << "0" << tr("Network Connections");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "1" << tr("Feeds");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));

  systemProxyButton_ = new QRadioButton(tr("System proxy configuration (if available)"));
//  systemProxyButton_->setEnabled(false);
  directConnectionButton_ = new QRadioButton(tr("Direct connection to the Internet"));
  manualProxyButton_ = new QRadioButton(tr("Manual proxy configuration:"));

  QVBoxLayout *networkConnectionsLayout = new QVBoxLayout();
  networkConnectionsLayout->setMargin(5);
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
  networkConnectionsWidget_->setLayout(networkConnectionsLayout);

  // feeds
  updateFeedsStartUp_ = new QCheckBox(tr("Automatically update the feeds on start-up"));
  updateFeeds_ = new QCheckBox(tr("Automatically update the feeds every"));
  updateFeedsTime_ = new QSpinBox();
  updateFeedsTime_->setEnabled(false);
  updateFeedsTime_->setRange(5, 9999);
  connect(updateFeeds_, SIGNAL(toggled(bool)), updateFeedsTime_, SLOT(setEnabled(bool)));

  QHBoxLayout *updateFeedsLayout = new QHBoxLayout();
  updateFeedsLayout->setMargin(0);
  updateFeedsLayout->addWidget(updateFeeds_);
  updateFeedsLayout->addWidget(updateFeedsTime_);
  updateFeedsLayout->addStretch();

  QVBoxLayout *feedsLayout = new QVBoxLayout();
//  feedsLayout->setMargin(0);
  feedsLayout->addWidget(updateFeedsStartUp_);
  feedsLayout->addLayout(updateFeedsLayout);
  feedsLayout->addStretch();

  QWidget *feedsUpdateWidget_ = new QWidget();
  feedsUpdateWidget_->setLayout(feedsLayout);

  feedsWidget_ = new QTabWidget();
  feedsWidget_->addTab(feedsUpdateWidget_, tr("General"));
  //

  contentLabel_ = new QLabel(tr("ContentLabel"));
  contentLabel_->setObjectName("contentLabel_");

  contentStack_ = new QStackedWidget();
  contentStack_->setObjectName("contentStack_");
  contentStack_->addWidget(networkConnectionsWidget_);
  contentStack_->addWidget(feedsWidget_);

  QVBoxLayout *contentLayout = new QVBoxLayout();
  contentLayout->setMargin(0);
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
  mainLayout->setMargin(5);
  mainLayout->addWidget(splitter);
  mainLayout->addWidget(buttonBox_);

  setLayout(mainLayout);

  connect(categoriesTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
          this, SLOT(slotCategoriesItemCLicked(QTreeWidgetItem*,int)));
  connect(buttonBox_, SIGNAL(accepted()), this, SLOT(acceptSlot()));
  connect(buttonBox_, SIGNAL(rejected()), this, SLOT(reject()));
  connect(manualProxyButton_, SIGNAL(toggled(bool)),
      this, SLOT(manualProxyToggle(bool)));

  categoriesTree->setCurrentItem(categoriesTree->topLevelItem(0));
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
      directConnectionButton_->setChecked(true);
      break;
    case QNetworkProxy::DefaultProxy:
    default:
      systemProxyButton_->setChecked(true);
  }
  editHost_->setText(networkProxy_.hostName());
  editPort_->setText(QVariant(networkProxy_.port()).toString());
  editUser_->setText(networkProxy_.user());
  editPassword_->setText(networkProxy_.password());
}

void OptionsDialog::applyProxy()
{
  if (systemProxyButton_->isChecked())
    networkProxy_.setType(QNetworkProxy::DefaultProxy);
  else if (manualProxyButton_->isChecked())
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
