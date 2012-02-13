#include "optionsdialog.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Options"));

  categoriesTree = new QTreeWidget();
  categoriesTree->setObjectName("categoriesTree");
  categoriesTree->setHeaderHidden(true);
  categoriesTree->setColumnCount(3);
  categoriesTree->setColumnHidden(0, true);
  categoriesTree->header()->setStretchLastSection(false);
  categoriesTree->header()->resizeSection(2, 5);
  categoriesTree->header()->setResizeMode(1, QHeaderView::Stretch);
  categoriesTree->setMinimumWidth(150);
  QStringList treeItem;
  treeItem << "0" << tr("System tray");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "1" << tr("Network Connections");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "2" << tr("Browser");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "3" << tr("Feeds");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "4" << tr("Notifier");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "5" << tr("Language");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "6" << tr("Fonts");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));

  //{ system tray
  startingTray_ = new QCheckBox(tr("starting QuiteRSS"));
  minimizingTray_ = new QCheckBox(tr("minimizing QuiteRSS"));
  closingTray_ = new QCheckBox(tr("closing QuiteRSS"));
  QVBoxLayout *moveTrayLayout = new QVBoxLayout();
  moveTrayLayout->setContentsMargins(15, 0, 5, 10);
  moveTrayLayout->addWidget(startingTray_);
  moveTrayLayout->addWidget(minimizingTray_);
  moveTrayLayout->addWidget(closingTray_);

  staticIconTray_ = new QRadioButton(tr("Show static icon"));
  changeIconTray_ = new QRadioButton(tr("Change icon for incoming new news"));
  newCountTray_ = new QRadioButton(tr("Show count of new news"));
  unreadCountTray_ = new QRadioButton(tr("Show count of unread news"));
  QVBoxLayout *behaviorLayout = new QVBoxLayout();
  behaviorLayout->setContentsMargins(15, 0, 5, 10);
  behaviorLayout->addWidget(staticIconTray_);
  behaviorLayout->addWidget(changeIconTray_);
  behaviorLayout->addWidget(newCountTray_);
  behaviorLayout->addWidget(unreadCountTray_);

  singleClickTray_ = new QCheckBox(tr("Single click instead of double click for show window"));
  clearStatusNew_ = new QCheckBox(tr("Clear status new on minimize to tray"));
  emptyWorking_ = new QCheckBox(tr("Empty working set on minimize to tray"));

  QVBoxLayout *generalLayout = new QVBoxLayout();
  generalLayout->setMargin(2);
  generalLayout->addWidget(new QLabel(tr("Move to the system tray when:")));
  generalLayout->addLayout(moveTrayLayout);
  generalLayout->addWidget(new QLabel(tr("Tray icon behavior:")));
  generalLayout->addLayout(behaviorLayout);
  generalLayout->addWidget(singleClickTray_);
  generalLayout->addWidget(clearStatusNew_);
#if defined(Q_WS_WIN)
  generalLayout->addWidget(emptyWorking_);
#endif
  generalLayout->addStretch(1);

  generalWidget_ = new QWidget();
  generalWidget_->setLayout(generalLayout);
  //}

  //{ networkConnections
  systemProxyButton_ = new QRadioButton(tr("System proxy configuration (if available)"));
//  systemProxyButton_->setEnabled(false);
  directConnectionButton_ = new QRadioButton(tr("Direct connection to the Internet"));
  manualProxyButton_ = new QRadioButton(tr("Manual proxy configuration:"));

  QVBoxLayout *networkConnectionsLayout = new QVBoxLayout();
  networkConnectionsLayout->setMargin(2);
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
  //} networkConnections

  //{ browser
  embeddedBrowserOn_ = new QCheckBox(tr("Use the embedded browser"));

  QVBoxLayout *browserLayout = new QVBoxLayout();
  browserLayout->setMargin(0);
  browserLayout->addWidget(embeddedBrowserOn_);
  browserLayout->addStretch();

  browserWidget_ = new QWidget();
  browserWidget_->setLayout(browserLayout);
  //}

  //{ feeds
  updateFeedsStartUp_ = new QCheckBox(tr("Automatically update the feeds on start-up"));
  updateFeeds_ = new QCheckBox(tr("Automatically update the feeds every"));
  updateFeedsTime_ = new QSpinBox();
  updateFeedsTime_->setEnabled(false);
  updateFeedsTime_->setRange(5, 9999);
  connect(updateFeeds_, SIGNAL(toggled(bool)), updateFeedsTime_, SLOT(setEnabled(bool)));

  QHBoxLayout *updateFeedsLayout1 = new QHBoxLayout();
  updateFeedsLayout1->setMargin(0);
  updateFeedsLayout1->addWidget(updateFeeds_);
  updateFeedsLayout1->addWidget(updateFeedsTime_);
  updateFeedsLayout1->addWidget(new QLabel(tr("minutes")));
  updateFeedsLayout1->addStretch();

  QVBoxLayout *updateFeedsLayout = new QVBoxLayout();
  updateFeedsLayout->addWidget(updateFeedsStartUp_);
  updateFeedsLayout->addLayout(updateFeedsLayout1);
  updateFeedsLayout->addStretch();

  QWidget *updateFeedsWidget_ = new QWidget();
  updateFeedsWidget_->setLayout(updateFeedsLayout);

  markNewsReadOn_ = new QCheckBox(tr("Mark selected news as read after"));
  markNewsReadTime_ = new QSpinBox();
  markNewsReadTime_->setEnabled(false);
  markNewsReadTime_->setRange(0, 100);
  connect(markNewsReadOn_, SIGNAL(toggled(bool)), markNewsReadTime_, SLOT(setEnabled(bool)));

  QHBoxLayout *readingFeedsLayout1 = new QHBoxLayout();
  readingFeedsLayout1->setMargin(0);
  readingFeedsLayout1->addWidget(markNewsReadOn_);
  readingFeedsLayout1->addWidget(markNewsReadTime_);
  readingFeedsLayout1->addWidget(new QLabel(tr("seconds")));
  readingFeedsLayout1->addStretch();

  QVBoxLayout *readingFeedsLayout = new QVBoxLayout();
  readingFeedsLayout->addLayout(readingFeedsLayout1);
  readingFeedsLayout->addStretch();

  QWidget *readingFeedsWidget_ = new QWidget();
  readingFeedsWidget_->setLayout(readingFeedsLayout);

  feedsWidget_ = new QTabWidget();
  feedsWidget_->addTab(updateFeedsWidget_, tr("General"));
  feedsWidget_->addTab(readingFeedsWidget_, tr("Reading"));
  //} feeds

  //{ notifier
  soundNewNews_ = new QCheckBox(tr("Play sound for incoming new news"));

  QVBoxLayout *notifierLayout = new QVBoxLayout();
  notifierLayout->setMargin(0);
  notifierLayout->addWidget(soundNewNews_);
  notifierLayout->addStretch();

  notifierWidget_ = new QWidget();
  notifierWidget_->setLayout(notifierLayout);
  //} notifier

  //{ language
  languageFileList_ = new QListWidget();
  languageFileList_->setObjectName("languageFileList_");

  QVBoxLayout *languageLayout = new QVBoxLayout();
  languageLayout->setMargin(0);
  languageLayout->addWidget(new QLabel(tr("Choose language:")));
  languageLayout->addWidget(languageFileList_);

  languageWidget_ = new QWidget();
  languageWidget_->setLayout(languageLayout);

  // init list  
  QStringList languageList;
  languageList << QString(tr("English (%1)")).arg("en")
               << QString(tr("Russian (%1)")).arg("ru");
  foreach (QString str, languageList) {
    new QListWidgetItem(str, languageFileList_);
  }
  //} language

  //{ fonts
  fontTree = new QTreeWidget();
  fontTree->setObjectName("fontTree");
  fontTree->setColumnCount(3);
  fontTree->setColumnHidden(0, true);
  fontTree->setColumnWidth(1, 140);

  treeItem.clear();
  treeItem << tr("Id") << tr("Type") << tr("Font");
  fontTree->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << "0" << tr("Feeds list font");
  fontTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "1" << tr("News list font");
  fontTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "2" << tr("News font");
  fontTree->addTopLevelItem(new QTreeWidgetItem(treeItem));

  fontTree->setCurrentItem(fontTree->topLevelItem(0));
  connect(fontTree, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotFontChange()));

  QPushButton *fontChange = new QPushButton(tr("Change..."));
  connect(fontChange, SIGNAL(clicked()), this, SLOT(slotFontChange()));
  QPushButton *fontReset = new QPushButton(tr("Reset"));
  connect(fontReset, SIGNAL(clicked()), this, SLOT(slotFontReset()));
  QVBoxLayout *fontsButtonLayout = new QVBoxLayout();
  fontsButtonLayout->addWidget(fontChange);
  fontsButtonLayout->addWidget(fontReset);
  fontsButtonLayout->addStretch(1);

  QHBoxLayout *fontsLayout = new QHBoxLayout();
  fontsLayout->setMargin(0);
  fontsLayout->addWidget(fontTree, 1);
  fontsLayout->addLayout(fontsButtonLayout);

  fontsWidget_ = new QWidget();
  fontsWidget_->setLayout(fontsLayout);
  //} fonts

  contentLabel_ = new QLabel(tr("ContentLabel"));
  contentLabel_->setObjectName("contentLabel_");

  contentStack_ = new QStackedWidget();
  contentStack_->setObjectName("contentStack_");
  contentStack_->addWidget(generalWidget_);
  contentStack_->addWidget(networkConnectionsWidget_);
  contentStack_->addWidget(browserWidget_);
  contentStack_->addWidget(feedsWidget_);
  contentStack_->addWidget(notifierWidget_);
  contentStack_->addWidget(languageWidget_);
  contentStack_->addWidget(fontsWidget_);

  QVBoxLayout *contentLayout = new QVBoxLayout();
  contentLayout->setMargin(0);
  contentLayout->addWidget(contentLabel_);
  contentLayout->addWidget(contentStack_);

  QWidget *contentWidget = new QWidget();
  contentWidget->setObjectName("contentWidget");
  contentWidget->setLayout(contentLayout);

  QVBoxLayout *categoriesTreeLayout = new QVBoxLayout();
  categoriesTreeLayout->setMargin(0);
  categoriesTreeLayout->addWidget(categoriesTree);
  QWidget *categoriesTreeWidget = new QWidget();
  categoriesTreeWidget->setLayout(categoriesTreeLayout);

  QSplitter *splitter = new QSplitter();
  splitter->setChildrenCollapsible(false);
  splitter->addWidget(categoriesTreeWidget);
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

  // не нужно, т.к. после создания окна из главного окна передаются настройки
//  manualProxyToggle(manualProxyButton_->isChecked());
}

void OptionsDialog::slotCategoriesItemCLicked(QTreeWidgetItem* item, int column)
{
  Q_UNUSED(column)
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

QString OptionsDialog::language()
{
  QString langFileName = languageFileList_->currentItem()->data(Qt::DisplayRole).toString();
  return langFileName.section("(", 1).replace(")", "");
}

void OptionsDialog::setLanguage(QString langFileName)
{
  // установка курсора на выбранный файл
  langFileName = QString("(%1)").arg(langFileName);

  QList<QListWidgetItem*> list =
      languageFileList_->findItems(langFileName, Qt::MatchContains);
  if (list.count()) {
    languageFileList_->setCurrentItem(list.at(0));
  } else {
    // если не удалось найти, выбираем английский
    QList<QListWidgetItem*> list =
        languageFileList_->findItems("(en)", Qt::MatchContains);
    languageFileList_->setCurrentItem(list.at(0));
  }
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

void OptionsDialog::slotFontChange()
{
  bool bOk;
  QFont curFont;
  curFont.setFamily(fontTree->currentItem()->text(2).section(", ", 0, 0));
  curFont.setPointSize(fontTree->currentItem()->text(2).section(", ", 1).toInt());

  QFont font = QFontDialog::getFont(&bOk, curFont);
  if (bOk) {
    QString strFont = QString("%1, %2").
        arg(font.family()).
        arg(font.pointSize());
    fontTree->currentItem()->setText(2, strFont);
  }
}

void OptionsDialog::slotFontReset()
{
  switch (fontTree->currentItem()->text(0).toInt()) {
  case 2: fontTree->currentItem()->setText(
                  2, QString("%1, 12").arg(qApp->font().family()));
    break;
  default: fontTree->currentItem()->setText(
                  2, QString("%1, 8").arg(qApp->font().family()));
  }
}

int OptionsDialog::currentIndex()
{
  return categoriesTree->currentItem()->text(0).toInt();
}

void OptionsDialog::setCurrentItem(int index)
{
  categoriesTree->setCurrentItem(categoriesTree->topLevelItem(index));
  slotCategoriesItemCLicked(categoriesTree->topLevelItem(index), 0);
}

void OptionsDialog::setBehaviorIconTray(int behavior)
{
  switch (behavior) {
  case 1: changeIconTray_->setChecked(true);  break;
  case 2: newCountTray_->setChecked(true);    break;
  case 3: unreadCountTray_->setChecked(true); break;
  default: staticIconTray_->setChecked(true);
  }
}

int OptionsDialog::behaviorIconTray()
{
  if (staticIconTray_->isChecked())       return 0;
  else if (changeIconTray_->isChecked())  return 1;
  else if (newCountTray_->isChecked())    return 2;
  else if (unreadCountTray_->isChecked()) return 3;
  else return 0;
}
