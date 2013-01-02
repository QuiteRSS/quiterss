#include "optionsdialog.h"
#include "labeldialog.h"
#include "VersionNo.h"

OptionsDialog::OptionsDialog(QWidget *parent, QSqlDatabase *db) :
  Dialog(parent),
  db_(db)
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
  treeItem << "0" << tr("General");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "1" << tr("System Tray");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "2" << tr("Network Connections");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "3" << tr("Browser");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "4" << tr("Feeds");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "5" << tr("Labels");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "6" << tr("Notifications");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "7" << tr("Language");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "8" << tr("Fonts");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "9" << tr("Keyboard Shortcuts");
  categoriesTree->addTopLevelItem(new QTreeWidgetItem(treeItem));

  //{ general
  showSplashScreen_ = new QCheckBox(tr("Show splash screen on startup"));
  reopenFeedStartup_ = new QCheckBox(tr("Reopen last opened feeds on startup"));

  storeDBMemory_ = new QCheckBox(tr("Store a DB in memory (requires program restart)"));

  QVBoxLayout *generalLayout = new QVBoxLayout();
  generalLayout->addWidget(showSplashScreen_);
  generalLayout->addWidget(reopenFeedStartup_);
  generalLayout->addStretch();
  generalLayout->addWidget(storeDBMemory_);

  QFrame *generalWidget_ = new QFrame();
  generalWidget_->setFrameStyle(QFrame::Box | QFrame::Sunken);
  generalWidget_->setLayout(generalLayout);
  //} general

  //{ system tray
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
  traySystemWidget_->setFrameStyle(QFrame::Box | QFrame::Sunken);
  traySystemWidget_->setLayout(boxTrayLayout);
  //} system tray

  //{ networkConnections
  systemProxyButton_ = new QRadioButton(
        tr("System proxy configuration (if available)"));
  //  systemProxyButton_->setEnabled(false);
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

  networkConnectionsWidget_ = new QFrame();
  networkConnectionsWidget_->setFrameStyle(QFrame::Box | QFrame::Sunken);
  networkConnectionsWidget_->setLayout(networkConnectionsLayout);
  //} networkConnections

  //{ browser
  embeddedBrowserOn_ = new QRadioButton(tr("Use embedded browser"));
  standartBrowserOn_ = new QRadioButton(tr("Use standard external browser"));
  externalBrowserOn_ = new QRadioButton(tr("Use following external browser:"));

  editExternalBrowser_ = new LineEdit();
  selectionExternalBrowser_ = new QPushButton(tr("Browse..."));

  javaScriptEnable_ = new QCheckBox(tr("Enable JavaScript"));
  pluginsEnable_ = new QCheckBox(tr("Enable plug-ins"));

  openLinkInBackgroundEmbedded_ = new QCheckBox(tr("Open links in embedded browser in background"));
  openLinkInBackground_ = new QCheckBox(tr("Open links in external browser in background (experimental)"));

  QGridLayout *browserLayout = new QGridLayout();
  browserLayout->setContentsMargins(15, 0, 5, 10);
  browserLayout->addWidget(embeddedBrowserOn_, 0, 0);
  browserLayout->addWidget(standartBrowserOn_, 1, 0);
  browserLayout->addWidget(externalBrowserOn_, 2, 0);
  browserLayout->addWidget(editExternalBrowser_, 3, 0);
  browserLayout->addWidget(selectionExternalBrowser_, 3, 1, Qt::AlignRight);

  QVBoxLayout *contentBrowserLayout = new QVBoxLayout();
  contentBrowserLayout->setContentsMargins(15, 0, 5, 10);
  contentBrowserLayout->addWidget(javaScriptEnable_);
  contentBrowserLayout->addWidget(pluginsEnable_);

  QVBoxLayout *browserLayoutV = new QVBoxLayout();
  browserLayoutV->addWidget(new QLabel(tr("Browser selection:")));
  browserLayoutV->addLayout(browserLayout);
  browserLayoutV->addWidget(new QLabel(tr("Content:")));
  browserLayoutV->addLayout(contentBrowserLayout);
  browserLayoutV->addWidget(openLinkInBackgroundEmbedded_);
  browserLayoutV->addWidget(openLinkInBackground_);
  browserLayoutV->addStretch();

  browserWidget_ = new QFrame();
  browserWidget_->setFrameStyle(QFrame::Box | QFrame::Sunken);
  browserWidget_->setLayout(browserLayoutV);

  connect(externalBrowserOn_, SIGNAL(toggled(bool)),
          editExternalBrowser_, SLOT(setEnabled(bool)));
  connect(externalBrowserOn_, SIGNAL(toggled(bool)),
          selectionExternalBrowser_, SLOT(setEnabled(bool)));
  externalBrowserOn_->setChecked(true);

  connect(selectionExternalBrowser_, SIGNAL(clicked()),
          this, SLOT(selectionBrowser()));

#if !(defined(Q_WS_WIN) || defined(Q_WS_X11))
  externalBrowserOn_->setVisible(false);
  editExternalBrowser_->setVisible(false);
  selectionExternalBrowser_->setVisible(false);
#endif
  //} browser

  //{ feeds
  updateFeedsStartUp_ = new QCheckBox(
        tr("Automatically update the feeds on startup"));
  updateFeeds_ = new QCheckBox(tr("Automatically update the feeds every"));
  updateFeedsTime_ = new QSpinBox();
  updateFeedsTime_->setEnabled(false);
  updateFeedsTime_->setRange(5, 9999);
  connect(updateFeeds_, SIGNAL(toggled(bool)),
          updateFeedsTime_, SLOT(setEnabled(bool)));

  intervalTime_ = new QComboBox(this);
  intervalTime_->setEnabled(false);
  QStringList intervalList;
  intervalList << tr("minutes")  << tr("hours");
  intervalTime_->addItems(intervalList);
  connect(updateFeeds_, SIGNAL(toggled(bool)),
          intervalTime_, SLOT(setEnabled(bool)));
  connect(intervalTime_, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(intervalTimeChang(QString)));

  QHBoxLayout *updateFeedsLayout1 = new QHBoxLayout();
  updateFeedsLayout1->setMargin(0);
  updateFeedsLayout1->addWidget(updateFeeds_);
  updateFeedsLayout1->addWidget(updateFeedsTime_);
  updateFeedsLayout1->addWidget(intervalTime_);
  updateFeedsLayout1->addStretch();

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

  formatDateTime_ = new QComboBox(this);

  QStringList itemList;
  itemList << "31.12.99 13:37" << "31.12.1999 13:37"
           << QString("31. %1. 1999 13:37").arg(tr("Dec"))
           << QString("31. %1 1999 13:37").arg(tr("December"))
           << "99-12-31 13:37" << "1999-12-31 13:37";
  formatDateTime_->addItems(itemList);
  formatDateTime_->setItemData(0, "dd.MM.yy hh:mm");
  formatDateTime_->setItemData(1, "dd.MM.yyyy hh:mm");
  formatDateTime_->setItemData(2, "dd. MMM. yyyy hh:mm");
  formatDateTime_->setItemData(3, "dd. MMMM yyyy hh:mm");
  formatDateTime_->setItemData(4, "yy-MM-dd hh:mm");
  formatDateTime_->setItemData(5, "yyyy-MM-dd hh:mm");

  QHBoxLayout *formatDateLayout = new QHBoxLayout();
  formatDateLayout->setMargin(0);
  formatDateLayout->addWidget(new QLabel(tr("Display format for date and time:")));
  formatDateLayout->addWidget(formatDateTime_);
  formatDateLayout->addStretch();

  QVBoxLayout *generalFeedsLayout = new QVBoxLayout();
  generalFeedsLayout->addWidget(updateFeedsStartUp_);
  generalFeedsLayout->addLayout(updateFeedsLayout1);
  generalFeedsLayout->addSpacing(10);
  generalFeedsLayout->addWidget(new QLabel(tr("Opening feed:")));
  generalFeedsLayout->addLayout(openingFeedsLayout);
  generalFeedsLayout->addLayout(formatDateLayout);
  generalFeedsLayout->addStretch();

  QWidget *generalFeedsWidget_ = new QWidget();
  generalFeedsWidget_->setLayout(generalFeedsLayout);
//
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
  readingFeedsLayout->addStretch();

  QWidget *readingFeedsWidget_ = new QWidget();
  readingFeedsWidget_->setLayout(readingFeedsLayout);
//
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

  QGridLayout *cleanUpFeedsLayout1 = new QGridLayout();
  cleanUpFeedsLayout1->setMargin(0);
  cleanUpFeedsLayout1->setColumnStretch(1, 1);
  cleanUpFeedsLayout1->addWidget(dayCleanUpOn_, 0, 0, 1, 1);
  cleanUpFeedsLayout1->addWidget(maxDayCleanUp_, 0, 1, 1, 1, Qt::AlignLeft);
  cleanUpFeedsLayout1->addWidget(newsCleanUpOn_, 1, 0, 1, 1);
  cleanUpFeedsLayout1->addWidget(maxNewsCleanUp_, 1, 1, 1, 1, Qt::AlignLeft);
  cleanUpFeedsLayout1->addWidget(readCleanUp_, 2, 0, 1, 1);
  cleanUpFeedsLayout1->addWidget(neverUnreadCleanUp_, 3, 0, 1, 1);
  cleanUpFeedsLayout1->addWidget(neverStarCleanUp_, 4, 0, 1, 1);

  QVBoxLayout *cleanUpFeedsLayout = new QVBoxLayout();
  cleanUpFeedsLayout->addLayout(cleanUpFeedsLayout1);
  cleanUpFeedsLayout->addStretch();

  QWidget *cleanUpFeedsWidget_ = new QWidget();
  cleanUpFeedsWidget_->setLayout(cleanUpFeedsLayout);

  feedsWidget_ = new QTabWidget();
  feedsWidget_->addTab(generalFeedsWidget_, tr("General"));
  feedsWidget_->addTab(readingFeedsWidget_, tr("Reading"));
  feedsWidget_->addTab(cleanUpFeedsWidget_, tr("Clean Up"));
  //} feeds

  //{ labels
  createLabelsWidget();
  //} labels

  //{ notifier
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
  feedsTreeNotify_->setColumnCount(2);
  feedsTreeNotify_->setColumnHidden(1, true);
  feedsTreeNotify_->header()->hide();
  feedsTreeNotify_->setEnabled(false);

  itemNotChecked_ = true;

  treeItem.clear();
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
  notifierWidget_->setFrameStyle(QFrame::Box | QFrame::Sunken);
  notifierWidget_->setLayout(notifierMainLayout);
  //} notifier

  //{ language
  createLanguageWidget();
  //} language

  //{ fonts
  fontTree = new QTreeWidget();
  fontTree->setObjectName("fontTree");
  fontTree->setColumnCount(3);
  fontTree->setColumnHidden(0, true);
  fontTree->setColumnWidth(1, 260);

  treeItem.clear();
  treeItem << "Id" << tr("Type") << tr("Font");
  fontTree->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << "0" << tr("Feeds list font");
  fontTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "1" << tr("News list font");
  fontTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "2" << tr("News panel font (Title, Author)");
  fontTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "3" << tr("News font");
  fontTree->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "4" << tr("Notification font");
  fontTree->addTopLevelItem(new QTreeWidgetItem(treeItem));

  fontTree->setCurrentItem(fontTree->topLevelItem(0));
  connect(fontTree, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(slotFontChange()));

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

  //{ shortcut
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

  treeItem.clear();
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

  QVBoxLayout *shortcutLayout = new QVBoxLayout();
  shortcutLayout->setMargin(0);
  shortcutLayout->addWidget(shortcutTree_, 1);
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

  shortcutTree_->installEventFilter(this);
  editShortcut_->installEventFilter(this);
  //} shortcut

  contentLabel_ = new QLabel();
  contentLabel_->setObjectName("contentLabel_");
  contentLabel_->setFrameStyle(QFrame::Box | QFrame::Sunken);
  contentLabel_->setMinimumHeight(30);
  contentLabel_->setMargin(4);
  QFont fontContentLabel = contentLabel_->font();
  fontContentLabel.setBold(true);
  fontContentLabel.setPointSize(fontContentLabel.pointSize()+2);
  contentLabel_->setFont(fontContentLabel);

  contentStack_ = new QStackedWidget();
  contentStack_->setObjectName("contentStack_");
  contentStack_->addWidget(generalWidget_);
  contentStack_->addWidget(traySystemWidget_);
  contentStack_->addWidget(networkConnectionsWidget_);
  contentStack_->addWidget(browserWidget_);
  contentStack_->addWidget(feedsWidget_);
  contentStack_->addWidget(labelsWidget_);
  contentStack_->addWidget(notifierWidget_);
  contentStack_->addWidget(languageWidget_);
  contentStack_->addWidget(fontsWidget_);
  contentStack_->addWidget(shortcutWidget_);

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

  pageLayout->addWidget(splitter);

  buttonBox->addButton(QDialogButtonBox::Ok);
  buttonBox->addButton(QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptDialog()));

  connect(categoriesTree, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
          this, SLOT(slotCategoriesItemClicked(QTreeWidgetItem*,int)));
  connect(this, SIGNAL(signalCategoriesTreeKeyUpDownPressed()),
          SLOT(slotCategoriesTreeKeyUpDownPressed()), Qt::QueuedConnection);

  connect(manualProxyButton_, SIGNAL(toggled(bool)),
          this, SLOT(manualProxyToggle(bool)));

  categoriesTree->installEventFilter(this);

  // не нужно, т.к. после создания окна из главного окна передаются настройки
  //  manualProxyToggle(manualProxyButton_->isChecked());
}

bool OptionsDialog::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    if (obj != editShortcut_) {
      if ((keyEvent->key() == Qt::Key_Up) ||
          (keyEvent->key() == Qt::Key_Down)) {
        if (obj == categoriesTree)
          emit signalCategoriesTreeKeyUpDownPressed();
        else if (obj == shortcutTree_)
          emit signalShortcutTreeUpDownPressed();
      }
    } else {
      if (((keyEvent->key() < Qt::Key_Shift) ||
          (keyEvent->key() > Qt::Key_Alt)) &&
          !(((keyEvent->key() == Qt::Key_Return) || (keyEvent->key() == Qt::Key_Enter)) &&
            ((keyEvent->modifiers() == Qt::NoModifier) || (keyEvent->modifiers() == Qt::KeypadModifier))) &&
          !((keyEvent->modifiers() & Qt::ControlModifier) &&
            (keyEvent->key() == Qt::Key_F))) {
        QString str;
        if ((keyEvent->modifiers() & Qt::ShiftModifier) ||
            (keyEvent->modifiers() & Qt::ControlModifier) ||
            (keyEvent->modifiers() & Qt::AltModifier))
          str.append(QKeySequence(keyEvent->modifiers()).toString());
        str.append(QKeySequence(keyEvent->key()).toString());
        editShortcut_->setText(str);
        shortcutTree_->currentItem()->setText(3, str);
      }
      return true;
    }
    return false;
  } else {
    return QDialog::eventFilter(obj, event);
  }
}

void OptionsDialog::createLanguageWidget()
{
  languageFileList_ = new QTreeWidget();
  languageFileList_->setObjectName("languageFileList_");
  languageFileList_->setColumnCount(5);
  languageFileList_->setColumnHidden(0, true);
  languageFileList_->setColumnWidth(1, 120);
  languageFileList_->setColumnWidth(2, 80);
  languageFileList_->setColumnWidth(3, 120);

  QStringList treeItem;
  treeItem.clear();
  treeItem << "Id" << tr("Language") << tr("Version")
           << tr("Author") << tr("Contact");
  languageFileList_->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << "0" << QString::fromUtf8("English (EN)")
           << QString(STRPRODUCTVER)
           << "QuiteRSS Team" << "";
  QTreeWidgetItem *languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_EN"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "0" << QString::fromUtf8("Español (ES)")
           << "0.10.3"
           << QString::fromUtf8("Cesar Muñoz") << "csarg@live.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_ES"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "0" << QString::fromUtf8("فارسی (FA)")
           << "0.11.0"
           << "H.Mohamadi" << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_FA"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "0" << QString::fromUtf8("Français (FR)")
           << "0.11.0"
           << "Glad Deschrijver" << "glad.deschrijver@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_FR"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "0" << QString::fromUtf8("Deutsch (DE)")
           << QString(STRPRODUCTVER)
           << "QuiteRSS Team" << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_DE"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "0" << QString::fromUtf8("Magyar (HU)")
           << "0.11.0"
           << "ZityiSoft" << "zityisoft@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_HU"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "0" << QString::fromUtf8("Italiano (IT)")
           << "0.11.0"
           << "ZeroWis" << "lightflash@hotmail.it";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_IT"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "0" << QString::fromUtf8("Nederlands (NL)")
           << "0.11.0"
           << "TeLLie" << "elbert.pol@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_NL"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "0" << QString::fromUtf8("Русский (RU)")
           << QString(STRPRODUCTVER)
           << "QuiteRSS Team" << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_RU"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "0" << QString::fromUtf8("Српски (SR)")
           << "0.11.0"
           << "Ozzii" << "ozzii.translate@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_SR"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "0" << QString::fromUtf8("Svenska (SV)")
           << "0.11.0"
           << QString::fromUtf8("Åke Engelbrektson") << "eson57@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/images/flag_SV"));
  languageFileList_->addTopLevelItem(languageItem);


  QVBoxLayout *languageLayout = new QVBoxLayout();
  languageLayout->setMargin(0);
  languageLayout->addWidget(new QLabel(tr("Choose language:")));
  languageLayout->addWidget(languageFileList_);

  languageWidget_ = new QWidget();
  languageWidget_->setLayout(languageLayout);
}

void OptionsDialog::createLabelsWidget()
{
  idLabels_.clear();

  labelsTree_ = new QTreeWidget(this);
  labelsTree_->setObjectName("labelsTree_");
  labelsTree_->setColumnCount(5);
  labelsTree_->setColumnHidden(0, true);
  labelsTree_->setColumnHidden(2, true);
  labelsTree_->setColumnHidden(3, true);
  labelsTree_->setColumnHidden(4, true);
  labelsTree_->header()->hide();

  QSqlQuery q(*db_);
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
}

void OptionsDialog::slotCategoriesTreeKeyUpDownPressed()
{
  slotCategoriesItemClicked(categoriesTree->currentItem(), 1);
}

void OptionsDialog::slotCategoriesItemClicked(QTreeWidgetItem* item, int)
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

QString OptionsDialog::language()
{
  QString langFileName = languageFileList_->currentItem()->data(1, Qt::DisplayRole).toString();
  return langFileName.section("(", 1).replace(")", "");
}

void OptionsDialog::setLanguage(QString langFileName)
{
  // установка курсора на выбранный файл
  langFileName = QString("(%1)").arg(langFileName);

  QList<QTreeWidgetItem*> list =
      languageFileList_->findItems(langFileName, Qt::MatchContains, 1);
  if (list.count()) {
    languageFileList_->setCurrentItem(list.at(0));
  } else {
    // если не удалось найти, выбираем английский
    QList<QTreeWidgetItem*> list =
        languageFileList_->findItems("(en)", Qt::MatchContains, 1);
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

void OptionsDialog::acceptDialog()
{
  applyProxy();
  applyLabels();
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
  case 3: fontTree->currentItem()->setText(
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
  categoriesTree->setCurrentItem(categoriesTree->topLevelItem(index), 1);
  slotCategoriesItemClicked(categoriesTree->topLevelItem(index), 1);
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

void  OptionsDialog::intervalTimeChang(QString str)
{
  if (str == tr("minutes")) {
    updateFeedsTime_->setRange(5, 9999);
  } else if (str == tr("hours")) {
    updateFeedsTime_->setRange(1, 9999);
  }
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
             << pAction->toolTip() << pAction->shortcut()
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
  editShortcut_->setFocus();
  editShortcutBox->setEnabled(true);
}

void OptionsDialog::slotClearShortcut()
{
  editShortcut_->clear();
  shortcutTree_->currentItem()->setText(3, "");
}

void OptionsDialog::slotResetShortcut()
{
  int id = shortcutTree_->currentItem()->data(0, Qt::DisplayRole).toInt();
  QString str = listDefaultShortcut_->at(id);
  editShortcut_->setText(str);
  shortcutTree_->currentItem()->setText(3, str);
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

  QFileInfo file(editExternalBrowser_->text());
  if (file.isFile()) path = editExternalBrowser_->text();
  else path = file.path();

  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Open File..."),
                                                  path);
  if (!fileName.isEmpty())
    editExternalBrowser_->setText(fileName);
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
    if (item->childCount()) {
      QTreeWidgetItem *childItem = feedsTreeNotify_->itemBelow(item);
      while (childItem) {
        childItem->setCheckState(0, Qt::Unchecked);
        childItem = feedsTreeNotify_->itemBelow(childItem);
        if (childItem) {
          if (item->parent() == childItem->parent()) break;
        }
      }
    }
    QTreeWidgetItem *parentItem = item->parent();
    while (parentItem) {
      parentItem->setCheckState(0, Qt::Unchecked);
      parentItem = parentItem->parent();
    }
  } else if (item->childCount()) {
    QTreeWidgetItem *childItem = feedsTreeNotify_->itemBelow(item);
    while (childItem) {
      childItem->setCheckState(0, Qt::Checked);
      childItem = feedsTreeNotify_->itemBelow(childItem);
      if (childItem) {
        if (item->parent() == childItem->parent()) break;
      }
    }
  }
  itemNotChecked_ = false;
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
  db_->transaction();
  QSqlQuery q(*db_);

  foreach (QString idLabel, idLabels_) {
    QList<QTreeWidgetItem *> treeItems =
        labelsTree_->findItems(idLabel, Qt::MatchFixedString, 0);
    if (treeItems.count() == 0) {
      q.exec(QString("DELETE FROM labels WHERE id=='%1'").arg(idLabel));
      q.exec(QString("SELECT id, label FROM news WHERE label LIKE '\%,%1,\%'").arg(idLabel));
      while (q.next()) {
        QString strIdLabels = q.value(1).toString();
        strIdLabels.replace(QString(",%1,").arg(idLabel), ",");
        QSqlQuery q1(*db_);
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

  db_->commit();
}

/**
 * @brief Добавление ид редактированной метки
 * @param idLabel ид метки
 ******************************************************************************/
void OptionsDialog::addIdLabelList(QString idLabel)
{
  if (!idLabels_.contains(idLabel)) idLabels_.append(idLabel);
}
