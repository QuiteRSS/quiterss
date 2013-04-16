#include "optionsdialog.h"
#include "labeldialog.h"
#include "VersionNo.h"
#include "rsslisting.h"

OptionsDialog::OptionsDialog(QWidget *parent) : Dialog(parent)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Options"));

  db_ = QSqlDatabase::database();

  contentLabel_ = new QLabel();
  contentLabel_->setObjectName("contentLabel_");
  contentLabel_->setAlignment(Qt::AlignCenter);
  contentLabel_->setStyleSheet(
        QString("#contentLabel_ {border-bottom: 1px solid %1;}").
        arg(qApp->palette().color(QPalette::Dark).name()));
  contentLabel_->setMinimumHeight(36);
  contentLabel_->setMargin(4);
  QFont fontContentLabel = contentLabel_->font();
  fontContentLabel.setBold(true);
  fontContentLabel.setPointSize(fontContentLabel.pointSize()+2);
  contentLabel_->setFont(fontContentLabel);
  mainLayout->insertWidget(0, contentLabel_);

  categoriesTree_ = new QTreeWidget();
  categoriesTree_->setObjectName("categoriesTree");
  categoriesTree_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  categoriesTree_->setHeaderHidden(true);
  categoriesTree_->setColumnCount(3);
  categoriesTree_->setColumnHidden(0, true);
  categoriesTree_->header()->setStretchLastSection(false);
  categoriesTree_->header()->resizeSection(2, 5);
  categoriesTree_->header()->setResizeMode(1, QHeaderView::Stretch);
  categoriesTree_->setMinimumWidth(150);
  QStringList treeItem;
  treeItem << "0" << tr("General");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "1" << tr("System Tray");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "2" << tr("Network Connections");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "3" << tr("Browser");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "4" << tr("Feeds");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "5" << tr("Labels");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "6" << tr("Notifications");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "7" << tr("Passwords");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "8" << tr("Language");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "9" << tr("Fonts");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "10" << tr("Keyboard Shortcuts");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));

  createGeneralWidget();

  createTraySystemWidget();

  createNetworkConnectionsWidget();

  createBrowserWidget();

  createFeedsWidget();

  createLabelsWidget();

  createNotifierWidget();

  createPasswordsWidget();

  createLanguageWidget();

  createFontsWidget();

  createShortcutWidget();

  contentStack_ = new QStackedWidget();
  contentStack_->setObjectName("contentStack_");
  contentStack_->addWidget(generalWidget_);
  contentStack_->addWidget(traySystemWidget_);
  contentStack_->addWidget(networkConnectionsWidget_);
  contentStack_->addWidget(browserWidget_);
  contentStack_->addWidget(feedsWidget_);
  contentStack_->addWidget(labelsWidget_);
  contentStack_->addWidget(notifierWidget_);
  contentStack_->addWidget(passwordsWidget_);
  contentStack_->addWidget(languageWidget_);
  contentStack_->addWidget(fontsWidget_);
  contentStack_->addWidget(shortcutWidget_);

  QSplitter *splitter = new QSplitter();
  splitter->setChildrenCollapsible(false);
  splitter->addWidget(categoriesTree_);
  splitter->addWidget(contentStack_);
  QList<int> sizes;
  sizes << 150 << 600;
  splitter->setSizes(sizes);

  pageLayout->addWidget(splitter, 1);
  pageLayout->setContentsMargins(10, 5, 10, 5);

  buttonBox->addButton(QDialogButtonBox::Ok);
  buttonBox->addButton(QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptDialog()));
  connect(this, SIGNAL(finished(int)), this, SLOT(closeDialog()));

  connect(categoriesTree_, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
          this, SLOT(slotCategoriesItemClicked(QTreeWidgetItem*,int)));
  connect(this, SIGNAL(signalCategoriesTreeKeyUpDownPressed()),
          SLOT(slotCategoriesTreeKeyUpDownPressed()), Qt::QueuedConnection);

  categoriesTree_->installEventFilter(this);

  resize(700, 500);

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  restoreGeometry(rssl_->settings_->value("options/geometry").toByteArray());
}

void OptionsDialog::acceptDialog()
{
  applyProxy();
  applyWhitelist();
  applyLabels();
  applyNotifier();
  applyPass();
  accept();
}

void OptionsDialog::closeDialog()
{
  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  rssl_->settings_->setValue("options/geometry", saveGeometry());
}

bool OptionsDialog::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    if (obj != editShortcut_) {
      if ((keyEvent->key() == Qt::Key_Up) ||
          (keyEvent->key() == Qt::Key_Down)) {
        if (obj == categoriesTree_)
          emit signalCategoriesTreeKeyUpDownPressed();
        else if (obj == shortcutTree_)
          emit signalShortcutTreeUpDownPressed();
      }
    } else {
      if ((keyEvent->key() < Qt::Key_Shift) ||
          (keyEvent->key() > Qt::Key_Alt)) {
        QString str;
        if ((keyEvent->modifiers() & Qt::ShiftModifier) ||
            (keyEvent->modifiers() & Qt::ControlModifier) ||
            (keyEvent->modifiers() & Qt::AltModifier))
          str.append(QKeySequence(keyEvent->modifiers()).toString());
        str.append(QKeySequence(keyEvent->key()).toString());
        editShortcut_->setText(str);
        shortcutTree_->currentItem()->setText(3, str);

        QList<QTreeWidgetItem *> treeItems =
            shortcutTree_->findItems(str, Qt::MatchFixedString, 3);
        for (int i = 0; i < treeItems.count(); i++) {
          if ((treeItems.at(i) != shortcutTree_->currentItem()) && !str.isEmpty()) {
            warningShortcut_->setText(tr("Warning: key is already assigned to") +
                                      " \"" + treeItems.at(i)->text(1) + "\"");
          }
          if ((treeItems.count() > 1) && !str.isEmpty()) {
            treeItems.at(i)->setTextColor(3, Qt::red);
          }
        }
      }
      return true;
    }
    return false;
  } else {
    return QDialog::eventFilter(obj, event);
  }
}

/** @brief Создание виджета "Общие"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createGeneralWidget()
{
  showSplashScreen_ = new QCheckBox(tr("Show splash screen on startup"));
  reopenFeedStartup_ = new QCheckBox(tr("Reopen last opened feeds on startup"));
  hideFeedsOpenTab_ = new QCheckBox(tr("Hide feeds tree when opening tabs"));
  defaultIconFeeds_ = new QCheckBox(tr("Show default rss-icon instead of favourite one"));
  autocollapseFolder_ = new QCheckBox(tr("Automatically collapse folders"));

  updateCheckEnabled_ = new QCheckBox(tr("Automatically check for updates"));
  storeDBMemory_ = new QCheckBox(tr("Store a DB in memory (requires program restart)"));

  QVBoxLayout *generalLayout = new QVBoxLayout();
  generalLayout->addWidget(showSplashScreen_);
  generalLayout->addWidget(reopenFeedStartup_);
  generalLayout->addWidget(hideFeedsOpenTab_);
  generalLayout->addWidget(defaultIconFeeds_);
  generalLayout->addWidget(autocollapseFolder_);
  generalLayout->addStretch();
  generalLayout->addWidget(updateCheckEnabled_);
  generalLayout->addWidget(storeDBMemory_);

  generalWidget_ = new QFrame();
  generalWidget_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  generalWidget_->setLayout(generalLayout);
}

/** @brief Создание виджета "Системный трей"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createTraySystemWidget()
{
  showTrayIconBox_ = new QGroupBox(tr("Show system tray icon"));
  showTrayIconBox_->setCheckable(true);
  showTrayIconBox_->setChecked(false);

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

  singleClickTray_ = new QCheckBox(
        tr("Single click instead of double click for show window"));
  clearStatusNew_ = new QCheckBox(tr("Clear status new on minimize to tray"));
  emptyWorking_ = new QCheckBox(tr("Empty working set on minimize to tray"));

  QVBoxLayout *trayLayout = new QVBoxLayout();
  trayLayout->addWidget(new QLabel(tr("Move to the system tray when:")));
  trayLayout->addLayout(moveTrayLayout);
  trayLayout->addWidget(new QLabel(tr("Tray icon behavior:")));
  trayLayout->addLayout(behaviorLayout);
  trayLayout->addWidget(singleClickTray_);
  trayLayout->addWidget(clearStatusNew_);
#if defined(Q_WS_WIN)
  trayLayout->addWidget(emptyWorking_);
#endif
  trayLayout->addStretch(1);

  showTrayIconBox_->setLayout(trayLayout);

  QVBoxLayout *boxTrayLayout = new QVBoxLayout();
  boxTrayLayout->setMargin(5);
  boxTrayLayout->addWidget(showTrayIconBox_);

  traySystemWidget_ = new QFrame();
  traySystemWidget_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  traySystemWidget_->setLayout(boxTrayLayout);
}

/** @brief Создание виджета "Сетевые подключения"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createNetworkConnectionsWidget()
{
  systemProxyButton_ = new QRadioButton(
        tr("System proxy configuration (if available)"));
  directConnectionButton_ = new QRadioButton(
        tr("Direct connection to the Internet"));
  manualProxyButton_ = new QRadioButton(tr("Manual proxy configuration:"));

  QVBoxLayout *networkConnectionsLayout = new QVBoxLayout();
  networkConnectionsLayout->addWidget(systemProxyButton_);
  networkConnectionsLayout->addWidget(directConnectionButton_);
  networkConnectionsLayout->addWidget(manualProxyButton_);

  editHost_ = new LineEdit();
  editPort_ = new LineEdit();
  editPort_->setFixedWidth(60);
  editUser_ = new LineEdit();
  editPassword_ = new LineEdit();
  editPassword_->setEchoMode(QLineEdit::Password);

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
  manualWidget_->setLayout(manualLayout);

  networkConnectionsLayout->addWidget(manualWidget_);

  networkConnectionsWidget_ = new QFrame();
  networkConnectionsWidget_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  networkConnectionsWidget_->setLayout(networkConnectionsLayout);

  connect(manualProxyButton_, SIGNAL(toggled(bool)),
          this, SLOT(manualProxyToggle(bool)));
}

/** @brief Создание виджета "Браузер"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createBrowserWidget()
{
  //! tab "General"
  embeddedBrowserOn_ = new QRadioButton(tr("Use embedded browser"));
  externalBrowserOn_ = new QRadioButton(tr("Use external browser"));
  defaultExternalBrowserOn_ = new QRadioButton(tr("Default external browser"));
  otherExternalBrowserOn_ = new QRadioButton(tr("Following external browser:"));

  otherExternalBrowserEdit_ = new LineEdit();
  otherExternalBrowserButton_ = new QPushButton(tr("Browse..."));

  javaScriptEnable_ = new QCheckBox(tr("Enable JavaScript"));
  pluginsEnable_ = new QCheckBox(tr("Enable plug-ins"));

  openLinkInBackgroundEmbedded_ = new QCheckBox(tr("Open links in embedded browser in background"));
  openLinkInBackground_ = new QCheckBox(tr("Open links in external browser in background (experimental)"));

  userStyleBrowserEdit_ = new LineEdit();
  QPushButton *userStyleBrowserButton = new QPushButton(tr("Browse..."));
  connect(userStyleBrowserButton, SIGNAL(clicked()),
          this, SLOT(selectionUserStyleBrowser()));

  QGridLayout *browserSelectionLayout = new QGridLayout();
  browserSelectionLayout->setContentsMargins(15, 0, 5, 10);
  browserSelectionLayout->addWidget(embeddedBrowserOn_, 0, 0);
  browserSelectionLayout->addWidget(externalBrowserOn_, 1, 0);
  QButtonGroup *browserSelectionBox = new QButtonGroup();
  browserSelectionBox->addButton(embeddedBrowserOn_);
  browserSelectionBox->addButton(externalBrowserOn_);

  QGridLayout *externalBrowserLayout = new QGridLayout();
  externalBrowserLayout->setContentsMargins(15, 0, 5, 10);
  externalBrowserLayout->addWidget(defaultExternalBrowserOn_, 0, 0);
  externalBrowserLayout->addWidget(otherExternalBrowserOn_, 1, 0);
  externalBrowserLayout->addWidget(otherExternalBrowserEdit_, 2, 0);
  externalBrowserLayout->addWidget(otherExternalBrowserButton_, 2, 1, Qt::AlignRight);
  QButtonGroup *externalBrowserBox = new QButtonGroup();
  externalBrowserBox->addButton(defaultExternalBrowserOn_);
  externalBrowserBox->addButton(otherExternalBrowserOn_);

  QVBoxLayout *contentBrowserLayout = new QVBoxLayout();
  contentBrowserLayout->setContentsMargins(15, 0, 5, 10);
  contentBrowserLayout->addWidget(javaScriptEnable_);
  contentBrowserLayout->addWidget(pluginsEnable_);

  QGridLayout *userStyleBrowserLayout = new QGridLayout();
  userStyleBrowserLayout->setContentsMargins(15, 0, 5, 10);
  userStyleBrowserLayout->addWidget(userStyleBrowserEdit_, 0, 0);
  userStyleBrowserLayout->addWidget(userStyleBrowserButton, 0, 1, Qt::AlignRight);

  QVBoxLayout *browserLayoutV = new QVBoxLayout();
  browserLayoutV->addWidget(new QLabel(tr("Browser selection:")));
  browserLayoutV->addLayout(browserSelectionLayout);
  browserLayoutV->addWidget(new QLabel(tr("External browser:")));
  browserLayoutV->addLayout(externalBrowserLayout);
  browserLayoutV->addWidget(new QLabel(tr("Content:")));
  browserLayoutV->addLayout(contentBrowserLayout);
  browserLayoutV->addWidget(new QLabel(tr("User style sheet:")));
  browserLayoutV->addLayout(userStyleBrowserLayout);
  browserLayoutV->addWidget(openLinkInBackgroundEmbedded_);
  browserLayoutV->addWidget(openLinkInBackground_);
  browserLayoutV->addStretch();

  QWidget *generalBrowserWidget = new QWidget();
  generalBrowserWidget->setLayout(browserLayoutV);

  connect(otherExternalBrowserOn_, SIGNAL(toggled(bool)),
          otherExternalBrowserEdit_, SLOT(setEnabled(bool)));
  connect(otherExternalBrowserOn_, SIGNAL(toggled(bool)),
          otherExternalBrowserButton_, SLOT(setEnabled(bool)));
  otherExternalBrowserOn_->setChecked(true);

  connect(otherExternalBrowserButton_, SIGNAL(clicked()),
          this, SLOT(selectionBrowser()));

#if !(defined(Q_WS_WIN) || defined(Q_WS_X11))
  otherExternalBrowserOn_->setVisible(false);
  otherExternalBrowserEdit_->setVisible(false);
  otherExternalBrowserButton_->setVisible(false);
#endif

  //! tab "History"
  maxPagesInCache_ = new QSpinBox();
  maxPagesInCache_->setRange(0, 20);

  QHBoxLayout *historyLayout1 = new QHBoxLayout();
  historyLayout1->addWidget(new QLabel(tr("Maximum pages in cache")));
  historyLayout1->addWidget(maxPagesInCache_);
  historyLayout1->addStretch();

  dirDiskCacheEdit_ = new LineEdit();
  dirDiskCacheButton_ = new QPushButton(tr("Browse..."));

  connect(dirDiskCacheButton_, SIGNAL(clicked()),
          this, SLOT(selectionDirDiskCache()));

  QHBoxLayout *historyLayout2 = new QHBoxLayout();
  historyLayout2->addWidget(new QLabel(tr("Store cache in:")));
  historyLayout2->addWidget(dirDiskCacheEdit_, 1);
  historyLayout2->addWidget(dirDiskCacheButton_);

  maxDiskCache_ = new QSpinBox();
  maxDiskCache_->setRange(10, 99);

  QHBoxLayout *historyLayout3 = new QHBoxLayout();
  historyLayout3->addWidget(new QLabel(tr("Maximum size of disk cache")));
  historyLayout3->addWidget(maxDiskCache_);
  historyLayout3->addWidget(new QLabel(tr("MB")), 1);

  QVBoxLayout *historyLayout4 = new QVBoxLayout();
  historyLayout4->addLayout(historyLayout2);
  historyLayout4->addLayout(historyLayout3);

  diskCacheOn_ = new QGroupBox(tr("Use disk cache"));
  diskCacheOn_->setCheckable(true);
  diskCacheOn_->setChecked(false);
  diskCacheOn_->setLayout(historyLayout4);

  QVBoxLayout *historyMainLayout = new QVBoxLayout();
  historyMainLayout->addLayout(historyLayout1);
  historyMainLayout->addWidget(diskCacheOn_);
  historyMainLayout->addStretch();

  QWidget *historyBrowserWidget_ = new QWidget();
  historyBrowserWidget_->setLayout(historyMainLayout);

  //! tab "Click to Flash"
  QLabel *c2fInfo = new QLabel(tr("Click To Flash is a plugin which blocks auto loading of "
                                 "Flash content at page. You can always load it manually "
                                 "by clicking on the Flash play icon."));
  c2fInfo->setWordWrap(true);

  c2fEnabled_ = new QCheckBox(tr("Use Click to Flash"));
  c2fEnabled_->setChecked(false);

  c2fWhitelist_ = new QTreeWidget(this);
  c2fWhitelist_->setObjectName("c2fWhiteList_");
  c2fWhitelist_->setRootIsDecorated(false);
  c2fWhitelist_->setColumnCount(1);

  QStringList treeItem;
  treeItem << "Whitelist";
  c2fWhitelist_->setHeaderLabels(treeItem);

  QPushButton *addButton = new QPushButton(tr("Add..."), this);
  connect(addButton, SIGNAL(clicked()), this, SLOT(addWhitelist()));
  QPushButton *removeButton = new QPushButton(tr("Remove..."), this);
  connect(removeButton, SIGNAL(clicked()), this, SLOT(removeWhitelist()));

  QVBoxLayout *click2FlashLayout1 = new QVBoxLayout();
  click2FlashLayout1->addWidget(addButton);
  click2FlashLayout1->addWidget(removeButton);
  click2FlashLayout1->addStretch(1);

  QHBoxLayout *click2FlashLayout2 = new QHBoxLayout();
  click2FlashLayout2->setMargin(0);
  click2FlashLayout2->addWidget(c2fWhitelist_, 1);
  click2FlashLayout2->addLayout(click2FlashLayout1);

  QWidget *c2fWhitelistWidget = new QWidget(this);
  c2fWhitelistWidget->setLayout(click2FlashLayout2);
  c2fWhitelistWidget->setEnabled(false);

  connect(c2fEnabled_, SIGNAL(toggled(bool)),
          c2fWhitelistWidget, SLOT(setEnabled(bool)));

  QVBoxLayout *click2FlashLayout = new QVBoxLayout();
  click2FlashLayout->addWidget(c2fInfo);
  click2FlashLayout->addWidget(c2fEnabled_);
  click2FlashLayout->addWidget(c2fWhitelistWidget, 1);

  QWidget *click2FlashWidget_ = new QWidget(this);
  click2FlashWidget_->setLayout(click2FlashLayout);

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  c2fEnabled_->setChecked(rssl_->c2fEnabled_);
  foreach(const QString & site, rssl_->c2fWhitelist_) {
    QTreeWidgetItem* item = new QTreeWidgetItem(c2fWhitelist_);
    item->setText(0, site);
  }

  browserWidget_ = new QTabWidget();
  browserWidget_->addTab(generalBrowserWidget, tr("General"));
  browserWidget_->addTab(historyBrowserWidget_, tr("History"));
  browserWidget_->addTab(click2FlashWidget_, tr("Click to Flash"));
}

/** @brief Создание виджета "Новостные ленты"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createFeedsWidget()
{
//! tab "General"
  updateFeedsStartUp_ = new QCheckBox(
        tr("Automatically update the feeds on startup"));
  updateFeeds_ = new QCheckBox(tr("Automatically update the feeds every"));
  updateFeedsTime_ = new QSpinBox();
  updateFeedsTime_->setEnabled(false);
  updateFeedsTime_->setRange(1, 9999);
  connect(updateFeeds_, SIGNAL(toggled(bool)),
          updateFeedsTime_, SLOT(setEnabled(bool)));

  intervalTime_ = new QComboBox(this);
  intervalTime_->setEnabled(false);
  QStringList intervalList;
  intervalList << tr("seconds") << tr("minutes")  << tr("hours");
  intervalTime_->addItems(intervalList);
  connect(updateFeeds_, SIGNAL(toggled(bool)),
          intervalTime_, SLOT(setEnabled(bool)));

  QHBoxLayout *updateFeedsLayout = new QHBoxLayout();
  updateFeedsLayout->setMargin(0);
  updateFeedsLayout->addWidget(updateFeeds_);
  updateFeedsLayout->addWidget(updateFeedsTime_);
  updateFeedsLayout->addWidget(intervalTime_);
  updateFeedsLayout->addStretch();

  positionLastNews_ = new QRadioButton(tr("Position on last opened news"));
  positionFirstNews_ = new QRadioButton(tr("Position at top of list news"));
  positionUnreadNews_ = new QRadioButton(tr("Position on unread news"));
  openNewsWebViewOn_ = new QCheckBox(tr("Open news"));
  nottoOpenNews_ = new QRadioButton(tr("Nothing to do"));
  connect(nottoOpenNews_, SIGNAL(toggled(bool)),
          openNewsWebViewOn_, SLOT(setDisabled(bool)));

  QGridLayout *openingFeedsLayout = new QGridLayout();
  openingFeedsLayout->setContentsMargins(15, 0, 0, 10);
  openingFeedsLayout->setColumnStretch(1, 1);
  openingFeedsLayout->addWidget(positionLastNews_, 0, 0, 1, 1);
  openingFeedsLayout->addWidget(openNewsWebViewOn_, 0, 1, 1, 1);
  openingFeedsLayout->addWidget(positionFirstNews_, 1, 0, 1, 1);
  openingFeedsLayout->addWidget(positionUnreadNews_, 2, 0, 1, 1);
  openingFeedsLayout->addWidget(nottoOpenNews_, 3, 0, 1, 1);

  QStringList itemList;
  itemList << "31.12.99" << "31.12.1999"
           << QString("31. %1. 1999").arg(tr("Dec"))
           << QString("31. %1 1999").arg(tr("December"))
           << "99-12-31" << "1999-12-31" << "12/31/1999";
  formatDate_ = new QComboBox(this);
  formatDate_->addItems(itemList);
  formatDate_->setItemData(0, "dd.MM.yy");
  formatDate_->setItemData(1, "dd.MM.yyyy");
  formatDate_->setItemData(2, "dd. MMM. yyyy");
  formatDate_->setItemData(3, "dd. MMMM yyyy");
  formatDate_->setItemData(4, "yy-MM-dd");
  formatDate_->setItemData(5, "yyyy-MM-dd");
  formatDate_->setItemData(6, "MM/dd/yyyy");

  itemList.clear();
  itemList << "13:37"  << "13:37:09" << "01:37 PM"<< "01:37:09 PM";
  formatTime_ = new QComboBox(this);
  formatTime_->addItems(itemList);
  formatTime_->setItemData(0, "hh:mm");
  formatTime_->setItemData(1, "hh:mm:ss");
  formatTime_->setItemData(2, "hh:mm AP");
  formatTime_->setItemData(3, "hh:mm:ss AP");

  QHBoxLayout *formatDateLayout = new QHBoxLayout();
  formatDateLayout->setMargin(0);
  formatDateLayout->addWidget(new QLabel(tr("Display format for date:")));
  formatDateLayout->addWidget(formatDate_);
  formatDateLayout->addSpacing(10);
  formatDateLayout->addWidget(new QLabel(tr("time:")));
  formatDateLayout->addWidget(formatTime_);
  formatDateLayout->addStretch();

  alternatingRowColorsNews_ = new QCheckBox(tr("Alternating row background colors"));
  simplifiedDateTime_ = new QCheckBox(tr("Simplified representation of date and time"));

  QVBoxLayout *generalFeedsLayout = new QVBoxLayout();
  generalFeedsLayout->addWidget(updateFeedsStartUp_);
  generalFeedsLayout->addLayout(updateFeedsLayout);
  generalFeedsLayout->addSpacing(10);
  generalFeedsLayout->addWidget(new QLabel(tr("Opening feed:")));
  generalFeedsLayout->addLayout(openingFeedsLayout);
  generalFeedsLayout->addWidget(alternatingRowColorsNews_);
  generalFeedsLayout->addSpacing(10);
  generalFeedsLayout->addWidget(simplifiedDateTime_);
  generalFeedsLayout->addLayout(formatDateLayout);
  generalFeedsLayout->addStretch();

  QWidget *generalFeedsWidget = new QWidget();
  generalFeedsWidget->setLayout(generalFeedsLayout);

//! tab "Reading"
  markNewsReadOn_ = new QGroupBox(tr("Mark news as read:"));
  markNewsReadOn_->setCheckable(true);
  markCurNewsRead_ = new QRadioButton(tr("on selecting. With timeout"));
  markPrevNewsRead_ = new QRadioButton(tr("after switching to another news"));
  markNewsReadTime_ = new QSpinBox();
  markNewsReadTime_->setEnabled(false);
  markNewsReadTime_->setRange(0, 100);
  connect(markCurNewsRead_, SIGNAL(toggled(bool)),
          markNewsReadTime_, SLOT(setEnabled(bool)));
  markReadSwitchingFeed_ = new QCheckBox(tr("Mark displayed news as read when switching feeds"));
  markReadClosingTab_ = new QCheckBox(tr("Mark displayed news as read when closing tab"));
  markReadMinimize_ = new QCheckBox(tr("Mark displayed news as read on minimize"));

  showDescriptionNews_ = new QCheckBox(
        tr("Show news' description instead of loading web page"));

  changeBehaviorActionNUN_ = new QCheckBox(tr("Change behavior of action 'Next Unread News'"));

  notDeleteStarred_ = new QCheckBox(tr("Not delete starred news"));
  notDeleteLabeled_ = new QCheckBox(tr("Not delete labeled news"));

  QHBoxLayout *readingFeedsLayout1 = new QHBoxLayout();
  readingFeedsLayout1->setMargin(0);
  readingFeedsLayout1->addWidget(markCurNewsRead_);
  readingFeedsLayout1->addWidget(markNewsReadTime_);
  readingFeedsLayout1->addWidget(new QLabel(tr("seconds")));
  readingFeedsLayout1->addStretch();

  QVBoxLayout *readingFeedsLayout2 = new QVBoxLayout();
  readingFeedsLayout2->addLayout(readingFeedsLayout1);
  readingFeedsLayout2->addWidget(markPrevNewsRead_);
  markNewsReadOn_->setLayout(readingFeedsLayout2);

  QVBoxLayout *readingFeedsLayout = new QVBoxLayout();
  readingFeedsLayout->addWidget(markNewsReadOn_);
  readingFeedsLayout->addWidget(markReadSwitchingFeed_);
  readingFeedsLayout->addWidget(markReadClosingTab_);
  readingFeedsLayout->addWidget(markReadMinimize_);
  readingFeedsLayout->addSpacing(10);
  readingFeedsLayout->addWidget(showDescriptionNews_);
  readingFeedsLayout->addSpacing(10);
  readingFeedsLayout->addWidget(notDeleteStarred_);
  readingFeedsLayout->addWidget(notDeleteLabeled_);
  readingFeedsLayout->addWidget(changeBehaviorActionNUN_);
  readingFeedsLayout->addStretch();

  QWidget *readingFeedsWidget = new QWidget();
  readingFeedsWidget->setLayout(readingFeedsLayout);

//! tab "Clean Up"
  dayCleanUpOn_ = new QCheckBox(tr("Maximum age of news in days to keep:"));
  maxDayCleanUp_ = new QSpinBox();
  maxDayCleanUp_->setEnabled(false);
  maxDayCleanUp_->setRange(0, 9999);
  connect(dayCleanUpOn_, SIGNAL(toggled(bool)),
          maxDayCleanUp_, SLOT(setEnabled(bool)));

  newsCleanUpOn_ = new QCheckBox(tr("Maximum number of news to keep:"));
  maxNewsCleanUp_ = new QSpinBox();
  maxNewsCleanUp_->setEnabled(false);
  maxNewsCleanUp_->setRange(0, 9999);
  connect(newsCleanUpOn_, SIGNAL(toggled(bool)),
          maxNewsCleanUp_, SLOT(setEnabled(bool)));

  readCleanUp_ = new QCheckBox(tr("Delete read news"));
  neverUnreadCleanUp_ = new QCheckBox(tr("Never delete unread news"));
  neverStarCleanUp_ = new QCheckBox(tr("Never delete starred news"));
  neverLabelCleanUp_ = new QCheckBox(tr("Never delete labeled news"));

  QGridLayout *cleanUpFeedsLayout = new QGridLayout();
  cleanUpFeedsLayout->setColumnStretch(1, 1);
  cleanUpFeedsLayout->setRowStretch(6, 1);
  cleanUpFeedsLayout->addWidget(dayCleanUpOn_, 0, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(maxDayCleanUp_, 0, 1, 1, 1, Qt::AlignLeft);
  cleanUpFeedsLayout->addWidget(newsCleanUpOn_, 1, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(maxNewsCleanUp_, 1, 1, 1, 1, Qt::AlignLeft);
  cleanUpFeedsLayout->addWidget(readCleanUp_, 2, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(neverUnreadCleanUp_, 3, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(neverStarCleanUp_, 4, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(neverLabelCleanUp_, 5, 0, 1, 1);

  QWidget *cleanUpFeedsWidget = new QWidget();
  cleanUpFeedsWidget->setLayout(cleanUpFeedsLayout);

  feedsWidget_ = new QTabWidget();
  feedsWidget_->addTab(generalFeedsWidget, tr("General"));
  feedsWidget_->addTab(readingFeedsWidget, tr("Reading"));
  feedsWidget_->addTab(cleanUpFeedsWidget, tr("Clean Up"));
}

/** @brief Создание виджета "Метки"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createLabelsWidget()
{
  labelsTree_ = new QTreeWidget(this);
  labelsTree_->setObjectName("labelsTree_");
  labelsTree_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  labelsTree_->setColumnCount(5);
  labelsTree_->setColumnHidden(0, true);
  labelsTree_->setColumnHidden(2, true);
  labelsTree_->setColumnHidden(3, true);
  labelsTree_->setColumnHidden(4, true);
  labelsTree_->header()->hide();

  newLabelButton_ = new QPushButton(tr("New..."), this);
  connect(newLabelButton_, SIGNAL(clicked()), this, SLOT(newLabel()));
  editLabelButton_ = new QPushButton(tr("Edit..."), this);
  editLabelButton_->setEnabled(false);
  connect(editLabelButton_, SIGNAL(clicked()), this, SLOT(editLabel()));
  deleteLabelButton_ = new QPushButton(tr("Delete..."), this);
  deleteLabelButton_->setEnabled(false);
  connect(deleteLabelButton_, SIGNAL(clicked()), this, SLOT(deleteLabel()));

  moveUpLabelButton_ = new QPushButton(tr("Move up"), this);
  moveUpLabelButton_->setEnabled(false);
  connect(moveUpLabelButton_, SIGNAL(clicked()), this, SLOT(moveUpLabel()));
  moveDownLabelButton_ = new QPushButton(tr("Move down"), this);
  moveDownLabelButton_->setEnabled(false);
  connect(moveDownLabelButton_, SIGNAL(clicked()), this, SLOT(moveDownLabel()));

  QVBoxLayout *buttonsLayout = new QVBoxLayout();
  buttonsLayout->addWidget(newLabelButton_);
  buttonsLayout->addWidget(editLabelButton_);
  buttonsLayout->addWidget(deleteLabelButton_);
  buttonsLayout->addSpacing(10);
  buttonsLayout->addWidget(moveUpLabelButton_);
  buttonsLayout->addWidget(moveDownLabelButton_);
  buttonsLayout->addStretch();

  QHBoxLayout *labelsLayout = new QHBoxLayout();
  labelsLayout->setMargin(0);
  labelsLayout->addWidget(labelsTree_);
  labelsLayout->addLayout(buttonsLayout);

  labelsWidget_ = new QWidget(this);
  labelsWidget_->setLayout(labelsLayout);

  connect(labelsTree_, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this, SLOT(slotCurrentLabelChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(labelsTree_, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(editLabel()));

  loadLabelsOk_ = false;
}

/** @brief Создание виджета "Уведомления"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createNotifierWidget()
{
  soundNewNews_ = new QCheckBox(tr("Play sound for incoming new news"));
  soundNewNews_->setChecked(true);
  editSoundNotifer_ = new LineEdit();
  selectionSoundNotifer_ = new QPushButton(tr("Browse..."));

  connect(soundNewNews_, SIGNAL(toggled(bool)),
          editSoundNotifer_, SLOT(setEnabled(bool)));
  connect(soundNewNews_, SIGNAL(toggled(bool)),
          selectionSoundNotifer_, SLOT(setEnabled(bool)));
  connect(selectionSoundNotifer_, SIGNAL(clicked()),
          this, SLOT(selectionSoundNotifer()));

  QHBoxLayout *notifierLayout0 = new QHBoxLayout();
  notifierLayout0->addWidget(soundNewNews_);
  notifierLayout0->addWidget(editSoundNotifer_, 1);
  notifierLayout0->addWidget(selectionSoundNotifer_);

  showNotifyOn_ = new QGroupBox(tr("Display notification for incoming news"));
  showNotifyOn_->setCheckable(true);
  showNotifyOn_->setChecked(false);

  countShowNewsNotify_ = new QSpinBox();
  countShowNewsNotify_->setRange(1, 30);
  widthTitleNewsNotify_ = new QSpinBox();
  widthTitleNewsNotify_->setRange(50, 500);
  timeShowNewsNotify_ = new QSpinBox();
  timeShowNewsNotify_->setRange(1, 99);

  QHBoxLayout *notifierLayout1 = new QHBoxLayout();
  notifierLayout1->addWidget(new QLabel(tr("Show maximum of")));
  notifierLayout1->addWidget(countShowNewsNotify_);
  notifierLayout1->addWidget(new QLabel(tr("news on page notification")), 1);

  QHBoxLayout *notifierLayout2 = new QHBoxLayout();
  notifierLayout2->addWidget(new QLabel(tr("Width news list")));
  notifierLayout2->addWidget(widthTitleNewsNotify_);
  notifierLayout2->addWidget(new QLabel(tr("pixels")), 1);

  QHBoxLayout *notifierLayout3 = new QHBoxLayout();
  notifierLayout3->addWidget(new QLabel(tr("Close notification after")));
  notifierLayout3->addWidget(timeShowNewsNotify_);
  notifierLayout3->addWidget(new QLabel(tr("seconds")), 1);

  onlySelectedFeeds_ = new QCheckBox(tr("Only show selected feeds:"));

  feedsTreeNotify_ = new QTreeWidget(this);
  feedsTreeNotify_->setObjectName("feedsTreeNotify_");
  feedsTreeNotify_->setColumnCount(2);
  feedsTreeNotify_->setColumnHidden(1, true);
  feedsTreeNotify_->header()->hide();
  feedsTreeNotify_->setEnabled(false);

  itemNotChecked_ = false;

  QStringList treeItem;
  treeItem << "Feeds" << "Id";
  feedsTreeNotify_->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << tr("All Feeds") << "0";
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setCheckState(0, Qt::Checked);
  feedsTreeNotify_->addTopLevelItem(treeWidgetItem);
  connect(feedsTreeNotify_, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
          this, SLOT(feedsTreeNotifyItemChanged(QTreeWidgetItem*,int)));

  QVBoxLayout *notificationLayout = new QVBoxLayout();
  notificationLayout->addLayout(notifierLayout1);
  notificationLayout->addLayout(notifierLayout2);
  notificationLayout->addLayout(notifierLayout3);
  notificationLayout->addWidget(onlySelectedFeeds_);
  notificationLayout->addWidget(feedsTreeNotify_, 1);

  connect(onlySelectedFeeds_, SIGNAL(toggled(bool)),
          feedsTreeNotify_, SLOT(setEnabled(bool)));

  showNotifyOn_->setLayout(notificationLayout);

  QVBoxLayout *notifierMainLayout = new QVBoxLayout();
  notifierMainLayout->addLayout(notifierLayout0);
  notifierMainLayout->addSpacing(5);
  notifierMainLayout->addWidget(showNotifyOn_, 1);

  notifierWidget_ = new QFrame();
  notifierWidget_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  notifierWidget_->setLayout(notifierMainLayout);

  loadNotifierOk_ = false;
}

/** @brief Создание виджета "Пароли"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createPasswordsWidget()
{
  passTree_ = new QTreeWidget(this);
  passTree_->setObjectName("labelsTree_");
  passTree_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  passTree_->setColumnCount(4);
  passTree_->setColumnHidden(0, true);
  passTree_->setColumnHidden(3, true);
  passTree_->setColumnWidth(1, 250);
  passTree_->header()->setMinimumSectionSize(22);

  QStringList strTreeItem;
  strTreeItem.clear();
  strTreeItem << "Id" << tr("Site") << tr("User") << tr("Password");
  passTree_->setHeaderLabels(strTreeItem);

  QSqlQuery q;
  q.exec("SELECT id, server, username, password FROM passwords");
  while (q.next()) {
    QString id = q.value(0).toString();
    QString server = q.value(1).toString();
    QString user = q.value(2).toString();
    QString pass = QString::fromUtf8(QByteArray::fromBase64(q.value(3).toByteArray()));

    strTreeItem.clear();
    strTreeItem << id << server << user << pass;
    QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(strTreeItem);
    passTree_->addTopLevelItem(treeWidgetItem);
  }

  QPushButton *deletePass = new QPushButton(tr("Delete"));
  connect(deletePass, SIGNAL(clicked()), this, SLOT(slotDeletePass()));
  QPushButton *deleteAllPass = new QPushButton(tr("Delete All"));
  connect(deleteAllPass, SIGNAL(clicked()), this, SLOT(slotDeleteAllPass()));
  QPushButton *showPass = new QPushButton(tr("Show Passwords"));
  connect(showPass, SIGNAL(clicked()), this, SLOT(slotShowPass()));
  QVBoxLayout *passButtonLayout = new QVBoxLayout();
  passButtonLayout->addWidget(deletePass);
  passButtonLayout->addWidget(deleteAllPass);
  passButtonLayout->addSpacing(10);
  passButtonLayout->addWidget(showPass);
  passButtonLayout->addStretch(1);

  QHBoxLayout *passLayout = new QHBoxLayout();
  passLayout->setMargin(0);
  passLayout->addWidget(passTree_);
  passLayout->addLayout(passButtonLayout);

  passwordsWidget_ = new QWidget();
  passwordsWidget_->setLayout(passLayout);
}

/** @brief Создание виджета "Язык"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createLanguageWidget()
{
  languageFileList_ = new QTreeWidget();
  languageFileList_->setObjectName("languageFileList_");
  languageFileList_->setColumnCount(5);
  languageFileList_->setColumnHidden(0, true);
  languageFileList_->setColumnWidth(1, 180);
  languageFileList_->setColumnWidth(2, 60);
  languageFileList_->setColumnWidth(3, 120);

  QStringList treeItem;
  treeItem.clear();
  treeItem << "Id" << tr("Language") << tr("Version")
           << tr("Author") << tr("Contact");
  languageFileList_->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << "en" << QString::fromUtf8("English [EN]")
           << QString(STRPRODUCTVER)
           << "QuiteRSS Team" << "";
  QTreeWidgetItem *languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_EN"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "cs_CZ" << QString::fromUtf8("Čeština [cs_CZ]")
           << QString(STRPRODUCTVER)
           << QString::fromUtf8("Matej Szendi") << "matej.szendi@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_CZ"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "de" << QString::fromUtf8("Deutsch [DE]")
           << QString(STRPRODUCTVER)
           << "Lyudmila Kremova" << "alis-dcm@yandex.ru";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_DE"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "el_GR" << QString::fromUtf8("Ελληνικά (Greece) [el_GR]")
           << QString(STRPRODUCTVER)
           << "Dimitris Siakavelis" << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_GR"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "es" << QString::fromUtf8("Español [ES]")
           << "0.12.3"
           << QString::fromUtf8("Cesar Muñoz") << "csarg@live.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_ES"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "fa" << QString::fromUtf8("فارسی [FA]")
           << QString(STRPRODUCTVER)
           << "H.Mohamadi" << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_FA"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "fr" << QString::fromUtf8("Français [FR]")
           << QString(STRPRODUCTVER)
           << "Glad Deschrijver" << "glad.deschrijver@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_FR"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "hu" << QString::fromUtf8("Magyar [HU]")
           << QString(STRPRODUCTVER)
           << "ZityiSoft" << "zityisoft@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_HU"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "it" << QString::fromUtf8("Italiano [IT]")
           << QString(STRPRODUCTVER)
           << "ZeroWis" << "lightflash@hotmail.it";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_IT"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "ja" << QString::fromUtf8("日本語 [JA]")
           << QString(STRPRODUCTVER)
           << "Masato Hashimoto" << "cabezon.hashimoto@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_JA"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "ko" << QString::fromUtf8("한국어 [KO]")
           << QString(STRPRODUCTVER)
           << QString::fromUtf8("Yonghee Lee") << "v4321v@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_KO"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "lt" << QString::fromUtf8("Lietuvių [LT]")
           << "0.12.4"
           << QString::fromUtf8("keturidu") << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_LT"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "nl" << QString::fromUtf8("Nederlands [NL]")
           << QString(STRPRODUCTVER)
           << "TeLLie" << "elbert.pol@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_NL"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "pl" << QString::fromUtf8("Polski [PL]")
           << QString(STRPRODUCTVER)
           << QString::fromUtf8("Piotr Pecka") << "piotr.pecka@outlook.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_PL"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "pt_BR" << QString::fromUtf8("Português (Brazil) [pt_BR]")
           << "0.12.4"
           << QString::fromUtf8("Marcos M. Ribeiro") << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_BR"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "ru" << QString::fromUtf8("Русский [RU]")
           << QString(STRPRODUCTVER)
           << "QuiteRSS Team" << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_RU"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "sr" << QString::fromUtf8("Српски [SR]")
           << QString(STRPRODUCTVER)
           << "Ozzii" << "ozzii.translate@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_SR"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "sv" << QString::fromUtf8("Svenska [SV]")
           << QString(STRPRODUCTVER)
           << QString::fromUtf8("Åke Engelbrektson") << "eson57@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_SV"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "uk" << QString::fromUtf8("Українська [UK]")
           << QString(STRPRODUCTVER)
           << QString::fromUtf8("Сергій Левицький") << "leon21sl@myopera.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_UK"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "zh_CN" << QString::fromUtf8("中文 (China) [zh_CN]")
           << QString(STRPRODUCTVER)
           << QString::fromUtf8("wwj402") << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_CN"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "zh_TW" << QString::fromUtf8("中文 (Taiwan) [zh_TW]")
           << QString(STRPRODUCTVER)
           << QString::fromUtf8("Hulen (破滅刃)") << "shift0106@hotmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_TW"));
  languageFileList_->addTopLevelItem(languageItem);

  QString linkWikiStr =
      QString("<a href='http://code.google.com/p/quite-rss/wiki/Translations'>Wiki for translators</a>");
  QLabel *linkWiki = new QLabel(linkWikiStr);
  linkWiki->setOpenExternalLinks(true);

  QVBoxLayout *languageLayout = new QVBoxLayout();
  languageLayout->setMargin(0);
  languageLayout->addWidget(new QLabel(tr("Choose language:")));
  languageLayout->addWidget(languageFileList_);
  languageLayout->addWidget(linkWiki);

  languageWidget_ = new QWidget();
  languageWidget_->setLayout(languageLayout);
}

/** @brief Создание виджета "Шрифты"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createFontsWidget()
{
  fontsTree_ = new QTreeWidget();
  fontsTree_->setObjectName("fontTree");
  fontsTree_->setColumnCount(3);
  fontsTree_->setColumnHidden(0, true);
  fontsTree_->setColumnWidth(1, 260);

  QStringList treeItem;
  treeItem << "Id" << tr("Type") << tr("Font");
  fontsTree_->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << "0" << tr("Feeds list font");
  fontsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "1" << tr("News list font");
  fontsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "2" << tr("News title font");
  fontsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "3" << tr("News text font");
  fontsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "4" << tr("Notification font");
  fontsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));

  fontsTree_->setCurrentItem(fontsTree_->topLevelItem(0));
  connect(fontsTree_, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(slotFontChange()));

  QPushButton *fontChange = new QPushButton(tr("Change..."));
  connect(fontChange, SIGNAL(clicked()), this, SLOT(slotFontChange()));
  QPushButton *fontReset = new QPushButton(tr("Reset"));
  connect(fontReset, SIGNAL(clicked()), this, SLOT(slotFontReset()));
  QVBoxLayout *fontsButtonLayout = new QVBoxLayout();
  fontsButtonLayout->addWidget(fontChange);
  fontsButtonLayout->addWidget(fontReset);
  fontsButtonLayout->addStretch(1);

  browserMinFontSize_ = new QSpinBox();
  browserMinFontSize_->setRange(0, 99);
  browserMinLogFontSize_ = new QSpinBox();
  browserMinLogFontSize_->setRange(0, 99);

  QGridLayout *browserFontSizeLayout = new QGridLayout();
  browserFontSizeLayout->addWidget(
        new QLabel(tr("Minimum font size of browser")), 0, 0);
  browserFontSizeLayout->addWidget(browserMinFontSize_, 0, 1);
  browserFontSizeLayout->addWidget(
        new QLabel(tr("Minimum logical font size of browser")), 1, 0);
  browserFontSizeLayout->addWidget(browserMinLogFontSize_, 1, 1);
  browserFontSizeLayout->setColumnStretch(2, 1);

  QVBoxLayout *fontsLayout = new QVBoxLayout();
  fontsLayout->setMargin(0);
  fontsLayout->addWidget(fontsTree_, 1);
  fontsLayout->addLayout(browserFontSizeLayout);

  QHBoxLayout *mainLayout = new QHBoxLayout();
  mainLayout->setMargin(0);
  mainLayout->addLayout(fontsLayout, 1);
  mainLayout->addLayout(fontsButtonLayout);

  fontsWidget_ = new QWidget();
  fontsWidget_->setLayout(mainLayout);
}

/** @brief Создание виджета "Горячие клавиши"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createShortcutWidget()
{
  shortcutTree_ = new QTreeWidget();
  shortcutTree_->setObjectName("shortcutTree");
  shortcutTree_->setSortingEnabled(false);
  shortcutTree_->setColumnCount(6);
  shortcutTree_->hideColumn(0);
  shortcutTree_->hideColumn(4);
  shortcutTree_->hideColumn(5);
  shortcutTree_->setSelectionBehavior(QAbstractItemView::SelectRows);
  shortcutTree_->header()->setStretchLastSection(false);
  shortcutTree_->header()->setResizeMode(1, QHeaderView::ResizeToContents);
  shortcutTree_->header()->setResizeMode(2, QHeaderView::Stretch);
  shortcutTree_->header()->setResizeMode(3, QHeaderView::ResizeToContents);

  QStringList treeItem;
  treeItem << "Id" << tr("Action") << tr("Description") << tr("Shortcut")
           << "ObjectName" << "Data";
  shortcutTree_->setHeaderLabels(treeItem);

  editShortcut_ = new LineEdit();
  QPushButton *resetShortcutButton = new QPushButton(tr("Reset"));

  QHBoxLayout *editShortcutLayout = new QHBoxLayout();
  editShortcutLayout->addWidget(new QLabel(tr("Shortcut:")));
  editShortcutLayout->addWidget(editShortcut_, 1);
  editShortcutLayout->addWidget(resetShortcutButton);

  editShortcutBox = new QGroupBox();
  editShortcutBox->setEnabled(false);
  editShortcutBox->setLayout(editShortcutLayout);

  warningShortcut_ = new QLabel();

  QVBoxLayout *shortcutLayout = new QVBoxLayout();
  shortcutLayout->setMargin(0);
  shortcutLayout->addWidget(shortcutTree_, 1);
  shortcutLayout->addWidget(warningShortcut_);
  shortcutLayout->addWidget(editShortcutBox);

  shortcutWidget_ = new QWidget();
  shortcutWidget_->setLayout(shortcutLayout);

  connect(shortcutTree_, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
          this, SLOT(shortcutTreeClicked(QTreeWidgetItem*,int)));
  connect(this, SIGNAL(signalShortcutTreeUpDownPressed()),
          SLOT(slotShortcutTreeUpDownPressed()), Qt::QueuedConnection);
  connect(editShortcut_, SIGNAL(signalClear()),
          this, SLOT(slotClearShortcut()));
  connect(resetShortcutButton, SIGNAL(clicked()),
          this, SLOT(slotResetShortcut()));

  editShortcut_->installEventFilter(this);
}

void OptionsDialog::slotCategoriesTreeKeyUpDownPressed()
{
  slotCategoriesItemClicked(categoriesTree_->currentItem(), 1);
}

void OptionsDialog::slotCategoriesItemClicked(QTreeWidgetItem* item, int)
{
  contentLabel_->setText(item->data(1, Qt::DisplayRole).toString());
  contentStack_->setCurrentIndex(item->data(0, Qt::DisplayRole).toInt());

  if (item->data(1, Qt::DisplayRole).toString() == tr("Labels")) {
    loadLabels();
  } else if (item->data(1, Qt::DisplayRole).toString() == tr("Notifications")) {
    loadNotifier();
  }
}

int OptionsDialog::currentIndex()
{
  return categoriesTree_->currentItem()->text(0).toInt();
}

void OptionsDialog::setCurrentItem(int index)
{
  categoriesTree_->setCurrentItem(categoriesTree_->topLevelItem(index), 1);
  slotCategoriesTreeKeyUpDownPressed();
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

QString OptionsDialog::language()
{
  QString langFileName = languageFileList_->currentItem()->data(0, Qt::DisplayRole).toString();
  return langFileName;
}

void OptionsDialog::setLanguage(QString langFileName)
{
  // установка курсора на выбранный файл
  QList<QTreeWidgetItem*> list =
      languageFileList_->findItems(langFileName, Qt::MatchFixedString, 0);
  if (list.count()) {
    languageFileList_->setCurrentItem(list.at(0));
  } else {
    // если не удалось найти, выбираем английский
    QList<QTreeWidgetItem*> list =
        languageFileList_->findItems("en", Qt::MatchFixedString, 0);
    languageFileList_->setCurrentItem(list.at(0));
  }
}

void OptionsDialog::slotFontChange()
{
  bool bOk;
  QFont curFont;
  curFont.setFamily(fontsTree_->currentItem()->text(2).section(", ", 0, 0));
  curFont.setPointSize(fontsTree_->currentItem()->text(2).section(", ", 1).toInt());

  QFont font = QFontDialog::getFont(&bOk, curFont);
  if (bOk) {
    QString strFont = QString("%1, %2").
        arg(font.family()).
        arg(font.pointSize());
    fontsTree_->currentItem()->setText(2, strFont);
  }
}

void OptionsDialog::slotFontReset()
{
  switch (fontsTree_->currentItem()->text(0).toInt()) {
  case 2: case 3: fontsTree_->currentItem()->setText(
          2, QString("%1, 10").arg(qApp->font().family()));
    break;
  default: fontsTree_->currentItem()->setText(
          2, QString("%1, 8").arg(qApp->font().family()));
  }
}

void OptionsDialog::setBehaviorIconTray(int behavior)
{
  switch (behavior) {
  case CHANGE_ICON_TRAY:       changeIconTray_->setChecked(true);  break;
  case NEW_COUNT_ICON_TRAY:    newCountTray_->setChecked(true);    break;
  case UNREAD_COUNT_ICON_TRAY: unreadCountTray_->setChecked(true); break;
  default: staticIconTray_->setChecked(true);
  }
}

int OptionsDialog::behaviorIconTray()
{
  if (staticIconTray_->isChecked())       return STATIC_ICON_TRAY;
  else if (changeIconTray_->isChecked())  return CHANGE_ICON_TRAY;
  else if (newCountTray_->isChecked())    return NEW_COUNT_ICON_TRAY;
  else if (unreadCountTray_->isChecked()) return UNREAD_COUNT_ICON_TRAY;
  else return STATIC_ICON_TRAY;
}

void OptionsDialog::loadActionShortcut(QList<QAction *> actions, QStringList *list)
{
  shortcutTree_->clear();
  QListIterator<QAction *> iter(actions);
  while (iter.hasNext()) {
    QAction *pAction = iter.next();

    QStringList treeItem;
    treeItem << QString::number(shortcutTree_->topLevelItemCount())
             << pAction->text().remove("&")
             << pAction->toolTip() << pAction->shortcut().toString()
             << pAction->objectName() << pAction->data().toString();
    QTreeWidgetItem *item = new QTreeWidgetItem(treeItem);
    if (pAction->icon().isNull())
      item->setIcon(1, QIcon(":/images/images/noicon.png"));
    else {
      if (pAction->objectName() == "autoLoadImagesToggle") {
        item->setIcon(1, QIcon(":/images/imagesOn"));
        item->setText(1, tr("Load images"));
        item->setText(2, tr("Auto load images to news view"));
      } else item->setIcon(1, pAction->icon());
    }
    shortcutTree_->addTopLevelItem(item);

    QString str = pAction->shortcut().toString();
    QList<QTreeWidgetItem *> treeItems =
        shortcutTree_->findItems(str, Qt::MatchFixedString, 3);
    for (int i = 0; i < treeItems.count(); i++) {
      if ((treeItems.count() > 1) && !str.isEmpty()) {
        treeItems.at(i)->setTextColor(3, Qt::red);
      }
    }
  }

  listDefaultShortcut_ = list;
}

void OptionsDialog::saveActionShortcut(QList<QAction *> actions, QActionGroup *labelGroup)
{
  for (int i = 0; i < shortcutTree_->topLevelItemCount(); i++) {
    QString objectName = shortcutTree_->topLevelItem(i)->text(4);
    if (objectName.contains("labelAction_")) {
      QAction *action = new QAction(parent());
      action->setIcon(shortcutTree_->topLevelItem(i)->icon(1));
      action->setText(shortcutTree_->topLevelItem(i)->text(1));
      action->setShortcut(QKeySequence(shortcutTree_->topLevelItem(i)->text(3)));
      action->setObjectName(shortcutTree_->topLevelItem(i)->text(4));
      action->setCheckable(true);
      action->setData(shortcutTree_->topLevelItem(i)->text(5));
      labelGroup->addAction(action);
      actions.append(action);
    } else {
      int id = shortcutTree_->topLevelItem(i)->text(0).toInt();
      actions.at(id)->setShortcut(
            QKeySequence(shortcutTree_->topLevelItem(i)->text(3)));
    }
  }
}

void OptionsDialog::slotShortcutTreeUpDownPressed()
{
  shortcutTreeClicked(shortcutTree_->currentItem(), 1);
}

void OptionsDialog::shortcutTreeClicked(QTreeWidgetItem* item, int)
{
  editShortcut_->setText(item->text(3));
  editShortcutBox->setEnabled(true);
  editShortcut_->setFocus();
  warningShortcut_->clear();
}

void OptionsDialog::slotClearShortcut()
{
  QString str = shortcutTree_->currentItem()->text(3);
  QList<QTreeWidgetItem *> treeItems =
      shortcutTree_->findItems(str, Qt::MatchFixedString, 3);
  for (int i = 0; i < treeItems.count(); i++) {
    if (((treeItems.count() == 2) && !str.isEmpty()) ||
        (treeItems.at(i) == shortcutTree_->currentItem())) {
      treeItems.at(i)->setTextColor(3, treeItems.at(i)->textColor(2));
    }
  }

  editShortcut_->clear();
  shortcutTree_->currentItem()->setText(3, "");
  warningShortcut_->clear();
}

void OptionsDialog::slotResetShortcut()
{
  QString str = shortcutTree_->currentItem()->text(3);
  QList<QTreeWidgetItem *> treeItems =
      shortcutTree_->findItems(str, Qt::MatchFixedString, 3);
  for (int i = 0; i < treeItems.count(); i++) {
    if (((treeItems.count() == 2) && !str.isEmpty()) ||
        (treeItems.at(i) == shortcutTree_->currentItem())) {
      treeItems.at(i)->setTextColor(3, treeItems.at(i)->textColor(2));
    }
  }

  int id = shortcutTree_->currentItem()->data(0, Qt::DisplayRole).toInt();
  str = listDefaultShortcut_->at(id);
  editShortcut_->setText(str);
  shortcutTree_->currentItem()->setText(3, str);
  warningShortcut_->clear();

  treeItems = shortcutTree_->findItems(str, Qt::MatchFixedString, 3);
  for (int i = 0; i < treeItems.count(); i++) {
    if ((treeItems.at(i) != shortcutTree_->currentItem()) && !str.isEmpty()) {
      warningShortcut_->setText(tr("Warning: key is already assigned to") +
                                " \"" + treeItems.at(i)->text(1) + "\"");
    }
    if ((treeItems.count() > 1) && !str.isEmpty()) {
      treeItems.at(i)->setTextColor(3, Qt::red);
    }
  }
}

void OptionsDialog::setOpeningFeed(int action)
{
  switch (action) {
  case 1: positionFirstNews_->setChecked(true); break;
  case 2: nottoOpenNews_->setChecked(true); break;
  case 3: positionUnreadNews_->setChecked(true); break;
  default: positionLastNews_->setChecked(true);
  }
}

int OptionsDialog::getOpeningFeed()
{
  if (positionLastNews_->isChecked())     return 0;
  else if (positionFirstNews_->isChecked()) return 1;
  else if (nottoOpenNews_->isChecked()) return 2;
  else if (positionUnreadNews_->isChecked()) return 3;
  else return 0;
}

void OptionsDialog::selectionBrowser()
{
  QString path;

  QFileInfo file(otherExternalBrowserEdit_->text());
  if (file.isFile()) path = otherExternalBrowserEdit_->text();
  else path = file.path();

  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Open File..."),
                                                  path);
  if (!fileName.isEmpty())
    otherExternalBrowserEdit_->setText(fileName);
}

void OptionsDialog::applyWhitelist()
{
  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  rssl_->c2fEnabled_ = c2fEnabled_->isChecked();
  rssl_->c2fWhitelist_.clear();
  for (int i = 0; i < c2fWhitelist_->topLevelItemCount(); i++) {
    rssl_->c2fWhitelist_.append(c2fWhitelist_->topLevelItem(i)->text(0));
  }
}

void OptionsDialog::selectionSoundNotifer()
{
  QString path;

  QFileInfo file(editSoundNotifer_->text());
  if (file.isFile()) path = editSoundNotifer_->text();
  else path = file.path();

  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Open File..."),
                                                  path, "*.wav");
  if (!fileName.isEmpty())
    editSoundNotifer_->setText(fileName);
}

void OptionsDialog::feedsTreeNotifyItemChanged(QTreeWidgetItem *item, int column)
{
  if ((column != 0) || itemNotChecked_) return;

  itemNotChecked_ = true;
  if (item->checkState(0) == Qt::Unchecked) {
    setCheckStateItem(item, Qt::Unchecked);

    QTreeWidgetItem *parentItem = item->parent();
    while (parentItem) {
      parentItem->setCheckState(0, Qt::Unchecked);
      parentItem = parentItem->parent();
    }
  } else {
    setCheckStateItem(item, Qt::Checked);
  }
  itemNotChecked_ = false;
}

void OptionsDialog::setCheckStateItem(QTreeWidgetItem *item, Qt::CheckState state)
{
  for(int i = 0; i < item->childCount(); ++i) {
    QTreeWidgetItem *childItem = item->child(i);
    childItem->setCheckState(0, state);
    setCheckStateItem(childItem, state);
  }
}

void OptionsDialog::loadLabels()
{
  if (loadLabelsOk_) return;
  loadLabelsOk_ = true;

  idLabels_.clear();

  QSqlQuery q;
  q.exec("SELECT id, name, image, color_text, color_bg, num FROM labels ORDER BY num");
  while (q.next()) {
    int idLabel = q.value(0).toInt();
    QString nameLabel = q.value(1).toString();
    QByteArray byteArray = q.value(2).toByteArray();
    QString colorText = q.value(3).toString();
    QString colorBg = q.value(4).toString();
    int numLabel = q.value(5).toInt();
    QPixmap imageLabel;
    if (!byteArray.isNull())
      imageLabel.loadFromData(byteArray);
    QStringList strTreeItem;
    strTreeItem << QString::number(idLabel) << nameLabel
                << colorText << colorBg << QString::number(numLabel);
    QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(strTreeItem);
    treeWidgetItem->setIcon(1, QIcon(imageLabel));
    if (!colorText.isEmpty())
      treeWidgetItem->setTextColor(1, QColor(colorText));
    if (!colorBg.isEmpty())
      treeWidgetItem->setBackgroundColor(1, QColor(colorBg));
    labelsTree_->addTopLevelItem(treeWidgetItem);
  }
}

void OptionsDialog::newLabel()
{
  LabelDialog *labelDialog = new LabelDialog(this);

  if (labelDialog->exec() == QDialog::Rejected) {
    delete labelDialog;
    return;
  }

  QString nameLabel = labelDialog->nameEdit_->text();
  QString colorText = labelDialog->colorTextStr_;
  QString colorBg = labelDialog->colorBgStr_;

  int idLabel = 0;
  for (int i = 0; i < labelsTree_->topLevelItemCount(); i++) {
    QString str = labelsTree_->topLevelItem(i)->text(0);
    if (idLabel < str.toInt())
      idLabel = str.toInt();
  }
  idLabel = idLabel + 1;

  QStringList strTreeItem;
  strTreeItem << QString::number(idLabel) << nameLabel
              << colorText << colorBg
              << QString::number(idLabel);
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(strTreeItem);
  treeWidgetItem->setIcon(1, labelDialog->icon_);
  if (!colorText.isEmpty())
    treeWidgetItem->setTextColor(1, QColor(colorText));
  if (!colorBg.isEmpty())
    treeWidgetItem->setBackgroundColor(1, QColor(colorBg));
  labelsTree_->addTopLevelItem(treeWidgetItem);
  addIdLabelList(treeWidgetItem->text(0));

  strTreeItem.clear();
  strTreeItem << QString::number(shortcutTree_->topLevelItemCount())
           << nameLabel << nameLabel << ""
           << QString("labelAction_%1").arg(idLabel) << QString::number(idLabel);
  treeWidgetItem = new QTreeWidgetItem(strTreeItem);
  treeWidgetItem->setIcon(1, labelDialog->icon_);
  shortcutTree_->addTopLevelItem(treeWidgetItem);

  delete labelDialog;
}

void OptionsDialog::editLabel()
{
  QTreeWidgetItem *treeWidgetItem = labelsTree_->currentItem();

  QString idLabelStr = treeWidgetItem->text(0);
  QString nameLabel = treeWidgetItem->text(1);
  QString colorText = treeWidgetItem->text(2);
  QString colorBg = treeWidgetItem->text(3);

  LabelDialog *labelDialog = new LabelDialog(this);

  labelDialog->nameEdit_->setText(nameLabel);
  labelDialog->icon_ = treeWidgetItem->icon(1);
  labelDialog->colorTextStr_ = colorText;
  labelDialog->colorBgStr_ = colorBg;
  labelDialog->loadData();

  if (labelDialog->exec() == QDialog::Rejected) {
    delete labelDialog;
    return;
  }

  nameLabel = labelDialog->nameEdit_->text();
  colorText = labelDialog->colorTextStr_;
  colorBg = labelDialog->colorBgStr_;

  treeWidgetItem->setText(1, nameLabel);
  treeWidgetItem->setText(2, colorText);
  treeWidgetItem->setText(3, colorBg);
  treeWidgetItem->setIcon(1, labelDialog->icon_);
  if (!colorText.isEmpty())
    treeWidgetItem->setTextColor(1, QColor(colorText));
  if (!colorBg.isEmpty())
    treeWidgetItem->setBackgroundColor(1, QColor(colorBg));
  addIdLabelList(idLabelStr);

  QList<QTreeWidgetItem *> treeItems = shortcutTree_->findItems(
        idLabelStr, Qt::MatchFixedString, 5);
  treeWidgetItem = treeItems.first();
  treeWidgetItem->setIcon(1, labelDialog->icon_);
  treeWidgetItem->setText(1 , nameLabel);

  delete labelDialog;
}

void OptionsDialog::deleteLabel()
{
  int labelRow = labelsTree_->currentIndex().row();
  addIdLabelList(labelsTree_->topLevelItem(labelRow)->text(0));

  QList<QTreeWidgetItem *> treeItems = shortcutTree_->findItems(
        labelsTree_->topLevelItem(labelRow)->text(0), Qt::MatchFixedString, 5);
  int indexItem = shortcutTree_->indexOfTopLevelItem(treeItems.first());
  QTreeWidgetItem *treeItem = shortcutTree_->takeTopLevelItem(indexItem);
  delete treeItem;

  treeItem = labelsTree_->takeTopLevelItem(labelRow);
  delete treeItem;
}

void OptionsDialog::moveUpLabel()
{
  int labelRow = labelsTree_->currentIndex().row();

  addIdLabelList(labelsTree_->topLevelItem(labelRow)->text(0));
  addIdLabelList(labelsTree_->topLevelItem(labelRow-1)->text(0));

  QList<QTreeWidgetItem *> treeItems = shortcutTree_->findItems(
        labelsTree_->topLevelItem(labelRow)->text(0), Qt::MatchFixedString, 5);
  int indexItem = shortcutTree_->indexOfTopLevelItem(treeItems.first());
  QTreeWidgetItem *treeItem = shortcutTree_->takeTopLevelItem(indexItem-1);
  shortcutTree_->insertTopLevelItem(indexItem, treeItem);

  int num1 = labelsTree_->topLevelItem(labelRow)->text(4).toInt();
  int num2 = labelsTree_->topLevelItem(labelRow-1)->text(4).toInt();
  labelsTree_->topLevelItem(labelRow-1)->setText(4, QString::number(num1));
  labelsTree_->topLevelItem(labelRow)->setText(4, QString::number(num2));

  treeItem = labelsTree_->takeTopLevelItem(labelRow-1);
  labelsTree_->insertTopLevelItem(labelRow, treeItem);

  if (labelsTree_->currentIndex().row() == 0)
    moveUpLabelButton_->setEnabled(false);
  if (labelsTree_->currentIndex().row() != (labelsTree_->topLevelItemCount()-1))
    moveDownLabelButton_->setEnabled(true);
}

void OptionsDialog::moveDownLabel()
{
  int labelRow = labelsTree_->currentIndex().row();

  addIdLabelList(labelsTree_->topLevelItem(labelRow)->text(0));
  addIdLabelList(labelsTree_->topLevelItem(labelRow+1)->text(0));

  QList<QTreeWidgetItem *> treeItems = shortcutTree_->findItems(
        labelsTree_->topLevelItem(labelRow)->text(0), Qt::MatchFixedString, 5);
  int indexItem = shortcutTree_->indexOfTopLevelItem(treeItems.first());
  QTreeWidgetItem *treeItem = shortcutTree_->takeTopLevelItem(indexItem+1);
  shortcutTree_->insertTopLevelItem(indexItem, treeItem);

  int num1 = labelsTree_->topLevelItem(labelRow)->text(4).toInt();
  int num2 = labelsTree_->topLevelItem(labelRow+1)->text(4).toInt();
  labelsTree_->topLevelItem(labelRow+1)->setText(4, QString::number(num1));
  labelsTree_->topLevelItem(labelRow)->setText(4, QString::number(num2));

  treeItem = labelsTree_->takeTopLevelItem(labelRow+1);
  labelsTree_->insertTopLevelItem(labelRow, treeItem);

  if (labelsTree_->currentIndex().row() == (labelsTree_->topLevelItemCount()-1))
    moveDownLabelButton_->setEnabled(false);
  if (labelsTree_->currentIndex().row() != 0)
    moveUpLabelButton_->setEnabled(true);
}

void OptionsDialog::slotCurrentLabelChanged(QTreeWidgetItem *current,
                                           QTreeWidgetItem *)
{
  if (labelsTree_->indexOfTopLevelItem(current) == 0)
    moveUpLabelButton_->setEnabled(false);
  else moveUpLabelButton_->setEnabled(true);

  if (labelsTree_->indexOfTopLevelItem(current) == (labelsTree_->topLevelItemCount()-1))
    moveDownLabelButton_->setEnabled(false);
  else moveDownLabelButton_->setEnabled(true);

  if (labelsTree_->indexOfTopLevelItem(current) < 0) {
    editLabelButton_->setEnabled(false);
    deleteLabelButton_->setEnabled(false);
    moveUpLabelButton_->setEnabled(false);
    moveDownLabelButton_->setEnabled(false);
  } else {
    editLabelButton_->setEnabled(true);
    deleteLabelButton_->setEnabled(true);
  }
}

void OptionsDialog::applyLabels()
{
  db_.transaction();
  QSqlQuery q;

  foreach (QString idLabel, idLabels_) {
    QList<QTreeWidgetItem *> treeItems =
        labelsTree_->findItems(idLabel, Qt::MatchFixedString, 0);
    if (treeItems.count() == 0) {
      q.exec(QString("DELETE FROM labels WHERE id=='%1'").arg(idLabel));
      q.exec(QString("SELECT id, label FROM news WHERE label LIKE '%,%1,%'").arg(idLabel));
      while (q.next()) {
        QString strIdLabels = q.value(1).toString();
        strIdLabels.replace(QString(",%1,").arg(idLabel), ",");
        QSqlQuery q1;
        q1.exec(QString("UPDATE news SET label='%1' WHERE id=='%2'").
               arg(strIdLabels).arg(q.value(0).toInt()));
      }
    } else {
      QString nameLabel = treeItems.at(0)->text(1);
      QPixmap icon = treeItems.at(0)->icon(1).pixmap(16, 16);
      QByteArray iconData;
      QBuffer    buffer(&iconData);
      buffer.open(QIODevice::WriteOnly);
      icon.save(&buffer, "PNG");
      buffer.close();
      QString colorText = treeItems.at(0)->text(2);
      QString colorBg = treeItems.at(0)->text(3);
      int numLabel = treeItems.at(0)->text(4).toInt();

      q.exec(QString("SELECT * FROM labels WHERE id=='%1'").arg(idLabel));
      if (q.next()) {
        q.prepare("UPDATE labels SET name=?, image=?, color_text=?, color_bg=?, num=? "
                  "WHERE id=?");
        q.addBindValue(nameLabel);
        q.addBindValue(iconData);
        q.addBindValue(colorText);
        q.addBindValue(colorBg);
        q.addBindValue(numLabel);
        q.addBindValue(idLabel);
        q.exec();
      } else {
        q.prepare("INSERT INTO labels(name, image, color_text, color_bg, num) "
                  "VALUES (:name, :image, :color_text, :color_bg, :num)");
        q.bindValue(":name", nameLabel);
        q.bindValue(":image", iconData);
        q.bindValue(":color_text", colorText);
        q.bindValue(":color_bg", colorBg);
        q.bindValue(":num", numLabel);
        q.exec();
      }
    }
  }

  db_.commit();
}

/**
 * @brief Добавление ид редактированной метки
 * @param idLabel ид метки
 ******************************************************************************/
void OptionsDialog::addIdLabelList(QString idLabel)
{
  if (!idLabels_.contains(idLabel)) idLabels_.append(idLabel);
}

void OptionsDialog::loadNotifier()
{
  if (loadNotifierOk_) return;
  loadNotifierOk_ = true;

  itemNotChecked_ = true;

  QSqlQuery q;
  QQueue<int> parentIds;
  parentIds.enqueue(0);
  while (!parentIds.empty()) {
    int parentId = parentIds.dequeue();
    QString qStr = QString("SELECT text, id, image, xmlUrl FROM feeds WHERE parentId='%1' ORDER BY rowToParent").
        arg(parentId);
    q.exec(qStr);
    while (q.next()) {
      QString feedText = q.value(0).toString();
      QString feedId = q.value(1).toString();
      QByteArray byteArray = q.value(2).toByteArray();
      QString xmlUrl = q.value(3).toString();

      QStringList treeItem;
      treeItem << feedText << feedId;
      QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);

      treeWidgetItem->setCheckState(0, Qt::Unchecked);
      QSqlQuery q1;
      qStr = QString("SELECT value FROM feeds_ex WHERE feedId='%1' AND name='showNotification'").
          arg(feedId);
      q1.exec(qStr);
      if (q1.next()) {
        if (q1.value(0).toInt() == 1)
          treeWidgetItem->setCheckState(0, Qt::Checked);
      } else {
        qStr = QString("INSERT INTO feeds_ex(feedId, name, value) VALUES ('%1', 'showNotification', '0')").
            arg(feedId);
        q1.exec(qStr);
      }
      if (treeWidgetItem->checkState(0) == Qt::Unchecked)
        feedsTreeNotify_->topLevelItem(0)->setCheckState(0, Qt::Unchecked);

      RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
      QPixmap iconItem;
      if (xmlUrl.isEmpty()) {
        iconItem.load(":/images/folder");
      } else {
        if (byteArray.isNull() || rssl_->defaultIconFeeds_) {
          iconItem.load(":/images/feed");
        } else {
          iconItem.loadFromData(QByteArray::fromBase64(byteArray));
        }
      }
      treeWidgetItem->setIcon(0, iconItem);

      QList<QTreeWidgetItem *> treeItems =
            feedsTreeNotify_->findItems(QString::number(parentId),
                                                       Qt::MatchFixedString | Qt::MatchRecursive,
                                                       1);
      treeItems.at(0)->addChild(treeWidgetItem);
      parentIds.enqueue(feedId.toInt());
    }
  }
  feedsTreeNotify_->expandAll();
  itemNotChecked_ = false;
}

void OptionsDialog::applyNotifier()
{
  feedsTreeNotify_->expandAll();
  QTreeWidgetItem *treeWidgetItem =
      feedsTreeNotify_->itemBelow(feedsTreeNotify_->topLevelItem(0));
  db_.transaction();
  while (treeWidgetItem) {
    int check = 0;
    if (treeWidgetItem->checkState(0) == Qt::Checked)
      check = 1;
    QSqlQuery q;
    QString qStr = QString("UPDATE feeds_ex SET value='%1' WHERE feedId='%2' AND name='showNotification'").
        arg(check).arg(treeWidgetItem->text(1).toInt());
    q.exec(qStr);

    treeWidgetItem = feedsTreeNotify_->itemBelow(treeWidgetItem);
  }
  db_.commit();
}

void OptionsDialog::slotDeletePass()
{
  if (passTree_->topLevelItemCount())
    passTree_->currentItem()->setHidden(true);
}

void OptionsDialog::slotDeleteAllPass()
{
  for (int i = 0; i < passTree_->topLevelItemCount(); i++) {
    passTree_->topLevelItem(i)->setHidden(true);
  }
}

void OptionsDialog::slotShowPass()
{
  if (passTree_->isColumnHidden(3)) {
    passTree_->showColumn(3);
    passTree_->setColumnWidth(1, passTree_->columnWidth(1) - passTree_->columnWidth(3));
  }
}

void OptionsDialog::applyPass()
{
  db_.transaction();
  QSqlQuery q;
  for (int i = 0; i < passTree_->topLevelItemCount(); i++) {
    if (passTree_->isItemHidden(passTree_->topLevelItem(i))) {
      QString id = passTree_->topLevelItem(i)->text(0);
      q.exec(QString("DELETE FROM passwords WHERE id=='%1'").arg(id));
    }
  }
  db_.commit();
}

void OptionsDialog::selectionUserStyleBrowser()
{
  QString path;

  QFileInfo file(userStyleBrowserEdit_->text());
  if (file.isFile()) path = userStyleBrowserEdit_->text();
  else path = file.path();

  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Select Style Sheet File"),
                                                  path, "*.css");

  if (!fileName.isEmpty())
    userStyleBrowserEdit_->setText(fileName);
}

void OptionsDialog::selectionDirDiskCache()
{
  QString dirStr = QFileDialog::getExistingDirectory(this, tr("Open Directory..."),
                                                       dirDiskCacheEdit_->text(),
                                                       QFileDialog::ShowDirsOnly
                                                       | QFileDialog::DontResolveSymlinks);
  if (!dirStr.isEmpty())
    dirDiskCacheEdit_->setText(dirStr);
}

void OptionsDialog::addWhitelist()
{
  QString site = QInputDialog::getText(this, tr("Add site to whitelist"),
                                       tr("Site without 'http://' (ex. youtube.com)"));
  if (site.isEmpty())
    return;

  c2fWhitelist_->insertTopLevelItem(0, new QTreeWidgetItem(QStringList(site)));
}

void OptionsDialog::removeWhitelist()
{
  QTreeWidgetItem* item = c2fWhitelist_->currentItem();
  if (!item)
    return;

  delete item;
}
