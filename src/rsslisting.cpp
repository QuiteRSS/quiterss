/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "rsslisting.h"
#include "aboutdialog.h"
#include "addfeedwizard.h"
#include "addfolderdialog.h"
#include "authenticationdialog.h"
#include "cleanupwizard.h"
#include "customizetoolbardialog.h"
#include "db_func.h"
#include "delegatewithoutfocus.h"
#include "feedpropertiesdialog.h"
#include "filterrulesdialog.h"
#include "newsfiltersdialog.h"
#include "webpage.h"

#if defined(Q_OS_WIN)
#include <windows.h>
#include <psapi.h>
#endif

/** @brief Process messages from other application copy
 *---------------------------------------------------------------------------*/
void RSSListing::receiveMessage(const QString& message)
{
  qDebug() << QString("Received message: '%1'").arg(message);
  if (!message.isEmpty()){
    QStringList params = message.split('\n');
    foreach (QString param, params) {
      if (param == "--show") {
        if (closeApp_) return;
        slotShowWindows();
      }
      if (param == "--exit") slotClose();
      if (param.contains("feed:", Qt::CaseInsensitive)) {
        QClipboard *clipboard = QApplication::clipboard();
        if (param.contains("https://", Qt::CaseInsensitive)) {
          param.remove(0, 5);
          clipboard->setText(param);
        } else {
          param.remove(0, 7);
          clipboard->setText("http://" + param);
        }
        activateWindow();
        addFeed();
      }
    }
  }
}

// ---------------------------------------------------------------------------
RSSListing::RSSListing(QSettings *settings,
                       const QString &appDataDirPath,
                       const QString &dataDirPath,
                       QWidget *parent)
  : QMainWindow(parent)
  , settings_(settings)
  , appDataDirPath_(appDataDirPath)
  , dataDirPath_(dataDirPath)
  , currentNewsTab(NULL)
  , openingLink_(false)
  , diskCache_(NULL)
  , openNewsTab_(0)
  , closeApp_(false)
  , newsView_(NULL)
  , updateTimeCount_(0)
  , updateFeedsCount_(0)
  , notificationWidget(NULL)
  , feedIdOld_(-2)
  , importFeedStart_(false)
  , recountCategoryCountsOn_(false)
  , optionsDialog_(NULL)
{
  setWindowTitle("QuiteRSS");
  setContextMenuPolicy(Qt::CustomContextMenu);

  dbFileName_ = dataDirPath_ + QDir::separator() + kDbName;
  QString versionDB = initDB(dbFileName_, settings_);
  settings_->setValue("VersionDB", versionDB);

  storeDBMemory_ = settings_->value("Settings/storeDBMemory", true).toBool();
  storeDBMemoryT_ = storeDBMemory_;

  db_ = QSqlDatabase::addDatabase("QSQLITE");
  if (storeDBMemory_)
    db_.setDatabaseName(":memory:");
  else
    db_.setDatabaseName(dbFileName_);
  if (!db_.open()) {
    QMessageBox::critical(0, tr("Error"), tr("SQLite driver not loaded!"));
  }

  if (storeDBMemory_) {
    dbMemFileThread_ = new DBMemFileThread(dbFileName_, this);
    dbMemFileThread_->sqliteDBMemFile(false);
    while(dbMemFileThread_->isRunning()) qApp->processEvents();
  }

  if (settings_->value("Settings/createLastFeed", false).toBool())
    lastFeedPath_ = dataDirPath_;

  networkManager_ = new NetworkManager(parent);
  connect(networkManager_, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          this, SLOT(slotAuthentication(QNetworkReply*,QAuthenticator*)));

  int saveCookies = settings_->value("Settings/saveCookies", 1).toInt();
  cookieJar_ = new CookieJar(dataDirPath_, saveCookies, this);

  networkManager_->setCookieJar(cookieJar_);

  bool useDiskCache = settings_->value("Settings/useDiskCache", true).toBool();
  if (useDiskCache) {
    diskCache_ = new QNetworkDiskCache(this);
#if defined(Q_OS_UNIX)
#ifdef HAVE_QT5
    diskCacheDirPathDefault_ = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
    diskCacheDirPathDefault_ = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
#endif
#else
    diskCacheDirPathDefault_ = dataDirPath_ + "/cache";
#endif
    QString diskCacheDirPath = settings_->value(
          "Settings/dirDiskCache", diskCacheDirPathDefault_).toString();
    if (diskCacheDirPath.isEmpty()) diskCacheDirPath = diskCacheDirPathDefault_;

    bool cleanDiskCache = settings_->value("Settings/cleanDiskCache", true).toBool();
    if (cleanDiskCache) {
      removePath(diskCacheDirPath);
      settings_->setValue("Settings/cleanDiskCache", false);
    }

    diskCache_->setCacheDirectory(diskCacheDirPath);
    int maxDiskCache = settings_->value("Settings/maxDiskCache", 50).toInt();
    diskCache_->setMaximumCacheSize(maxDiskCache*1024*1024);

    networkManager_->setCache(diskCache_);
  }

  downloadManager_ = new DownloadManager(this);
  connect(downloadManager_, SIGNAL(signalShowDownloads(bool)),
          this, SLOT(showDownloadManager(bool)));
  connect(downloadManager_, SIGNAL(signalUpdateInfo(QString)),
          this, SLOT(updateInfoDownloads(QString)));

  int requestTimeout = settings_->value("Settings/requestTimeout", 15).toInt();
  int replyCount = settings_->value("Settings/replyCount", 10).toInt();
  int numberRepeats = settings_->value("Settings/numberRepeats", 2).toInt();
  persistentUpdateThread_ = new UpdateThread(this, requestTimeout, replyCount, numberRepeats);

  persistentParseThread_ = new ParseThread(this);

  faviconThread_ = new FaviconThread(this);

  createFeedsWidget();
  createToolBarNull();

  createActions();
  createShortcut();
  createMenu();
  createToolBar();

  createStatusBar();
  createTray();

  createTabBar();

  QHBoxLayout *tabBarLayout = new QHBoxLayout();
  tabBarLayout->setContentsMargins(5, 0, 0, 0);
  tabBarLayout->setSpacing(0);
  tabBarLayout->addWidget(tabBar_);

  QWidget *tabBarWidget = new QWidget(this);
  tabBarWidget->setObjectName("tabBarWidget");
  tabBarWidget->setLayout(tabBarLayout);

  stackedWidget_ = new QStackedWidget(this);
  stackedWidget_->setObjectName("stackedWidget_");
  stackedWidget_->setFrameStyle(QFrame::NoFrame);

  mainSplitter_ = new QSplitter(this);
  mainSplitter_ ->setFrameStyle(QFrame::NoFrame);
  mainSplitter_->setHandleWidth(1);
  mainSplitter_->setStyleSheet(
        QString("QSplitter::handle {background: qlineargradient("
                "x1: 0, y1: 0, x2: 0, y2: 1,"
                "stop: 0 %1, stop: 0.07 %2);}").
        arg(feedsPanel_->palette().background().color().name()).
        arg(qApp->palette().color(QPalette::Dark).name()));
  mainSplitter_->setChildrenCollapsible(false);
  mainSplitter_->addWidget(feedsWidget_);
  mainSplitter_->addWidget(stackedWidget_);
  mainSplitter_->setStretchFactor(1, 1);

#define FEEDS_WIDTH 180
  QList <int> sizes;
  sizes << FEEDS_WIDTH << QApplication::desktop()->width();
  mainSplitter_->setSizes(sizes);

  QHBoxLayout *mainLayout1 = new QHBoxLayout();
  mainLayout1->addWidget(pushButtonNull_);
  mainLayout1->addWidget(mainSplitter_, 1);

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setMargin(0);
  mainLayout->setSpacing(0);
  mainLayout->addWidget(tabBarWidget);
  mainLayout->addLayout(mainLayout1, 1);

  QWidget *centralWidget = new QWidget(this);
  centralWidget->setLayout(mainLayout);

  setCentralWidget(centralWidget);

  connect(this, SIGNAL(signalCloseApp()),
          SLOT(slotCloseApp()), Qt::QueuedConnection);
  connect(qApp, SIGNAL(commitDataRequest(QSessionManager&)),
          this, SLOT(slotCommitDataRequest(QSessionManager&)));

  connect(this, SIGNAL(signalShowNotification()),
          SLOT(showNotification()), Qt::QueuedConnection);
  connect(this, SIGNAL(signalRefreshInfoTray()),
          SLOT(slotRefreshInfoTray()), Qt::QueuedConnection);
  connect(this, SIGNAL(signalRecountCategoryCounts()),
          SLOT(slotRecountCategoryCounts()), Qt::QueuedConnection);

  updateDelayer_ = new UpdateDelayer(this);
  connect(updateDelayer_, SIGNAL(signalUpdateNeeded(QString, bool, int)),
          this, SLOT(slotUpdateFeedDelayed(QString, bool, int)), Qt::QueuedConnection);
  connect(this, SIGNAL(signalNextUpdate()),
          updateDelayer_, SLOT(slotNextUpdateFeed()));
  connect(updateDelayer_, SIGNAL(signalUpdateModel(bool)),
          this, SLOT(feedsModelReload(bool)));

  connect(&timerLinkOpening_, SIGNAL(timeout()),
          this, SLOT(slotTimerLinkOpening()));

  loadSettingsFeeds();

  setStyleSheet("QMainWindow::separator { width: 1px; }");

  readSettings();

  initUpdateFeeds();

  QTimer::singleShot(10000, this, SLOT(slotUpdateAppCheck()));

  translator_ = new QTranslator(this);
  appInstallTranslator();

  installEventFilter(this);
}

// ---------------------------------------------------------------------------
RSSListing::~RSSListing()
{
  qDebug("App_Closing");
}

// ---------------------------------------------------------------------------
void RSSListing::slotCommitDataRequest(QSessionManager &manager)
{
  slotClose();
  manager.release();
}

// ---------------------------------------------------------------------------
/*virtual*/ void RSSListing::closeEvent(QCloseEvent* event)
{
  event->ignore();

  if (closingTray_ && showTrayIcon_) {
    oldState = windowState();
    emit signalPlaceToTray();
  } else {
    slotClose();
  }
}

/** @brief Process close application event
 *---------------------------------------------------------------------------*/
void RSSListing::slotClose()
{
  closeApp_ = true;

  traySystem->hide();
  hide();
  writeSettings();
  cookieJar_->saveCookies();

  persistentUpdateThread_->quit();
  persistentParseThread_->quit();
  faviconThread_->quit();

  cleanUpShutdown();

  if (storeDBMemory_) {
    dbMemFileThread_->sqliteDBMemFile(true);
    while(dbMemFileThread_->isRunning()) {
      int ms = 100;
#if defined(Q_OS_WIN)
      Sleep(DWORD(ms));
#else
      struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
      nanosleep(&ts, NULL);
#endif
    }
  }

  while (persistentUpdateThread_->isRunning());
  while (persistentParseThread_->isRunning());
  while (faviconThread_->isRunning());

  db_.close();

  QSqlDatabase::removeDatabase(QString());

  emit signalCloseApp();
}

// ---------------------------------------------------------------------------
void RSSListing::slotCloseApp()
{
  qApp->quit();
}

// ---------------------------------------------------------------------------
bool RSSListing::eventFilter(QObject *obj, QEvent *event)
{
  static int deactivateState = 0;

  if (obj == this) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
      if ((keyEvent->key() == Qt::Key_Up) ||
          (keyEvent->key() == Qt::Key_Down) ||
          (keyEvent->key() == Qt::Key_Left) ||
          (keyEvent->key() == Qt::Key_Right)) {
        QListIterator<QAction *> iter(listActions_);
        while (iter.hasNext()) {
          QAction *pAction = iter.next();
          if (pAction->shortcut() == QKeySequence(keyEvent->key())) {
            pAction->activate(QAction::Trigger);
            break;
          }
        }
      }
    }
  }

  if (obj == statusBar()) {
    if (event->type() == QEvent::MouseButtonRelease) {
      if (windowState() & Qt::WindowMaximized) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if ((mouseEvent->pos().x() > (statusBar()->width()-statusBar()->height())) &&
            (mouseEvent->pos().y() > 0)) {
          setFullScreen();
        }
      }
    }
    return false;
  } else if (obj == categoriesLabel_) {
    if (event->type() == QEvent::MouseButtonRelease) {
      showNewsCategoriesTree();
    }
    return false;
  }
  // Process  open link in browser in background
  else if (event->type() == QEvent::WindowDeactivate) {
    if (openingLink_ && openLinkInBackground_) {
      openingLink_ = false;
      timerLinkOpening_.start(openingLinkTimeout_);
      deactivateState = 1;
    }
    activationStateChangedTime_ = QDateTime::currentMSecsSinceEpoch();
  }
  // deactivation has painted
  else if ((event->type() == QEvent::Paint) && (deactivateState == 1)) {
    deactivateState = 2;
  }
  // deactivation in done. Reactivating
  else if ((deactivateState == 2) && timerLinkOpening_.isActive()) {
    deactivateState = 3;
    if (!isActiveWindow()) {
      setWindowState(windowState() & ~Qt::WindowActive);
      show();
      raise();
      activateWindow();
    }
  }
  // activating had painted
  else if ((deactivateState == 3) && (event->type() == QEvent::Paint)) {
    deactivateState = 0;
  }
  // pass the event on to the parent class
  return QMainWindow::eventFilter(obj, event);
}

/** @brief Process send link to external browser in background
 *---------------------------------------------------------------------------*/
void RSSListing::slotTimerLinkOpening()
{
  timerLinkOpening_.stop();
  if (!isActiveWindow()) {
    setWindowState(windowState() & ~Qt::WindowActive);
    show();
    raise();
    activateWindow();
  }
}

/** @brief Process changing window state
 *---------------------------------------------------------------------------*/
/*virtual*/ void RSSListing::changeEvent(QEvent *event)
{
  if(event->type() == QEvent::WindowStateChange) {
    if(isMinimized()) {
      oldState = ((QWindowStateChangeEvent*)event)->oldState();
      if (minimizingTray_ && showTrayIcon_) {
        event->ignore();
        emit signalPlaceToTray();
        return;
      }
    } else {
      oldState = windowState();
    }
  } else if(event->type() == QEvent::ActivationChange) {
    if (isActiveWindow() && (behaviorIconTray_ == CHANGE_ICON_TRAY)) {
#if defined(QT_NO_DEBUG_OUTPUT)
      traySystem->setIcon(QIcon(":/images/quiterss16"));
#else
      traySystem->setIcon(QIcon(":/images/quiterssDebug"));
#endif
    }
  } else if(event->type() == QEvent::LanguageChange) {
    retranslateStrings();
  }
  QMainWindow::changeEvent(event);
}

/** @brief Process placing to tray event
 *---------------------------------------------------------------------------*/
void RSSListing::slotPlaceToTray()
{
  hide();
  if (emptyWorking_)
    QTimer::singleShot(10000, this, SLOT(myEmptyWorkingSet()));
  if (markReadMinimize_)
    setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_, FeedReadPlaceToTray, currentNewsTab);
  if (clearStatusNew_)
    markAllFeedsOld();
  idFeedList_.clear();
  cntNewNewsList_.clear();

  writeSettings();
  cookieJar_->saveCookies();

  if (storeDBMemory_) {
    db_.commit();
    dbMemFileThread_->sqliteDBMemFile(true, QThread::LowestPriority);
  }
}

/** @brief Process tray event
 *---------------------------------------------------------------------------*/
void RSSListing::slotActivationTray(QSystemTrayIcon::ActivationReason reason)
{
  bool activated = false;

  switch (reason) {
  case QSystemTrayIcon::Unknown:
    break;
  case QSystemTrayIcon::Context:
    trayMenu_->activateWindow();
    break;
  case QSystemTrayIcon::DoubleClick:
    if (!singleClickTray_) {
      if ((QDateTime::currentMSecsSinceEpoch() - activationStateChangedTime_ < 300) ||
          isActiveWindow())
        activated = true;
      slotShowWindows(activated);
    }
    break;
  case QSystemTrayIcon::Trigger:
    if (singleClickTray_) {
      if ((QDateTime::currentMSecsSinceEpoch() - activationStateChangedTime_ < 200) ||
          isActiveWindow())
        activated = true;
      slotShowWindows(activated);
    }
    break;
  case QSystemTrayIcon::MiddleClick:
    break;
  }
}

/** @brief Show window on event
 *---------------------------------------------------------------------------*/
void RSSListing::slotShowWindows(bool trayClick)
{
  if (!trayClick || isHidden()){
    if (oldState & Qt::WindowFullScreen) {
      show();
    } else if (oldState & Qt::WindowMaximized) {
      showMaximized();
    } else {
      showNormal();
      restoreGeometry(settings_->value("GeometryState").toByteArray());
    }
    activateWindow();
  } else {
    if (minimizingTray_)
      emit signalPlaceToTray();
    else
      showMinimized();
  }
}
// ---------------------------------------------------------------------------
void RSSListing::createFeedsWidget()
{
  feedsTreeView_ = new FeedsTreeView(this);
  feedsTreeView_->setFrameStyle(QFrame::NoFrame);

  feedsTreeModel_ = new FeedsTreeModel("feeds",
                                       QStringList() << "" << "" << "" << "",
                                       QStringList() << "text" << "unread" << "undeleteCount" << "updated",
                                       0,
                                       feedsTreeView_);
  feedsTreeView_->setModel(feedsTreeModel_);
  for (int i = 0; i < feedsTreeModel_->columnCount(); ++i)
    feedsTreeView_->hideColumn(i);
  feedsTreeView_->showColumn(feedsTreeModel_->proxyColumnByOriginal("text"));
#ifdef HAVE_QT5
  feedsTreeView_->header()->setSectionResizeMode(feedsTreeModel_->proxyColumnByOriginal("text"), QHeaderView::Stretch);
  feedsTreeView_->header()->setSectionResizeMode(feedsTreeModel_->proxyColumnByOriginal("unread"), QHeaderView::ResizeToContents);
  feedsTreeView_->header()->setSectionResizeMode(feedsTreeModel_->proxyColumnByOriginal("undeleteCount"), QHeaderView::ResizeToContents);
  feedsTreeView_->header()->setSectionResizeMode(feedsTreeModel_->proxyColumnByOriginal("updated"), QHeaderView::ResizeToContents);
#else
  feedsTreeView_->header()->setResizeMode(feedsTreeModel_->proxyColumnByOriginal("text"), QHeaderView::Stretch);
  feedsTreeView_->header()->setResizeMode(feedsTreeModel_->proxyColumnByOriginal("unread"), QHeaderView::ResizeToContents);
  feedsTreeView_->header()->setResizeMode(feedsTreeModel_->proxyColumnByOriginal("undeleteCount"), QHeaderView::ResizeToContents);
  feedsTreeView_->header()->setResizeMode(feedsTreeModel_->proxyColumnByOriginal("updated"), QHeaderView::ResizeToContents);
#endif
  feedsTreeView_->sortByColumn(feedsTreeView_->columnIndex("rowToParent"),Qt::AscendingOrder);

  feedsToolBar_ = new QToolBar(this);
  feedsToolBar_->setObjectName("feedsToolBar");
  feedsToolBar_->setStyleSheet("QToolBar { border: none; padding: 0px; }");
  feedsToolBar_->setIconSize(QSize(18, 18));

  QHBoxLayout *feedsPanelLayout = new QHBoxLayout();
  feedsPanelLayout->setMargin(2);
  feedsPanelLayout->addWidget(feedsToolBar_, 1);

  feedsPanel_ = new QWidget(this);
  feedsPanel_->setObjectName("feedsPanel_");
  feedsPanel_->setStyleSheet(
        QString("#feedsPanel_ {border-bottom: 1px solid %1;}").
        arg(qApp->palette().color(QPalette::Dark).name()));
  feedsPanel_->setLayout(feedsPanelLayout);

  findFeeds_ = new FindFeed(this);
  QVBoxLayout *findFeedsLayout = new QVBoxLayout();
  findFeedsLayout->setMargin(2);
  findFeedsLayout->addWidget(findFeeds_);
  findFeedsWidget_ = new QWidget(this);
  findFeedsWidget_->hide();
  findFeedsWidget_->setLayout(findFeedsLayout);

  categoriesTree_ = new CategoriesTreeWidget(this);

  categoriesLabel_ = new QLabel(this);
  categoriesLabel_->setObjectName("categoriesLabel_");

  showCategoriesButton_ = new QToolButton(this);
  showCategoriesButton_->setFocusPolicy(Qt::NoFocus);
  showCategoriesButton_->setMaximumSize(16, 16);
  showCategoriesButton_->setAutoRaise(true);

  QHBoxLayout *categoriesPanelLayout = new QHBoxLayout();
  categoriesPanelLayout->setMargin(2);
  categoriesPanelLayout->addSpacing(2);
  categoriesPanelLayout->addWidget(categoriesLabel_, 1);
  categoriesPanelLayout->addWidget(showCategoriesButton_);

  categoriesPanel_ = new QWidget(this);
  categoriesPanel_->setObjectName("categoriesPanel_");
  categoriesPanel_->setLayout(categoriesPanelLayout);

  QVBoxLayout *categoriesLayout = new QVBoxLayout();
  categoriesLayout->setMargin(0);
  categoriesLayout->setSpacing(0);
  categoriesLayout->addWidget(categoriesPanel_);
  categoriesLayout->addWidget(categoriesTree_, 1);

  categoriesWidget_ = new QWidget(this);
  categoriesWidget_->setLayout(categoriesLayout);

  feedsSplitter_ = new QSplitter(Qt::Vertical);
  feedsSplitter_->setChildrenCollapsible(false);
  feedsSplitter_->setHandleWidth(1);
  feedsSplitter_->setStyleSheet(
        QString("QSplitter::handle {background: %1;}").
        arg(qApp->palette().color(QPalette::Dark).name()));
  feedsSplitter_->addWidget(feedsTreeView_);
  feedsSplitter_->addWidget(categoriesWidget_);
  feedsSplitter_->setStretchFactor(0, 1);

#define CATEGORIES_HEIGHT 210
  QList <int> sizes;
  sizes << QApplication::desktop()->height() << CATEGORIES_HEIGHT;
  feedsSplitter_->setSizes(sizes);

  QVBoxLayout *feedsLayout = new QVBoxLayout();
  feedsLayout->setMargin(0);
  feedsLayout->setSpacing(0);
  feedsLayout->addWidget(feedsPanel_);
  feedsLayout->addWidget(findFeedsWidget_);
  feedsLayout->addWidget(feedsSplitter_, 1);

  feedsWidget_ = new QFrame(this);
  feedsWidget_->setFrameStyle(QFrame::NoFrame);
  feedsWidget_->setLayout(feedsLayout);

  connect(feedsTreeView_, SIGNAL(pressed(QModelIndex)),
          this, SLOT(slotFeedClicked(QModelIndex)));
  connect(feedsTreeView_, SIGNAL(signalMiddleClicked()),
          this, SLOT(slotOpenFeedNewTab()));
  connect(feedsTreeView_, SIGNAL(signalDoubleClicked()),
          this, SLOT(slotGetFeed()));
  connect(feedsTreeView_, SIGNAL(pressKeyUp()), this, SLOT(slotFeedUpPressed()));
  connect(feedsTreeView_, SIGNAL(pressKeyDown()), this, SLOT(slotFeedDownPressed()));
  connect(feedsTreeView_, SIGNAL(pressKeyHome()), this, SLOT(slotFeedHomePressed()));
  connect(feedsTreeView_, SIGNAL(pressKeyEnd()), this, SLOT(slotFeedEndPressed()));
  connect(feedsTreeView_, SIGNAL(signalDropped(QModelIndex&,QModelIndex&,int)),
          this, SLOT(slotMoveIndex(QModelIndex&,QModelIndex&,int)));
  connect(feedsTreeView_, SIGNAL(customContextMenuRequested(QPoint)),
          this, SLOT(showContextMenuFeed(const QPoint &)));

  connect(findFeeds_, SIGNAL(textChanged(QString)),
          this, SLOT(slotFindFeeds(QString)));
  connect(findFeeds_, SIGNAL(signalSelectFind()),
          this, SLOT(slotSelectFind()));
  connect(findFeeds_, SIGNAL(returnPressed()),
          this, SLOT(slotSelectFind()));

  connect(categoriesTree_, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
          this, SLOT(slotCategoriesClicked(QTreeWidgetItem*,int)));
  connect(categoriesTree_, SIGNAL(signalMiddleClicked()),
          this, SLOT(slotOpenCategoryNewTab()));
  connect(categoriesTree_, SIGNAL(signalClearDeleted()),
          this, SLOT(clearDeleted()));
  connect(showCategoriesButton_, SIGNAL(clicked()),
          this, SLOT(showNewsCategoriesTree()));
  connect(feedsSplitter_, SIGNAL(splitterMoved(int,int)),
          this, SLOT(feedsSplitterMoved(int,int)));

  categoriesLabel_->installEventFilter(this);
}
// ---------------------------------------------------------------------------
void RSSListing::createToolBarNull()
{
  pushButtonNull_ = new QPushButton(this);
  pushButtonNull_->setObjectName("pushButtonNull");
  pushButtonNull_->setIcon(QIcon(":/images/images/triangleR.png"));
  pushButtonNull_->setFixedWidth(6);
  pushButtonNull_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  pushButtonNull_->setFocusPolicy(Qt::NoFocus);
  pushButtonNull_->setStyleSheet("background: #E8E8E8; border: none; padding: 0px;");
}
// ---------------------------------------------------------------------------
void RSSListing::createNewsTab(int index)
{
  currentNewsTab = (NewsTabWidget*)stackedWidget_->widget(index);
  currentNewsTab->setSettings();
  currentNewsTab->retranslateStrings();
  currentNewsTab->setBrowserPosition();

  newsModel_ = currentNewsTab->newsModel_;
  newsView_ = currentNewsTab->newsView_;
}
// ---------------------------------------------------------------------------
void RSSListing::createStatusBar()
{
  progressBar_ = new QProgressBar(this);
  progressBar_->setObjectName("progressBar_");
  progressBar_->setFormat("%p%");
  progressBar_->setAlignment(Qt::AlignCenter);
  progressBar_->setFixedWidth(100);
  progressBar_->setFixedHeight(15);
  progressBar_->setMinimum(0);
  progressBar_->setMaximum(0);
  progressBar_->setValue(0);
  progressBar_->setVisible(false);
  connect(this, SIGNAL(loadProgress(int)),
          progressBar_, SLOT(setValue(int)), Qt::QueuedConnection);

  statusBar()->setMinimumHeight(22);

  QToolButton *loadImagesButton = new QToolButton(this);
  loadImagesButton->setFocusPolicy(Qt::NoFocus);
  loadImagesButton->setDefaultAction(autoLoadImagesToggle_);
  loadImagesButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");

  QToolButton *fullScreenButton = new QToolButton(this);
  fullScreenButton->setFocusPolicy(Qt::NoFocus);
  fullScreenButton->setDefaultAction(fullScreenAct_);
  fullScreenButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  statusBar()->installEventFilter(this);

  statusBar()->addPermanentWidget(progressBar_);
  statusUnread_ = new QLabel(this);
  statusUnread_->hide();
  statusBar()->addPermanentWidget(statusUnread_);
  statusAll_ = new QLabel(this);
  statusAll_->hide();
  statusBar()->addPermanentWidget(statusAll_);
  statusBar()->addPermanentWidget(loadImagesButton);
  statusBar()->addPermanentWidget(fullScreenButton);
  statusBar()->setVisible(true);
}
// ---------------------------------------------------------------------------
void RSSListing::createTray()
{
#if defined(QT_NO_DEBUG_OUTPUT)
  traySystem = new QSystemTrayIcon(QIcon(":/images/quiterss16"), this);
#else
  traySystem = new QSystemTrayIcon(QIcon(":/images/quiterssDebug"), this);
#endif
  connect(traySystem,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this, SLOT(slotActivationTray(QSystemTrayIcon::ActivationReason)));
  connect(this, SIGNAL(signalPlaceToTray()),
          this, SLOT(slotPlaceToTray()), Qt::QueuedConnection);
  traySystem->setToolTip("QuiteRSS");
  createTrayMenu();
}

/** @brief Create tabbar widget
 *---------------------------------------------------------------------------*/
void RSSListing::createTabBar()
{
  tabBar_ = new TabBar(this);
  connect(tabBar_, SIGNAL(closeTab(int)),
          this, SLOT(slotCloseTab(int)));
  connect(tabBar_, SIGNAL(currentChanged(int)),
          this, SLOT(slotTabCurrentChanged(int)));
  connect(tabBar_, SIGNAL(tabMoved(int,int)),
          SLOT(slotTabMoved(int,int)));
  connect(this, SIGNAL(signalSetCurrentTab(int,bool)),
          SLOT(setCurrentTab(int,bool)), Qt::QueuedConnection);

  connect(closeTabAct_, SIGNAL(triggered()), tabBar_, SLOT(slotCloseTab()));
  connect(closeOtherTabsAct_, SIGNAL(triggered()),
          tabBar_, SLOT(slotCloseOtherTabs()));
  connect(closeAllTabsAct_, SIGNAL(triggered()),
          tabBar_, SLOT(slotCloseAllTab()));
  connect(nextTabAct_, SIGNAL(triggered()), tabBar_, SLOT(slotNextTab()));
  connect(prevTabAct_, SIGNAL(triggered()), tabBar_, SLOT(slotPrevTab()));
}

/** @brief Create actions for main menu and toolbar
 *---------------------------------------------------------------------------*/
void RSSListing::createActions()
{
  addAct_ = new QAction(this);
  addAct_->setObjectName("newAct");
  addAct_->setIcon(QIcon(":/images/add"));
  this->addAction(addAct_);
  connect(addAct_, SIGNAL(triggered()), this, SLOT(addFeed()));

  addFeedAct_ = new QAction(this);
  addFeedAct_->setObjectName("addFeedAct");
  addFeedAct_->setIcon(QIcon(":/images/feed"));
  this->addAction(addFeedAct_);
  connect(addFeedAct_, SIGNAL(triggered()), this, SLOT(addFeed()));

  addFolderAct_ = new QAction(this);
  addFolderAct_->setObjectName("addFolderAct");
  addFolderAct_->setIcon(QIcon(":/images/folder"));
  this->addAction(addFolderAct_);
  connect(addFolderAct_, SIGNAL(triggered()), this, SLOT(addFolder()));

  openFeedNewTabAct_ = new QAction(this);
  openFeedNewTabAct_->setObjectName("openNewTabAct");
  this->addAction(openFeedNewTabAct_);
  connect(openFeedNewTabAct_, SIGNAL(triggered()), this, SLOT(slotOpenFeedNewTab()));

  deleteFeedAct_ = new QAction(this);
  deleteFeedAct_->setObjectName("deleteFeedAct");
  deleteFeedAct_->setIcon(QIcon(":/images/delete"));
  this->addAction(deleteFeedAct_);
  connect(deleteFeedAct_, SIGNAL(triggered()), this, SLOT(deleteItemFeedsTree()));

  importFeedsAct_ = new QAction(this);
  importFeedsAct_->setObjectName("importFeedsAct");
  importFeedsAct_->setIcon(QIcon(":/images/importFeeds"));
  this->addAction(importFeedsAct_);
  connect(importFeedsAct_, SIGNAL(triggered()), this, SLOT(slotImportFeeds()));

  exportFeedsAct_ = new QAction(this);
  exportFeedsAct_->setObjectName("exportFeedsAct");
  exportFeedsAct_->setIcon(QIcon(":/images/exportFeeds"));
  this->addAction(exportFeedsAct_);
  connect(exportFeedsAct_, SIGNAL(triggered()), this, SLOT(slotExportFeeds()));

  exitAct_ = new QAction(this);
  exitAct_->setObjectName("exitAct");
  this->addAction(exitAct_);
  connect(exitAct_, SIGNAL(triggered()), this, SLOT(slotClose()));

  mainToolbarToggle_ = new QAction(this);
  mainToolbarToggle_->setCheckable(true);
  feedsToolbarToggle_ = new QAction(this);
  feedsToolbarToggle_->setCheckable(true);
  newsToolbarToggle_ = new QAction(this);
  newsToolbarToggle_->setCheckable(true);
  browserToolbarToggle_ = new QAction(this);
  browserToolbarToggle_->setCheckable(true);
  categoriesPanelToggle_ = new QAction(this);
  categoriesPanelToggle_->setCheckable(true);

  connect(feedsToolbarToggle_, SIGNAL(toggled(bool)),
          feedsPanel_, SLOT(setVisible(bool)));
  connect(categoriesPanelToggle_, SIGNAL(toggled(bool)),
          categoriesWidget_, SLOT(setVisible(bool)));

  customizeMainToolbarAct_ = new QAction(this);
  customizeMainToolbarAct_->setObjectName("customizeMainToolbarAct");
  customizeMainToolbarAct2_ = new QAction(this);
  connect(customizeMainToolbarAct2_, SIGNAL(triggered()),
          this, SLOT(customizeMainToolbar()));

  toolBarLockAct_ = new QAction(this);
  toolBarLockAct_->setCheckable(true);
  toolBarHideAct_ = new QAction(this);

  customizeFeedsToolbarAct_ = new QAction(this);
  customizeFeedsToolbarAct_->setObjectName("customizeFeedsToolbarAct");

  customizeNewsToolbarAct_ = new QAction(this);
  customizeNewsToolbarAct_->setObjectName("customizeNewsToolbarAct");

  systemStyle_ = new QAction(this);
  systemStyle_->setObjectName("systemStyle_");
  systemStyle_->setCheckable(true);
  system2Style_ = new QAction(this);
  system2Style_->setObjectName("system2Style_");
  system2Style_->setCheckable(true);
  greenStyle_ = new QAction(this);
  greenStyle_->setObjectName("greenStyle_");
  greenStyle_->setCheckable(true);
  greenStyle_->setChecked(true);
  orangeStyle_ = new QAction(this);
  orangeStyle_->setObjectName("orangeStyle_");
  orangeStyle_->setCheckable(true);
  purpleStyle_ = new QAction(this);
  purpleStyle_->setObjectName("purpleStyle_");
  purpleStyle_->setCheckable(true);
  pinkStyle_ = new QAction(this);
  pinkStyle_->setObjectName("pinkStyle_");
  pinkStyle_->setCheckable(true);
  grayStyle_ = new QAction(this);
  grayStyle_->setObjectName("grayStyle_");
  grayStyle_->setCheckable(true);

  topBrowserPositionAct_ = new QAction(this);
  topBrowserPositionAct_->setCheckable(true);
  topBrowserPositionAct_->setData(TOP_POSITION);
  bottomBrowserPositionAct_ = new QAction(this);
  bottomBrowserPositionAct_->setCheckable(true);
  bottomBrowserPositionAct_->setData(BOTTOM_POSITION);
  rightBrowserPositionAct_ = new QAction(this);
  rightBrowserPositionAct_->setCheckable(true);
  rightBrowserPositionAct_->setData(RIGHT_POSITION);
  leftBrowserPositionAct_ = new QAction(this);
  leftBrowserPositionAct_->setCheckable(true);
  leftBrowserPositionAct_->setData(LEFT_POSITION);

  autoLoadImagesToggle_ = new QAction(this);
  autoLoadImagesToggle_->setObjectName("autoLoadImagesToggle");
  autoLoadImagesToggle_->setIcon(QIcon(":/images/imagesOn"));
  this->addAction(autoLoadImagesToggle_);
  connect(autoLoadImagesToggle_, SIGNAL(triggered()),
          this, SLOT(setAutoLoadImages()));

  printAct_ = new QAction(this);
  printAct_->setObjectName("printAct");
  printAct_->setIcon(QIcon(":/images/printer"));
  this->addAction(printAct_);
  connect(printAct_, SIGNAL(triggered()), this, SLOT(slotPrint()));
  printPreviewAct_ = new QAction(this);
  printPreviewAct_->setObjectName("printPreviewAct");
  printPreviewAct_->setIcon(QIcon(":/images/printer"));
  this->addAction(printPreviewAct_);
  connect(printPreviewAct_, SIGNAL(triggered()), this, SLOT(slotPrintPreview()));

  savePageAsAct_ = new QAction(this);
  savePageAsAct_->setObjectName("savePageAsAct");
  savePageAsAct_->setIcon(QIcon(":/images/save_as"));
  this->addAction(savePageAsAct_);
  connect(savePageAsAct_, SIGNAL(triggered()), this, SLOT(slotSavePageAs()));

  zoomInAct_ = new QAction(this);
  zoomInAct_->setObjectName("zoomInAct");
  zoomInAct_->setIcon(QIcon(":/images/zoomIn"));
  this->addAction(zoomInAct_);
  zoomOutAct_ = new QAction(this);
  zoomOutAct_->setObjectName("zoomOutAct");
  zoomOutAct_->setIcon(QIcon(":/images/zoomOut"));
  this->addAction(zoomOutAct_);
  zoomTo100Act_ = new QAction(this);
  zoomTo100Act_->setObjectName("zoomTo100Act");
  this->addAction(zoomTo100Act_);

  updateFeedAct_ = new QAction(this);
  updateFeedAct_->setObjectName("updateFeedAct");
  updateFeedAct_->setIcon(QIcon(":/images/updateFeed"));
  this->addAction(updateFeedAct_);
  connect(updateFeedAct_, SIGNAL(triggered()), this, SLOT(slotGetFeed()));

  updateAllFeedsAct_ = new QAction(this);
  updateAllFeedsAct_->setObjectName("updateAllFeedsAct");
  updateAllFeedsAct_->setIcon(QIcon(":/images/updateAllFeeds"));
  this->addAction(updateAllFeedsAct_);
  connect(updateAllFeedsAct_, SIGNAL(triggered()), this, SLOT(slotGetAllFeeds()));

  markAllFeedsRead_ = new QAction(this);
  markAllFeedsRead_->setObjectName("markAllFeedRead");
  markAllFeedsRead_->setIcon(QIcon(":/images/markReadAll"));
  this->addAction(markAllFeedsRead_);
  connect(markAllFeedsRead_, SIGNAL(triggered()), this, SLOT(markAllFeedsRead()));

  indentationFeedsTreeAct_ = new QAction(this);
  indentationFeedsTreeAct_->setCheckable(true);
  connect(indentationFeedsTreeAct_, SIGNAL(triggered()),
          this, SLOT(slotIndentationFeedsTree()));

  sortedByTitleFeedsTreeAct_ = new QAction(this);
  connect(sortedByTitleFeedsTreeAct_, SIGNAL(triggered()),
          this, SLOT(sortedByTitleFeedsTree()));

  collapseAllFoldersAct_ = new QAction(this);
  collapseAllFoldersAct_->setObjectName("collapseAllFolderAct");
  collapseAllFoldersAct_->setIcon(QIcon(":/images/bulletMinus"));
  this->addAction(collapseAllFoldersAct_);
  connect(collapseAllFoldersAct_, SIGNAL(triggered()),
          feedsTreeView_, SLOT(collapseAll()));

  expandAllFoldersAct_ = new QAction(this);
  expandAllFoldersAct_->setObjectName("expandAllFolderAct");
  expandAllFoldersAct_->setIcon(QIcon(":/images/bulletPlus"));
  this->addAction(expandAllFoldersAct_);
  connect(expandAllFoldersAct_, SIGNAL(triggered()),
          feedsTreeView_, SLOT(expandAll()));

  markNewsRead_ = new QAction(this);
  markNewsRead_->setObjectName("markNewsRead");
  markNewsRead_->setIcon(QIcon(":/images/markRead"));
  this->addAction(markNewsRead_);

  markAllNewsRead_ = new QAction(this);
  markAllNewsRead_->setObjectName("markAllNewsRead");
  markAllNewsRead_->setIcon(QIcon(":/images/markReadAll"));
  this->addAction(markAllNewsRead_);

  showDownloadManagerAct_ = new QAction(this);
  showDownloadManagerAct_->setObjectName("showDownloadManagerAct");
  showDownloadManagerAct_->setIcon(QIcon(":/images/download"));
  this->addAction(showDownloadManagerAct_);
  connect(showDownloadManagerAct_, SIGNAL(triggered()), this, SLOT(showDownloadManager()));

  showCleanUpWizardAct_ = new QAction(this);
  showCleanUpWizardAct_->setObjectName("showCleanUpWizardAct");
  this->addAction(showCleanUpWizardAct_);
  connect(showCleanUpWizardAct_, SIGNAL(triggered()), this, SLOT(cleanUp()));

  setNewsFiltersAct_ = new QAction(this);
  setNewsFiltersAct_->setObjectName("setNewsFiltersAct");
  setNewsFiltersAct_->setIcon(QIcon(":/images/filterOff"));
  this->addAction(setNewsFiltersAct_);
  connect(setNewsFiltersAct_, SIGNAL(triggered()), this, SLOT(showNewsFiltersDlg()));
  setFilterNewsAct_ = new QAction(this);
  setFilterNewsAct_->setObjectName("setFilterNewsAct");
  setFilterNewsAct_->setIcon(QIcon(":/images/filterOff"));
  this->addAction(setFilterNewsAct_);
  connect(setFilterNewsAct_, SIGNAL(triggered()), this, SLOT(showFilterRulesDlg()));

  optionsAct_ = new QAction(this);
  optionsAct_->setObjectName("optionsAct");
  optionsAct_->setIcon(QIcon(":/images/options"));
  this->addAction(optionsAct_);
  connect(optionsAct_, SIGNAL(triggered()), this, SLOT(showOptionDlg()));

  feedsFilter_ = new QAction(this);
  feedsFilter_->setObjectName("feedsFilter");
  feedsFilter_->setIcon(QIcon(":/images/filterOff"));
  this->addAction(feedsFilter_);
  filterFeedsAll_ = new QAction(this);
  filterFeedsAll_->setObjectName("filterFeedsAll_");
  filterFeedsAll_->setCheckable(true);
  filterFeedsAll_->setChecked(true);
  filterFeedsNew_ = new QAction(this);
  filterFeedsNew_->setObjectName("filterFeedsNew_");
  filterFeedsNew_->setCheckable(true);
  filterFeedsUnread_ = new QAction(this);
  filterFeedsUnread_->setObjectName("filterFeedsUnread_");
  filterFeedsUnread_->setCheckable(true);
  filterFeedsStarred_ = new QAction(this);
  filterFeedsStarred_->setObjectName("filterFeedsStarred_");
  filterFeedsStarred_->setCheckable(true);

  newsFilter_ = new QAction(this);
  newsFilter_->setObjectName("newsFilter");
  newsFilter_->setIcon(QIcon(":/images/filterOff"));
  this->addAction(newsFilter_);
  filterNewsAll_ = new QAction(this);
  filterNewsAll_->setObjectName("filterNewsAll_");
  filterNewsAll_->setCheckable(true);
  filterNewsAll_->setChecked(true);
  filterNewsNew_ = new QAction(this);
  filterNewsNew_->setObjectName("filterNewsNew_");
  filterNewsNew_->setCheckable(true);
  filterNewsUnread_ = new QAction(this);
  filterNewsUnread_->setObjectName("filterNewsUnread_");
  filterNewsUnread_->setCheckable(true);
  filterNewsStar_ = new QAction(this);
  filterNewsStar_->setObjectName("filterNewsStar_");
  filterNewsStar_->setCheckable(true);
  filterNewsNotStarred_ = new QAction(this);
  filterNewsNotStarred_->setObjectName("filterNewsNotStarred_");
  filterNewsNotStarred_->setCheckable(true);
  filterNewsUnreadStar_ = new QAction(this);
  filterNewsUnreadStar_->setObjectName("filterNewsUnreadStar_");
  filterNewsUnreadStar_->setCheckable(true);
  filterNewsLastDay_ = new QAction(this);
  filterNewsLastDay_->setObjectName("filterNewsLastDay_");
  filterNewsLastDay_->setCheckable(true);
  filterNewsLastWeek_ = new QAction(this);
  filterNewsLastWeek_->setObjectName("filterNewsLastWeek_");
  filterNewsLastWeek_->setCheckable(true);

  newsSortByColumnGroup_ = new QActionGroup(this);
  newsSortByColumnGroup_->setExclusive(true);
  connect(newsSortByColumnGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(setNewsSortByColumn()));

  newsSortOrderGroup_ = new QActionGroup(this);
  newsSortOrderGroup_->setExclusive(true);
  QStringList listAct;
  listAct << "sortOrderAsc" << "sortOrderDesc";
  foreach (QString actionStr, listAct) {
    QAction *newsSortOrderAct = new QAction(this);
    newsSortOrderAct->setObjectName(actionStr);
    newsSortOrderAct->setCheckable(true);
    newsSortOrderGroup_->addAction(newsSortOrderAct);
  }
  connect(newsSortOrderGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(setNewsSortByColumn()));

  aboutAct_ = new QAction(this);
  aboutAct_->setObjectName("AboutAct_");
  connect(aboutAct_, SIGNAL(triggered()), this, SLOT(slotShowAboutDlg()));

  updateAppAct_ = new QAction(this);
  updateAppAct_->setObjectName("UpdateApp_");
  connect(updateAppAct_, SIGNAL(triggered()), this, SLOT(slotShowUpdateAppDlg()));

  reportProblemAct_ = new QAction(this);
  reportProblemAct_->setObjectName("reportProblemAct_");
  connect(reportProblemAct_, SIGNAL(triggered()), this, SLOT(slotReportProblem()));

  openInBrowserAct_ = new QAction(this);
  openInBrowserAct_->setObjectName("openInBrowserAct");
  this->addAction(openInBrowserAct_);

  openInExternalBrowserAct_ = new QAction(this);
  openInExternalBrowserAct_->setObjectName("openInExternalBrowserAct");
  openInExternalBrowserAct_->setIcon(QIcon(":/images/openBrowser"));
  this->addAction(openInExternalBrowserAct_);

  openNewsNewTabAct_ = new QAction(this);
  openNewsNewTabAct_->setObjectName("openInNewTabAct");
  openNewsNewTabAct_->setIcon(QIcon(":/images/images/tab_go.png"));
  this->addAction(openNewsNewTabAct_);
  openNewsBackgroundTabAct_ = new QAction(this);
  openNewsBackgroundTabAct_->setObjectName("openInBackgroundTabAct");
  this->addAction(openNewsBackgroundTabAct_);

  markStarAct_ = new QAction(this);
  markStarAct_->setObjectName("markStarAct");
  markStarAct_->setIcon(QIcon(":/images/starOn"));
  this->addAction(markStarAct_);

  deleteNewsAct_ = new QAction(this);
  deleteNewsAct_->setObjectName("deleteNewsAct");
  deleteNewsAct_->setIcon(QIcon(":/images/delete"));
  this->addAction(deleteNewsAct_);
  deleteAllNewsAct_ = new QAction(this);
  deleteAllNewsAct_->setObjectName("deleteAllNewsAct");
  this->addAction(deleteAllNewsAct_);

  restoreNewsAct_ = new QAction(this);
  restoreNewsAct_->setObjectName("restoreNewsAct");
  restoreNewsAct_->setIcon(QIcon(":/images/images/arrow_turn_left.png"));

  restoreLastNewsAct_ = new QAction(this);
  restoreLastNewsAct_->setObjectName("restoreLastNewsAct");
  restoreLastNewsAct_->setIcon(QIcon(":/images/images/arrow_turn_left.png"));
  this->addAction(restoreLastNewsAct_);
  connect(restoreLastNewsAct_, SIGNAL(triggered()), this, SLOT(restoreLastNews()));

  markFeedRead_ = new QAction(this);
  markFeedRead_->setObjectName("markFeedRead");
  markFeedRead_->setIcon(QIcon(":/images/markRead"));
  this->addAction(markFeedRead_);
  connect(markFeedRead_, SIGNAL(triggered()), this, SLOT(markFeedRead()));

  feedProperties_ = new QAction(this);
  feedProperties_->setObjectName("feedProperties");
  feedProperties_->setIcon(QIcon(":/images/preferencesFeed"));
  this->addAction(feedProperties_);
  connect(feedProperties_, SIGNAL(triggered()), this, SLOT(showFeedPropertiesDlg()));

  feedKeyUpAct_ = new QAction(this);
  feedKeyUpAct_->setObjectName("feedKeyUp");
  connect(feedKeyUpAct_, SIGNAL(triggered()), this, SLOT(slotFeedPrevious()));
  this->addAction(feedKeyUpAct_);

  feedKeyDownAct_ = new QAction(this);
  feedKeyDownAct_->setObjectName("feedKeyDownAct");
  connect(feedKeyDownAct_, SIGNAL(triggered()), this, SLOT(slotFeedNext()));
  this->addAction(feedKeyDownAct_);

  newsKeyUpAct_ = new QAction(this);
  newsKeyUpAct_->setObjectName("newsKeyUpAct");
  this->addAction(newsKeyUpAct_);
  newsKeyDownAct_ = new QAction(this);
  newsKeyDownAct_->setObjectName("newsKeyDownAct");
  this->addAction(newsKeyDownAct_);
  newsKeyPageUpAct_ = new QAction(this);
  newsKeyPageUpAct_->setObjectName("newsKeyPageUpAct");
  this->addAction(newsKeyPageUpAct_);
  newsKeyPageDownAct_ = new QAction(this);
  newsKeyPageDownAct_->setObjectName("newsKeyPageDownAct");
  this->addAction(newsKeyPageDownAct_);

  switchFocusAct_ = new QAction(this);
  switchFocusAct_->setObjectName("switchFocusAct");
  connect(switchFocusAct_, SIGNAL(triggered()), this, SLOT(slotSwitchFocus()));
  this->addAction(switchFocusAct_);
  switchFocusPrevAct_ = new QAction(this);
  switchFocusPrevAct_->setObjectName("switchFocusPrevAct");
  connect(switchFocusPrevAct_, SIGNAL(triggered()), this, SLOT(slotSwitchPrevFocus()));
  this->addAction(switchFocusPrevAct_);

  feedsWidgetVisibleAct_ = new QAction(this);
  feedsWidgetVisibleAct_->setObjectName("visibleFeedsWidgetAct");
  feedsWidgetVisibleAct_->setCheckable(true);
  connect(feedsWidgetVisibleAct_, SIGNAL(triggered()), this, SLOT(slotVisibledFeedsWidget()));
  connect(pushButtonNull_, SIGNAL(clicked()), feedsWidgetVisibleAct_, SLOT(trigger()));
  this->addAction(feedsWidgetVisibleAct_);

  showUnreadCount_ = new QAction(this);
  showUnreadCount_->setData(feedsTreeModel_->proxyColumnByOriginal("unread"));
  showUnreadCount_->setCheckable(true);
  showUndeleteCount_ = new QAction(this);
  showUndeleteCount_->setData(feedsTreeModel_->proxyColumnByOriginal("undeleteCount"));
  showUndeleteCount_->setCheckable(true);
  showLastUpdated_ = new QAction(this);
  showLastUpdated_->setData(feedsTreeModel_->proxyColumnByOriginal("updated"));
  showLastUpdated_->setCheckable(true);

  openDescriptionNewsAct_ = new QAction(this);
  openDescriptionNewsAct_->setObjectName("openDescriptionNewsAct");
  connect(openDescriptionNewsAct_, SIGNAL(triggered()),
          this, SLOT(slotOpenNewsWebView()));
  this->addAction(openDescriptionNewsAct_);

  findTextAct_ = new QAction(this);
  findTextAct_->setObjectName("findTextAct");
  connect(findTextAct_, SIGNAL(triggered()),
          this, SLOT(findText()));
  this->addAction(findTextAct_);

  placeToTrayAct_ = new QAction(this);
  placeToTrayAct_->setObjectName("placeToTrayAct");
  connect(placeToTrayAct_, SIGNAL(triggered()), this, SLOT(slotPlaceToTray()));
  this->addAction(placeToTrayAct_);

  findFeedAct_ = new QAction(this);
  findFeedAct_->setObjectName("findFeedAct");
  findFeedAct_->setCheckable(true);
  findFeedAct_->setChecked(false);
  findFeedAct_->setIcon(QIcon(":/images/images/findFeed.png"));
  this->addAction(findFeedAct_);
  connect(findFeedAct_, SIGNAL(triggered(bool)),
          this, SLOT(findFeedVisible(bool)));

  fullScreenAct_ = new QAction(this);
  fullScreenAct_->setObjectName("fullScreenAct");
  fullScreenAct_->setIcon(QIcon(":/images/images/fullScreen.png"));
  this->addAction(fullScreenAct_);
  connect(fullScreenAct_, SIGNAL(triggered()),
          this, SLOT(setFullScreen()));

  stayOnTopAct_ = new QAction(this);
  stayOnTopAct_->setObjectName("stayOnTopAct");
  stayOnTopAct_->setCheckable(true);
  this->addAction(stayOnTopAct_);
  connect(stayOnTopAct_, SIGNAL(triggered()),
          this, SLOT(setStayOnTop()));

  newsLabelGroup_ = new QActionGroup(this);
  newsLabelGroup_->setExclusive(false);
  QSqlQuery q;
  q.exec("SELECT id, name, image FROM labels ORDER BY num");
  while (q.next()) {
    int idLabel = q.value(0).toInt();
    QString nameLabel = q.value(1).toString();
    QByteArray byteArray = q.value(2).toByteArray();
    QPixmap imageLabel;
    if (!byteArray.isNull())
      imageLabel.loadFromData(byteArray);
    QAction *action = new QAction(QIcon(imageLabel), nameLabel, this);
    action->setObjectName(QString("labelAction_%1").arg(idLabel));
    action->setCheckable(true);
    action->setData(idLabel);
    newsLabelGroup_->addAction(action);
  }
  this->addActions(newsLabelGroup_->actions());

  newsLabelAction_ = new QAction(this);
  newsLabelAction_->setObjectName("newsLabelAction");
  this->addAction(newsLabelAction_);
  if (newsLabelGroup_->actions().count()) {
    newsLabelAction_->setIcon(newsLabelGroup_->actions().at(0)->icon());
    newsLabelAction_->setToolTip(newsLabelGroup_->actions().at(0)->text());
    newsLabelAction_->setData(newsLabelGroup_->actions().at(0)->data());
  }
  connect(newsLabelAction_, SIGNAL(triggered()),
          this, SLOT(setDefaultLabelNews()));
  connect(newsLabelGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(setLabelNews(QAction*)));

  closeTabAct_ = new QAction(this);
  closeTabAct_->setObjectName("closeTabAct");
  this->addAction(closeTabAct_);

  closeOtherTabsAct_ = new QAction(this);
  closeOtherTabsAct_->setObjectName("closeOtherTabsAct");
  this->addAction(closeOtherTabsAct_);

  closeAllTabsAct_ = new QAction(this);
  closeAllTabsAct_->setObjectName("closeAllTabsAct");
  this->addAction(closeAllTabsAct_);

  nextTabAct_ = new QAction(this);
  nextTabAct_->setObjectName("nextTabAct");
  this->addAction(nextTabAct_);

  prevTabAct_ = new QAction(this);
  prevTabAct_->setObjectName("prevTabAct");
  this->addAction(prevTabAct_);

  reduceNewsListAct_ = new QAction(this);
  reduceNewsListAct_->setObjectName("reduceNewsListAct");
  this->addAction(reduceNewsListAct_);
  connect(reduceNewsListAct_, SIGNAL(triggered()),
          this, SLOT(reduceNewsList()));
  increaseNewsListAct_ = new QAction(this);
  increaseNewsListAct_->setObjectName("increaseNewsListAct");
  this->addAction(increaseNewsListAct_);
  connect(increaseNewsListAct_, SIGNAL(triggered()),
          this, SLOT(increaseNewsList()));

  nextUnreadNewsAct_ = new QAction(this);
  nextUnreadNewsAct_->setObjectName("nextUnreadNewsAct");
  nextUnreadNewsAct_->setIcon(QIcon(":/images/moveDown"));
  this->addAction(nextUnreadNewsAct_);
  connect(nextUnreadNewsAct_, SIGNAL(triggered()), this, SLOT(nextUnreadNews()));
  prevUnreadNewsAct_ = new QAction(this);
  prevUnreadNewsAct_->setObjectName("prevUnreadNewsAct");
  prevUnreadNewsAct_->setIcon(QIcon(":/images/moveUp"));
  this->addAction(prevUnreadNewsAct_);
  connect(prevUnreadNewsAct_, SIGNAL(triggered()), this, SLOT(prevUnreadNews()));

  openHomeFeedAct_ = new QAction(this);
  openHomeFeedAct_->setObjectName("openHomeFeedsAct");
  openHomeFeedAct_->setIcon(QIcon(":/images/homePage"));
  this->addAction(openHomeFeedAct_);
  connect(openHomeFeedAct_, SIGNAL(triggered()), this, SLOT(slotOpenHomeFeed()));

  copyLinkAct_ = new QAction(this);
  copyLinkAct_->setObjectName("copyLinkAct");
  copyLinkAct_->setIcon(QIcon(":/images/copy"));
  this->addAction(copyLinkAct_);
  connect(copyLinkAct_, SIGNAL(triggered()), this, SLOT(slotCopyLinkNews()));

  nextFolderAct_ = new QAction(this);
  nextFolderAct_->setObjectName("nextFolderAct");
  this->addAction(nextFolderAct_);
  connect(nextFolderAct_, SIGNAL(triggered()), this, SLOT(slotNextFolder()));
  prevFolderAct_ = new QAction(this);
  prevFolderAct_->setObjectName("prevFolderAct");
  this->addAction(prevFolderAct_);
  connect(prevFolderAct_, SIGNAL(triggered()), this, SLOT(slotPrevFolder()));
  expandFolderAct_ = new QAction(this);
  expandFolderAct_->setObjectName("expandFolderAct");
  this->addAction(expandFolderAct_);
  connect(expandFolderAct_, SIGNAL(triggered()), this, SLOT(slotExpandFolder()));

  shareGroup_ = new QActionGroup(this);
  shareGroup_->setExclusive(false);

  emailShareAct_ = new QAction(this);
  emailShareAct_->setObjectName("emailShareAct");
  emailShareAct_->setText("Email");
  emailShareAct_->setIcon(QIcon(":/images/images/email.png"));
  shareGroup_->addAction(emailShareAct_);

  evernoteShareAct_ = new QAction(this);
  evernoteShareAct_->setObjectName("evernoteShareAct");
  evernoteShareAct_->setText("Evernote");
  evernoteShareAct_->setIcon(QIcon(":/images/images/share_evernote.png"));
  shareGroup_->addAction(evernoteShareAct_);

  gplusShareAct_ = new QAction(this);
  gplusShareAct_->setObjectName("gplusShareAct");
  gplusShareAct_->setText("Google+");
  gplusShareAct_->setIcon(QIcon(":/images/images/share_gplus.png"));
  shareGroup_->addAction(gplusShareAct_);

  facebookShareAct_ = new QAction(this);
  facebookShareAct_->setObjectName("facebookShareAct");
  facebookShareAct_->setText("Facebook");
  facebookShareAct_->setIcon(QIcon(":/images/images/share_facebook.png"));
  shareGroup_->addAction(facebookShareAct_);

  livejournalShareAct_ = new QAction(this);
  livejournalShareAct_->setObjectName("livejournalShareAct");
  livejournalShareAct_->setText("LiveJournal");
  livejournalShareAct_->setIcon(QIcon(":/images/images/share_livejournal.png"));
  shareGroup_->addAction(livejournalShareAct_);

  pocketShareAct_ = new QAction(this);
  pocketShareAct_->setObjectName("pocketShareAct");
  pocketShareAct_->setText("Pocket");
  pocketShareAct_->setIcon(QIcon(":/images/images/share_pocket.png"));
  shareGroup_->addAction(pocketShareAct_);

  twitterShareAct_ = new QAction(this);
  twitterShareAct_->setObjectName("twitterShareAct");
  twitterShareAct_->setText("Twitter");
  twitterShareAct_->setIcon(QIcon(":/images/images/share_twitter.png"));
  shareGroup_->addAction(twitterShareAct_);

  vkShareAct_ = new QAction(this);
  vkShareAct_->setObjectName("vkShareAct");
  vkShareAct_->setText("VK");
  vkShareAct_->setIcon(QIcon(":/images/images/share_vk.png"));
  shareGroup_->addAction(vkShareAct_);

  this->addActions(shareGroup_->actions());
  connect(shareGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(slotShareNews(QAction*)));


  connect(markNewsRead_, SIGNAL(triggered()),
          this, SLOT(markNewsRead()));
  connect(markAllNewsRead_, SIGNAL(triggered()),
          this, SLOT(markAllNewsRead()));
  connect(markStarAct_, SIGNAL(triggered()),
          this, SLOT(markNewsStar()));
  connect(deleteNewsAct_, SIGNAL(triggered()),
          this, SLOT(deleteNews()));
  connect(deleteAllNewsAct_, SIGNAL(triggered()),
          this, SLOT(deleteAllNewsList()));
  connect(restoreNewsAct_, SIGNAL(triggered()),
          this, SLOT(restoreNews()));

  connect(newsKeyUpAct_, SIGNAL(triggered()),
          this, SLOT(slotNewsUpPressed()));
  connect(newsKeyDownAct_, SIGNAL(triggered()),
          this, SLOT(slotNewsDownPressed()));
  connect(newsKeyPageUpAct_, SIGNAL(triggered()),
          this, SLOT(slotNewsPageUpPressed()));
  connect(newsKeyPageDownAct_, SIGNAL(triggered()),
          this, SLOT(slotNewsPageDownPressed()));

  connect(openInBrowserAct_, SIGNAL(triggered()),
          this, SLOT(openInBrowserNews()));
  connect(openInExternalBrowserAct_, SIGNAL(triggered()),
          this, SLOT(openInExternalBrowserNews()));
  connect(openNewsNewTabAct_, SIGNAL(triggered()),
          this, SLOT(slotOpenNewsNewTab()));
  connect(openNewsBackgroundTabAct_, SIGNAL(triggered()),
          this, SLOT(slotOpenNewsBackgroundTab()));
}
// ---------------------------------------------------------------------------
void RSSListing::createShortcut()
{
  addFeedAct_->setShortcut(QKeySequence(QKeySequence::New));
  listActions_.append(addFeedAct_);
  addFolderAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_N));
  listActions_.append(addFolderAct_);
  listActions_.append(deleteFeedAct_);
  exitAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));  // standart on other OS
  listActions_.append(exitAct_);
  updateFeedAct_->setShortcut(QKeySequence(Qt::Key_F5));
  listActions_.append(updateFeedAct_);
  updateAllFeedsAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F5));
  listActions_.append(updateAllFeedsAct_);
  listActions_.append(openHomeFeedAct_);
  listActions_.append(showDownloadManagerAct_);
  listActions_.append(showCleanUpWizardAct_);
  optionsAct_->setShortcut(QKeySequence(Qt::Key_F8));
  listActions_.append(optionsAct_);
  deleteNewsAct_->setShortcut(QKeySequence(Qt::Key_Delete));
  listActions_.append(deleteNewsAct_);
  deleteAllNewsAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Delete));
  listActions_.append(deleteAllNewsAct_);
  feedProperties_->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_E));
  listActions_.append(feedProperties_);
  feedKeyUpAct_->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Up));
  listActions_.append(feedKeyUpAct_);
  feedKeyDownAct_->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Down));
  listActions_.append(feedKeyDownAct_);
  newsKeyUpAct_->setShortcut(QKeySequence(Qt::Key_Left));
  listActions_.append(newsKeyUpAct_);
  newsKeyDownAct_->setShortcut(QKeySequence(Qt::Key_Right));
  listActions_.append(newsKeyDownAct_);
  listActions_.append(newsKeyPageUpAct_);
  listActions_.append(newsKeyPageDownAct_);

  listActions_.append(nextUnreadNewsAct_);
  listActions_.append(prevUnreadNewsAct_);

  listActions_.append(nextFolderAct_);
  listActions_.append(prevFolderAct_);

  listActions_.append(importFeedsAct_);
  listActions_.append(exportFeedsAct_);
  listActions_.append(autoLoadImagesToggle_);
  listActions_.append(markAllFeedsRead_);
  listActions_.append(markFeedRead_);
  listActions_.append(markNewsRead_);
  listActions_.append(markAllNewsRead_);
  listActions_.append(markStarAct_);
  listActions_.append(collapseAllFoldersAct_);
  listActions_.append(expandAllFoldersAct_);
  listActions_.append(expandFolderAct_);

  listActions_.append(openDescriptionNewsAct_);
  openDescriptionNewsAct_->setShortcut(QKeySequence(Qt::Key_Return));
  listActions_.append(openInBrowserAct_);
  openInBrowserAct_->setShortcut(QKeySequence(Qt::Key_Space));
  listActions_.append(openInExternalBrowserAct_);
  openInExternalBrowserAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
  openNewsNewTabAct_->setShortcut(QKeySequence(Qt::Key_T));
  listActions_.append(openNewsNewTabAct_);
  openNewsBackgroundTabAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_T));
  listActions_.append(openNewsBackgroundTabAct_);

  switchFocusAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Tab));
  listActions_.append(switchFocusAct_);
  switchFocusPrevAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Tab));
  listActions_.append(switchFocusPrevAct_);

  feedsWidgetVisibleAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
  listActions_.append(feedsWidgetVisibleAct_);

  listActions_.append(placeToTrayAct_);

  zoomInAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus));
  listActions_.append(zoomInAct_);
  zoomOutAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));
  listActions_.append(zoomOutAct_);
  zoomTo100Act_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
  listActions_.append(zoomTo100Act_);

  printAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
  listActions_.append(printAct_);
  printPreviewAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_P));
  listActions_.append(printPreviewAct_);

  savePageAsAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
  listActions_.append(savePageAsAct_);

  fullScreenAct_->setShortcut(QKeySequence(Qt::Key_F11));
  listActions_.append(fullScreenAct_);

  stayOnTopAct_->setShortcut(QKeySequence(Qt::Key_F10));
  listActions_.append(stayOnTopAct_);

  closeTabAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
  listActions_.append(closeTabAct_);
  listActions_.append(closeOtherTabsAct_);
  listActions_.append(closeAllTabsAct_);
  listActions_.append(nextTabAct_);
  listActions_.append(prevTabAct_);

  reduceNewsListAct_->setShortcut(QKeySequence(Qt::ALT+ Qt::Key_Up));
  listActions_.append(reduceNewsListAct_);
  increaseNewsListAct_->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Down));
  listActions_.append(increaseNewsListAct_);

  restoreLastNewsAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Z));
  listActions_.append(restoreLastNewsAct_);

  findTextAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F));
  listActions_.append(findTextAct_);

  listActions_.append(copyLinkAct_);

  listActions_.append(shareGroup_->actions());

  // Actions for labels do add at the end
  listActions_.append(newsLabelGroup_->actions());

  loadActionShortcuts();
}
// ---------------------------------------------------------------------------
void RSSListing::loadActionShortcuts()
{
  settings_->beginGroup("/Shortcuts");

  QListIterator<QAction *> iter(listActions_);
  while (iter.hasNext()) {
    QAction *pAction = iter.next();
    if (pAction->objectName().isEmpty())
      continue;

    listDefaultShortcut_.append(pAction->shortcut().toString());

    const QString& sKey = '/' + pAction->objectName();
    const QString& sValue = settings_->value('/' + sKey, pAction->shortcut().toString()).toString();
    pAction->setShortcut(QKeySequence(sValue));
  }

  settings_->endGroup();
}
// ---------------------------------------------------------------------------
void RSSListing::saveActionShortcuts()
{
  settings_->beginGroup("/Shortcuts/");

  QListIterator<QAction *> iter(listActions_);
  while (iter.hasNext()) {
    QAction *pAction = iter.next();
    if (pAction->objectName().isEmpty())
      continue;

    const QString& sKey = '/' + pAction->objectName();
    const QString& sValue = QString(pAction->shortcut().toString());
    settings_->setValue(sKey, sValue);
  }

  settings_->endGroup();
}
// ---------------------------------------------------------------------------
void RSSListing::createMenu()
{
  newMenu_ = new QMenu(this);
  newMenu_->addAction(addFeedAct_);
  newMenu_->addAction(addFolderAct_);
  addAct_->setMenu(newMenu_);

  fileMenu_ = new QMenu(this);
  fileMenu_->addAction(addAct_);
  fileMenu_->addSeparator();
  fileMenu_->addAction(importFeedsAct_);
  fileMenu_->addAction(exportFeedsAct_);
  fileMenu_->addSeparator();
  fileMenu_->addAction(exitAct_);
  menuBar()->addMenu(fileMenu_);

  editMenu_ = new QMenu(this);
  editMenu_->setVisible(false);
  //  menuBar()->addMenu(editMenu_);

  toolbarsMenu_ = new QMenu(this);
  toolbarsMenu_->addAction(mainToolbarToggle_);
  toolbarsMenu_->addAction(feedsToolbarToggle_);
  toolbarsMenu_->addAction(newsToolbarToggle_);
  toolbarsMenu_->addAction(browserToolbarToggle_);
  toolbarsMenu_->addAction(categoriesPanelToggle_);

  customizeToolbarGroup_ = new QActionGroup(this);
  customizeToolbarGroup_->addAction(customizeMainToolbarAct_);
  customizeToolbarGroup_->addAction(customizeFeedsToolbarAct_);
  customizeToolbarGroup_->addAction(customizeNewsToolbarAct_);
  connect(customizeToolbarGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(showCustomizeToolbarDlg(QAction*)));
  customizeToolbarMenu_ = new QMenu(this);
  customizeToolbarMenu_->addActions(customizeToolbarGroup_->actions());

  styleGroup_ = new QActionGroup(this);
  styleGroup_->addAction(systemStyle_);
  styleGroup_->addAction(system2Style_);
  styleGroup_->addAction(greenStyle_);
  styleGroup_->addAction(orangeStyle_);
  styleGroup_->addAction(purpleStyle_);
  styleGroup_->addAction(pinkStyle_);
  styleGroup_->addAction(grayStyle_);
  connect(styleGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(setStyleApp(QAction*)));

  styleMenu_ = new QMenu(this);
  styleMenu_->addActions(styleGroup_->actions());

  browserPositionGroup_ = new QActionGroup(this);
  browserPositionGroup_->addAction(topBrowserPositionAct_);
  browserPositionGroup_->addAction(bottomBrowserPositionAct_);
  browserPositionGroup_->addAction(rightBrowserPositionAct_);
  browserPositionGroup_->addAction(leftBrowserPositionAct_);
  connect(browserPositionGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(setBrowserPosition(QAction*)));

  browserPositionMenu_ = new QMenu(this);
  browserPositionMenu_->addActions(browserPositionGroup_->actions());

  viewMenu_  = new QMenu(this);
  viewMenu_->addMenu(toolbarsMenu_);
  viewMenu_->addMenu(customizeToolbarMenu_);
  viewMenu_->addSeparator();
  viewMenu_->addMenu(browserPositionMenu_);
  viewMenu_->addMenu(styleMenu_);
  viewMenu_->addSeparator();
  viewMenu_->addAction(stayOnTopAct_);
  viewMenu_->addAction(fullScreenAct_);
  menuBar()->addMenu(viewMenu_);

  feedMenu_ = new QMenu(this);
  feedMenu_->addAction(updateFeedAct_);
  feedMenu_->addAction(updateAllFeedsAct_);
  feedMenu_->addSeparator();
  feedMenu_->addAction(markFeedRead_);
  feedMenu_->addAction(markAllFeedsRead_);
  feedMenu_->addSeparator();
  menuBar()->addMenu(feedMenu_);

  feedsFilterGroup_ = new QActionGroup(this);
  feedsFilterGroup_->setExclusive(true);
  feedsFilterGroup_->addAction(filterFeedsAll_);
  feedsFilterGroup_->addAction(filterFeedsNew_);
  feedsFilterGroup_->addAction(filterFeedsUnread_);
  feedsFilterGroup_->addAction(filterFeedsStarred_);
  connect(feedsFilterGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(setFeedsFilter(QAction*)));

  feedsFilterMenu_ = new QMenu(this);
  feedsFilterMenu_->addActions(feedsFilterGroup_->actions());
  feedsFilterMenu_->insertSeparator(filterFeedsNew_);

  feedsFilter_->setMenu(feedsFilterMenu_);
  feedMenu_->addAction(feedsFilter_);
  feedsFilterAction_ = NULL;
  connect(feedsFilter_, SIGNAL(triggered()), this, SLOT(slotFeedsFilter()));

  feedsColumnsGroup_ = new QActionGroup(this);
  feedsColumnsGroup_->setExclusive(false);
  feedsColumnsGroup_->addAction(showUnreadCount_);
  feedsColumnsGroup_->addAction(showUndeleteCount_);
  feedsColumnsGroup_->addAction(showLastUpdated_);
  connect(feedsColumnsGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(feedsColumnVisible(QAction*)));

  feedsColumnsMenu_ = new QMenu(this);
  feedsColumnsMenu_->addActions(feedsColumnsGroup_->actions());
  feedMenu_->addMenu(feedsColumnsMenu_);

  feedMenu_->addAction(sortedByTitleFeedsTreeAct_);
  feedMenu_->addAction(indentationFeedsTreeAct_);

  feedMenu_->addSeparator();
  feedMenu_->addAction(deleteFeedAct_);
  feedMenu_->addSeparator();
  feedMenu_->addAction(feedProperties_);
  feedMenu_->addSeparator();
  //  feedMenu_->addAction(editFeedsTree_);
  connect(feedMenu_, SIGNAL(aboutToShow()), this, SLOT(slotFeedMenuShow()));

  newsMenu_ = new QMenu(this);
  newsMenu_->addAction(markNewsRead_);
  newsMenu_->addAction(markAllNewsRead_);
  newsMenu_->addSeparator();
  newsMenu_->addAction(markStarAct_);
  menuBar()->addMenu(newsMenu_);

  newsLabelMenu_ = new QMenu(this);
  newsLabelMenu_->addActions(newsLabelGroup_->actions());
  newsLabelMenuAction_ = new QAction(this);
  newsLabelMenuAction_->setIcon(QIcon(":/images/label_3"));
  newsLabelAction_->setMenu(newsLabelMenu_);
  newsLabelMenuAction_->setMenu(newsLabelMenu_);
  newsMenu_->addAction(newsLabelMenuAction_);
  connect(newsLabelMenu_, SIGNAL(aboutToShow()),
          this, SLOT(getLabelNews()));

  shareMenu_ = new QMenu(this);
  shareMenu_->addActions(shareGroup_->actions());
  shareMenuAct_ = new QAction(this);
  shareMenuAct_->setObjectName("shareMenuAct");
  shareMenuAct_->setIcon(QIcon(":/images/images/share.png"));
  shareMenuAct_->setMenu(shareMenu_);
  newsMenu_->addAction(shareMenuAct_);
  this->addAction(shareMenuAct_);

  connect(shareMenuAct_, SIGNAL(triggered()),
          this, SLOT(showMenuShareNews()));

  newsMenu_->addSeparator();

  newsFilterGroup_ = new QActionGroup(this);
  newsFilterGroup_->setExclusive(true);
  newsFilterGroup_->addAction(filterNewsAll_);
  newsFilterGroup_->addAction(filterNewsNew_);
  newsFilterGroup_->addAction(filterNewsUnread_);
  newsFilterGroup_->addAction(filterNewsStar_);
  newsFilterGroup_->addAction(filterNewsNotStarred_);
  newsFilterGroup_->addAction(filterNewsUnreadStar_);
  newsFilterGroup_->addAction(filterNewsLastDay_);
  newsFilterGroup_->addAction(filterNewsLastWeek_);
  connect(newsFilterGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(setNewsFilter(QAction*)));

  newsFilterMenu_ = new QMenu(this);
  newsFilterMenu_->addActions(newsFilterGroup_->actions());
  newsFilterMenu_->insertSeparator(filterNewsNew_);
  newsFilterMenu_->insertSeparator(filterNewsLastDay_);

  newsFilter_->setMenu(newsFilterMenu_);
  newsMenu_->addAction(newsFilter_);
  newsFilterAction_ = NULL;
  connect(newsFilter_, SIGNAL(triggered()), this, SLOT(slotNewsFilter()));

  newsSortByMenu_ = new QMenu(this);
  newsSortByMenu_->addSeparator();
  newsSortByMenu_->addActions(newsSortOrderGroup_->actions());

  newsMenu_->addMenu(newsSortByMenu_);
  connect(newsSortByMenu_, SIGNAL(aboutToShow()),
          this, SLOT(showNewsSortByMenu()));

  newsMenu_->addSeparator();
  newsMenu_->addAction(deleteNewsAct_);
  newsMenu_->addAction(deleteAllNewsAct_);
  connect(newsMenu_, SIGNAL(aboutToShow()),
          this, SLOT(showNewsMenu()));

  browserMenu_ = new QMenu(this);
  menuBar()->addMenu(browserMenu_);

  browserZoomGroup_ = new QActionGroup(this);
  browserZoomGroup_->addAction(zoomInAct_);
  browserZoomGroup_->addAction(zoomOutAct_);
  browserZoomGroup_->addAction(zoomTo100Act_);
  connect(browserZoomGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(browserZoom(QAction*)));

  browserZoomMenu_ = new QMenu(this);
  browserZoomMenu_->setIcon(QIcon(":/images/zoom"));
  browserZoomMenu_->addActions(browserZoomGroup_->actions());
  browserZoomMenu_->insertSeparator(zoomTo100Act_);

  browserMenu_->addAction(autoLoadImagesToggle_);
  browserMenu_->addMenu(browserZoomMenu_);
  browserMenu_->addSeparator();
  browserMenu_->addAction(printAct_);
  browserMenu_->addAction(printPreviewAct_);
  browserMenu_->addSeparator();
  browserMenu_->addAction(savePageAsAct_);

  toolsMenu_ = new QMenu(this);
  toolsMenu_->addAction(showDownloadManagerAct_);
  toolsMenu_->addSeparator();
  toolsMenu_->addAction(showCleanUpWizardAct_);
  toolsMenu_->addAction(setNewsFiltersAct_);
  toolsMenu_->addSeparator();
  toolsMenu_->addAction(optionsAct_);
  menuBar()->addMenu(toolsMenu_);

  helpMenu_ = new QMenu(this);
  helpMenu_->addAction(updateAppAct_);
  helpMenu_->addSeparator();
  helpMenu_->addAction(reportProblemAct_);
  helpMenu_->addAction(aboutAct_);
  menuBar()->addMenu(helpMenu_);
}
// ---------------------------------------------------------------------------
void RSSListing::createToolBar()
{
  mainToolbar_ = new QToolBar(this);
  mainToolbar_->setObjectName("ToolBar_General");
  mainToolbar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  mainToolbar_->setContextMenuPolicy(Qt::CustomContextMenu);
  addToolBar(mainToolbar_);

  connect(mainToolbarToggle_, SIGNAL(toggled(bool)),
          mainToolbar_, SLOT(setVisible(bool)));
  connect(mainToolbar_, SIGNAL(customContextMenuRequested(QPoint)),
          this, SLOT(showContextMenuToolBar(const QPoint &)));

  connect(toolBarLockAct_, SIGNAL(toggled(bool)), this, SLOT(lockMainToolbar(bool)));
  connect(toolBarHideAct_, SIGNAL(triggered()), this, SLOT(hideMainToolbar()));
}

/** @brief Read settings from ini-file
 *---------------------------------------------------------------------------*/
void RSSListing::readSettings()
{
  settings_->beginGroup("/Settings");

  showSplashScreen_ = settings_->value("showSplashScreen", true).toBool();
  reopenFeedStartup_ = settings_->value("reopenFeedStartup", true).toBool();
  openNewTabNextToActive_ = settings_->value("openNewTabNextToActive", false).toBool();

  showTrayIcon_ = settings_->value("showTrayIcon", true).toBool();
  startingTray_ = settings_->value("startingTray", false).toBool();
  minimizingTray_ = settings_->value("minimizingTray", true).toBool();
  closingTray_ = settings_->value("closingTray", false).toBool();
  singleClickTray_ = settings_->value("singleClickTray", false).toBool();
  clearStatusNew_ = settings_->value("clearStatusNew", true).toBool();
  emptyWorking_ = settings_->value("emptyWorking", true).toBool();

  QString strLang;
  QString strLocalLang = QLocale::system().name();
  bool findLang = false;
  QDir langDir = appDataDirPath_ + "/lang";
  foreach (QString file, langDir.entryList(QStringList("*.qm"), QDir::Files)) {
    strLang = file.section('.', 0, 0).section('_', 1);
    if (strLocalLang == strLang) {
      strLang = strLocalLang;
      findLang = true;
      break;
    }
  }
  if (!findLang) {
    strLocalLang = strLocalLang.left(2);
    foreach (QString file, langDir.entryList(QStringList("*.qm"), QDir::Files)) {
      strLang = file.section('.', 0, 0).section('_', 1);
      if (strLocalLang.contains(strLang, Qt::CaseInsensitive)) {
        strLang = strLocalLang;
        findLang = true;
        break;
      }
    }
  }
  if (!findLang) strLang = "en";

  langFileName_ = settings_->value("langFileName", strLang).toString();

  QString fontFamily = settings_->value("/feedsFontFamily", qApp->font().family()).toString();
  int fontSize = settings_->value("/feedsFontSize", 8).toInt();
  feedsTreeView_->setFont(QFont(fontFamily, fontSize));
  feedsTreeModel_->font_ = feedsTreeView_->font();

  newsListFontFamily_ = settings_->value("/newsFontFamily", qApp->font().family()).toString();
  newsListFontSize_ = settings_->value("/newsFontSize", 8).toInt();
  newsTitleFontFamily_ = settings_->value("/newsTitleFontFamily", qApp->font().family()).toString();
  newsTitleFontSize_ = settings_->value("/newsTitleFontSize", 10).toInt();
  newsTextFontFamily_ = settings_->value("/newsTextFontFamily", qApp->font().family()).toString();
  newsTextFontSize_ = settings_->value("/newsTextFontSize", 10).toInt();
  notificationFontFamily_ = settings_->value("/notificationFontFamily", qApp->font().family()).toString();
  notificationFontSize_ = settings_->value("/notificationFontSize", 8).toInt();
  browserMinFontSize_ = settings_->value("/browserMinFontSize", 0).toInt();
  browserMinLogFontSize_ = settings_->value("/browserMinLogFontSize", 0).toInt();

  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::StandardFont, newsTextFontFamily_);
  QWebSettings::globalSettings()->setFontSize(
        QWebSettings::MinimumFontSize, browserMinFontSize_);
  QWebSettings::globalSettings()->setFontSize(
        QWebSettings::MinimumLogicalFontSize, browserMinLogFontSize_);

  updateFeedsStartUp_ = settings_->value("autoUpdatefeedsStartUp", false).toBool();
  updateFeedsEnable_ = settings_->value("autoUpdatefeeds", false).toBool();
  updateFeedsInterval_ = settings_->value("autoUpdatefeedsTime", 10).toInt();
  updateFeedsIntervalType_ = settings_->value("autoUpdatefeedsInterval", 0).toInt();

  openingFeedAction_ = settings_->value("openingFeedAction", 0).toInt();
  openNewsWebViewOn_ = settings_->value("openNewsWebViewOn", true).toBool();

  markNewsReadOn_ = settings_->value("markNewsReadOn", true).toBool();
  markCurNewsRead_ = settings_->value("markCurNewsRead", true).toBool();
  markNewsReadTime_ = settings_->value("markNewsReadTime", 0).toInt();
  markPrevNewsRead_= settings_->value("markPrevNewsRead", false).toBool();
  markReadSwitchingFeed_ = settings_->value("markReadSwitchingFeed", false).toBool();
  markReadClosingTab_ = settings_->value("markReadClosingTab", false).toBool();
  markReadMinimize_ = settings_->value("markReadMinimize", false).toBool();

  showDescriptionNews_ = settings_->value("showDescriptionNews", true).toBool();

  formatDate_ = settings_->value("formatData", "dd.MM.yy").toString();
  formatTime_ = settings_->value("formatTime", "hh:mm").toString();
  feedsTreeModel_->formatDate_ = formatDate_;
  feedsTreeModel_->formatTime_ = formatTime_;

  alternatingRowColorsNews_ = settings_->value("alternatingColorsNews", false).toBool();
  changeBehaviorActionNUN_ = settings_->value("changeBehaviorActionNUN", false).toBool();
  simplifiedDateTime_ = settings_->value("simplifiedDateTime", true).toBool();
  notDeleteStarred_ = settings_->value("notDeleteStarred", false).toBool();
  notDeleteLabeled_ = settings_->value("notDeleteLabeled", false).toBool();
  markIdenticalNewsRead_ = settings_->value("markIdenticalNewsRead", true).toBool();

  mainNewsFilter_ = settings_->value("mainNewsFilter", "filterNewsAll_").toString();

  cleanupOnShutdown_ = settings_->value("cleanupOnShutdown", true).toBool();
  maxDayCleanUp_ = settings_->value("maxDayClearUp", 30).toInt();
  maxNewsCleanUp_ = settings_->value("maxNewsClearUp", 200).toInt();
  dayCleanUpOn_ = settings_->value("dayClearUpOn", true).toBool();
  newsCleanUpOn_ = settings_->value("newsClearUpOn", true).toBool();
  readCleanUp_ = settings_->value("readClearUp", false).toBool();
  neverUnreadCleanUp_ = settings_->value("neverUnreadClearUp", true).toBool();
  neverStarCleanUp_ = settings_->value("neverStarClearUp", true).toBool();
  neverLabelCleanUp_ = settings_->value("neverLabelClearUp", true).toBool();
  cleanUpDeleted_ = settings_->value("cleanUpDeleted", false).toBool();
  optimizeDB_ = settings_->value("optimizeDB", false).toBool();

  externalBrowserOn_ = settings_->value("externalBrowserOn", 0).toInt();
  externalBrowser_ = settings_->value("externalBrowser", "").toString();
  javaScriptEnable_ = settings_->value("javaScriptEnable", true).toBool();
  pluginsEnable_ = settings_->value("pluginsEnable", true).toBool();
  userStyleBrowser_ = settings_->value("userStyleBrowser", "").toString();
  maxPagesInCache_ = settings_->value("maxPagesInCache", 3).toInt();
  downloadLocation_ = settings_->value("downloadLocation", "").toString();
  askDownloadLocation_ = settings_->value("askDownloadLocation", true).toBool();

  QWebSettings::globalSettings()->setAttribute(
        QWebSettings::JavascriptEnabled, javaScriptEnable_);
  QWebSettings::globalSettings()->setAttribute(
        QWebSettings::PluginsEnabled, pluginsEnable_);
  QWebSettings::globalSettings()->setMaximumPagesInCache(maxPagesInCache_);
  QWebSettings::globalSettings()->setUserStyleSheetUrl(userStyleSheet(userStyleBrowser_));

  soundNewNews_ = settings_->value("soundNewNews", true).toBool();
  QString soundNotifyPathStr = appDataDirPath_ + "/sound/notification.wav";
  soundNotifyPath_ = settings_->value("soundNotifyPath", soundNotifyPathStr).toString();
  showNotifyOn_ = settings_->value("showNotifyOn", true).toBool();
  positionNotify_ = settings_->value("positionNotify", 3).toInt();
  countShowNewsNotify_ = settings_->value("countShowNewsNotify", 10).toInt();
  widthTitleNewsNotify_ = settings_->value("widthTitleNewsNotify", 300).toInt();
  timeShowNewsNotify_ = settings_->value("timeShowNewsNotify", 10).toInt();
  fullscreenModeNotify_ = settings_->value("fullscreenModeNotify", true).toBool();
  onlySelectedFeeds_ = settings_->value("onlySelectedFeeds", false).toBool();

  toolBarLockAct_->setChecked(settings_->value("mainToolbarLock", true).toBool());
  lockMainToolbar(toolBarLockAct_->isChecked());

  mainToolbarToggle_->setChecked(settings_->value("mainToolbarShow", true).toBool());
  feedsToolbarToggle_->setChecked(settings_->value("feedsToolbarShow", true).toBool());
  newsToolbarToggle_->setChecked(settings_->value("newsToolbarShow", true).toBool());
  browserToolbarToggle_->setChecked(settings_->value("browserToolbarShow", true).toBool());
  categoriesPanelToggle_->setChecked(settings_->value("categoriesPanelShow", true).toBool());
  categoriesWidget_->setVisible(categoriesPanelToggle_->isChecked());

  if (!mainToolbarToggle_->isChecked())
    mainToolbar_->hide();
  if (!feedsToolbarToggle_->isChecked())
    feedsPanel_->hide();

  QString str = settings_->value("mainToolBar",
                                 "newAct,Separator,updateFeedAct,updateAllFeedsAct,"
                                 "Separator,markFeedRead,Separator,autoLoadImagesToggle").toString();

  foreach (QString actionStr, str.split(",", QString::SkipEmptyParts)) {
    if (actionStr == "Separator") {
      mainToolbar_->addSeparator();
    } else {
      QListIterator<QAction *> iter(actions());
      while (iter.hasNext()) {
        QAction *pAction = iter.next();
        if (!pAction->icon().isNull()) {
          if (pAction->objectName() == actionStr) {
            mainToolbar_->addAction(pAction);
            break;
          }
        }
      }
    }
  }

  str = settings_->value("feedsToolBar", "findFeedAct,feedsFilter").toString();

  foreach (QString actionStr, str.split(",", QString::SkipEmptyParts)) {
    if (actionStr == "Separator") {
      feedsToolBar_->addSeparator();
    } else {
      QListIterator<QAction *> iter(actions());
      while (iter.hasNext()) {
        QAction *pAction = iter.next();
        if (!pAction->icon().isNull()) {
          if (pAction->objectName() == actionStr) {
            feedsToolBar_->addAction(pAction);
            break;
          }
        }
      }
    }
  }

  setToolBarStyle(settings_->value("toolBarStyle", "toolBarStyleTuI_").toString());
  setToolBarIconSize(settings_->value("toolBarIconSize", "toolBarIconNormal_").toString());

  str = settings_->value("styleApplication", "defaultStyle_").toString();
  QList<QAction*> listActions = styleGroup_->actions();
  foreach(QAction *action, listActions) {
    if (action->objectName() == str) {
      action->setChecked(true);
      break;
    }
  }

  showUnreadCount_->setChecked(settings_->value("showUnreadCount", true).toBool());
  showUndeleteCount_->setChecked(settings_->value("showUndeleteCount", false).toBool());
  showLastUpdated_->setChecked(settings_->value("showLastUpdated", false).toBool());
  feedsColumnVisible(showUnreadCount_);
  feedsColumnVisible(showUndeleteCount_);
  feedsColumnVisible(showLastUpdated_);

  indentationFeedsTreeAct_->setChecked(settings_->value("indentationFeedsTree", true).toBool());
  slotIndentationFeedsTree();

  browserPosition_ = settings_->value("browserPosition", BOTTOM_POSITION).toInt();
  switch (browserPosition_) {
  case TOP_POSITION:   topBrowserPositionAct_->setChecked(true); break;
  case RIGHT_POSITION: rightBrowserPositionAct_->setChecked(true); break;
  case LEFT_POSITION:  leftBrowserPositionAct_->setChecked(true); break;
  default: bottomBrowserPositionAct_->setChecked(true);
  }

  openLinkInBackground_ = settings_->value("openLinkInBackground", true).toBool();
  openLinkInBackgroundEmbedded_ = settings_->value("openLinkInBackgroundEmbedded", true).toBool();
  openingLinkTimeout_ = settings_->value("openingLinkTimeout", 1000).toInt();

  stayOnTopAct_->setChecked(settings_->value("stayOnTop", false).toBool());
  if (stayOnTopAct_->isChecked())
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
  else
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);

  updateCheckEnabled_ = settings_->value("updateCheckEnabled", true).toBool();

  hideFeedsOpenTab_ = settings_->value("hideFeedsOpenTab", false).toBool();
  showToggleFeedsTree_ = settings_->value("showToggleFeedsTree", true).toBool();
  pushButtonNull_->setVisible(showToggleFeedsTree_);

  defaultIconFeeds_ = settings_->value("defaultIconFeeds", false).toBool();
  feedsTreeModel_->defaultIconFeeds_ = defaultIconFeeds_;
  feedsTreeView_->autocollapseFolder_ =
      settings_->value("autocollapseFolder", false).toBool();

  settings_->endGroup();

  settings_->beginGroup("ClickToFlash");
  c2fWhitelist_ = settings_->value("whitelist", QStringList()).toStringList();
  c2fEnabled_ = settings_->value("enabled", true).toBool();
  settings_->endGroup();

  settings_->beginGroup("Color");
  QString windowTextColor = qApp->palette().brush(QPalette::WindowText).color().name();
  feedsTreeModel_->textColor_ = settings_->value("feedsListTextColor", windowTextColor).toString();
  feedsTreeModel_->backgroundColor_ = settings_->value("feedsListBackgroundColor", "").toString();
  feedsTreeView_->setStyleSheet(QString("#feedsTreeView_ {background: %1;}").arg(feedsTreeModel_->backgroundColor_));
  newsListTextColor_ = settings_->value("newsListTextColor", windowTextColor).toString();
  newsListBackgroundColor_ = settings_->value("newsListBackgroundColor", "").toString();
  focusedNewsTextColor_ = settings_->value("focusedNewsTextColor", windowTextColor).toString();
  focusedNewsBGColor_ = settings_->value("focusedNewsBGColor", "").toString();
  linkColor_ = settings_->value("linkColor", "#0066CC").toString();
  titleColor_ = settings_->value("titleColor", "#0066CC").toString();
  dateColor_ = settings_->value("dateColor", "#666666").toString();
  authorColor_ = settings_->value("authorColor", "#666666").toString();
  newsTitleBackgroundColor_ = settings_->value("newsTitleBackgroundColor", "#FFFFFF").toString();
  newsBackgroundColor_ = settings_->value("newsBackgroundColor", "#FFFFFF").toString();
  feedsTreeModel_->feedWithNewNewsColor_ =
      settings_->value("feedWithNewNewsColor", qApp->palette().brush(QPalette::Link).color().name()).toString();
  feedsTreeModel_->countNewsUnreadColor_ =
      settings_->value("countNewsUnreadColor", qApp->palette().brush(QPalette::Link).color().name()).toString();
  settings_->endGroup();

  resize(800, 600);
  restoreGeometry(settings_->value("GeometryState").toByteArray());
  restoreState(settings_->value("ToolBarsState").toByteArray());

  mainSplitter_->restoreState(settings_->value("MainSplitterState").toByteArray());
  feedsWidgetVisibleAct_->setChecked(settings_->value("FeedsWidgetVisible", true).toBool());
  slotVisibledFeedsWidget();

  feedsWidgetSplitterState_ = settings_->value("FeedsWidgetSplitterState").toByteArray();
  bool showCategories = settings_->value("NewsCategoriesTreeVisible", true).toBool();
  categoriesTree_->setVisible(showCategories);
  if (showCategories) {
    showCategoriesButton_->setIcon(QIcon(":/images/images/panel_hide.png"));
    showCategoriesButton_->setToolTip(tr("Hide Categories"));
    feedsSplitter_->restoreState(feedsWidgetSplitterState_);
  } else {
    showCategoriesButton_->setIcon(QIcon(":/images/images/panel_show.png"));
    showCategoriesButton_->setToolTip(tr("Show Categories"));
    QList <int> sizes;
    sizes << QApplication::desktop()->height() << 20;
    feedsSplitter_->setSizes(sizes);
  }
  bool expandCategories = settings_->value("categoriesTreeExpanded", true).toBool();
  if (expandCategories)
      categoriesTree_->expandAll();

  if (isFullScreen())
    menuBar()->hide();

  networkProxy_.setType(static_cast<QNetworkProxy::ProxyType>(
                          settings_->value("networkProxy/type", QNetworkProxy::DefaultProxy).toInt()));
  networkProxy_.setHostName(settings_->value("networkProxy/hostName", "").toString());
  networkProxy_.setPort(    settings_->value("networkProxy/port",     "").toUInt());
  networkProxy_.setUser(    settings_->value("networkProxy/user",     "").toString());
  networkProxy_.setPassword(settings_->value("networkProxy/password", "").toString());
  setProxy(networkProxy_);
}

/** @brief Write settings in ini-file
 *---------------------------------------------------------------------------*/
void RSSListing::writeSettings()
{
  settings_->beginGroup("/Settings");

  settings_->setValue("showSplashScreen", showSplashScreen_);
  settings_->setValue("reopenFeedStartup", reopenFeedStartup_);
  settings_->setValue("openNewTabNextToActive", openNewTabNextToActive_);

  settings_->setValue("storeDBMemory", storeDBMemoryT_);

  settings_->setValue("createLastFeed", !lastFeedPath_.isEmpty());

  settings_->setValue("showTrayIcon", showTrayIcon_);
  settings_->setValue("startingTray", startingTray_);
  settings_->setValue("minimizingTray", minimizingTray_);
  settings_->setValue("closingTray", closingTray_);
  settings_->setValue("behaviorIconTray", behaviorIconTray_);
  settings_->setValue("singleClickTray", singleClickTray_);
  settings_->setValue("clearStatusNew", clearStatusNew_);
  settings_->setValue("emptyWorking", emptyWorking_);

  settings_->setValue("langFileName", langFileName_);

  QString fontFamily = feedsTreeView_->font().family();
  settings_->setValue("/feedsFontFamily", fontFamily);
  int fontSize = feedsTreeView_->font().pointSize();
  settings_->setValue("/feedsFontSize", fontSize);

  settings_->setValue("/newsFontFamily", newsListFontFamily_);
  settings_->setValue("/newsFontSize", newsListFontSize_);
  settings_->setValue("/newsTitleFontFamily", newsTitleFontFamily_);
  settings_->setValue("/newsTitleFontSize", newsTitleFontSize_);
  settings_->setValue("/newsTextFontFamily", newsTextFontFamily_);
  settings_->setValue("/newsTextFontSize", newsTextFontSize_);
  settings_->setValue("/notificationFontFamily", notificationFontFamily_);
  settings_->setValue("/notificationFontSize", notificationFontSize_);
  settings_->setValue("/browserMinFontSize", browserMinFontSize_);
  settings_->setValue("/browserMinLogFontSize", browserMinLogFontSize_);

  settings_->setValue("autoUpdatefeedsStartUp", updateFeedsStartUp_);
  settings_->setValue("autoUpdatefeeds", updateFeedsEnable_);
  settings_->setValue("autoUpdatefeedsTime", updateFeedsInterval_);
  settings_->setValue("autoUpdatefeedsInterval", updateFeedsIntervalType_);

  settings_->setValue("openingFeedAction", openingFeedAction_);
  settings_->setValue("openNewsWebViewOn", openNewsWebViewOn_);

  settings_->setValue("markNewsReadOn", markNewsReadOn_);
  settings_->setValue("markCurNewsRead", markCurNewsRead_);
  settings_->setValue("markNewsReadTime", markNewsReadTime_);
  settings_->setValue("markPrevNewsRead", markPrevNewsRead_);
  settings_->setValue("markReadSwitchingFeed", markReadSwitchingFeed_);
  settings_->setValue("markReadClosingTab", markReadClosingTab_);
  settings_->setValue("markReadMinimize", markReadMinimize_);

  settings_->setValue("showDescriptionNews", showDescriptionNews_);

  settings_->setValue("formatData", formatDate_);
  settings_->setValue("formatTime", formatTime_);

  settings_->setValue("alternatingColorsNews", alternatingRowColorsNews_);
  settings_->setValue("changeBehaviorActionNUN", changeBehaviorActionNUN_);
  settings_->setValue("simplifiedDateTime", simplifiedDateTime_);
  settings_->setValue("notDeleteStarred", notDeleteStarred_);
  settings_->setValue("notDeleteLabeled", notDeleteLabeled_);
  settings_->setValue("markIdenticalNewsRead", markIdenticalNewsRead_);

  settings_->setValue("mainNewsFilter", mainNewsFilter_);

  settings_->setValue("cleanupOnShutdown", cleanupOnShutdown_);
  settings_->setValue("maxDayClearUp", maxDayCleanUp_);
  settings_->setValue("maxNewsClearUp", maxNewsCleanUp_);
  settings_->setValue("dayClearUpOn", dayCleanUpOn_);
  settings_->setValue("newsClearUpOn", newsCleanUpOn_);
  settings_->setValue("readClearUp", readCleanUp_);
  settings_->setValue("neverUnreadClearUp", neverUnreadCleanUp_);
  settings_->setValue("neverStarClearUp", neverStarCleanUp_);
  settings_->setValue("neverLabelClearUp", neverLabelCleanUp_);
  settings_->setValue("cleanUpDeleted", cleanUpDeleted_);
  settings_->setValue("optimizeDB", optimizeDB_);

  settings_->setValue("externalBrowserOn", externalBrowserOn_);
  settings_->setValue("externalBrowser", externalBrowser_);
  settings_->setValue("javaScriptEnable", javaScriptEnable_);
  settings_->setValue("pluginsEnable", pluginsEnable_);
  settings_->setValue("userStyleBrowser", userStyleBrowser_);
  settings_->setValue("maxPagesInCache", maxPagesInCache_);
  settings_->setValue("downloadLocation", downloadLocation_);
  settings_->setValue("saveCookies", cookieJar_->saveCookies_);
  settings_->setValue("askDownloadLocation", askDownloadLocation_);

  settings_->setValue("soundNewNews", soundNewNews_);
  settings_->setValue("soundNotifyPath", soundNotifyPath_);
  settings_->setValue("showNotifyOn", showNotifyOn_);
  settings_->setValue("positionNotify", positionNotify_);
  settings_->setValue("countShowNewsNotify", countShowNewsNotify_);
  settings_->setValue("widthTitleNewsNotify", widthTitleNewsNotify_);
  settings_->setValue("timeShowNewsNotify", timeShowNewsNotify_);
  settings_->setValue("fullscreenModeNotify", fullscreenModeNotify_);
  settings_->setValue("onlySelectedFeeds", onlySelectedFeeds_);

  settings_->setValue("mainToolbarLock", toolBarLockAct_->isChecked());

  settings_->setValue("mainToolbarShow", mainToolbarToggle_->isChecked());
  settings_->setValue("feedsToolbarShow", feedsToolbarToggle_->isChecked());
  settings_->setValue("newsToolbarShow", newsToolbarToggle_->isChecked());
  settings_->setValue("browserToolbarShow", browserToolbarToggle_->isChecked());
  settings_->setValue("categoriesPanelShow", categoriesPanelToggle_->isChecked());

  settings_->setValue("styleApplication",
                      styleGroup_->checkedAction()->objectName());

  settings_->setValue("showUnreadCount", showUnreadCount_->isChecked());
  settings_->setValue("showUndeleteCount", showUndeleteCount_->isChecked());
  settings_->setValue("showLastUpdated", showLastUpdated_->isChecked());

  settings_->setValue("indentationFeedsTree", indentationFeedsTreeAct_->isChecked());

  settings_->setValue("browserPosition", browserPosition_);

  settings_->setValue("openLinkInBackground", openLinkInBackground_);
  settings_->setValue("openLinkInBackgroundEmbedded", openLinkInBackgroundEmbedded_);
  settings_->setValue("openingLinkTimeout", openingLinkTimeout_);

  settings_->setValue("stayOnTop", stayOnTopAct_->isChecked());

  settings_->setValue("updateCheckEnabled", updateCheckEnabled_);

  settings_->setValue("hideFeedsOpenTab", hideFeedsOpenTab_);
  settings_->setValue("showToggleFeedsTree", showToggleFeedsTree_);

  settings_->setValue("defaultIconFeeds", defaultIconFeeds_);
  settings_->setValue("autocollapseFolder", feedsTreeView_->autocollapseFolder_);

  settings_->endGroup();

  settings_->beginGroup("Color");
  settings_->setValue("feedsListTextColor", feedsTreeModel_->textColor_);
  settings_->setValue("feedsListBackgroundColor", feedsTreeModel_->backgroundColor_);
  settings_->setValue("newsListTextColor", newsListTextColor_);
  settings_->setValue("newsListBackgroundColor", newsListBackgroundColor_);
  settings_->setValue("focusedNewsTextColor", focusedNewsTextColor_);
  settings_->setValue("focusedNewsBGColor", focusedNewsBGColor_);
  settings_->setValue("linkColor", linkColor_);
  settings_->setValue("titleColor", titleColor_);
  settings_->setValue("dateColor", dateColor_);
  settings_->setValue("authorColor", authorColor_);
  settings_->setValue("newsTitleBackgroundColor", newsTitleBackgroundColor_);
  settings_->setValue("newsBackgroundColor", newsBackgroundColor_);
  settings_->setValue("feedWithNewNewsColor", feedsTreeModel_->feedWithNewNewsColor_);
  settings_->setValue("countNewsUnreadColor", feedsTreeModel_->countNewsUnreadColor_);
  settings_->endGroup();

  settings_->beginGroup("ClickToFlash");
  settings_->setValue("whitelist", c2fWhitelist_);
  settings_->setValue("enabled", c2fEnabled_);
  settings_->endGroup();

  settings_->setValue("GeometryState", saveGeometry());
  settings_->setValue("ToolBarsState", saveState());

  settings_->setValue("MainSplitterState", mainSplitter_->saveState());
  settings_->setValue("FeedsWidgetVisible", showFeedsTabPermanent_);

  bool newsCategoriesTreeVisible = true;
  if (categoriesWidget_->height() <= (categoriesPanel_->height()+2)) {
    newsCategoriesTreeVisible = false;
    settings_->setValue("FeedsWidgetSplitterState", feedsWidgetSplitterState_);
  } else {
    settings_->setValue("FeedsWidgetSplitterState", feedsSplitter_->saveState());
  }
  settings_->setValue("NewsCategoriesTreeVisible", newsCategoriesTreeVisible);
  settings_->setValue("categoriesTreeExpanded", categoriesTree_->topLevelItem(3)->isExpanded());

  if (stackedWidget_->count()) {
    NewsTabWidget *widget;
    if (currentNewsTab->type_ < TAB_WEB)
      widget = currentNewsTab;
    else
      widget = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);

    widget->newsHeader_->saveStateColumns(this, widget);
    settings_->setValue("NewsTabSplitterState",
                        widget->newsTabWidgetSplitter_->saveState());
  }

  settings_->setValue("networkProxy/type",     networkProxy_.type());
  settings_->setValue("networkProxy/hostName", networkProxy_.hostName());
  settings_->setValue("networkProxy/port",     networkProxy_.port());
  settings_->setValue("networkProxy/user",     networkProxy_.user());
  settings_->setValue("networkProxy/password", networkProxy_.password());

  NewsTabWidget* widget = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);
  settings_->setValue("feedSettings/currentId", widget->feedId_);
  settings_->setValue("feedSettings/filterName",
                      feedsFilterGroup_->checkedAction()->objectName());
  settings_->setValue("newsSettings/filterName",
                      newsFilterGroup_->checkedAction()->objectName());
}
// ---------------------------------------------------------------------------
void RSSListing::setProxy(const QNetworkProxy proxy)
{
  networkProxy_ = proxy;
  if (QNetworkProxy::DefaultProxy == networkProxy_.type())
    QNetworkProxyFactory::setUseSystemConfiguration(true);
  else
    QNetworkProxy::setApplicationProxy(networkProxy_);
}

/** @brief Add feed to feed list
 *---------------------------------------------------------------------------*/
void RSSListing::addFeed()
{
  int curFolderId = 0;
  QPersistentModelIndex curIndex = feedsTreeView_->selectIndex();
  if (feedsTreeModel_->isFolder(curIndex)) {
    curFolderId = feedsTreeModel_->getIdByIndex(curIndex);
  } else {
    curFolderId = feedsTreeModel_->getParidByIndex(curIndex);
  }

  AddFeedWizard *addFeedWizard = new AddFeedWizard(this, curFolderId);
  addFeedWizard->restoreGeometry(settings_->value("addFeedWizard/geometry").toByteArray());

  int result = addFeedWizard->exec();
  settings_->setValue("addFeedWizard/geometry", addFeedWizard->saveGeometry());
  if (result == QDialog::Rejected) {
    delete addFeedWizard;
    return;
  }

  emit faviconRequestUrl(addFeedWizard->htmlUrlString_, addFeedWizard->feedUrlString_);

  QList<int> categoriesList;
  categoriesList << addFeedWizard->feedParentId_;
  recountFeedCategories(categoriesList);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  feedsTreeView_->setCurrentIndex(QModelIndex());
  feedsModelReload();
  QModelIndex index = feedsTreeModel_->getIndexById(addFeedWizard->feedId_);
  feedsTreeView_->selectIdEn_ = true;
  feedsTreeView_->setCurrentIndex(index);
  slotFeedClicked(index);
  QApplication::restoreOverrideCursor();
  slotUpdateFeedDelayed(addFeedWizard->feedUrlString_, true, addFeedWizard->newCount_);

  delete addFeedWizard;
}

/** @brief Add folder to feed list
 *---------------------------------------------------------------------------*/
void RSSListing::addFolder()
{
  int curFolderId = 0;
  QPersistentModelIndex curIndex = feedsTreeView_->selectIndex();
  if (feedsTreeModel_->isFolder(curIndex)) {
    curFolderId = feedsTreeModel_->getIdByIndex(curIndex);
  } else {
    curFolderId = feedsTreeModel_->getParidByIndex(curIndex);
  }

  AddFolderDialog *addFolderDialog = new AddFolderDialog(this, curFolderId);

  if (addFolderDialog->exec() == QDialog::Rejected) {
    delete addFolderDialog;
    return;
  }

  QString folderText = addFolderDialog->nameFeedEdit_->text();
  int parentId = addFolderDialog->foldersTree_->currentItem()->text(1).toInt();

  QSqlQuery q;

  // Determine row number for folder
  int rowToParent = 0;
  q.exec(QString("SELECT count(id) FROM feeds WHERE parentId='%1'").
         arg(parentId));
  if (q.next()) rowToParent = q.value(0).toInt();

  // Add feed to DB
  q.prepare("INSERT INTO feeds(text, created, parentId, rowToParent) "
            "VALUES (:text, :feedCreateTime, :parentId, :rowToParent)");
  q.bindValue(":text", folderText);
  q.bindValue(":feedCreateTime",
              QLocale::c().toString(QDateTime::currentDateTimeUtc(), "yyyy-MM-ddTHH:mm:ss"));
  q.bindValue(":parentId", parentId);
  q.bindValue(":rowToParent", rowToParent);
  q.exec();

  delete addFolderDialog;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  feedsModelReload();
  QApplication::restoreOverrideCursor();
}

/** @brief Delete feed list item with confirmation
 *---------------------------------------------------------------------------*/
void RSSListing::deleteItemFeedsTree()
{
  if (!feedsTreeView_->selectIndex().isValid()) return;

  QPersistentModelIndex index = feedsTreeView_->selectIndex();
  int feedDeleteId = feedsTreeModel_->getIdByIndex(index);
  int feedParentId = feedsTreeModel_->getParidByIndex(index);

  QPersistentModelIndex currentIndex = feedsTreeView_->currentIndex();
  int feedCurrentId = feedsTreeModel_->getIdByIndex(currentIndex);

  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  if (feedsTreeModel_->isFolder(index)) {
    msgBox.setWindowTitle(tr("Delete Folder"));
    msgBox.setText(QString(tr("Are you sure to delete the folder '%1'?")).
                   arg(feedsTreeModel_->dataField(index, "text").toString()));
  } else {
    msgBox.setWindowTitle(tr("Delete Feed"));
    msgBox.setText(QString(tr("Are you sure to delete the feed '%1'?")).
                   arg(feedsTreeModel_->dataField(index, "text").toString()));
  }
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::No);

  if (msgBox.exec() == QMessageBox::No) return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  db_.transaction();
  QSqlQuery q;
  QString idStr(QString("id='%1'").arg(feedDeleteId));
  QString feedIdStr(QString("feedId='%1'").arg(feedDeleteId));
  QQueue<int> parentIds;
  parentIds.enqueue(feedDeleteId);
  while (!parentIds.empty()) {
    int parentId = parentIds.dequeue();
    q.exec(QString("SELECT id FROM feeds WHERE parentId='%1'").arg(parentId));
    while (q.next()) {
      int feedId = q.value(0).toInt();

      idStr.append(QString(" OR id='%1'").arg(feedId));
      feedIdStr.append(QString(" OR feedId='%1'").arg(feedId));

      parentIds.enqueue(feedId);
    }
  }

  q.exec(QString("DELETE FROM feeds WHERE %1").arg(idStr));
  q.exec(QString("DELETE FROM news WHERE %1").arg(feedIdStr));

  // Correction row
  QList<int> idList;
  q.exec(QString("SELECT id FROM feeds WHERE parentId='%1' ORDER BY rowToParent").
         arg(feedParentId));
  while (q.next()) {
    idList << q.value(0).toInt();
  }
  for (int i = 0; i < idList.count(); i++) {
    q.exec(QString("UPDATE feeds SET rowToParent='%1' WHERE id=='%2'").
           arg(i).arg(idList.at(i)));
  }
  db_.commit();

  QList<int> categoriesList;
  categoriesList << feedParentId;
  recountFeedCategories(categoriesList);

  // Set focus on previous feed when deleting last feed in list.
  // In other case focus disappears.
  // Compare ids as selectedIndex after hiding popup menu points to
  // currentIndex()
  if (feedCurrentId == feedDeleteId) {
    QModelIndex index = feedsTreeView_->indexBelow(currentIndex);
    if (!index.isValid())
      index = feedsTreeView_->indexAbove(currentIndex);
    currentIndex = index;
  }
  feedsTreeView_->setCurrentIndex(currentIndex);
  feedsModelReload();
  slotFeedClicked(feedsTreeView_->currentIndex());

  QApplication::restoreOverrideCursor();
}

/** @brief Import feeds from OPML-file
 *
 * Calls open file system dialog with filter *.opml.
 * Adds all feeds to DB include hierarchy, ignore duplicate feeds
 *---------------------------------------------------------------------------*/
void RSSListing::slotImportFeeds()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select OPML-File"),
                                                  QDir::homePath(),
                                                  tr("OPML-Files (*.opml *.xml)"));

  if (fileName.isNull()) {
    statusBar()->showMessage(tr("Import canceled"), 3000);
    return;
  }

  qDebug() << "import file:" << fileName;

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    statusBar()->showMessage(tr("Import: can't open a file"), 3000);
    return;
  }

  importFeedStart_ = true;

  timer_.start();
  //  qCritical() << "Start update";
  //  qCritical() << "------------------------------------------------------------";

  db_.transaction();

  QXmlStreamReader xml(&file);

  int elementCount = 0;
  int outlineCount = 0;
  QSqlQuery q;

  // Store hierarchy of "outline" tags. Next nested outline is pushed to stack.
  // When it closes, pop it out from stack. Top of stack is the root outline.
  QStack<int> parentIdsStack;
  parentIdsStack.push(0);
  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isStartElement()) {
      statusBar()->showMessage(QVariant(elementCount).toString(), 3000);
      // Search for "outline" only
      if (xml.name() == "outline") {
        qDebug() << outlineCount << "+:" << xml.prefix().toString()
                 << ":" << xml.name().toString();

        QString textString(xml.attributes().value("text").toString());
        QString titleString(xml.attributes().value("title").toString());
        QString xmlUrlString(xml.attributes().value("xmlUrl").toString());
        if (textString.isEmpty()) textString = titleString;

        //Folder finded
        if (xmlUrlString.isEmpty()) {

          // If this folder exists, then add feeds to it...
          bool isFolderDuplicated = false;
          q.prepare("SELECT id FROM feeds WHERE text=:text AND (xmlUrl='' OR xmlUrl IS NULL)");
          q.bindValue(":text", textString);
          q.exec();
          if (q.next()) {
            isFolderDuplicated = true;
            parentIdsStack.push(q.value(0).toInt());
          }

          // ... If not - create it
          if (!isFolderDuplicated) {
            int rowToParent = 0;
            q.exec(QString("SELECT count(id) FROM feeds WHERE parentId='%1'").
                   arg(parentIdsStack.top()));
            if (q.next()) rowToParent = q.value(0).toInt();

            q.prepare("INSERT INTO feeds(text, title, xmlUrl, created, f_Expanded, parentId, rowToParent) "
                      "VALUES (:text, :title, :xmlUrl, :feedCreateTime, 0, :parentId, :rowToParent)");
            q.bindValue(":text", textString);
            q.bindValue(":title", textString);
            q.bindValue(":xmlUrl", "");
            q.bindValue(":feedCreateTime",
                        QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
            q.bindValue(":parentId", parentIdsStack.top());
            q.bindValue(":rowToParent", rowToParent);
            q.exec();
            parentIdsStack.push(q.lastInsertId().toInt());
            // qDebug() << q.lastQuery() << q.boundValues() << q.lastInsertId();
            // qDebug() << q.lastError().number() << ": " << q.lastError().text();
          }
        }
        // Feed finded
        else {
          bool isFeedDuplicated = false;
          q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
          q.bindValue(":xmlUrl", xmlUrlString);
          q.exec();
          if (q.next())
            isFeedDuplicated = true;

          if (isFeedDuplicated) {
            qDebug() << "duplicate feed:" << xmlUrlString << textString;
          } else {
            int rowToParent = 0;
            q.exec(QString("SELECT count(id) FROM feeds WHERE parentId='%1'").
                   arg(parentIdsStack.top()));
            if (q.next()) rowToParent = q.value(0).toInt();

            q.prepare("INSERT INTO feeds(text, title, description, xmlUrl, htmlUrl, created, parentId, rowToParent) "
                      "VALUES(?, ?, ?, ?, ?, ?, ?, ?)");
            q.addBindValue(textString);
            q.addBindValue(xml.attributes().value("title").toString());
            q.addBindValue(xml.attributes().value("description").toString());
            q.addBindValue(xmlUrlString);
            q.addBindValue(xml.attributes().value("htmlUrl").toString());
            q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
            q.addBindValue(parentIdsStack.top());
            q.addBindValue(rowToParent);
            q.exec();
            // qDebug() << q.lastQuery() << q.boundValues();
            // qDebug() << q.lastError().number() << ": " << q.lastError().text();

            updateFeedsCount_ = updateFeedsCount_ + 2;
            emit signalRequestUrl(xmlUrlString, QDateTime(), "");
          }
          parentIdsStack.push(q.lastInsertId().toInt());
        }
      }
    } else if (xml.isEndElement()) {
      if (xml.name() == "outline") {
        parentIdsStack.pop();
        ++outlineCount;
      }
      ++elementCount;
    }
    qDebug() << parentIdsStack;
  }
  if (xml.error()) {
    statusBar()->showMessage(QString("Import error: Line=%1, ErrorString=%2").
                             arg(xml.lineNumber()).arg(xml.errorString()), 3000);
  } else {
    statusBar()->showMessage(QString("Import: file read done"), 3000);
  }
  db_.commit();

  file.close();

  showProgressBar(updateFeedsCount_);

  //  qCritical() << "Start update: " << timer_.elapsed();
  feedsModelReload();
  //  qCritical() << "Start update: " << timer_.elapsed() << updateFeedsCount_;
  //  qCritical() << "------------------------------------------------------------";
}

/** @brief Export feeds to OPML-file
 *---------------------------------------------------------------------------*/
void RSSListing::slotExportFeeds()
{
  QString fileName = QFileDialog::getSaveFileName(this, tr("Select OPML-File"),
                                                  QDir::homePath(),
                                                  tr("OPML-Files (*.opml)"));

  if (fileName.isNull()) {
    statusBar()->showMessage(tr("Export canceled"), 3000);
    return;
  }

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    statusBar()->showMessage(tr("Export: can't open a file"), 3000);
    return;
  }

  QXmlStreamWriter xml(&file);
  xml.setAutoFormatting(true);
  xml.writeStartDocument();
  xml.writeStartElement("opml");
  xml.writeAttribute("version", "2.0");
  xml.writeStartElement("head");
  xml.writeTextElement("title", "QuiteRSS");
  xml.writeTextElement("dateModified", QDateTime::currentDateTime().toString());
  xml.writeEndElement(); // </head>

  xml.writeStartElement("body");

  // Create model and view for export
  // Expand the view to step on every item
  FeedsTreeModel exportTreeModel("feeds",
                                 QStringList() << "" << "" << "",
                                 QStringList() << "id" << "text" << "parentId",
                                 0);
  FeedsTreeView exportTreeView(this);
  exportTreeView.setModel(&exportTreeModel);
  exportTreeView.sortByColumn(exportTreeView.columnIndex("rowToParent"), Qt::AscendingOrder);
  exportTreeView.expandAll();

  QModelIndex index = exportTreeModel.index(0, 0);
  QStack<int> parentIdsStack;
  parentIdsStack.push(0);
  while (index.isValid()) {
    int feedId = exportTreeModel.getIdByIndex(index);
    int feedParId = exportTreeModel.getParidByIndex(index);

    // Parent differs from previouse one - close folder
    while (feedParId != parentIdsStack.top()) {
      xml.writeEndElement();  // "outline" - folder finishes
      parentIdsStack.pop();
    }

    // Folder has found. Open it
    if (exportTreeModel.isFolder(index)) {
      parentIdsStack.push(feedId);
      xml.writeStartElement("outline");  // Folder starts
      xml.writeAttribute("text", exportTreeModel.dataField(index, "text").toString());
    }
    // Feed has found. Save it
    else {
      xml.writeEmptyElement("outline");
      xml.writeAttribute("text",    exportTreeModel.dataField(index, "text").toString());
      xml.writeAttribute("type",    "rss");
      xml.writeAttribute("htmlUrl", exportTreeModel.dataField(index, "htmlUrl").toString());
      xml.writeAttribute("xmlUrl",  exportTreeModel.dataField(index, "xmlUrl").toString());
    }

    index = exportTreeView.indexBelow(index);
  }

  xml.writeEndElement(); // </body>

  xml.writeEndElement(); // </opml>
  xml.writeEndDocument();

  file.close();
}

/** @brief Process network request completion
 *---------------------------------------------------------------------------*/
void RSSListing::getUrlDone(const int &result, const QString &feedUrlStr,
                            const QString &error, const QByteArray &data,
                            const QDateTime &dtReply)
{
  qDebug() << "getUrl result = " << result << "error: " << error << "url: " << feedUrlStr;

  if (updateFeedsCount_ > 0) {
    updateFeedsCount_--;
    emit loadProgress(progressBar_->maximum() - updateFeedsCount_);
  }

  if (!data.isEmpty()) {
    emit xmlReadyParse(data, feedUrlStr, dtReply);
  } else {
    slotUpdateFeed(feedUrlStr, false, 0);
  }
}

/** @brief Update feed counters and all its parents
 *
 * Update fields: unread news number, new news number,
 *   last update feed timestamp
 * Update only feeds, categories are ignored
 * Update right into DB, update view if feed is visible in feed tree
 * @param feedId Feed identifier
 * @param updateViewport Need viewport update flag
 *----------------------------------------------------------------------------*/
void RSSListing::recountFeedCounts(int feedId, bool updateViewport)
{
  QSqlQuery q;
  QString qStr;

  db_.transaction();

  int feedParId = 0;
  bool isFolder = false;
  qStr = QString("SELECT parentId, xmlUrl FROM feeds WHERE id=='%1'").
      arg(feedId);
  q.exec(qStr);
  if (q.next()) {
    feedParId = q.value(0).toInt();
    if (q.value(1).toString().isEmpty())
      isFolder = true;
  }

  QModelIndex index = feedsTreeModel_->getIndexById(feedId);
  QModelIndex indexParent = QModelIndex();
  QModelIndex indexUnread;
  QModelIndex indexNew;
  QModelIndex indexUndelete;
  QModelIndex indexUpdated;
  int undeleteCount = 0;
  int unreadCount = 0;
  int newCount = 0;

  if (!isFolder) {
    // Calculate all news (not mark deleted)
    qStr = QString("SELECT count(id) FROM news WHERE feedId=='%1' AND deleted==0").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) undeleteCount = q.value(0).toInt();

    // Calculate unread news
    qStr = QString("SELECT count(read) FROM news WHERE feedId=='%1' AND read==0 AND deleted==0").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) unreadCount = q.value(0).toInt();

    // Calculate new news
    qStr = QString("SELECT count(new) FROM news WHERE feedId=='%1' AND new==1 AND deleted==0").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) newCount = q.value(0).toInt();

    int unreadCountOld = 0;
    int newCountOld = 0;
    int undeleteCountOld = 0;
    qStr = QString("SELECT unread, newCount, undeleteCount FROM feeds WHERE id=='%1'").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) {
      unreadCountOld = q.value(0).toInt();
      newCountOld = q.value(1).toInt();
      undeleteCountOld = q.value(2).toInt();
    }

    if ((unreadCount == unreadCountOld) && (newCount == newCountOld) &&
        (undeleteCount == undeleteCountOld)) {
      db_.commit();
      return;
    }

    // Save unread and new news number for feed
    qStr = QString("UPDATE feeds SET unread='%1', newCount='%2', undeleteCount='%3' "
                   "WHERE id=='%4'").
        arg(unreadCount).arg(newCount).arg(undeleteCount).arg(feedId);
    q.exec(qStr);

    // Update view of the feed, if it exist
    if (index.isValid()) {
      indexUnread   = feedsTreeModel_->indexSibling(index, "unread");
      indexNew      = feedsTreeModel_->indexSibling(index, "newCount");
      indexUndelete = feedsTreeModel_->indexSibling(index, "undeleteCount");
      feedsTreeModel_->setData(indexUnread, unreadCount);
      feedsTreeModel_->setData(indexNew, newCount);
      feedsTreeModel_->setData(indexUndelete, undeleteCount);
    }
  } else {
    bool changed = false;
    QList<int> idParList;
    QList<int> idList = getIdFeedsInList(feedId);
    if (idList.count()) {
      foreach (int id, idList) {
        int parId = 0;
        q.exec(QString("SELECT parentId FROM feeds WHERE id=='%1'").arg(id));
        if (q.next())
          parId = q.value(0).toInt();

        if (parId) {
          if (idParList.indexOf(parId) == -1) {
            idParList.append(parId);
          }
        }

        // Calculate all news (not mark deleted)
        qStr = QString("SELECT count(id) FROM news WHERE feedId=='%1' AND deleted==0").
            arg(id);
        q.exec(qStr);
        if (q.next()) undeleteCount = q.value(0).toInt();

        // Calculate unread news
        qStr = QString("SELECT count(read) FROM news WHERE feedId=='%1' AND read==0 AND deleted==0").
            arg(id);
        q.exec(qStr);
        if (q.next()) unreadCount = q.value(0).toInt();

        // Calculate new news
        qStr = QString("SELECT count(new) FROM news WHERE feedId=='%1' AND new==1 AND deleted==0").
            arg(id);
        q.exec(qStr);
        if (q.next()) newCount = q.value(0).toInt();

        int unreadCountOld = 0;
        int newCountOld = 0;
        int undeleteCountOld = 0;
        qStr = QString("SELECT unread, newCount, undeleteCount FROM feeds WHERE id=='%1'").
            arg(id);
        q.exec(qStr);
        if (q.next()) {
          unreadCountOld = q.value(0).toInt();
          newCountOld = q.value(1).toInt();
          undeleteCountOld = q.value(2).toInt();
        }

        if ((unreadCount == unreadCountOld) && (newCount == newCountOld) &&
            (undeleteCount == undeleteCountOld)) {
          continue;
        }
        changed = true;

        // Save unread and new news number for parent
        qStr = QString("UPDATE feeds SET unread='%1', newCount='%2', undeleteCount='%3' "
                       "WHERE id=='%4'").
            arg(unreadCount).arg(newCount).arg(undeleteCount).arg(id);
        q.exec(qStr);

        // Update view of the parent, if it exist
        QModelIndex index1 = feedsTreeModel_->getIndexById(id);
        if (index1.isValid()) {
          indexUnread   = feedsTreeModel_->indexSibling(index1, "unread");
          indexNew      = feedsTreeModel_->indexSibling(index1, "newCount");
          indexUndelete = feedsTreeModel_->indexSibling(index1, "undeleteCount");
          feedsTreeModel_->setData(indexUnread, unreadCount);
          feedsTreeModel_->setData(indexNew, newCount);
          feedsTreeModel_->setData(indexUndelete, undeleteCount);
        }
      }

      if (!changed) {
        db_.commit();
        return;
      }

      foreach (int l_feedParId, idParList) {
        while (l_feedParId) {
          QString updated;

          qStr = QString("SELECT sum(unread), sum(newCount), sum(undeleteCount), "
                         "max(updated) FROM feeds WHERE parentId=='%1'").
              arg(l_feedParId);
          q.exec(qStr);
          if (q.next()) {
            unreadCount   = q.value(0).toInt();
            newCount      = q.value(1).toInt();
            undeleteCount = q.value(2).toInt();
            updated       = q.value(3).toString();
          }
          qStr = QString("UPDATE feeds SET unread='%1', newCount='%2', undeleteCount='%3', "
                         "updated='%4' WHERE id=='%5'").
              arg(unreadCount).arg(newCount).arg(undeleteCount).arg(updated).
              arg(l_feedParId);
          q.exec(qStr);

          QModelIndex index1 = feedsTreeModel_->getIndexById(l_feedParId);

          // Update view, if it exist
          if (index1.isValid()) {
            indexUnread   = feedsTreeModel_->indexSibling(index1, "unread");
            indexNew      = feedsTreeModel_->indexSibling(index1, "newCount");
            indexUndelete = feedsTreeModel_->indexSibling(index1, "undeleteCount");
            indexUpdated  = feedsTreeModel_->indexSibling(index1, "updated");
            feedsTreeModel_->setData(indexUnread, unreadCount);
            feedsTreeModel_->setData(indexNew, newCount);
            feedsTreeModel_->setData(indexUndelete, undeleteCount);
            feedsTreeModel_->setData(indexUpdated, updated);
          }

          if (feedId == l_feedParId) break;
          q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(l_feedParId));
          if (q.next()) l_feedParId = q.value(0).toInt();
        }
      }
    }
  }

  // Recalculate counters for all parents
  indexParent = index.parent();
  int l_feedParId = feedParId;
  while (l_feedParId) {
    QString updated;

    qStr = QString("SELECT sum(unread), sum(newCount), sum(undeleteCount), "
                   "max(updated) FROM feeds WHERE parentId=='%1'").
        arg(l_feedParId);
    q.exec(qStr);
    if (q.next()) {
      unreadCount   = q.value(0).toInt();
      newCount      = q.value(1).toInt();
      undeleteCount = q.value(2).toInt();
      updated       = q.value(3).toString();
    }
    qStr = QString("UPDATE feeds SET unread='%1', newCount='%2', undeleteCount='%3', "
                   "updated='%4' WHERE id=='%5'").
        arg(unreadCount).arg(newCount).arg(undeleteCount).arg(updated).
        arg(l_feedParId);
    q.exec(qStr);

    // Update view, if it exist
    if (indexParent.isValid()) {
      indexUnread   = feedsTreeModel_->indexSibling(indexParent, "unread");
      indexNew      = feedsTreeModel_->indexSibling(indexParent, "newCount");
      indexUndelete = feedsTreeModel_->indexSibling(indexParent, "undeleteCount");
      indexUpdated  = feedsTreeModel_->indexSibling(indexParent, "updated");
      feedsTreeModel_->setData(indexUnread, unreadCount);
      feedsTreeModel_->setData(indexNew, newCount);
      feedsTreeModel_->setData(indexUndelete, undeleteCount);
      feedsTreeModel_->setData(indexUpdated, updated);
      indexParent = indexParent.parent();
    }

    q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(l_feedParId));
    if (q.next()) l_feedParId = q.value(0).toInt();
  }
  db_.commit();

  if (updateViewport) {
    feedsTreeView_->viewport()->update();
#ifdef HAVE_QT5
    feedsTreeView_->header()->setSectionResizeMode(feedsTreeModel_->proxyColumnByOriginal("unread"), QHeaderView::ResizeToContents);
    feedsTreeView_->header()->setSectionResizeMode(feedsTreeModel_->proxyColumnByOriginal("undeleteCount"), QHeaderView::ResizeToContents);
    feedsTreeView_->header()->setSectionResizeMode(feedsTreeModel_->proxyColumnByOriginal("updated"), QHeaderView::ResizeToContents);
#else
    feedsTreeView_->header()->setResizeMode(feedsTreeModel_->proxyColumnByOriginal("unread"), QHeaderView::ResizeToContents);
    feedsTreeView_->header()->setResizeMode(feedsTreeModel_->proxyColumnByOriginal("undeleteCount"), QHeaderView::ResizeToContents);
    feedsTreeView_->header()->setResizeMode(feedsTreeModel_->proxyColumnByOriginal("updated"), QHeaderView::ResizeToContents);
#endif
  }
}
// ----------------------------------------------------------------------------
void RSSListing::slotFeedCountsUpdate(FeedCountStruct counts)
{
  QModelIndex index = feedsTreeModel_->getIndexById(counts.feedId);
  if (index.isValid()) {
    QModelIndex indexUnread   = feedsTreeModel_->indexSibling(index, "unread");
    QModelIndex indexNew      = feedsTreeModel_->indexSibling(index, "newCount");
    QModelIndex indexUndelete = feedsTreeModel_->indexSibling(index, "undeleteCount");
    feedsTreeModel_->setData(indexUnread, counts.unreadCount);
    feedsTreeModel_->setData(indexNew, counts.newCount);
    feedsTreeModel_->setData(indexUndelete, counts.undeleteCount);

    if (!counts.updated.isEmpty()) {
      QModelIndex indexUpdated  = feedsTreeModel_->indexSibling(index, "updated");
      feedsTreeModel_->setData(indexUpdated, counts.updated);
    }

    if (!counts.lastBuildDate.isEmpty()) {
      QModelIndex indexLastBuildDate = feedsTreeModel_->indexSibling(index, "lastBuildDate");
      feedsTreeModel_->setData(indexLastBuildDate, counts.lastBuildDate);
    }

    if (!counts.htmlUrl.isEmpty()) {
      QModelIndex indexHtmlUrl = feedsTreeModel_->indexSibling(index, "htmlUrl");
      feedsTreeModel_->setData(indexHtmlUrl, counts.htmlUrl);
    }

    if (!counts.title.isEmpty()) {
      QModelIndex indexTitle = feedsTreeModel_->indexSibling(index, "title");
      feedsTreeModel_->setData(indexTitle, counts.title);
    }
  }

  if (importFeedStart_ && !counts.xmlUrl.isEmpty()) {
    emit faviconRequestUrl(counts.htmlUrl, counts.xmlUrl);
  }
}

/** @brief Recalculate counters for specified categories
 * @details Processing DB data. Model "reselect()" needed.
 * @param categoriesList - categories identifiers list for processing
 *---------------------------------------------------------------------------*/
void RSSListing::recountFeedCategories(const QList<int> &categoriesList)
{
  QSqlQuery q;
  QString qStr;

  foreach (int categoryIdStart, categoriesList) {
    if (categoryIdStart < 1) continue;

    int categoryId = categoryIdStart;
    // Process all parents
    while (0 < categoryId) {
      int unreadCount = -1;
      int undeleteCount = -1;
      int newCount = -1;

      // Calculate sum of all feeds with same parent
      qStr = QString("SELECT sum(unread), sum(undeleteCount), sum(newCount) "
                     "FROM feeds WHERE parentId=='%1'").arg(categoryId);
      q.exec(qStr);
      if (q.next()) {
        unreadCount   = q.value(0).toInt();
        undeleteCount = q.value(1).toInt();
        newCount = q.value(2).toInt();
      }

      if (unreadCount != -1) {
        qStr = QString("UPDATE feeds SET unread='%1', undeleteCount='%2', newCount='%3' WHERE id=='%4'").
            arg(unreadCount).arg(undeleteCount).arg(newCount).arg(categoryId);
        q.exec(qStr);
      }

      // go to next parent's parent
      qStr = QString("SELECT parentId FROM feeds WHERE id=='%1'").
          arg(categoryId);
      categoryId = 0;
      q.exec(qStr);
      if (q.next()) categoryId = q.value(0).toInt();
    }
  }
}
// ----------------------------------------------------------------------------
void RSSListing::recountCategoryCounts()
{
  if (recountCategoryCountsOn_) return;

  recountCategoryCountsOn_ = true;
  emit signalRecountCategoryCounts();
}

/** @brief Process recalculating categories counters
 *----------------------------------------------------------------------------*/
void RSSListing::slotRecountCategoryCounts()
{
  if (!categoriesTree_->isVisible()) {
    recountCategoryCountsOn_ = false;
    return;
  }

  int allStarredCount = 0;
  int unreadStarredCount = 0;
  int deletedCount = 0;
  QMap<int,int> allCountList;
  QMap<int,int> unreadCountList;
  int allLabelCount = 0;
  int unreadLabelCount = 0;

  QTreeWidgetItem *labelTreeItem = categoriesTree_->topLevelItem(3);
  for (int i = 0; i < labelTreeItem->childCount(); i++) {
    int id = labelTreeItem->child(i)->text(2).toInt();
    allCountList.insert(id, 0);
    unreadCountList.insert(id, 0);
  }

  QSqlQuery q;
  q.exec("SELECT deleted, starred, read, label FROM news WHERE deleted < 2");
  while (q.next()) {
    if (q.value(0).toInt() == 0) {
      if (q.value(1).toInt() == 1) {
        allStarredCount++;
        if (q.value(2).toInt() == 0)
          unreadStarredCount++;
      }
      QString idString = q.value(3).toString();
      if (!idString.isEmpty() && idString != ",") {
        QStringList idList = idString.split(",", QString::SkipEmptyParts);
        foreach (QString idStr, idList) {
          int id = idStr.toInt();
          if (allCountList.contains(id)) {
            allCountList[id]++;
            if (q.value(2).toInt() == 0)
              unreadCountList[id]++;
          }
        }
      }
    } else if (q.value(0).toInt() == 1) {
      deletedCount++;
    }
  }
  for (int i = 0; i < labelTreeItem->childCount(); i++) {
    int id = labelTreeItem->child(i)->text(2).toInt();
    QString countStr;
    if (!unreadCountList[id] && !allCountList[id])
      countStr = "";
    else
      countStr = QString("(%1/%2)").arg(unreadCountList[id]).arg(allCountList[id]);
    labelTreeItem->child(i)->setText(4, countStr);

    unreadLabelCount = unreadLabelCount + unreadCountList[id];
    allLabelCount = allLabelCount + allCountList[id];
  }

  QString countStr;
  if (!unreadStarredCount && !allStarredCount)
    countStr = "";
  else
    countStr = QString("(%1/%2)").arg(unreadStarredCount).arg(allStarredCount);
  categoriesTree_->topLevelItem(1)->setText(4, countStr);
  if (!deletedCount)
    countStr = "";
  else
    countStr = QString("(%1)").arg(deletedCount);
  categoriesTree_->topLevelItem(2)->setText(4, countStr);
  if (!unreadLabelCount && !allLabelCount)
    countStr = "";
  else
    countStr = QString("(%1/%2)").arg(unreadLabelCount).arg(allLabelCount);
  categoriesTree_->topLevelItem(3)->setText(4, countStr);

  NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(stackedWidget_->currentIndex());
  if ((widget->type_ > TAB_FEED) && (widget->type_ < TAB_WEB) &&
      categoriesTree_->currentIndex().isValid()) {
    int unreadCount = 0;
    int allCount = widget->newsModel_->rowCount();

    QString countStr = categoriesTree_->currentItem()->text(4);
    if (!countStr.isEmpty()) {
      countStr.remove(QRegExp("[()]"));
      switch (widget->type_) {
      case TAB_CAT_UNREAD:
        unreadCount = countStr.toInt();
        break;
      case TAB_CAT_STAR: case TAB_CAT_LABEL:
        unreadCount = countStr.section("/", 0, 0).toInt();
        break;
      }
    }
    statusUnread_->setText(QString(" " + tr("Unread: %1") + " ").arg(unreadCount));
    statusAll_->setText(QString(" " + tr("All: %1") + " ").arg(allCount));
  }

  recountCategoryCountsOn_ = false;
}

/** @brief Process updating feed view signal
 *
 * Calls after updating or adding feed
 * Actually calls updateing delay
 * @param url URL of updating feed
 * @param changed Flag indicating that feed is updated indeed
 *---------------------------------------------------------------------------*/
void RSSListing::slotUpdateFeed(const QString &feedUrl, const bool &changed, int newCount)
{
  updateDelayer_->delayUpdate(feedUrl, changed, newCount);
}

/** @brief Update feed view
 *
 * Slot is called by UpdateDelayer after some delay
 * @param feedId Feed identifier to update
 * @param changed Flag indicating that feed is updated indeed
 *---------------------------------------------------------------------------*/
void RSSListing::slotUpdateFeedDelayed(const QString &feedUrl, const bool &changed, int newCount)
{
  if (updateFeedsCount_ > 0) {
    updateFeedsCount_--;
    emit loadProgress(progressBar_->maximum() - updateFeedsCount_);
  }
  if (updateFeedsCount_ <= 0) {
    emit signalShowNotification();
    progressBar_->hide();
    progressBar_->setMaximum(0);
    emit loadProgress(0);
    updateFeedsCount_ = 0;
    importFeedStart_ = false;
    // qCritical() << "Stop update: " << timer_.elapsed();
    // qCritical() << "------------------------------------------------------------";
  }

  int feedUrlIndex = feedUrlList_.indexOf(feedUrl);
  if (feedUrlIndex > -1)
    feedUrlList_.takeAt(feedUrlIndex);

  if (!changed) {
    emit signalNextUpdate();
    return;
  }

  int feedId = -1;
  QSqlQuery q;
  q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
  q.bindValue(":xmlUrl", feedUrl);
  q.exec();
  if (q.next()) feedId = q.value(0).toInt();

  // Action after new news has arrived: tray, sound
  if (!isActiveWindow() && (newCount > 0) &&
      (behaviorIconTray_ == CHANGE_ICON_TRAY)) {
    traySystem->setIcon(QIcon(":/images/quiterss16_NewNews"));
  }
  emit signalRefreshInfoTray();
  if (newCount > 0) {
    playSoundNewNews();
  }
  recountCategoryCounts();

  // Manage notifications
  if (isActiveWindow()) {
    idFeedList_.clear();
    cntNewNewsList_.clear();
  }
  if ((newCount > 0) && !isActiveWindow()) {
    int feedIdIndex = idFeedList_.indexOf(feedId);
    if (onlySelectedFeeds_) {
      q.exec(QString("SELECT value FROM feeds_ex WHERE feedId='%1' AND name='showNotification'").
             arg(feedId));
      if (q.next()) {
        if (q.value(0).toInt() == 1) {
          if (-1 < feedIdIndex) {
            cntNewNewsList_[feedIdIndex] = newCount;
          } else {
            idFeedList_.append(feedId);
            cntNewNewsList_.append(newCount);
          }
        }
      }
    } else {
      if (-1 < feedIdIndex) {
        cntNewNewsList_[feedIdIndex] = newCount;
      } else {
        idFeedList_.append(feedId);
        cntNewNewsList_.append(newCount);
      }
    }
  }

  if (currentNewsTab->type_ == TAB_FEED) {
    bool folderUpdate = false;
    int feedParentId = 0;
    q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(feedId));
    if (q.next()) {
      feedParentId = q.value(0).toInt();
      if (feedParentId == currentNewsTab->feedId_) folderUpdate = true;
    }

    while (feedParentId && !folderUpdate) {
      q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(feedParentId));
      if (q.next()) {
        feedParentId = q.value(0).toInt();
        if (feedParentId == currentNewsTab->feedId_) folderUpdate = true;
      }
    }

    // Click on feed if it is displayed to update view
    if ((feedId == currentNewsTab->feedId_) || folderUpdate) {
      slotUpdateNews();
      int unreadCount = 0;
      int allCount = 0;
      q.exec(QString("SELECT unread, undeleteCount FROM feeds WHERE id=='%1'").arg(currentNewsTab->feedId_));
      if (q.next()) {
        unreadCount = q.value(0).toInt();
        allCount    = q.value(1).toInt();
      }
      statusUnread_->setText(QString(" " + tr("Unread: %1") + " ").arg(unreadCount));
      statusAll_->setText(QString(" " + tr("All: %1") + " ").arg(allCount));
    }
  }

  emit signalNextUpdate();
}

/** @brief Process updating news list
 *---------------------------------------------------------------------------*/
void RSSListing::slotUpdateNews()
{
  int newsId = newsModel_->index(
        newsView_->currentIndex().row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

  newsModel_->select();

  if (!newsModel_->rowCount()) {
    currentNewsTab->currentNewsIdOld = newsId;
    currentNewsTab->hideWebContent();
    return;
  }

  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("id"));
  QModelIndexList indexList = newsModel_->match(index, Qt::EditRole, newsId);
  if (indexList.count()) {
    int newsRow = indexList.first().row();
    newsView_->setCurrentIndex(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
  } else {
    currentNewsTab->currentNewsIdOld = newsId;
    currentNewsTab->hideWebContent();
  }
}

/** @brief Process click in feed tree
 *---------------------------------------------------------------------------*/
void RSSListing::slotFeedClicked(QModelIndex index)
{
  int feedIdCur = feedsTreeModel_->getIdByIndex(index);

  if (stackedWidget_->count() && currentNewsTab->type_ < TAB_WEB) {
    currentNewsTab->newsHeader_->saveStateColumns(this, currentNewsTab);
  }

  // Search open tab containing this feed
  int indexTab = -1;
  for (int i = 0; i < stackedWidget_->count(); i++) {
    NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
    if (widget->feedId_ == feedIdCur) {
      indexTab = i;
      break;
    }
  }

  if (indexTab == -1) {
    if (tabBar_->currentIndex() != TAB_WIDGET_PERMANENT) {
      setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_, FeedReadSwitchingTab, currentNewsTab);

      updateCurrentTab_ = false;
      tabBar_->setCurrentIndex(TAB_WIDGET_PERMANENT);
      updateCurrentTab_ = true;
      feedsTreeView_->setCurrentIndex(feedsTreeModel_->getIndexById(feedIdCur));

      currentNewsTab = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);
      newsModel_ = currentNewsTab->newsModel_;
      newsView_ = currentNewsTab->newsView_;
    } else {
      if (stackedWidget_->count() && currentNewsTab->type_ != TAB_FEED) {
        setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_, FeedReadSwitchingFeed, currentNewsTab);
      } else {
        // Mark previous feed Read while switching to another feed
        setFeedRead(TAB_FEED, feedIdOld_, FeedReadSwitchingFeed, 0, feedIdCur);
      }

      categoriesTree_->setCurrentIndex(QModelIndex());
    }

    slotFeedSelected(feedsTreeModel_->getIndexById(feedIdCur));
    feedsTreeView_->repaint();
  } else if (indexTab != -1) {
    tabBar_->setCurrentIndex(indexTab);
  }
  feedIdOld_ = feedIdCur;
}

/** @brief Process feed choosing
 *---------------------------------------------------------------------------*/
void RSSListing::slotFeedSelected(QModelIndex index, bool createTab)
{
  QElapsedTimer timer;
  timer.start();
  qDebug() << "--------------------------------";
  qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed();

  int feedId = feedsTreeModel_->getIdByIndex(index);
  int feedParId = feedsTreeModel_->getParidByIndex(index);

  // Open or create feed tab
  if (!stackedWidget_->count() || createTab) {
    NewsTabWidget *widget = new NewsTabWidget(this, TAB_FEED, feedId, feedParId);
    int indexTab = addTab(widget);
    createNewsTab(indexTab);

    if (indexTab == 0)
      currentNewsTab->closeButton_->setVisible(false);
    if (!index.isValid())
      currentNewsTab->setVisible(false);

    emit signalSetCurrentTab(indexTab);
  } else {
    currentNewsTab->type_ = TAB_FEED;
    currentNewsTab->feedId_ = feedId;
    currentNewsTab->feedParId_ = feedParId;
    currentNewsTab->setSettings(false);
    currentNewsTab->setVisible(index.isValid());
  }

  statusUnread_->setVisible(index.isValid());
  statusAll_->setVisible(index.isValid());

  // Set icon for tab has opened
  bool isFeed = (index.isValid() && feedsTreeModel_->isFolder(index)) ? false : true;

  QPixmap iconTab;
  QByteArray byteArray = feedsTreeModel_->dataField(index, "image").toByteArray();
  if (!isFeed) {
    iconTab.load(":/images/folder");
  } else {
    if (byteArray.isNull() || defaultIconFeeds_) {
      iconTab.load(":/images/feed");
    } else if (isFeed) {
      iconTab.loadFromData(QByteArray::fromBase64(byteArray));
    }
  }
  currentNewsTab->newsIconTitle_->setPixmap(iconTab);

  // Set title for tab has opened
  int padding = 15;
  if (tabBar_->currentIndex() == TAB_WIDGET_PERMANENT)
    padding = 0;
  currentNewsTab->setTextTab(feedsTreeModel_->dataField(index, "text").toString(),
                             currentNewsTab->newsTextTitle_->width() - padding);

  feedProperties_->setEnabled(index.isValid());

  qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed();

  // Set filter to make current feed visible when filter for feeds changes
  setFeedsFilter(feedsFilterGroup_->checkedAction(), false);

  qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed();

  setNewsFilter(newsFilterGroup_->checkedAction(), false);

  qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed();

  // Search feed news that displayed before
  int newsRow = -1;
  if (openingFeedAction_ == 0) {
    QModelIndex feedIndex = feedsTreeModel_->getIndexById(feedId);
    int newsIdCur = feedsTreeModel_->dataField(feedIndex, "currentNews").toInt();
    QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("id"));
    QModelIndexList indexList = newsModel_->match(index, Qt::EditRole, newsIdCur);

    if (!indexList.isEmpty()) newsRow = indexList.first().row();
  } else if (openingFeedAction_ == 1) {
    newsRow = 0;
  } else if ((openingFeedAction_ == 3) || (openingFeedAction_ == 4)) {
    QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("read"));
    QModelIndexList indexList;
    if ((newsView_->header()->sortIndicatorOrder() == Qt::DescendingOrder) &&
        (openingFeedAction_ != 4))
      indexList = newsModel_->match(index, Qt::EditRole, 0, -1);
    else
      indexList = newsModel_->match(index, Qt::EditRole, 0);

    if (!indexList.isEmpty()) newsRow = indexList.last().row();
  }

  qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed();

  // Focus feed news that displayed before
  newsView_->setCurrentIndex(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
  if (newsRow == -1) newsView_->verticalScrollBar()->setValue(newsRow);

  qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed();

  if ((openingFeedAction_ != 2) && openNewsWebViewOn_) {
    currentNewsTab->slotNewsViewSelected(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed();
  } else {
    currentNewsTab->slotNewsViewSelected(newsModel_->index(-1, newsModel_->fieldIndex("title")));
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed();
    QSqlQuery q;
    int newsId = newsModel_->index(newsRow, newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();
    QString qStr = QString("UPDATE feeds SET currentNews='%1' WHERE id=='%2'").arg(newsId).arg(feedId);
    q.exec(qStr);

    QModelIndex feedIndex = feedsTreeModel_->getIndexById(feedId);
    feedsTreeModel_->setData(feedsTreeModel_->indexSibling(feedIndex, "currentNews"),
                             newsId);
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed();
  }
}

// ----------------------------------------------------------------------------
void RSSListing::showOptionDlg()
{
  static int index = 0;

  if (optionsDialog_) {
    optionsDialog_->activateWindow();
    return;
  }

  optionsDialog_ = new OptionsDialog(this);

  optionsDialog_->showSplashScreen_->setChecked(showSplashScreen_);
  optionsDialog_->reopenFeedStartup_->setChecked(reopenFeedStartup_);
  optionsDialog_->openNewTabNextToActive_->setChecked(openNewTabNextToActive_);
  optionsDialog_->hideFeedsOpenTab_->setChecked(hideFeedsOpenTab_);
  optionsDialog_->showToggleFeedsTree_->setChecked(showToggleFeedsTree_);
  optionsDialog_->defaultIconFeeds_->setChecked(defaultIconFeeds_);
  optionsDialog_->autocollapseFolder_->setChecked(feedsTreeView_->autocollapseFolder_);

  optionsDialog_->updateCheckEnabled_->setChecked(updateCheckEnabled_);
  optionsDialog_->storeDBMemory_->setChecked(storeDBMemoryT_);

  optionsDialog_->showTrayIconBox_->setChecked(showTrayIcon_);
  optionsDialog_->startingTray_->setChecked(startingTray_);
  optionsDialog_->minimizingTray_->setChecked(minimizingTray_);
  optionsDialog_->closingTray_->setChecked(closingTray_);
  optionsDialog_->setBehaviorIconTray(behaviorIconTray_);
  optionsDialog_->singleClickTray_->setChecked(singleClickTray_);
  optionsDialog_->clearStatusNew_->setChecked(clearStatusNew_);
  optionsDialog_->emptyWorking_->setChecked(emptyWorking_);

  optionsDialog_->setProxy(networkProxy_);

  optionsDialog_->embeddedBrowserOn_->setChecked(externalBrowserOn_ <= 0);
  optionsDialog_->externalBrowserOn_->setChecked(externalBrowserOn_ >= 1);
  optionsDialog_->defaultExternalBrowserOn_->setChecked((externalBrowserOn_ == 0) ||
                                                       (externalBrowserOn_ == 1));
  optionsDialog_->otherExternalBrowserOn_->setChecked((externalBrowserOn_ == -1) ||
                                                     (externalBrowserOn_ == 2));
  optionsDialog_->otherExternalBrowserEdit_->setText(externalBrowser_);
  optionsDialog_->javaScriptEnable_->setChecked(javaScriptEnable_);
  optionsDialog_->pluginsEnable_->setChecked(pluginsEnable_);
  optionsDialog_->openLinkInBackground_->setChecked(openLinkInBackground_);
  optionsDialog_->openLinkInBackgroundEmbedded_->setChecked(openLinkInBackgroundEmbedded_);
  optionsDialog_->userStyleBrowserEdit_->setText(userStyleBrowser_);

  optionsDialog_->maxPagesInCache_->setValue(maxPagesInCache_);
  bool useDiskCache = settings_->value("Settings/useDiskCache", true).toBool();
  optionsDialog_->diskCacheOn_->setChecked(useDiskCache);
  QString diskCacheDirPath = settings_->value(
        "Settings/dirDiskCache", diskCacheDirPathDefault_).toString();
  if (diskCacheDirPath.isEmpty()) diskCacheDirPath = diskCacheDirPathDefault_;
  optionsDialog_->dirDiskCacheEdit_->setText(diskCacheDirPath);
  int maxDiskCache = settings_->value("Settings/maxDiskCache", 50).toInt();
  optionsDialog_->maxDiskCache_->setValue(maxDiskCache);

  optionsDialog_->saveCookies_->setChecked(cookieJar_->saveCookies_ == 1);
  optionsDialog_->deleteCookiesOnClose_->setChecked(cookieJar_->saveCookies_ == 2);
  optionsDialog_->blockCookies_->setChecked(cookieJar_->saveCookies_ == 0);

  optionsDialog_->downloadLocationEdit_->setText(downloadLocation_);
  optionsDialog_->askDownloadLocation_->setChecked(askDownloadLocation_);

  optionsDialog_->updateFeedsStartUp_->setChecked(updateFeedsStartUp_);
  optionsDialog_->updateFeedsEnable_->setChecked(updateFeedsEnable_);
  optionsDialog_->updateIntervalType_->setCurrentIndex(updateFeedsIntervalType_+1);
  optionsDialog_->updateFeedsInterval_->setValue(updateFeedsInterval_);

  optionsDialog_->setOpeningFeed(openingFeedAction_);
  optionsDialog_->openNewsWebViewOn_->setChecked(openNewsWebViewOn_);

  optionsDialog_->markNewsReadOn_->setChecked(markNewsReadOn_);
  optionsDialog_->markCurNewsRead_->setChecked(markCurNewsRead_);
  optionsDialog_->markNewsReadTime_->setValue(markNewsReadTime_);
  optionsDialog_->markPrevNewsRead_->setChecked(markPrevNewsRead_);
  optionsDialog_->markReadSwitchingFeed_->setChecked(markReadSwitchingFeed_);
  optionsDialog_->markReadClosingTab_->setChecked(markReadClosingTab_);
  optionsDialog_->markReadMinimize_->setChecked(markReadMinimize_);

  optionsDialog_->showDescriptionNews_->setChecked(showDescriptionNews_);

  for (int i = 0; i < optionsDialog_->formatDate_->count(); i++) {
    if (optionsDialog_->formatDate_->itemData(i).toString() == formatDate_) {
      optionsDialog_->formatDate_->setCurrentIndex(i);
      break;
    }
  }
  for (int i = 0; i < optionsDialog_->formatTime_->count(); i++) {
    if (optionsDialog_->formatTime_->itemData(i).toString() == formatTime_) {
      optionsDialog_->formatTime_->setCurrentIndex(i);
      break;
    }
  }

  optionsDialog_->alternatingRowColorsNews_->setChecked(alternatingRowColorsNews_);
  optionsDialog_->changeBehaviorActionNUN_->setChecked(changeBehaviorActionNUN_);
  optionsDialog_->simplifiedDateTime_->setChecked(simplifiedDateTime_);
  optionsDialog_->notDeleteStarred_->setChecked(notDeleteStarred_);
  optionsDialog_->notDeleteLabeled_->setChecked(notDeleteLabeled_);
  optionsDialog_->markIdenticalNewsRead_->setChecked(markIdenticalNewsRead_);

  for (int i = 0; i < optionsDialog_->mainNewsFilter_->count(); i++) {
    if (optionsDialog_->mainNewsFilter_->itemData(i).toString() == mainNewsFilter_) {
      optionsDialog_->mainNewsFilter_->setCurrentIndex(i);
      break;
    }
  }

  optionsDialog_->cleanupOnShutdownBox_->setChecked(cleanupOnShutdown_);
  optionsDialog_->dayCleanUpOn_->setChecked(dayCleanUpOn_);
  optionsDialog_->maxDayCleanUp_->setValue(maxDayCleanUp_);
  optionsDialog_->newsCleanUpOn_->setChecked(newsCleanUpOn_);
  optionsDialog_->maxNewsCleanUp_->setValue(maxNewsCleanUp_);
  optionsDialog_->readCleanUp_->setChecked(readCleanUp_);
  optionsDialog_->neverUnreadCleanUp_->setChecked(neverUnreadCleanUp_);
  optionsDialog_->neverStarCleanUp_->setChecked(neverStarCleanUp_);
  optionsDialog_->neverLabelCleanUp_->setChecked(neverLabelCleanUp_);
  optionsDialog_->cleanUpDeleted_->setChecked(cleanUpDeleted_);
  optionsDialog_->optimizeDB_->setChecked(optimizeDB_);

  optionsDialog_->soundNewNews_->setChecked(soundNewNews_);
  optionsDialog_->editSoundNotifer_->setText(soundNotifyPath_);
  optionsDialog_->showNotifyOn_->setChecked(showNotifyOn_);
  optionsDialog_->positionNotify_->setCurrentIndex(positionNotify_);
  optionsDialog_->countShowNewsNotify_->setValue(countShowNewsNotify_);
  optionsDialog_->widthTitleNewsNotify_->setValue(widthTitleNewsNotify_);
  optionsDialog_->timeShowNewsNotify_->setValue(timeShowNewsNotify_);
  optionsDialog_->fullscreenModeNotify_->setChecked(fullscreenModeNotify_);
  optionsDialog_->onlySelectedFeeds_->setChecked(onlySelectedFeeds_);

  optionsDialog_->setLanguage(langFileName_);

  QString strFont = QString("%1, %2").
      arg(feedsTreeView_->font().family()).
      arg(feedsTreeView_->font().pointSize());
  optionsDialog_->fontsTree_->topLevelItem(0)->setText(2, strFont);
  strFont = QString("%1, %2").arg(newsListFontFamily_).arg(newsListFontSize_);
  optionsDialog_->fontsTree_->topLevelItem(1)->setText(2, strFont);
  strFont = QString("%1, %2").arg(newsTitleFontFamily_).arg(newsTitleFontSize_);
  optionsDialog_->fontsTree_->topLevelItem(2)->setText(2, strFont);
  strFont = QString("%1, %2").arg(newsTextFontFamily_).arg(newsTextFontSize_);
  optionsDialog_->fontsTree_->topLevelItem(3)->setText(2, strFont);
  strFont = QString("%1, %2").arg(notificationFontFamily_).arg(notificationFontSize_);
  optionsDialog_->fontsTree_->topLevelItem(4)->setText(2, strFont);

  optionsDialog_->browserMinFontSize_->setValue(browserMinFontSize_);
  optionsDialog_->browserMinLogFontSize_->setValue(browserMinLogFontSize_);

  QPixmap pixmapColor(14, 14);
  pixmapColor.fill(feedsTreeModel_->textColor_);
  optionsDialog_->colorsTree_->topLevelItem(0)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(0)->setText(1, feedsTreeModel_->textColor_);
  if (feedsTreeModel_->backgroundColor_.isEmpty())
    pixmapColor.fill(QColor(0, 0, 0, 0));
  else
    pixmapColor.fill(feedsTreeModel_->backgroundColor_);
  optionsDialog_->colorsTree_->topLevelItem(1)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(1)->setText(1, feedsTreeModel_->backgroundColor_);
  pixmapColor.fill(newsListTextColor_);
  optionsDialog_->colorsTree_->topLevelItem(2)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(2)->setText(1, newsListTextColor_);
  if (newsListBackgroundColor_.isEmpty())
    pixmapColor.fill(QColor(0, 0, 0, 0));
  else
    pixmapColor.fill(newsListBackgroundColor_);
  optionsDialog_->colorsTree_->topLevelItem(3)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(3)->setText(1, newsListBackgroundColor_);
  pixmapColor.fill(focusedNewsTextColor_);
  optionsDialog_->colorsTree_->topLevelItem(4)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(4)->setText(1, focusedNewsTextColor_);
  if (focusedNewsBGColor_.isEmpty())
    pixmapColor.fill(QColor(0, 0, 0, 0));
  else
    pixmapColor.fill(focusedNewsBGColor_);
  optionsDialog_->colorsTree_->topLevelItem(5)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(5)->setText(1, focusedNewsBGColor_);
  pixmapColor.fill(linkColor_);
  optionsDialog_->colorsTree_->topLevelItem(6)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(6)->setText(1, linkColor_);
  pixmapColor.fill(titleColor_);
  optionsDialog_->colorsTree_->topLevelItem(7)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(7)->setText(1, titleColor_);
  pixmapColor.fill(dateColor_);
  optionsDialog_->colorsTree_->topLevelItem(8)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(8)->setText(1, dateColor_);
  pixmapColor.fill(authorColor_);
  optionsDialog_->colorsTree_->topLevelItem(9)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(9)->setText(1, authorColor_);
  pixmapColor.fill(newsTitleBackgroundColor_);
  optionsDialog_->colorsTree_->topLevelItem(10)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(10)->setText(1, newsTitleBackgroundColor_);
  pixmapColor.fill(newsBackgroundColor_);
  optionsDialog_->colorsTree_->topLevelItem(11)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(11)->setText(1, newsBackgroundColor_);
  pixmapColor.fill(feedsTreeModel_->feedWithNewNewsColor_);
  optionsDialog_->colorsTree_->topLevelItem(12)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(12)->setText(1, feedsTreeModel_->feedWithNewNewsColor_);
  pixmapColor.fill(feedsTreeModel_->countNewsUnreadColor_);
  optionsDialog_->colorsTree_->topLevelItem(13)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(13)->setText(1, feedsTreeModel_->countNewsUnreadColor_);

  optionsDialog_->loadActionShortcut(listActions_, &listDefaultShortcut_);

  // Display setting dialog

  optionsDialog_->setCurrentItem(index);
  int result = optionsDialog_->exec();
  index = optionsDialog_->currentIndex();

  if (result == QDialog::Rejected) {
    delete optionsDialog_;
    optionsDialog_ = NULL;
    return;
  }

  // Apply accepted settings

  foreach (QAction *action, listActions_) {
    QString objectName = action->objectName();
    if (objectName.contains("labelAction_")) {
      listActions_.removeOne(action);
      delete action;
    }
  }
  optionsDialog_->saveActionShortcut(listActions_, newsLabelGroup_);
  listActions_.append(newsLabelGroup_->actions());
  newsLabelMenu_->addActions(newsLabelGroup_->actions());
  this->addActions(newsLabelGroup_->actions());
  if (newsLabelGroup_->actions().count()) {
    newsLabelAction_->setIcon(newsLabelGroup_->actions().at(0)->icon());
    newsLabelAction_->setToolTip(newsLabelGroup_->actions().at(0)->text());
    newsLabelAction_->setData(newsLabelGroup_->actions().at(0)->data());
  }

  if (optionsDialog_->idLabels_.count()) {
    QTreeWidgetItem *labelTreeItem = categoriesTree_->topLevelItem(3);
    while (labelTreeItem->childCount()) {
      labelTreeItem->removeChild(labelTreeItem->child(0));
    }

    bool closeTab = true;
    int indexTab = -1;
    int tabLabelId = -1;
    for (int i = 0; i < stackedWidget_->count(); i++) {
      NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
      if (widget->type_ == TAB_CAT_LABEL) {
        indexTab = i;
        tabLabelId = widget->labelId_;
        break;
      }
    }

    QSqlQuery q;
    q.exec("SELECT id, name, image, currentNews FROM labels ORDER BY num");
    while (q.next()) {
      int idLabel = q.value(0).toInt();
      QString nameLabel = q.value(1).toString();
      QByteArray byteArray = q.value(2).toByteArray();
      QString currentNews = q.value(3).toString();
      QPixmap imageLabel;
      if (!byteArray.isNull())
        imageLabel.loadFromData(byteArray);
      QStringList dataItem;
      dataItem << nameLabel << QString::number(TAB_CAT_LABEL)
               << QString::number(idLabel) << currentNews;
      QTreeWidgetItem *childItem = new QTreeWidgetItem(dataItem);
      childItem->setIcon(0, QIcon(imageLabel));
      labelTreeItem->addChild(childItem);

      if (idLabel == tabLabelId) {
        closeTab = false;
        NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(indexTab);
        // Set icon and title for tab has opened
        widget->newsIconTitle_->setPixmap(imageLabel);
        widget->setTextTab(nameLabel);
      }
    }

    if (closeTab && (indexTab > 0) && (tabLabelId > 0)) {
      slotCloseTab(indexTab);
    }
    if ((tabBar_->currentIndex() == indexTab) && (indexTab > 0) && (tabLabelId == 0)) {
      slotUpdateNews();
    }
  }

  showSplashScreen_ = optionsDialog_->showSplashScreen_->isChecked();
  reopenFeedStartup_ = optionsDialog_->reopenFeedStartup_->isChecked();
  openNewTabNextToActive_ = optionsDialog_->openNewTabNextToActive_->isChecked();
  hideFeedsOpenTab_ = optionsDialog_->hideFeedsOpenTab_->isChecked();
  showToggleFeedsTree_ = optionsDialog_->showToggleFeedsTree_->isChecked();
  defaultIconFeeds_ = optionsDialog_->defaultIconFeeds_->isChecked();
  feedsTreeModel_->defaultIconFeeds_ = defaultIconFeeds_;
  feedsTreeView_->autocollapseFolder_ = optionsDialog_->autocollapseFolder_->isChecked();

  pushButtonNull_->setVisible(showToggleFeedsTree_);

  updateCheckEnabled_ = optionsDialog_->updateCheckEnabled_->isChecked();
  storeDBMemoryT_ = optionsDialog_->storeDBMemory_->isChecked();

  showTrayIcon_ = optionsDialog_->showTrayIconBox_->isChecked();
  startingTray_ = optionsDialog_->startingTray_->isChecked();
  minimizingTray_ = optionsDialog_->minimizingTray_->isChecked();
  closingTray_ = optionsDialog_->closingTray_->isChecked();
  behaviorIconTray_ = optionsDialog_->behaviorIconTray();
  if (behaviorIconTray_ > CHANGE_ICON_TRAY) {
    emit signalRefreshInfoTray();
  } else {
#if defined(QT_NO_DEBUG_OUTPUT)
    traySystem->setIcon(QIcon(":/images/quiterss16"));
#else
    traySystem->setIcon(QIcon(":/images/quiterssDebug"));
#endif
  }
  singleClickTray_ = optionsDialog_->singleClickTray_->isChecked();
  clearStatusNew_ = optionsDialog_->clearStatusNew_->isChecked();
  emptyWorking_ = optionsDialog_->emptyWorking_->isChecked();
  if (showTrayIcon_) traySystem->show();
  else traySystem->hide();

  networkProxy_ = optionsDialog_->proxy();
  setProxy(networkProxy_);

  if (optionsDialog_->embeddedBrowserOn_->isChecked()) {
    if (optionsDialog_->defaultExternalBrowserOn_->isChecked())
      externalBrowserOn_ = 0;
    else
      externalBrowserOn_ = -1;
  } else {
    if (optionsDialog_->defaultExternalBrowserOn_->isChecked())
      externalBrowserOn_ = 1;
    else
      externalBrowserOn_ = 2;
  }

  externalBrowser_ = optionsDialog_->otherExternalBrowserEdit_->text();
  javaScriptEnable_ = optionsDialog_->javaScriptEnable_->isChecked();
  pluginsEnable_ = optionsDialog_->pluginsEnable_->isChecked();
  openLinkInBackground_ = optionsDialog_->openLinkInBackground_->isChecked();
  openLinkInBackgroundEmbedded_ = optionsDialog_->openLinkInBackgroundEmbedded_->isChecked();
  userStyleBrowser_ = optionsDialog_->userStyleBrowserEdit_->text();
  maxPagesInCache_ = optionsDialog_->maxPagesInCache_->value();

  QWebSettings::globalSettings()->setAttribute(
        QWebSettings::JavascriptEnabled, javaScriptEnable_);
  QWebSettings::globalSettings()->setAttribute(
        QWebSettings::PluginsEnabled, pluginsEnable_);
  QWebSettings::globalSettings()->setMaximumPagesInCache(maxPagesInCache_);
  QWebSettings::globalSettings()->setUserStyleSheetUrl(userStyleSheet(userStyleBrowser_));

  useDiskCache = optionsDialog_->diskCacheOn_->isChecked();
  settings_->setValue("Settings/useDiskCache", useDiskCache);
  maxDiskCache = optionsDialog_->maxDiskCache_->value();
  settings_->setValue("Settings/maxDiskCache", maxDiskCache);

  if (diskCacheDirPath != optionsDialog_->dirDiskCacheEdit_->text()) {
    removePath(diskCacheDirPath);
  }
  diskCacheDirPath = optionsDialog_->dirDiskCacheEdit_->text();
  if (diskCacheDirPath.isEmpty()) diskCacheDirPath = diskCacheDirPathDefault_;
  settings_->setValue("Settings/dirDiskCache", diskCacheDirPath);

  if (useDiskCache) {
    if (diskCache_ == NULL) {
      diskCache_ = new QNetworkDiskCache(this);
      networkManager_->setCache(diskCache_);
    }
    diskCache_->setCacheDirectory(diskCacheDirPath);
    diskCache_->setMaximumCacheSize(maxDiskCache*1024*1024);
  } else {
    if (diskCache_ != NULL) {
      diskCache_->setMaximumCacheSize(0);
      diskCache_->clear();
    }
  }

  if (optionsDialog_->deleteCookiesOnClose_->isChecked())
    cookieJar_->saveCookies_ = 2;
  else if (optionsDialog_->blockCookies_->isChecked())
    cookieJar_->saveCookies_ = 0;
  else
    cookieJar_->saveCookies_ = 1;

  downloadLocation_ = optionsDialog_->downloadLocationEdit_->text();
  askDownloadLocation_ = optionsDialog_->askDownloadLocation_->isChecked();

  updateFeedsStartUp_ = optionsDialog_->updateFeedsStartUp_->isChecked();
  updateFeedsEnable_ = optionsDialog_->updateFeedsEnable_->isChecked();
  updateFeedsInterval_ = optionsDialog_->updateFeedsInterval_->value();
  updateFeedsIntervalType_ = optionsDialog_->updateIntervalType_->currentIndex()-1;

  int updateInterval = updateFeedsInterval_;
  if (updateFeedsIntervalType_ == 0)
    updateInterval = updateInterval*60;
  else if (updateFeedsIntervalType_ == 1)
    updateInterval = updateInterval*60*60;
  updateIntervalSec_ = updateInterval;

  openingFeedAction_ = optionsDialog_->getOpeningFeed();
  openNewsWebViewOn_ = optionsDialog_->openNewsWebViewOn_->isChecked();

  markNewsReadOn_ = optionsDialog_->markNewsReadOn_->isChecked();
  markCurNewsRead_ = optionsDialog_->markCurNewsRead_->isChecked();
  markNewsReadTime_ = optionsDialog_->markNewsReadTime_->value();
  markPrevNewsRead_ = optionsDialog_->markPrevNewsRead_->isChecked();
  markReadSwitchingFeed_ = optionsDialog_->markReadSwitchingFeed_->isChecked();
  markReadClosingTab_ = optionsDialog_->markReadClosingTab_->isChecked();
  markReadMinimize_ = optionsDialog_->markReadMinimize_->isChecked();

  showDescriptionNews_ = optionsDialog_->showDescriptionNews_->isChecked();

  formatDate_ = optionsDialog_->formatDate_->itemData(
        optionsDialog_->formatDate_->currentIndex()).toString();
  feedsTreeModel_->formatDate_ = formatDate_;
  formatTime_ = optionsDialog_->formatTime_->itemData(
        optionsDialog_->formatTime_->currentIndex()).toString();
  feedsTreeModel_->formatTime_ = formatTime_;

  alternatingRowColorsNews_ = optionsDialog_->alternatingRowColorsNews_->isChecked();
  changeBehaviorActionNUN_ = optionsDialog_->changeBehaviorActionNUN_->isChecked();
  simplifiedDateTime_ = optionsDialog_->simplifiedDateTime_->isChecked();
  notDeleteStarred_ = optionsDialog_->notDeleteStarred_->isChecked();
  notDeleteLabeled_ = optionsDialog_->notDeleteLabeled_->isChecked();
  markIdenticalNewsRead_ = optionsDialog_->markIdenticalNewsRead_->isChecked();

  mainNewsFilter_ = optionsDialog_->mainNewsFilter_->itemData(
        optionsDialog_->mainNewsFilter_->currentIndex()).toString();

  cleanupOnShutdown_ = optionsDialog_->cleanupOnShutdownBox_->isChecked();
  dayCleanUpOn_ = optionsDialog_->dayCleanUpOn_->isChecked();
  maxDayCleanUp_ = optionsDialog_->maxDayCleanUp_->value();
  newsCleanUpOn_ = optionsDialog_->newsCleanUpOn_->isChecked();
  maxNewsCleanUp_ = optionsDialog_->maxNewsCleanUp_->value();
  readCleanUp_ = optionsDialog_->readCleanUp_->isChecked();
  neverUnreadCleanUp_ = optionsDialog_->neverUnreadCleanUp_->isChecked();
  neverStarCleanUp_ = optionsDialog_->neverStarCleanUp_->isChecked();
  neverLabelCleanUp_ = optionsDialog_->neverLabelCleanUp_->isChecked();
  cleanUpDeleted_ = optionsDialog_->cleanUpDeleted_->isChecked();
  optimizeDB_ = optionsDialog_->optimizeDB_->isChecked();

  soundNewNews_ = optionsDialog_->soundNewNews_->isChecked();
  soundNotifyPath_ = optionsDialog_->editSoundNotifer_->text();
  showNotifyOn_ = optionsDialog_->showNotifyOn_->isChecked();
  positionNotify_ = optionsDialog_->positionNotify_->currentIndex();
  countShowNewsNotify_ = optionsDialog_->countShowNewsNotify_->value();
  widthTitleNewsNotify_ = optionsDialog_->widthTitleNewsNotify_->value();
  timeShowNewsNotify_ = optionsDialog_->timeShowNewsNotify_->value();
  fullscreenModeNotify_ = optionsDialog_->fullscreenModeNotify_->isChecked();
  onlySelectedFeeds_ = optionsDialog_->onlySelectedFeeds_->isChecked();

  langFileName_ = optionsDialog_->language();
  appInstallTranslator();

  QFont font = feedsTreeView_->font();
  font.setFamily(
        optionsDialog_->fontsTree_->topLevelItem(0)->text(2).section(", ", 0, 0));
  font.setPointSize(
        optionsDialog_->fontsTree_->topLevelItem(0)->text(2).section(", ", 1).toInt());
  feedsTreeView_->setFont(font);
  feedsTreeModel_->font_ = font;

  newsListFontFamily_ = optionsDialog_->fontsTree_->topLevelItem(1)->text(2).section(", ", 0, 0);
  newsListFontSize_ = optionsDialog_->fontsTree_->topLevelItem(1)->text(2).section(", ", 1).toInt();
  newsTitleFontFamily_ = optionsDialog_->fontsTree_->topLevelItem(2)->text(2).section(", ", 0, 0);
  newsTitleFontSize_ = optionsDialog_->fontsTree_->topLevelItem(2)->text(2).section(", ", 1).toInt();
  newsTextFontFamily_ = optionsDialog_->fontsTree_->topLevelItem(3)->text(2).section(", ", 0, 0);
  newsTextFontSize_ = optionsDialog_->fontsTree_->topLevelItem(3)->text(2).section(", ", 1).toInt();
  notificationFontFamily_ = optionsDialog_->fontsTree_->topLevelItem(4)->text(2).section(", ", 0, 0);
  notificationFontSize_ = optionsDialog_->fontsTree_->topLevelItem(4)->text(2).section(", ", 1).toInt();

  browserMinFontSize_ = optionsDialog_->browserMinFontSize_->value();
  browserMinLogFontSize_ = optionsDialog_->browserMinLogFontSize_->value();

  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::StandardFont, newsTextFontFamily_);
  QWebSettings::globalSettings()->setFontSize(
        QWebSettings::MinimumFontSize, browserMinFontSize_);
  QWebSettings::globalSettings()->setFontSize(
        QWebSettings::MinimumLogicalFontSize, browserMinLogFontSize_);

  feedsTreeModel_->textColor_ = optionsDialog_->colorsTree_->topLevelItem(0)->text(1);
  feedsTreeModel_->backgroundColor_ = optionsDialog_->colorsTree_->topLevelItem(1)->text(1);
  feedsTreeView_->setStyleSheet(QString("#feedsTreeView_ {background: %1;}").arg(feedsTreeModel_->backgroundColor_));
  newsListTextColor_ = optionsDialog_->colorsTree_->topLevelItem(2)->text(1);
  newsListBackgroundColor_ = optionsDialog_->colorsTree_->topLevelItem(3)->text(1);
  focusedNewsTextColor_ = optionsDialog_->colorsTree_->topLevelItem(4)->text(1);
  focusedNewsBGColor_ = optionsDialog_->colorsTree_->topLevelItem(5)->text(1);
  linkColor_ = optionsDialog_->colorsTree_->topLevelItem(6)->text(1);
  titleColor_ = optionsDialog_->colorsTree_->topLevelItem(7)->text(1);
  dateColor_ = optionsDialog_->colorsTree_->topLevelItem(8)->text(1);
  authorColor_ = optionsDialog_->colorsTree_->topLevelItem(9)->text(1);
  newsTitleBackgroundColor_ = optionsDialog_->colorsTree_->topLevelItem(10)->text(1);
  newsBackgroundColor_ = optionsDialog_->colorsTree_->topLevelItem(11)->text(1);
  feedsTreeModel_->feedWithNewNewsColor_ = optionsDialog_->colorsTree_->topLevelItem(12)->text(1);
  feedsTreeModel_->countNewsUnreadColor_ = optionsDialog_->colorsTree_->topLevelItem(13)->text(1);

  delete optionsDialog_;
  optionsDialog_ = NULL;

  writeSettings();
  saveActionShortcuts();

  if (currentNewsTab != NULL) {
    if (currentNewsTab->type_ < TAB_WEB)
      currentNewsTab->newsHeader_->saveStateColumns(this, currentNewsTab);
    currentNewsTab->setSettings();
  }
}

// ----------------------------------------------------------------------------
void RSSListing::createTrayMenu()
{
  trayMenu_ = new QMenu(this);
  showWindowAct_ = new QAction(this);
  connect(showWindowAct_, SIGNAL(triggered()), this, SLOT(slotShowWindows()));
  QFont font_ = showWindowAct_->font();
  font_.setBold(true);
  showWindowAct_->setFont(font_);
  trayMenu_->addAction(showWindowAct_);
  trayMenu_->addAction(updateAllFeedsAct_);
  trayMenu_->addSeparator();

  trayMenu_->addAction(optionsAct_);
  trayMenu_->addSeparator();

  trayMenu_->addAction(exitAct_);
  traySystem->setContextMenu(trayMenu_);
}

/** @brief Free memory working set in Windows
 *---------------------------------------------------------------------------*/
void RSSListing::myEmptyWorkingSet()
{
#if defined(Q_OS_WIN)
  if (isHidden())
    EmptyWorkingSet(GetCurrentProcess());
#endif
}

/** @brief Show update progress bar after feed update has started
 *---------------------------------------------------------------------------*/
void RSSListing::showProgressBar(int maximum)
{
  if (maximum == 0) {
    importFeedStart_ = false;
    return;
  }
  playSoundNewNews_ = false;

  progressBar_->setMaximum(maximum);
  progressBar_->show();
}

/** @brief Process update feed action
 *---------------------------------------------------------------------------*/
void RSSListing::slotGetFeed()
{
  QPersistentModelIndex index = feedsTreeView_->selectIndex();
  if (feedsTreeModel_->isFolder(index)) {
    QSqlQuery q;
    QString str = getIdFeedsString(feedsTreeModel_->dataField(index, "id").toInt());
    str.replace("feedId", "id");
    QString qStr = QString("SELECT xmlUrl, lastBuildDate, authentication FROM feeds WHERE (%1)").
        arg(str);
    q.exec(qStr);
    while (q.next()) {
      addFeedInQueue(q.value(0).toString(), q.value(1).toDateTime(), q.value(2).toInt());
    }
  } else {
    addFeedInQueue(feedsTreeModel_->dataField(index, "xmlUrl").toString(),
                   feedsTreeModel_->dataField(index, "lastBuildDate").toDateTime(),
                   feedsTreeModel_->dataField(index, "authentication").toInt());
  }

  showProgressBar(updateFeedsCount_);
}

/** @brief Process update all feeds action
 *---------------------------------------------------------------------------*/
void RSSListing::slotGetAllFeeds()
{
  QSqlQuery q;
  q.exec("SELECT xmlUrl, lastBuildDate, authentication FROM feeds WHERE xmlUrl!=''");
  while (q.next()) {
    addFeedInQueue(q.value(0).toString(), q.value(1).toDateTime(), q.value(2).toInt());
  }

  showProgressBar(updateFeedsCount_);

  timer_.start();
  //  qCritical() << "Start update";
  //  qCritical() << "------------------------------------------------------------";
}
// ----------------------------------------------------------------------------
void RSSListing::slotVisibledFeedsWidget()
{
  if (tabBar_->currentIndex() == TAB_WIDGET_PERMANENT) {
    showFeedsTabPermanent_ = feedsWidgetVisibleAct_->isChecked();
  }

  feedsWidget_->setVisible(feedsWidgetVisibleAct_->isChecked());
  updateIconToolBarNull(feedsWidgetVisibleAct_->isChecked());
}
// ----------------------------------------------------------------------------
void RSSListing::updateIconToolBarNull(bool feedsWidgetVisible)
{
  if (feedsWidgetVisible)
    pushButtonNull_->setIcon(QIcon(":/images/images/triangleR.png"));
  else
    pushButtonNull_->setIcon(QIcon(":/images/images/triangleL.png"));
}
// ----------------------------------------------------------------------------
void RSSListing::markFeedRead()
{
  bool openFeed = false;
  QString qStr;
  QPersistentModelIndex index = feedsTreeView_->selectIndex();
  bool isFolder = feedsTreeModel_->isFolder(index);
  int id = feedsTreeModel_->getIdByIndex(index);
  if (currentNewsTab->feedId_ == id)
    openFeed = true;

  db_.transaction();
  QSqlQuery q;
  if (isFolder) {
    if (currentNewsTab->feedParId_ == id)
      openFeed = true;

    qStr = QString("UPDATE news SET read=2 WHERE read!=2 AND deleted==0 AND (%1)").
        arg(getIdFeedsString(id));
    q.exec(qStr);
    qStr = QString("UPDATE news SET new=0 WHERE new==1 AND (%1)").
        arg(getIdFeedsString(id));
    q.exec(qStr);
  } else {
    if (openFeed) {
      qStr = QString("UPDATE news SET read=2 WHERE feedId=='%1' AND read!=2 AND deleted==0").
          arg(id);
      q.exec(qStr);
    } else {
      QString qStr = QString("UPDATE news SET read=1 WHERE feedId=='%1' AND read==0").
          arg(id);
      q.exec(qStr);
    }
    qStr = QString("UPDATE news SET new=0 WHERE feedId=='%1' AND new==1").
        arg(id);
    q.exec(qStr);
  }
  db_.commit();
  // Update feed view with focus
  if (openFeed) {
    if ((tabBar_->currentIndex() == TAB_WIDGET_PERMANENT) &&
        !isFolder) {
      QModelIndex indexNextUnread =
          feedsTreeView_->indexNextUnread(feedsTreeView_->currentIndex());
      feedsTreeView_->setCurrentIndex(indexNextUnread);
      slotFeedClicked(indexNextUnread);
    } else {
      int currentRow = newsView_->currentIndex().row();

      newsModel_->select();
      while (newsModel_->canFetchMore())
        newsModel_->fetchMore();

      newsView_->setCurrentIndex(newsModel_->index(currentRow, newsModel_->fieldIndex("title")));

      slotUpdateStatus(id);
      recountCategoryCounts();
    }
  }
  // Update feeds view without focus
  else {
    slotUpdateStatus(id);
    recountCategoryCounts();
  }
}

/** @brief Update status of current feed or feed of current tab
 *---------------------------------------------------------------------------*/
void RSSListing::slotUpdateStatus(int feedId, bool changed)
{
  if (changed) {
    recountFeedCounts(feedId);
  }

  emit signalRefreshInfoTray();

  if ((feedId > 0) && (feedId == currentNewsTab->feedId_)) {
    QSqlQuery q;
    int unreadCount = 0;
    int allCount = 0;
    QString qStr = QString("SELECT unread, undeleteCount FROM feeds WHERE id=='%1'").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) {
      unreadCount = q.value(0).toInt();
      allCount    = q.value(1).toInt();
    }

    statusUnread_->setText(QString(" " + tr("Unread: %1") + " ").arg(unreadCount));
    statusAll_->setText(QString(" " + tr("All: %1") + " ").arg(allCount));
  }
}

/** @brief Set filter for viewing feeds and categories
 * @param pAct Filter mode
 * @param clicked Flag to call function after user click or from programm code
 *    true  - user click
 *    false - programm call
 *----------------------------------------------------------------------------*/
void RSSListing::setFeedsFilter(QAction* pAct, bool clicked)
{
  QModelIndex index = feedsTreeView_->currentIndex();
  int feedId = feedsTreeModel_->getIdByIndex(index);
  int newCount = feedsTreeModel_->dataField(index, "newCount").toInt();
  int unRead   = feedsTreeModel_->dataField(index, "unread").toInt();

  QList<int> parentIdList;  //*< feed parent list
  while (index.parent().isValid()) {
    parentIdList << feedsTreeModel_->getParidByIndex(index);
    index = index.parent();
  }

  // Create feed filter from "filter"
  QString strFilter;
  if (pAct->objectName() == "filterFeedsAll_") {
    strFilter = "";
  } else if (pAct->objectName() == "filterFeedsNew_") {
    if (clicked && !newCount) {
      strFilter = QString("newCount > 0");
    } else {
      strFilter = QString("(newCount > 0 OR id=='%1'").arg(feedId);
      foreach (int parentId, parentIdList)
        strFilter.append(QString(" OR id=='%1'").arg(parentId));
      strFilter.append(")");
    }
  } else if (pAct->objectName() == "filterFeedsUnread_") {
    if (clicked && !unRead) {
      strFilter = QString("unread > 0");
    } else {
      strFilter = QString("(unread > 0 OR id=='%1'").arg(feedId);
      foreach (int parentId, parentIdList)
        strFilter.append(QString(" OR id=='%1'").arg(parentId));
      strFilter.append(")");
    }
  } else if (pAct->objectName() == "filterFeedsStarred_") {
    strFilter = QString("label LIKE '%starred%'");
  }

  // ... add filter from "search"
  if (findFeedsWidget_->isVisible()) {
    if (pAct->objectName() != "filterFeedsAll_")
      strFilter.append(" AND ");

    // add categories into filter to display nested feed
    strFilter.append("(");
    strFilter.append(QString("((xmlUrl = '') OR (xmlUrl IS NULL)) OR "));
    if (findFeeds_->findGroup_->checkedAction()->objectName() == "findNameAct") {
      strFilter.append(QString("text LIKE '%%1%'").arg(findFeeds_->text()));
    } else {
      strFilter.append(QString("xmlUrl LIKE '%%1%'").arg(findFeeds_->text()));
    }
    strFilter.append(")");
  }

  QElapsedTimer timer;
  timer.start();
  qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed() << strFilter;

  static QString strFilterOld = QString();

  if (strFilterOld.compare(strFilter) == 0) {
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed() << "No filter changes";
    return;
  }
  qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed() << "Applying new filter";
  strFilterOld = strFilter;

  // Set filter
  feedsTreeModel_->setFilter(strFilter);
  expandNodes();

  qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed();

  if (pAct->objectName() == "filterFeedsAll_") feedsFilter_->setIcon(QIcon(":/images/filterOff"));
  else feedsFilter_->setIcon(QIcon(":/images/filterOn"));

  // Restore cursor on previous displayed feed
  QModelIndex feedIndex = feedsTreeModel_->getIndexById(feedId);
  feedsTreeView_->setCurrentIndex(feedIndex);

  if (clicked) {
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed();

    if (tabBar_->currentIndex() == TAB_WIDGET_PERMANENT) {
      slotFeedClicked(feedIndex);
    }
  }

  qDebug() << __PRETTY_FUNCTION__ << __LINE__ << timer.elapsed();

  // Store filter to enable it as "last used filter"
  if (pAct->objectName() != "filterFeedsAll_")
    feedsFilterAction_ = pAct;
}

/** @brief Set filter for news list
 *---------------------------------------------------------------------------*/
void RSSListing::setNewsFilter(QAction* pAct, bool clicked)
{
  if (currentNewsTab == NULL) return;
  if (currentNewsTab->type_ >= TAB_WEB) {
    filterNewsAll_->setChecked(true);
    return;
  }

  QElapsedTimer timer;
  timer.start();
  qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

  QModelIndex index = newsView_->currentIndex();

  int feedId = currentNewsTab->feedId_;
  int newsId = newsModel_->index(
        index.row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

  // Hide news has marrked "Read"
  // read=1 - show regardless of filter
  // read=2 - don't display at all
  if (clicked) {
    QString qStr = QString("UPDATE news SET read=2 WHERE feedId='%1' AND read=1").
        arg(feedId);
    QSqlQuery q;
    q.exec(qStr);
  }

  // Create filter for category or for feed
  if (feedsTreeModel_->isFolder(feedsTreeModel_->getIndexById(feedId))) {
    newsFilterStr = QString("(%1) AND ").arg(getIdFeedsString(feedId));
  } else {
    newsFilterStr = QString("feedId=%1 AND ").arg(feedId);
  }

  // ... add filter from "filter"
  if (pAct->objectName() == "filterNewsAll_") {
    newsFilterStr.append("deleted = 0");
  } else if (pAct->objectName() == "filterNewsNew_") {
    newsFilterStr.append(QString("new = 1 AND deleted = 0"));
  } else if (pAct->objectName() == "filterNewsUnread_") {
    newsFilterStr.append(QString("read < 2 AND deleted = 0"));
  } else if (pAct->objectName() == "filterNewsStar_") {
    newsFilterStr.append(QString("starred = 1 AND deleted = 0"));
  } else if (pAct->objectName() == "filterNewsNotStarred_") {
    newsFilterStr.append(QString("starred = 0 AND deleted = 0"));
  } else if (pAct->objectName() == "filterNewsUnreadStar_") {
    newsFilterStr.append(QString("(read < 2 OR starred = 1) AND deleted = 0"));
  } else if (pAct->objectName() == "filterNewsLastDay_") {
    newsFilterStr.append(QString("(published >= datetime('now', '-1 day')) AND deleted = 0"));
  } else if (pAct->objectName() == "filterNewsLastWeek_") {
    newsFilterStr.append(QString("(published >= datetime('now', '-7 day')) AND deleted = 0"));
  }

  // ... add filter from "search"
  QString filterStr = newsFilterStr;
  if (currentNewsTab->findText_->findGroup_->checkedAction()->objectName() == "findInNewsAct") {
    filterStr.append(
          QString(" AND (title LIKE '%%1%' OR author_name LIKE '%%1%' OR category LIKE '%%1%')").
          arg(currentNewsTab->findText_->text()));
  }

  qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed() << filterStr;
  newsModel_->setFilter(filterStr);

  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

  if ((currentNewsTab->newsHeader_->sortIndicatorSection() == newsModel_->fieldIndex("read")) ||
      currentNewsTab->newsHeader_->sortIndicatorSection() == newsModel_->fieldIndex("starred")) {
    currentNewsTab->slotSort(currentNewsTab->newsHeader_->sortIndicatorSection(),
                             currentNewsTab->newsHeader_->sortIndicatorOrder());
  }

  // Set icon right before user click
  if (pAct->objectName() == "filterNewsAll_") newsFilter_->setIcon(QIcon(":/images/filterOff"));
  else newsFilter_->setIcon(QIcon(":/images/filterOn"));

  // Set focus on previous displayed feed, if user click has been
  if (clicked) {
    QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("id"));
    QModelIndexList indexList = newsModel_->match(index, Qt::EditRole, newsId);
    if (indexList.count()) {
      int newsRow = indexList.first().row();
      newsView_->setCurrentIndex(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
    } else {
      currentNewsTab->currentNewsIdOld = newsId;
      currentNewsTab->hideWebContent();
    }
  }

  qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

  // Store filter to enable it as "last used filter"
  if (pAct->objectName() != mainNewsFilter_)
    newsFilterAction_ = pAct;
}

/** @brief Mark feed Read while clicking on unfocused one
 *---------------------------------------------------------------------------*/
void RSSListing::setFeedRead(int type, int feedId, FeedReedType feedReadType,
                             NewsTabWidget *widgetTab, int idException)
{
  if ((type >= TAB_WEB) || (type == TAB_CAT_DEL))
    return;

  if ((type == TAB_FEED) && (feedReadType != FeedReadSwitchingTab)) {
    if (feedId <= -1) return;

    db_.transaction();
    QString idFeedsStr = getIdFeedsString(feedId, idException);
    QSqlQuery q;
    if (((feedReadType == FeedReadSwitchingFeed) && markReadSwitchingFeed_) ||
        ((feedReadType == FeedReadClosingTab) && markReadClosingTab_) ||
        ((feedReadType == FeedReadPlaceToTray) && markReadMinimize_)) {
      if (idFeedsStr == "feedId=-1") {
        q.exec(QString("UPDATE news SET read=2 WHERE feedId='%1' AND read!=2").arg(feedId));
      } else {
        q.exec(QString("UPDATE news SET read=2 WHERE (%1) AND read=1").arg(idFeedsStr));
      }
    } else {
      if (idFeedsStr == "feedId=-1") {
        q.exec(QString("UPDATE news SET read=2 WHERE feedId='%1' AND read=1").arg(feedId));
      } else {
        q.exec(QString("UPDATE news SET read=2 WHERE (%1) AND read=1").arg(idFeedsStr));
      }
    }
    if (idFeedsStr == "feedId=-1") {
      q.exec(QString("UPDATE news SET new=0 WHERE feedId='%1' AND new=1").arg(feedId));
    } else {
      q.exec(QString("UPDATE news SET new=0 WHERE (%1) AND new=1").arg(idFeedsStr));
    }

    if (markNewsReadOn_ && markPrevNewsRead_)
      q.exec(QString("UPDATE news SET read=2 WHERE id IN (SELECT currentNews FROM feeds WHERE id='%1')").arg(feedId));

    db_.commit();

    recountFeedCounts(feedId, false);
    recountCategoryCounts();
    if (feedReadType != FeedReadPlaceToTray) {
      emit signalRefreshInfoTray();
    }
  } else {
    int cnt = widgetTab->newsModel_->rowCount();
    if (cnt == 0) return;

    QList <int> idNewsList;
    for (int i = 0; i < stackedWidget_->count(); i++) {
      NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
      if ((widget->type_ < TAB_WEB) &&
          !((feedReadType == FeedReadSwitchingFeed) && (i == TAB_WIDGET_PERMANENT))) {
        int row = widget->newsView_->currentIndex().row();
        int newsId = widget->newsModel_->index(row, widget->newsModel_->fieldIndex("id")).data().toInt();
        idNewsList.append(newsId);
      }
    }

    db_.transaction();
    QSqlQuery q;
    for (int i = cnt-1; i >= 0; --i) {
      int newsId = widgetTab->newsModel_->index(i, widgetTab->newsModel_->fieldIndex("id")).data().toInt();
      if (!idNewsList.contains(newsId)) {
        q.exec(QString("UPDATE news SET read=2 WHERE id=='%1' AND read==1").arg(newsId));
        q.exec(QString("UPDATE news SET new=0 WHERE id=='%1' AND new==1").arg(newsId));
      }
    }
    db_.commit();

    if (feedId > -1)
      recountFeedCounts(feedId, false);
  }
}
// ----------------------------------------------------------------------------
void RSSListing::slotShowAboutDlg()
{
  AboutDialog *aboutDialog = new AboutDialog(langFileName_, this);
  aboutDialog->exec();
  delete aboutDialog;
}

/** @brief Call context menu of the feeds tree
 *----------------------------------------------------------------------------*/
void RSSListing::showContextMenuFeed(const QPoint &pos)
{
  slotFeedMenuShow();

  QModelIndex index = feedsTreeView_->indexAt(pos);
  if (index.isValid()) {
    QRect rectText = feedsTreeView_->visualRect(index);
    if (pos.x() >= rectText.x()) {
      QMenu menu;
      menu.addAction(addAct_);
      menu.addSeparator();
      menu.addAction(openFeedNewTabAct_);
      menu.addSeparator();
      menu.addAction(updateFeedAct_);
      menu.addSeparator();
      menu.addAction(markFeedRead_);
      menu.addSeparator();
      menu.addAction(deleteFeedAct_);
      menu.addSeparator();
      menu.addAction(setFilterNewsAct_);
      menu.addAction(feedProperties_);

      menu.exec(feedsTreeView_->viewport()->mapToGlobal(pos));
    }
  } else {
    QMenu menu;
    menu.addAction(addAct_);

    menu.exec(feedsTreeView_->viewport()->mapToGlobal(pos));
  }

  index = feedsTreeView_->currentIndex();
  feedsTreeView_->selectId_ = feedsTreeModel_->getIdByIndex(index);

  feedProperties_->setEnabled(feedsTreeView_->selectIndex().isValid());
}
// ----------------------------------------------------------------------------
void RSSListing::slotFeedMenuShow()
{
  feedProperties_->setEnabled(feedsTreeView_->selectIndex().isValid());
}
// ----------------------------------------------------------------------------
void RSSListing::setAutoLoadImages(bool set)
{
  if (currentNewsTab->type_ == TAB_DOWNLOADS) return;

  autoLoadImages_ = !autoLoadImages_;
  if (autoLoadImages_) {
    autoLoadImagesToggle_->setText(tr("Load Images"));
    autoLoadImagesToggle_->setToolTip(tr("Auto Load Images to News View"));
    autoLoadImagesToggle_->setIcon(QIcon(":/images/imagesOn"));
  } else {
    autoLoadImagesToggle_->setText(tr("No Load Images"));
    autoLoadImagesToggle_->setToolTip(tr("No Load Images to News View"));
    autoLoadImagesToggle_->setIcon(QIcon(":/images/imagesOff"));
  }

  if (set) {
    currentNewsTab->autoLoadImages_ = autoLoadImages_;
    currentNewsTab->webView_->settings()->setAttribute(
          QWebSettings::AutoLoadImages, autoLoadImages_);
    if (autoLoadImages_) {
      if ((currentNewsTab->webView_->title() == "news_descriptions") &&
          (currentNewsTab->type_ == TAB_FEED))
        currentNewsTab->updateWebView(newsView_->currentIndex());
      else currentNewsTab->webView_->reload();
    }
  }
}
// ----------------------------------------------------------------------------
void RSSListing::loadSettingsFeeds()
{
  markCurNewsRead_ = false;
  behaviorIconTray_ = settings_->value("Settings/behaviorIconTray", NEW_COUNT_ICON_TRAY).toInt();

  QString filterName = settings_->value("feedSettings/filterName", "filterFeedsAll_").toString();
  QList<QAction*> listActions = feedsFilterGroup_->actions();
  foreach(QAction *action, listActions) {
    if (action->objectName() == filterName) {
      action->setChecked(true);
      break;
    }
  }
  filterName = settings_->value("newsSettings/filterName", "filterNewsAll_").toString();
  listActions = newsFilterGroup_->actions();
  foreach(QAction *action, listActions) {
    if (action->objectName() == filterName) {
      action->setChecked(true);
      break;
    }
  }

  setFeedsFilter(feedsFilterGroup_->checkedAction(), false);
}

/** @brief Restore feeds state on application startup
 *---------------------------------------------------------------------------*/
void RSSListing::restoreFeedsOnStartUp()
{
  qApp->processEvents();
  //* Restore current feed
  QModelIndex feedIndex = QModelIndex();
  if (reopenFeedStartup_) {
    int feedId = settings_->value("feedSettings/currentId", 0).toInt();
    feedIndex = feedsTreeModel_->getIndexById(feedId);
  }
  feedsTreeView_->setCurrentIndex(feedIndex);
  updateCurrentTab_ = false;
  slotFeedClicked(feedIndex);
  updateCurrentTab_ = true;

  slotUpdateStatus(-1, false);
  recountCategoryCounts();

  // Open feeds in tabs
  QSqlQuery q;
  q.exec(QString("SELECT id, parentId FROM feeds WHERE displayOnStartup=1"));
  while(q.next()) {
    creatFeedTab(q.value(0).toInt(), q.value(1).toInt());
  }

  if (updateFeedsStartUp_) slotGetAllFeeds();
}

/** @brief Expanding items with corresponding flag in DB
 *---------------------------------------------------------------------------*/
void RSSListing::expandNodes()
{
  QSqlQuery q;
  q.exec("SELECT id FROM feeds WHERE f_Expanded=1 AND (xmlUrl='' OR xmlUrl IS NULL)");
  while (q.next()) {
    int feedId = q.value(0).toInt();
    QModelIndex index = feedsTreeModel_->getIndexById(feedId);
    feedsTreeView_->setExpanded(index, true);
  }
}
// ----------------------------------------------------------------------------
void RSSListing::slotFeedsFilter()
{
  if (feedsFilterGroup_->checkedAction()->objectName() == "filterFeedsAll_") {
    if (feedsFilterAction_ != NULL) {
      feedsFilterAction_->setChecked(true);
      setFeedsFilter(feedsFilterAction_);
    } else {
      if (mainToolbar_->widgetForAction(feedsFilter_)) {
        QWidget *widget = mainToolbar_->widgetForAction(feedsFilter_);
        if (widget->underMouse()) {
          feedsFilterMenu_->popup(widget->mapToGlobal(QPoint(0, mainToolbar_->height()-1)));
        }
      }
      if (feedsToolBar_->widgetForAction(feedsFilter_)) {
        QWidget *widget = feedsToolBar_->widgetForAction(feedsFilter_);
        if (widget->underMouse()) {
          feedsFilterMenu_->popup(widget->mapToGlobal(QPoint(0, feedsToolBar_->height()-1)));
        }
      }
      if (currentNewsTab->newsToolBar_->widgetForAction(feedsFilter_)) {
        QWidget *widget = currentNewsTab->newsToolBar_->widgetForAction(feedsFilter_);
        if (widget->underMouse()) {
          feedsFilterMenu_->popup(widget->mapToGlobal(QPoint(0, currentNewsTab->newsToolBar_->height()-1)));
        }
      }
    }
  } else {
    filterFeedsAll_->setChecked(true);
    setFeedsFilter(filterFeedsAll_);
  }
}
// ----------------------------------------------------------------------------
void RSSListing::slotNewsFilter()
{
  if (newsFilterGroup_->checkedAction()->objectName() == mainNewsFilter_) {
    if (newsFilterAction_ != NULL) {
      newsFilterAction_->setChecked(true);
      setNewsFilter(newsFilterAction_);
    } else {
      if (mainToolbar_->widgetForAction(newsFilter_)) {
        QWidget *widget = mainToolbar_->widgetForAction(newsFilter_);
        if (widget->underMouse()) {
          newsFilterMenu_->popup(widget->mapToGlobal(QPoint(0, mainToolbar_->height()-1)));
        }
      }
      if (feedsToolBar_->widgetForAction(newsFilter_)) {
        QWidget *widget = feedsToolBar_->widgetForAction(newsFilter_);
        if (widget->underMouse()) {
          newsFilterMenu_->popup(widget->mapToGlobal(QPoint(0, feedsToolBar_->height()-1)));
        }
      }
      if (currentNewsTab->newsToolBar_->widgetForAction(newsFilter_)) {
        QWidget *widget = currentNewsTab->newsToolBar_->widgetForAction(newsFilter_);
        if (widget->underMouse()) {
          newsFilterMenu_->popup(widget->mapToGlobal(QPoint(0, currentNewsTab->newsToolBar_->height()-1)));
        }
      }
    }
  } else {
    foreach(QAction *action, newsFilterGroup_->actions()) {
      if (action->objectName() == mainNewsFilter_) {
        action->setChecked(true);
        setNewsFilter(action);
        break;
      }
    }
  }
}
// ----------------------------------------------------------------------------
void RSSListing::initUpdateFeeds()
{
  QSqlQuery q;
  q.exec("SELECT id, updateInterval, updateIntervalType FROM feeds WHERE xmlUrl != '' AND updateIntervalEnable == 1");
  while (q.next()) {
    int updateInterval = q.value(1).toInt();
    int updateIntervalType = q.value(2).toInt();
    if (updateIntervalType == 0)
      updateInterval = updateInterval*60;
    else if (updateIntervalType == 1)
      updateInterval = updateInterval*60*60;

    updateFeedsIntervalSec_.insert(q.value(0).toInt(), updateInterval);
    updateFeedsTimeCount_.insert(q.value(0).toInt(), 0);
  }

  int updateInterval = updateFeedsInterval_;
  if (updateFeedsIntervalType_ == 0)
    updateInterval = updateInterval*60;
  else if (updateFeedsIntervalType_ == 1)
    updateInterval = updateInterval*60*60;
  updateIntervalSec_ = updateInterval;

  updateFeedsTimer_ = new QTimer(this);
  connect(updateFeedsTimer_, SIGNAL(timeout()),
          this, SLOT(slotUpdateFeedsTimer()));
  updateFeedsTimer_->start(1000);
}
// ----------------------------------------------------------------------------
void RSSListing::slotUpdateFeedsTimer()
{
  if (updateFeedsEnable_) {
    updateTimeCount_++;
    if (updateTimeCount_ >= updateIntervalSec_) {
      updateTimeCount_ = 0;

      QSqlQuery q;
      q.exec("SELECT xmlUrl, lastBuildDate, authentication FROM feeds "
             "WHERE xmlUrl!='' AND (updateIntervalEnable==-1 OR updateIntervalEnable IS NULL)");
      while (q.next()) {
        addFeedInQueue(q.value(0).toString(), q.value(1).toDateTime(), q.value(2).toInt());
      }
      showProgressBar(updateFeedsCount_);
    }
  } else {
    updateTimeCount_ = 0;
  }

  QMapIterator<int, int> iterator(updateFeedsTimeCount_);
  while (iterator.hasNext()) {
    iterator.next();
    int feedId = iterator.key();
    updateFeedsTimeCount_[feedId]++;
    if (updateFeedsTimeCount_[feedId] >= updateFeedsIntervalSec_[feedId]) {
      updateFeedsTimeCount_[feedId] = 0;

      QSqlQuery q;
      q.exec(QString("SELECT xmlUrl, lastBuildDate, authentication FROM feeds WHERE id=='%1'")
             .arg(feedId));
      if (q.next()) {
        addFeedInQueue(q.value(0).toString(), q.value(1).toDateTime(), q.value(2).toInt());
      }
      showProgressBar(updateFeedsCount_);
    }
  }
}
// ----------------------------------------------------------------------------
void RSSListing::slotShowUpdateAppDlg()
{
  UpdateAppDialog *updateAppDialog = new UpdateAppDialog(langFileName_,
                                                         settings_, this);
  updateAppDialog->activateWindow();
  updateAppDialog->exec();
  delete updateAppDialog;
}
// ----------------------------------------------------------------------------
void RSSListing::appInstallTranslator()
{
  bool translatorLoad;
  qApp->removeTranslator(translator_);
  translatorLoad = translator_->load(appDataDirPath_ +
                                     QString("/lang/quiterss_%1").arg(langFileName_));
  if (translatorLoad) qApp->installTranslator(translator_);
  else retranslateStrings();
}
// ----------------------------------------------------------------------------
void RSSListing::retranslateStrings()
{
  QString str = statusUnread_->text();
  str = str.right(str.length() - str.indexOf(':') - 1).replace(" ", "");
  statusUnread_->setText(QString(" " + tr("Unread: %1") + " ").arg(str));
  str = statusAll_->text();
  str = str.right(str.length() - str.indexOf(':') - 1).replace(" ", "");
  statusAll_->setText(QString(" " + tr("All: %1") + " ").arg(str));

  str = traySystem->toolTip();
  QString info =
      "QuiteRSS\n" +
      QString(tr("New News: %1")).arg(str.section(": ", 1).section("\n", 0, 0)) +
      QString("\n") +
      QString(tr("Unread News: %1")).arg(str.section(": ", 2));
  traySystem->setToolTip(info);

  addAct_->setText(tr("&Add"));
  addAct_->setToolTip(tr("Add New Feed"));

  addFeedAct_->setText(tr("&Feed..."));
  addFeedAct_->setToolTip(tr("Add New Feed"));

  addFolderAct_->setText(tr("F&older..."));
  addFolderAct_->setToolTip(tr("Add New Folder"));

  openFeedNewTabAct_->setText(tr("Open in New Tab"));

  deleteFeedAct_->setText(tr("&Delete..."));
  deleteFeedAct_->setToolTip(tr("Delete Selected Feed"));

  importFeedsAct_->setText(tr("&Import Feeds..."));
  importFeedsAct_->setToolTip(tr("Import Feeds from OPML File"));

  exportFeedsAct_->setText(tr("&Export Feeds..."));
  exportFeedsAct_->setToolTip(tr("Export Feeds to OPML File"));

  exitAct_->setText(tr("E&xit"));

  if (autoLoadImages_) {
    autoLoadImagesToggle_->setText(tr("Load Images"));
    autoLoadImagesToggle_->setToolTip(tr("Auto Load Images to News View"));
  } else {
    autoLoadImagesToggle_->setText(tr("No Load Images"));
    autoLoadImagesToggle_->setToolTip(tr("No Load Images to News View"));
  }

  updateFeedAct_->setText(tr("Update Feed"));
  updateFeedAct_->setToolTip(tr("Update Current Feed"));

  updateAllFeedsAct_->setText(tr("Update All"));
  updateAllFeedsAct_->setToolTip(tr("Update All Feeds"));

  markAllFeedsRead_->setText(tr("Mark All Feeds Read"));

  markNewsRead_->setText(tr("Mark Read/Unread"));
  markNewsRead_->setToolTip(tr("Mark Current News Read/Unread"));

  markAllNewsRead_->setText(tr("Mark All News Read"));
  markAllNewsRead_->setToolTip(tr("Mark All News Read"));

  showDownloadManagerAct_->setText(tr("Downloads"));

  showCleanUpWizardAct_->setText(tr("Clean Up..."));

  setNewsFiltersAct_->setText(tr("News Filters..."));
  setFilterNewsAct_->setText(tr("Filter News..."));

  optionsAct_->setText(tr("Options..."));
  optionsAct_->setToolTip(tr("Open Options Dialog"));

  feedsFilter_->setText(tr("Filter Feeds"));
  filterFeedsAll_->setText(tr("Show All"));
  filterFeedsNew_->setText(tr("Show New"));
  filterFeedsUnread_->setText(tr("Show Unread"));
  filterFeedsStarred_->setText(tr("Show Starred Feeds"));

  newsFilter_->setText(tr("Filter News"));
  filterNewsAll_->setText(tr("Show All"));
  filterNewsNew_->setText(tr("Show New"));
  filterNewsUnread_->setText(tr("Show Unread"));
  filterNewsStar_->setText(tr("Show Starred"));
  filterNewsNotStarred_->setText(tr("Show Not Starred"));
  filterNewsUnreadStar_->setText(tr("Show Unread or Starred"));
  filterNewsLastDay_->setText(tr("Show Last Day"));
  filterNewsLastWeek_->setText(tr("Show Last 7 Days"));

  aboutAct_ ->setText(tr("About..."));
  aboutAct_->setToolTip(tr("Show 'About' Dialog"));

  updateAppAct_->setText(tr("Check for Updates..."));
  reportProblemAct_->setText(tr("Report a Problem..."));

  openDescriptionNewsAct_->setText(tr("Open News"));
  openDescriptionNewsAct_->setToolTip(tr("Open Description News"));
  openInBrowserAct_->setText(tr("Open in Browser"));
  openInExternalBrowserAct_->setText(tr("Open in External Browser"));
  openInExternalBrowserAct_->setToolTip(tr("Open News in External Browser"));
  openNewsNewTabAct_->setText(tr("Open in New Tab"));
  openNewsNewTabAct_->setToolTip(tr("Open News in New Tab"));
  openNewsBackgroundTabAct_->setText(tr("Open in Background Tab"));
  openNewsBackgroundTabAct_->setToolTip(tr("Open News in Background Tab"));
  markStarAct_->setText(tr("Star"));
  markStarAct_->setToolTip(tr("Mark News Star"));
  deleteNewsAct_->setText(tr("Delete"));
  deleteNewsAct_->setToolTip(tr("Delete Selected News"));
  deleteAllNewsAct_->setText(tr("Delete All News"));
  deleteAllNewsAct_->setToolTip(tr("Delete All News from List"));
  restoreNewsAct_->setText(tr("Restore"));
  restoreNewsAct_->setToolTip(tr("Restore News"));
  copyLinkAct_->setText(tr("Copy Link"));
  copyLinkAct_->setToolTip(tr("Copy Link News"));

  restoreLastNewsAct_->setText(tr("Restore last deleted news"));

  markFeedRead_->setText(tr("Mark Read"));
  markFeedRead_->setToolTip(tr("Mark Feed Read"));
  feedProperties_->setText(tr("Properties"));
  feedProperties_->setToolTip(tr("Properties"));

  fileMenu_->setTitle(tr("&File"));
  editMenu_->setTitle(tr("&Edit"));
  viewMenu_->setTitle(tr("&View"));
  feedMenu_->setTitle(tr("Fee&ds"));
  newsMenu_->setTitle(tr("&News"));
  browserMenu_->setTitle(tr("&Browser"));
  toolsMenu_->setTitle(tr("&Tools"));
  helpMenu_->setTitle(tr("&Help"));

  mainToolbar_->setWindowTitle(tr("Main Toolbar"));
  customizeToolbarMenu_->setTitle(tr("Customize Toolbar"));
  customizeMainToolbarAct_->setText(tr("Main Toolbar..."));
  customizeMainToolbarAct2_->setText(tr("Customize Toolbar..."));
  customizeFeedsToolbarAct_->setText(tr("Feeds Toolbar..."));
  customizeNewsToolbarAct_->setText(tr("News Toolbar..."));

  toolBarLockAct_->setText(tr("Lock Toolbar"));
  toolBarHideAct_->setText(tr("Hide Toolbar"));

  styleMenu_->setTitle(tr("Application Style"));
  systemStyle_->setText(tr("System"));
  system2Style_->setText(tr("System2"));
  greenStyle_->setText(tr("Green"));
  orangeStyle_->setText(tr("Orange"));
  purpleStyle_->setText(tr("Purple"));
  pinkStyle_->setText(tr("Pink"));
  grayStyle_->setText(tr("Gray"));

  browserPositionMenu_->setTitle(tr("Browser Position"));
  topBrowserPositionAct_->setText(tr("Top"));
  bottomBrowserPositionAct_->setText(tr("Bottom"));
  rightBrowserPositionAct_->setText(tr("Right"));
  leftBrowserPositionAct_->setText(tr("Left"));

  showWindowAct_->setText(tr("Show Window"));

  feedKeyUpAct_->setText(tr("Previous Feed"));
  feedKeyDownAct_->setText(tr("Next Feed"));
  newsKeyUpAct_->setText(tr("Previous News"));
  newsKeyDownAct_->setText(tr("Next News"));
  newsKeyPageUpAct_->setText(tr("News Page Up)"));
  newsKeyPageDownAct_->setText(tr("News Page Down)"));

  nextUnreadNewsAct_->setText(tr("Next Unread News"));
  prevUnreadNewsAct_->setText(tr("Previous Unread News"));

  switchFocusAct_->setText(tr("Switch Focus to Next Panel"));
  switchFocusAct_->setToolTip(
        tr("Switch Focus to Next Panel (Tree Feeds, List News, Browser)"));
  switchFocusPrevAct_->setText(tr("Switch Focus to Previous Panel"));
  switchFocusPrevAct_->setToolTip(
        tr("Switch Focus to Previous Panel (Tree Feeds, Browser, List News)"));

  feedsWidgetVisibleAct_->setText(tr("Show/Hide Tree Feeds"));

  placeToTrayAct_->setText(tr("Minimize to Tray"));
  placeToTrayAct_->setToolTip(
        tr("Minimize Application to Tray"));

  feedsColumnsMenu_->setTitle(tr("Columns"));
  showUnreadCount_->setText(tr("Count News Unread"));
  showUndeleteCount_->setText(tr("Count News All"));
  showLastUpdated_->setText(tr("Last Updated"));

  indentationFeedsTreeAct_->setText(tr("Show Indentation"));

  findFeedAct_->setText(tr("Search Feed"));
  findFeedAct_->setToolTip(tr("Search Feed"));

  browserZoomMenu_->setTitle(tr("Zoom"));
  zoomInAct_->setText(tr("Zoom In"));
  zoomInAct_->setToolTip(tr("Zoom in in browser"));
  zoomOutAct_->setText(tr("Zoom Out"));
  zoomOutAct_->setToolTip(tr("Zoom out in browser"));
  zoomTo100Act_->setText(tr("100%"));
  zoomTo100Act_->setToolTip(tr("Reset zoom in browser"));

  printAct_->setText(tr("Print..."));
  printAct_->setToolTip(tr("Print Web page"));
  printPreviewAct_->setText(tr("Print Preview..."));
  printPreviewAct_->setToolTip(tr("Preview Web page"));

  savePageAsAct_->setText(tr("Save As..."));
  savePageAsAct_->setToolTip(tr("Save Page As..."));

  toolbarsMenu_->setTitle(tr("Show/Hide"));
  mainToolbarToggle_->setText(tr("Main Toolbar"));
  feedsToolbarToggle_->setText(tr("Feeds Toolbar"));
  newsToolbarToggle_->setText(tr("News Toolbar"));
  browserToolbarToggle_->setText(tr("Browser Toolbar"));
  categoriesPanelToggle_->setText(tr("Panel Categories"));

  fullScreenAct_->setText(tr("Full Screen"));
  fullScreenAct_->setToolTip(tr("Full Screen"));

  stayOnTopAct_->setText(tr("Stay On Top"));
  stayOnTopAct_->setToolTip(tr("Stay On Top"));

  categoriesLabel_->setText(tr("Categories"));
  if (categoriesTree_->isHidden())
    showCategoriesButton_->setToolTip(tr("Show Categories"));
  else
    showCategoriesButton_->setToolTip(tr("Hide Categories"));

  newsLabelMenuAction_->setText(tr("Label"));
  newsLabelAction_->setText(tr("Label"));

  closeTabAct_->setText(tr("Close Tab"));
  closeOtherTabsAct_->setText(tr("Close Other Tabs"));
  closeAllTabsAct_->setText(tr("Close All Tabs"));
  nextTabAct_->setText(tr("Switch to next tab"));
  prevTabAct_->setText(tr("Switch to previous tab"));

  categoriesTree_->topLevelItem(0)->setText(0, tr("Unread"));
  categoriesTree_->topLevelItem(1)->setText(0, tr("Starred"));
  categoriesTree_->topLevelItem(2)->setText(0, tr("Deleted"));
  categoriesTree_->topLevelItem(3)->setText(0, tr("Labels"));

  reduceNewsListAct_->setText(tr("Decrease news list/increase browser"));
  increaseNewsListAct_->setText(tr("Increase news list/decrease browser"));

  findTextAct_->setText(tr("Find"));

  openHomeFeedAct_->setText(tr("Open Homepage Feed"));
  sortedByTitleFeedsTreeAct_->setText(tr("Sorted by Name"));
  collapseAllFoldersAct_->setText(tr("Collapse All Folders"));
  expandAllFoldersAct_->setText(tr("Expand All Folders"));
  nextFolderAct_->setText(tr("Next Folder"));
  prevFolderAct_->setText(tr("Previous Folder"));
  expandFolderAct_->setText(tr("Expand Folder"));

  shareMenuAct_->setText(tr("Share"));

  newsSortByMenu_->setTitle(tr("Sort By"));
  newsSortOrderGroup_->actions().at(0)->setText(tr("Ascending"));
  newsSortOrderGroup_->actions().at(1)->setText(tr("Descending"));

  downloadManager_->listClaerAct_->setText(tr("Clear"));

  QApplication::translate("QDialogButtonBox", "Close");
  QApplication::translate("QDialogButtonBox", "Cancel");
  QApplication::translate("QDialogButtonBox", "&Yes");
  QApplication::translate("QDialogButtonBox", "&No");

  QApplication::translate("QLineEdit", "&Undo");
  QApplication::translate("QLineEdit", "&Redo");
  QApplication::translate("QLineEdit", "Cu&t");
  QApplication::translate("QLineEdit", "&Copy");
  QApplication::translate("QLineEdit", "&Paste");
  QApplication::translate("QLineEdit", "Delete");
  QApplication::translate("QLineEdit", "Select All");

  QApplication::translate("QTextControl", "&Undo");
  QApplication::translate("QTextControl", "&Redo");
  QApplication::translate("QTextControl", "Cu&t");
  QApplication::translate("QTextControl", "&Copy");
  QApplication::translate("QTextControl", "&Paste");
  QApplication::translate("QTextControl", "Delete");
  QApplication::translate("QTextControl", "Select All");
  QApplication::translate("QTextControl", "Copy &Link Location");

  QApplication::translate("QAbstractSpinBox", "&Step up");
  QApplication::translate("QAbstractSpinBox", "Step &down");
  QApplication::translate("QAbstractSpinBox", "&Select All");

  QApplication::translate("QMultiInputContext", "Select IM");

  QApplication::translate("QWizard", "Cancel");
  QApplication::translate("QWizard", "< &Back");
  QApplication::translate("QWizard", "&Finish");
  QApplication::translate("QWizard", "&Next >");

  QSqlQuery q;
  q.exec("SELECT id, name FROM labels WHERE id <= 6");
  while (q.next()) {
    int idLabel = q.value(0).toInt();
    QString nameLabel = q.value(1).toString();
    QList<QTreeWidgetItem *> treeItems;
    treeItems = categoriesTree_->findItems(QString::number(idLabel),
                                               Qt::MatchFixedString|Qt::MatchRecursive,
                                               2);
    if (treeItems.count() && (nameLabels().at(idLabel-1) == nameLabel)) {
      treeItems.at(0)->setText(0, trNameLabels().at(idLabel-1));
      for (int i = 0; i < newsLabelGroup_->actions().count(); i++) {
        if (newsLabelGroup_->actions().at(i)->data().toInt() == idLabel) {
          newsLabelGroup_->actions().at(i)->setText(trNameLabels().at(idLabel-1));
          newsLabelGroup_->actions().at(i)->setToolTip(trNameLabels().at(idLabel-1));
          break;
        }
      }
      for (int i = 0; i < stackedWidget_->count(); i++) {
        NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
        if (widget->type_ == TAB_CAT_LABEL) {
          if (widget->labelId_ == idLabel) {
            widget->setTextTab(trNameLabels().at(idLabel-1));
          }
        }
      }
    }
  }

  if (newsView_) {
    currentNewsTab->retranslateStrings();
  }
  findFeeds_->retranslateStrings();
}
// ----------------------------------------------------------------------------
void RSSListing::setToolBarStyle(const QString &styleStr)
{
  if (mainToolbar_->widgetForAction(addAct_))
    mainToolbar_->widgetForAction(addAct_)->setMinimumWidth(10);
  if (styleStr == "toolBarStyleI_") {
    mainToolbar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
  } else if (styleStr == "toolBarStyleT_") {
    mainToolbar_->setToolButtonStyle(Qt::ToolButtonTextOnly);
  } else if (styleStr == "toolBarStyleTbI_") {
    mainToolbar_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  } else {
    mainToolbar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    if (mainToolbar_->widgetForAction(addAct_))
      mainToolbar_->widgetForAction(addAct_)->setMinimumWidth(60);
  }
}
// ----------------------------------------------------------------------------
void RSSListing::setToolBarIconSize(const QString &iconSizeStr)
{
  if (iconSizeStr == "toolBarIconBig_") {
    mainToolbar_->setIconSize(QSize(32, 32));
  } else if (iconSizeStr == "toolBarIconSmall_") {
    mainToolbar_->setIconSize(QSize(16, 16));
  } else {
    mainToolbar_->setIconSize(QSize(24, 24));
  }
}

/** @brief Call toolbar context menu
 *----------------------------------------------------------------------------*/
void RSSListing::showContextMenuToolBar(const QPoint &pos)
{
  QMenu menu;
  menu.addAction(customizeMainToolbarAct2_);
  menu.addSeparator();
  menu.addAction(toolBarLockAct_);
  menu.addAction(toolBarHideAct_);

  menu.exec(mainToolbar_->mapToGlobal(pos));
}
// ----------------------------------------------------------------------------
void RSSListing::lockMainToolbar(bool lock)
{
  mainToolbar_->setMovable(!lock);
}
// ----------------------------------------------------------------------------
void RSSListing::hideMainToolbar()
{
  mainToolbarToggle_->setChecked(false);
  mainToolbar_->hide();
}
// ----------------------------------------------------------------------------
void RSSListing::showFeedPropertiesDlg()
{
  if (!feedsTreeView_->selectIndex().isValid()) {
    feedProperties_->setEnabled(false);
    return;
  }

  QPersistentModelIndex index = feedsTreeView_->selectIndex();
  int feedId = feedsTreeModel_->getIdByIndex(index);
  bool isFeed = (index.isValid() && feedsTreeModel_->isFolder(index)) ? false : true;

  FeedPropertiesDialog *feedPropertiesDialog = new FeedPropertiesDialog(isFeed, this);

  FEED_PROPERTIES properties;
  FEED_PROPERTIES properties_tmp;

  QByteArray byteArray = feedsTreeModel_->dataField(index, "image").toByteArray();
  if (!byteArray.isNull()) {
    QPixmap icon;
    icon.loadFromData(QByteArray::fromBase64(byteArray));
    feedPropertiesDialog->setWindowIcon(icon);
  } else if (isFeed) {
    feedPropertiesDialog->setWindowIcon(QPixmap(":/images/feed"));
  } else {
    feedPropertiesDialog->setWindowIcon(QPixmap(":/images/folder"));
  }
  properties.general.image = QByteArray::fromBase64(byteArray);

  QString str(feedPropertiesDialog->windowTitle() +
              " '" +
              feedsTreeModel_->dataField(index, "text").toString() +
              "'");
  feedPropertiesDialog->setWindowTitle(str);

  properties.general.text =
      feedsTreeModel_->dataField(index, "text").toString();
  properties.general.title =
      feedsTreeModel_->dataField(index, "title").toString();
  properties.general.url =
      feedsTreeModel_->dataField(index, "xmlUrl").toString();
  properties.general.homepage =
      feedsTreeModel_->dataField(index, "htmlUrl").toString();
  properties.general.displayOnStartup =
      feedsTreeModel_->dataField(index, "displayOnStartup").toInt();
  properties.display.displayEmbeddedImages =
      feedsTreeModel_->dataField(index, "displayEmbeddedImages").toInt();
  if (feedsTreeModel_->dataField(index, "displayNews").toString().isEmpty())
    properties.display.displayNews = !showDescriptionNews_;
  else
    properties.display.displayNews =
        feedsTreeModel_->dataField(index, "displayNews").toInt();

  if (feedsTreeModel_->dataField(index, "updateIntervalEnable").isNull() ||
      (feedsTreeModel_->dataField(index, "updateIntervalEnable").toInt() == -1)) {
    properties.general.updateEnable = updateFeedsEnable_;
    properties.general.updateInterval = updateFeedsInterval_;
    properties.general.intervalType = updateFeedsIntervalType_;
  } else {
    properties.general.updateEnable =
        feedsTreeModel_->dataField(index, "updateIntervalEnable").toBool();
    properties.general.updateInterval =
        feedsTreeModel_->dataField(index, "updateInterval").toInt();
    properties.general.intervalType =
        feedsTreeModel_->dataField(index, "updateIntervalType").toInt();
  }

  if (feedsTreeModel_->dataField(index, "label").toString().contains("starred"))
    properties.general.starred = true;
  else
    properties.general.starred = false;

  properties.general.duplicateNewsMode =
      feedsTreeModel_->dataField(index, "duplicateNewsMode").toBool();

  settings_->beginGroup("NewsHeader");
  QString indexColumnsStr = settings_->value("columns").toString();
  QStringList indexColumnsList = indexColumnsStr.split(",", QString::SkipEmptyParts);
  foreach (QString indexStr, indexColumnsList) {
    properties.columnDefault.columns.append(indexStr.toInt());
  }
  NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);
  int sortBy = settings_->value("sortBy", widget->newsModel_->fieldIndex("published")).toInt();
  properties.columnDefault.sortBy = sortBy;
  int sortType = settings_->value("sortOrder", Qt::DescendingOrder).toInt();
  properties.columnDefault.sortType = sortType;
  settings_->endGroup();

  if (feedsTreeModel_->dataField(index, "columns").toString().isEmpty()) {
    widget = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);
    QListIterator<QAction *> iter(widget->newsHeader_->viewMenu_->actions());
    while (iter.hasNext()) {
      QAction *nextAction = iter.next();
      properties.column.indexList.append(nextAction->data().toInt());
      properties.column.nameList.append(nextAction->text());
      if (nextAction->isChecked())
        properties.column.columns.append(nextAction->data().toInt());
    }
    int section = widget->newsHeader_->sortIndicatorSection();
    properties.column.sortBy = section;
    if (widget->newsHeader_->sortIndicatorOrder() == Qt::AscendingOrder) {
      properties.column.sortType = 0;
    } else {
      properties.column.sortType = 1;
    }
  } else {
    widget = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);
    QListIterator<QAction *> iter(widget->newsHeader_->viewMenu_->actions());
    while (iter.hasNext()) {
      QAction *nextAction = iter.next();
      properties.column.indexList.append(nextAction->data().toInt());
      properties.column.nameList.append(nextAction->text());
    }
    indexColumnsStr = feedsTreeModel_->dataField(index, "columns").toString();
    indexColumnsList = indexColumnsStr.split(",", QString::SkipEmptyParts);
    foreach (QString indexStr, indexColumnsList) {
      properties.column.columns.append(indexStr.toInt());
    }
    properties.column.sortBy = feedsTreeModel_->dataField(index, "sort").toInt();
    properties.column.sortType = feedsTreeModel_->dataField(index, "sortType").toInt();
  }

  properties.authentication.on = false;
  if (feedsTreeModel_->dataField(index, "authentication").toInt() == 1) {
    properties.authentication.on = true;
  }
  QUrl url(feedsTreeModel_->dataField(index, "xmlUrl").toString());
  QSqlQuery q;
  q.prepare("SELECT username, password FROM passwords WHERE server=?");
  q.addBindValue(url.host());
  q.exec();
  if (q.next()) {
    properties.authentication.user = q.value(0).toString();
    properties.authentication.pass = QString::fromUtf8(QByteArray::fromBase64(q.value(1).toByteArray()));
  }

  QDateTime dtLocalTime = QDateTime::currentDateTime();
  QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
  int nTimeShift = dtLocalTime.secsTo(dtUTC);

  QDateTime dt = QDateTime::fromString(
        feedsTreeModel_->dataField(index, "created").toString(),
        Qt::ISODate);
  properties.status.createdTime = dt.addSecs(nTimeShift);

  dt = QDateTime::fromString(
        feedsTreeModel_->dataField(index, "updated").toString(),
        Qt::ISODate);
  properties.status.lastUpdate = dt.addSecs(nTimeShift);

  dt = QDateTime::fromString(
        feedsTreeModel_->dataField(index, "lastBuildDate").toString(),
        Qt::ISODate);
  properties.status.lastBuildDate = dt.addSecs(nTimeShift);

  properties.status.undeleteCount = feedsTreeModel_->dataField(index, "undeleteCount").toInt();
  properties.status.newCount      = feedsTreeModel_->dataField(index, "newCount").toInt();
  properties.status.unreadCount   = feedsTreeModel_->dataField(index, "unread").toInt();
  properties.status.description   = feedsTreeModel_->dataField(index, "description").toString();

  feedPropertiesDialog->setFeedProperties(properties);
  properties_tmp = properties;

  int result = feedPropertiesDialog->exec();
  if (result == QDialog::Rejected) {
    delete feedPropertiesDialog;
    return;
  }

  properties = feedPropertiesDialog->getFeedProperties();
  delete feedPropertiesDialog;

  index = feedsTreeModel_->getIndexById(feedId);

  q.prepare("UPDATE feeds SET text = ?, xmlUrl = ?, displayOnStartup = ?, "
            "displayEmbeddedImages = ?, displayNews = ?, label = ?, "
            "duplicateNewsMode = ?, authentication = ? WHERE id == ?");
  q.addBindValue(properties.general.text);
  q.addBindValue(properties.general.url);
  q.addBindValue(properties.general.displayOnStartup);
  q.addBindValue(properties.display.displayEmbeddedImages);
  q.addBindValue(properties.display.displayNews);
  if (properties.general.starred)
    q.addBindValue("starred");
  else
    q.addBindValue("");
  q.addBindValue(properties.general.duplicateNewsMode ? 1 : 0);
  q.addBindValue(properties.authentication.on ? 1 : 0);
  q.addBindValue(feedId);
  q.exec();


  indexColumnsStr = "";
  if ((properties.column.columns != properties.columnDefault.columns) ||
      (properties.column.sortBy != properties.columnDefault.sortBy) ||
      (properties.column.sortType != properties.columnDefault.sortType)) {
    for (int i = 0; i < properties.column.columns.count(); ++i) {
      int index = properties.column.columns.at(i);
      indexColumnsStr.append(",");
      indexColumnsStr.append(QString::number(index));
    }
    indexColumnsStr.append(",");
  } else {
    properties.column.sortBy = 0;
    properties.column.sortType = 0;
  }

  q.prepare("UPDATE feeds SET columns = ?, sort = ?, sortType = ? WHERE id == ?");
  q.addBindValue(indexColumnsStr);
  q.addBindValue(properties.column.sortBy);
  q.addBindValue(properties.column.sortType);
  q.addBindValue(feedId);
  q.exec();

  QModelIndex indexColumns = feedsTreeModel_->indexSibling(index, "columns");
  QModelIndex indexSort = feedsTreeModel_->indexSibling(index, "sort");
  QModelIndex indexSortType = feedsTreeModel_->indexSibling(index, "sortType");
  feedsTreeModel_->setData(indexColumns, indexColumnsStr);
  feedsTreeModel_->setData(indexSort, properties.column.sortBy);
  feedsTreeModel_->setData(indexSortType, properties.column.sortType);

  if (!isFeed) {
    QQueue<int> parentIds;
    parentIds.enqueue(feedId);
    while (!parentIds.empty()) {
      int parentId = parentIds.dequeue();
      q.exec(QString("SELECT id, xmlUrl FROM feeds WHERE parentId='%1'").
             arg(parentId));
      while (q.next()) {
        int id = q.value(0).toInt();
        QString xmlUrl = q.value(1).toString();

        QSqlQuery q1;
        q1.prepare("UPDATE feeds SET columns = ?, sort = ?, sortType = ? WHERE id == ?");
        q1.addBindValue(indexColumnsStr);
        q1.addBindValue(properties.column.sortBy);
        q1.addBindValue(properties.column.sortType);
        q1.addBindValue(id);
        q1.exec();

        QPersistentModelIndex index1 = feedsTreeModel_->getIndexById(id);
        indexColumns = feedsTreeModel_->indexSibling(index1, "columns");
        indexSort = feedsTreeModel_->indexSibling(index1, "sort");
        indexSortType = feedsTreeModel_->indexSibling(index1, "sortType");
        feedsTreeModel_->setData(indexColumns, indexColumnsStr);
        feedsTreeModel_->setData(indexSort, properties.column.sortBy);
        feedsTreeModel_->setData(indexSortType, properties.column.sortType);

        if (currentNewsTab->feedId_ == id)
          currentNewsTab->newsHeader_->setColumns(this, index1);

        if (xmlUrl.isEmpty())
          parentIds.enqueue(id);
      }
    }
  }

  if (currentNewsTab->feedId_ == feedId)
    currentNewsTab->newsHeader_->setColumns(this, index);


  if (!(!feedsTreeModel_->dataField(index, "authentication").toInt() && !properties.authentication.on)) {
    q.prepare("SELECT * FROM passwords WHERE server=?");
    q.addBindValue(url.host());
    q.exec();
    if (q.next()) {
      q.prepare("UPDATE passwords SET username = ?, password = ? WHERE server=?");
      q.addBindValue(properties.authentication.user);
      q.addBindValue(properties.authentication.pass.toUtf8().toBase64());
      q.addBindValue(url.host());
      q.exec();
    } else if (properties.authentication.on) {
      q.prepare("INSERT INTO passwords (server, username, password) "
                "VALUES (:server, :username, :password)");
      q.bindValue(":server", url.host());
      q.bindValue(":username", properties.authentication.user);
      q.bindValue(":password", properties.authentication.pass.toUtf8().toBase64());
      q.exec();
    }
  }

  QPersistentModelIndex indexText    = feedsTreeModel_->indexSibling(index, "text");
  QPersistentModelIndex indexUrl     = feedsTreeModel_->indexSibling(index, "xmlUrl");
  QPersistentModelIndex indexStartup = feedsTreeModel_->indexSibling(index, "displayOnStartup");
  QModelIndex indexImages  = feedsTreeModel_->indexSibling(index, "displayEmbeddedImages");
  QModelIndex indexNews    = feedsTreeModel_->indexSibling(index, "displayNews");
  QModelIndex indexLabel   = feedsTreeModel_->indexSibling(index, "label");
  QModelIndex indexDuplicate = feedsTreeModel_->indexSibling(index, "duplicateNewsMode");
  QModelIndex indexAuthentication = feedsTreeModel_->indexSibling(index, "authentication");
  feedsTreeModel_->setData(indexText, properties.general.text);
  feedsTreeModel_->setData(indexUrl, properties.general.url);
  feedsTreeModel_->setData(indexStartup, properties.general.displayOnStartup);
  feedsTreeModel_->setData(indexImages, properties.display.displayEmbeddedImages);
  feedsTreeModel_->setData(indexNews, properties.display.displayNews);
  feedsTreeModel_->setData(indexLabel, properties.general.starred ? "starred" : "");
  feedsTreeModel_->setData(indexDuplicate, properties.general.duplicateNewsMode ? 1 : 0);
  feedsTreeModel_->setData(indexAuthentication, properties.authentication.on ? 1 : 0);

  if (!properties.general.updateEnable ||
      (properties.general.updateEnable != updateFeedsEnable_) ||
      (properties.general.updateInterval != updateFeedsInterval_) ||
      (properties.general.intervalType != updateFeedsIntervalType_)) {
    q.prepare("UPDATE feeds SET updateIntervalEnable = ?, updateInterval = ?, "
              "updateIntervalType = ? WHERE id == ?");
    q.addBindValue(properties.general.updateEnable ? 1 : 0);
    q.addBindValue(properties.general.updateInterval);
    q.addBindValue(properties.general.intervalType);
    q.addBindValue(feedId);
    q.exec();

    QPersistentModelIndex indexUpdateEnable   = feedsTreeModel_->indexSibling(index, "updateIntervalEnable");
    QPersistentModelIndex indexUpdateInterval = feedsTreeModel_->indexSibling(index, "updateInterval");
    QPersistentModelIndex indexIntervalType   = feedsTreeModel_->indexSibling(index, "updateIntervalType");
    feedsTreeModel_->setData(indexUpdateEnable, properties.general.updateEnable ? 1 : 0);
    feedsTreeModel_->setData(indexUpdateInterval, properties.general.updateInterval);
    feedsTreeModel_->setData(indexIntervalType, properties.general.intervalType);

    int updateInterval = properties.general.updateInterval;
    int updateIntervalType = properties.general.intervalType;
    if (updateIntervalType == 0)
      updateInterval = updateInterval*60;
    else if (updateIntervalType == 1)
      updateInterval = updateInterval*60*60;

    if (!isFeed) {
      QQueue<int> parentIds;
      parentIds.enqueue(feedId);
      while (!parentIds.empty()) {
        int parentId = parentIds.dequeue();
        q.exec(QString("SELECT id, xmlUrl FROM feeds WHERE parentId='%1'").
               arg(parentId));
        while (q.next()) {
          int id = q.value(0).toInt();
          QString xmlUrl = q.value(1).toString();

          QSqlQuery q1;
          q1.prepare("UPDATE feeds SET updateIntervalEnable = ?, updateInterval = ?, "
                     "updateIntervalType = ? WHERE id == ?");
          q1.addBindValue(properties.general.updateEnable ? 1 : 0);
          q1.addBindValue(properties.general.updateInterval);
          q1.addBindValue(properties.general.intervalType);
          q1.addBindValue(id);
          q1.exec();

          QPersistentModelIndex index1 = feedsTreeModel_->getIndexById(id);
          indexUpdateEnable   = feedsTreeModel_->indexSibling(index1, "updateIntervalEnable");
          indexUpdateInterval = feedsTreeModel_->indexSibling(index1, "updateInterval");
          indexIntervalType   = feedsTreeModel_->indexSibling(index1, "updateIntervalType");
          feedsTreeModel_->setData(indexUpdateEnable, properties.general.updateEnable ? 1 : 0);
          feedsTreeModel_->setData(indexUpdateInterval, properties.general.updateInterval);
          feedsTreeModel_->setData(indexIntervalType, properties.general.intervalType);

          if (!q.value(2).toString().isEmpty()) {
            if (properties.general.updateEnable) {
              updateFeedsIntervalSec_.insert(id, updateInterval);
              updateFeedsTimeCount_.insert(id, 0);
            } else {
              updateFeedsIntervalSec_.remove(id);
              updateFeedsTimeCount_.remove(id);
            }
          }

          if (xmlUrl.isEmpty())
            parentIds.enqueue(id);
        }
      }
    } else {
      if (properties.general.updateEnable) {
        updateFeedsIntervalSec_.insert(feedId, updateInterval);
        updateFeedsTimeCount_.insert(feedId, 0);
      } else {
        updateFeedsIntervalSec_.remove(feedId);
        updateFeedsTimeCount_.remove(feedId);
      }
    }
  } else {
    q.prepare("UPDATE feeds SET updateIntervalEnable = -1 WHERE id == ?");
    q.addBindValue(feedId);
    q.exec();

    QPersistentModelIndex indexUpdateEnable = feedsTreeModel_->indexSibling(index, "updateIntervalEnable");
    feedsTreeModel_->setData(indexUpdateEnable, "-1");

    updateFeedsIntervalSec_.remove(feedId);
    updateFeedsTimeCount_.remove(feedId);
  }

  if (properties.general.image != properties_tmp.general.image) {
    q.prepare("UPDATE feeds SET image = ? WHERE id == ?");
    q.addBindValue(properties.general.image.toBase64());
    q.addBindValue(feedId);
    q.exec();
    slotIconFeedUpdate(feedId, properties.general.image);
  }

  if (properties.general.text != properties_tmp.general.text) {
    for (int i = 0; i < stackedWidget_->count(); i++) {
      NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
      if (widget->feedId_ == feedId) {
        widget->setTextTab(properties.general.text);
      }
    }
  }
}

/** @brief Update tray information: icon and tooltip text
 *---------------------------------------------------------------------------*/
void RSSListing::slotRefreshInfoTray()
{
  if (!showTrayIcon_) return;

  // Calculate new and unread news number
  int newCount = 0;
  int unreadCount = 0;
  QSqlQuery q;
  q.exec("SELECT sum(newCount), sum(unread) FROM feeds WHERE xmlUrl!=''");
  if (q.next()) {
    newCount    = q.value(0).toInt();
    unreadCount = q.value(1).toInt();
  }

  if (!unreadCount)
    categoriesTree_->topLevelItem(0)->setText(4, "");
  else
    categoriesTree_->topLevelItem(0)->setText(4, QString("(%1)").arg(unreadCount));

  // Setting tooltip text
  QString info =
      "QuiteRSS\n" +
      QString(tr("New News: %1")).arg(newCount) +
      QString("\n") +
      QString(tr("Unread News: %1")).arg(unreadCount);
  traySystem->setToolTip(info);

  // Display new number or unread number of news
  if (behaviorIconTray_ > CHANGE_ICON_TRAY) {
    int trayCount = (behaviorIconTray_ == UNREAD_COUNT_ICON_TRAY) ? unreadCount : newCount;
    // Display icon with number
    if (trayCount != 0) {
      // Prepare number
      QString trayCountStr;
      QFont font("Consolas");
      if (trayCount > 99) {
        font.setBold(false);
        if (trayCount < 1000) {
          font.setPixelSize(8);
          trayCountStr = QString::number(trayCount);
        } else {
          font.setPixelSize(11);
          trayCountStr = "#";
        }
      } else {
        font.setBold(true);
        font.setPixelSize(11);
        trayCountStr = QString::number(trayCount);
      }

      // Draw icon, text above it, and set this icon to tray icon
      QPixmap icon = QPixmap(":/images/countNew");
      QPainter trayPainter;
      trayPainter.begin(&icon);
      trayPainter.setFont(font);
      trayPainter.setPen(Qt::white);
      trayPainter.drawText(QRect(1, 0, 15, 16), Qt::AlignVCenter | Qt::AlignHCenter,
                           trayCountStr);
      trayPainter.end();
      traySystem->setIcon(icon);
    }
    // Draw icon without number
    else {
#if defined(QT_NO_DEBUG_OUTPUT)
      traySystem->setIcon(QIcon(":/images/quiterss16"));
#else
      traySystem->setIcon(QIcon(":/images/quiterssDebug"));
#endif
    }
  }
}

/** @brief Mark all feeds read
 *---------------------------------------------------------------------------*/
void RSSListing::markAllFeedsRead()
{
  QSqlQuery q;

  q.exec("UPDATE news SET read=2 WHERE read!=2 AND deleted==0");
  q.exec("UPDATE news SET new=0 WHERE new==1 AND deleted==0");

  q.exec("SELECT id FROM feeds WHERE unread!=0");
  while (q.next()) {
    qApp->processEvents();
    recountFeedCounts(q.value(0).toInt());
  }
  recountCategoryCounts();

  if (tabBar_->currentIndex() == TAB_WIDGET_PERMANENT) {
    QModelIndex index =
        feedsTreeModel_->index(-1, feedsTreeView_->columnIndex("text"));
    feedsTreeView_->setCurrentIndex(index);
    slotFeedClicked(index);
  } else {
    int currentRow = newsView_->currentIndex().row();

    newsModel_->select();

    while (newsModel_->canFetchMore())
      newsModel_->fetchMore();

    newsView_->setCurrentIndex(newsModel_->index(currentRow, newsModel_->fieldIndex("title")));
  }

  emit signalRefreshInfoTray();
}

/** @brief Mark all feeds Not New
 *---------------------------------------------------------------------------*/
void RSSListing::markAllFeedsOld()
{
  QSqlQuery q;
  q.exec("UPDATE news SET new=0 WHERE new==1 AND deleted==0");

  q.exec("SELECT id FROM feeds WHERE newCount!=0");
  while (q.next()) {
    qApp->processEvents();
    recountFeedCounts(q.value(0).toInt());
  }
  recountCategoryCounts();

  if (currentNewsTab != NULL) {
    int currentRow = newsView_->currentIndex().row();
    setNewsFilter(newsFilterGroup_->checkedAction(), false);
    newsView_->setCurrentIndex(newsModel_->index(currentRow, newsModel_->fieldIndex("title")));
  }

  emit signalRefreshInfoTray();
}

/** @brief Prepare feed icon for storing in DB
 *---------------------------------------------------------------------------*/
void RSSListing::slotIconFeedPreparing(const QString &feedUrl,
                                       const QByteArray &byteArray,
                                       const QString &format)
{
  QPixmap icon;
  if (icon.loadFromData(byteArray)) {
    icon = icon.scaled(16, 16, Qt::IgnoreAspectRatio,
                       Qt::SmoothTransformation);
    QByteArray faviconData;
    QBuffer    buffer(&faviconData);
    buffer.open(QIODevice::WriteOnly);
    if (icon.save(&buffer, "ICO")) {
      emit signalIconFeedReady(feedUrl, faviconData);
    }
  } else if (icon.loadFromData(byteArray, format.toUtf8().data())) {
    icon = icon.scaled(16, 16, Qt::IgnoreAspectRatio,
                       Qt::SmoothTransformation);
    QByteArray faviconData;
    QBuffer    buffer(&faviconData);
    buffer.open(QIODevice::WriteOnly);
    if (icon.save(&buffer, "ICO")) {
      emit signalIconFeedReady(feedUrl, faviconData);
    }
  }
}

/** @brief Update feed icon in model and view
 *---------------------------------------------------------------------------*/
void RSSListing::slotIconFeedUpdate(int feedId, const QByteArray &faviconData)
{
  QModelIndex index = feedsTreeModel_->getIndexById(feedId);
  if (index.isValid()) {
    QModelIndex indexImage = feedsTreeModel_->indexSibling(index, "image");
    feedsTreeModel_->setData(indexImage, faviconData.toBase64());
    feedsTreeView_->viewport()->update();
  }

  if (defaultIconFeeds_) return;

  for (int i = 0; i < stackedWidget_->count(); i++) {
    NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
    if (widget->feedId_ == feedId) {
      QPixmap iconTab;
      if (!faviconData.isNull()) {
        iconTab.loadFromData(faviconData);
      } else {
        iconTab.load(":/images/feed");
      }
      widget->newsIconTitle_->setPixmap(iconTab);
    }
  }
  if (currentNewsTab->type_ < TAB_WEB)
    currentNewsTab->newsView_->viewport()->update();
}
// ----------------------------------------------------------------------------
void RSSListing::playSoundNewNews()
{
  if (!playSoundNewNews_ && soundNewNews_) {
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
    QSound::play(soundNotifyPath_);
#else
    QProcess::startDetached(QString("play %1").arg(soundNotifyPath_));
#endif
    playSoundNewNews_ = true;
  }
}
// ----------------------------------------------------------------------------
void RSSListing::showNewsFiltersDlg(bool newFilter)
{
  NewsFiltersDialog *newsFiltersDialog = new NewsFiltersDialog(this, settings_);
  if (newFilter) {
    newsFiltersDialog->filtersTree_->setCurrentItem(
          newsFiltersDialog->filtersTree_->topLevelItem(
            newsFiltersDialog->filtersTree_->topLevelItemCount()-1));
  }

  newsFiltersDialog->exec();

  delete newsFiltersDialog;
}
// ----------------------------------------------------------------------------
void RSSListing::showFilterRulesDlg()
{
  if (!feedsTreeView_->selectIndex().isValid()) return;

  int feedId = feedsTreeView_->selectId_;

  FilterRulesDialog *filterRulesDialog = new FilterRulesDialog(
        this, -1, feedId);

  int result = filterRulesDialog->exec();
  if (result == QDialog::Rejected) {
    delete filterRulesDialog;
    return;
  }

  delete filterRulesDialog;

  showNewsFiltersDlg(true);
}
// ----------------------------------------------------------------------------
void RSSListing::slotUpdateAppCheck()
{
  if (!updateCheckEnabled_) return;

  updateAppDialog_ = new UpdateAppDialog(langFileName_, settings_, this, false);
  connect(updateAppDialog_, SIGNAL(signalNewVersion(QString)),
          this, SLOT(slotNewVersion(QString)), Qt::QueuedConnection);
}
// ----------------------------------------------------------------------------
void RSSListing::slotNewVersion(const QString &newVersion)
{
  delete updateAppDialog_;

  if (!newVersion.isEmpty()) {
    traySystem->showMessage(tr("Check for updates"),
                            tr("A new version of QuiteRSS..."));
    connect(traySystem, SIGNAL(messageClicked()),
            this, SLOT(slotShowUpdateAppDlg()));
  }
}

/** @brief Process Key_Up in feeds tree
 *---------------------------------------------------------------------------*/
void RSSListing::slotFeedUpPressed()
{
  QModelIndex indexBefore = feedsTreeView_->currentIndex();
  QModelIndex indexAfter;

  // Jump to bottom in case of the most top index
  if (!indexBefore.isValid())
    indexAfter = feedsTreeModel_->index(feedsTreeModel_->rowCount()-1, feedsTreeView_->columnIndex("text"));
  else
    indexAfter = feedsTreeView_->indexAbove(indexBefore);

  // There is no "upper" index
  if (!indexAfter.isValid()) return;

  feedsTreeView_->setCurrentIndex(indexAfter);
  slotFeedClicked(indexAfter);
}

/** @brief Process Key_Down in feeds tree
 *---------------------------------------------------------------------------*/
void RSSListing::slotFeedDownPressed()
{
  QModelIndex indexBefore = feedsTreeView_->currentIndex();
  QModelIndex indexAfter;

  // Jump to top in case of the most bottom index
  if (!indexBefore.isValid())
    indexAfter = feedsTreeModel_->index(0, feedsTreeView_->columnIndex("text"));
  else
    indexAfter = feedsTreeView_->indexBelow(indexBefore);

  // There is no "downer" index
  if (!indexAfter.isValid()) return;

  feedsTreeView_->setCurrentIndex(indexAfter);
  slotFeedClicked(indexAfter);
}

/** @brief Process previous feed shortcut
 *---------------------------------------------------------------------------*/
void RSSListing::slotFeedPrevious()
{
  QModelIndex indexBefore = feedsTreeView_->currentIndex();
  QModelIndex indexAfter;

  // Jump to bottom in case of the most top index
  if (!indexBefore.isValid())
    indexAfter = feedsTreeModel_->index(feedsTreeModel_->rowCount()-1, feedsTreeView_->columnIndex("text"));
  else
    indexAfter = feedsTreeView_->indexPrevious(indexBefore);

  // There is no "upper" index
  if (!indexAfter.isValid()) return;

  feedsTreeView_->setCurrentIndex(indexAfter);
  slotFeedClicked(indexAfter);
}

/** @brief Process next feed shortcut
 *---------------------------------------------------------------------------*/
void RSSListing::slotFeedNext()
{
  QModelIndex indexBefore = feedsTreeView_->currentIndex();
  QModelIndex indexAfter;

  // Jump to top in case of the most bottom index
  if (!indexBefore.isValid())
    indexAfter = feedsTreeModel_->index(0, feedsTreeView_->columnIndex("text"));
  else
    indexAfter = feedsTreeView_->indexNext(indexBefore);

  // There is no "downer" index
  if (!indexAfter.isValid()) return;

  feedsTreeView_->setCurrentIndex(indexAfter);
  slotFeedClicked(indexAfter);
}

/** @brief Process Key_Home in feeds tree
 *---------------------------------------------------------------------------*/
void RSSListing::slotFeedHomePressed()
{
  QModelIndex index = feedsTreeModel_->index(
        0, feedsTreeView_->columnIndex("text"));
  feedsTreeView_->setCurrentIndex(index);
  slotFeedClicked(index);
}

/** @brief Process Key_End in feeds tree
 *---------------------------------------------------------------------------*/
void RSSListing::slotFeedEndPressed()
{
  QModelIndex index = feedsTreeModel_->index(
        feedsTreeModel_->rowCount()-1, feedsTreeView_->columnIndex("text"));
  feedsTreeView_->setCurrentIndex(index);
  slotFeedClicked(index);
}

/** @brief Set application style
 *---------------------------------------------------------------------------*/
void RSSListing::setStyleApp(QAction *pAct)
{
  QString fileString(appDataDirPath_);
  if (pAct->objectName() == "systemStyle_") {
    fileString.append("/style/system.qss");
  } else if (pAct->objectName() == "system2Style_") {
    fileString.append("/style/system2.qss");
  } else if (pAct->objectName() == "orangeStyle_") {
    fileString.append("/style/orange.qss");
  } else if (pAct->objectName() == "purpleStyle_") {
    fileString.append("/style/purple.qss");
  } else if (pAct->objectName() == "pinkStyle_") {
    fileString.append("/style/pink.qss");
  } else if (pAct->objectName() == "grayStyle_") {
    fileString.append("/style/gray.qss");
  } else {
    fileString.append("/style/green.qss");
  }

  QFile file(fileString);
  if (!file.open(QFile::ReadOnly)) {
    file.setFileName(":/style/systemStyle");
    file.open(QFile::ReadOnly);
  }
  qApp->setStyleSheet(QLatin1String(file.readAll()));
  file.close();

  mainSplitter_->setStyleSheet(
        QString("QSplitter::handle {background: qlineargradient("
                "x1: 0, y1: 0, x2: 0, y2: 1,"
                "stop: 0 %1, stop: 0.07 %2);}").
        arg(feedsPanel_->palette().background().color().name()).
        arg(qApp->palette().color(QPalette::Dark).name()));
}

/** Switch focus forward between feed tree, news list and browser
 *---------------------------------------------------------------------------*/
void RSSListing::slotSwitchFocus()
{
  if (feedsTreeView_->hasFocus()) {
    newsView_->setFocus();
  } else if (newsView_->hasFocus()) {
    currentNewsTab->webView_->setFocus();
  } else {
    feedsTreeView_->setFocus();
  }
}

/** @brief Switch focus backward between feed tree, news list and browser
 *---------------------------------------------------------------------------*/
void RSSListing::slotSwitchPrevFocus()
{
  if (feedsTreeView_->hasFocus()) {
    currentNewsTab->webView_->setFocus();
  } else if (currentNewsTab->webView_->hasFocus()) {
    newsView_->setFocus();
  } else {
    feedsTreeView_->setFocus();
  }
}

/** @brief Open feed in a new tab
 *---------------------------------------------------------------------------*/
void RSSListing::slotOpenFeedNewTab()
{
  if (stackedWidget_->count() && currentNewsTab->type_ < TAB_WEB) {
    setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_, FeedReadSwitchingTab, currentNewsTab);
    currentNewsTab->newsHeader_->saveStateColumns(this, currentNewsTab);
    settings_->setValue("NewsTabSplitterState",
                        currentNewsTab->newsTabWidgetSplitter_->saveState());
  }

  feedsTreeView_->selectIdEn_ = false;
  feedsTreeView_->setCurrentIndex(feedsTreeView_->selectIndex());
  slotFeedSelected(feedsTreeView_->selectIndex(), true);
}

void RSSListing::slotOpenCategoryNewTab()
{
  slotCategoriesClicked(categoriesTree_->currentItem(), 0, true);
}

/** @brief Close tab with \a index
 *---------------------------------------------------------------------------*/
void RSSListing::slotCloseTab(int index)
{
  if (index != 0) {
    NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(index);

    setFeedRead(widget->type_, widget->feedId_, FeedReadClosingTab, widget);

    stackedWidget_->removeWidget(widget);
    tabBar_->removeTab(index);
    widget->newsTitleLabel_->deleteLater();
    widget->deleteLater();
  }
}

/** @brief Switch to tab with index \a index
 *---------------------------------------------------------------------------*/
void RSSListing::slotTabCurrentChanged(int index)
{
  if (!stackedWidget_->count()) return;
  if (tabBar_->closingTabState_ == TabBar::CloseTabOtherIndex) return;

  NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(index);

  if ((widget->type_ == TAB_FEED) || (widget->type_ >= TAB_WEB))
    categoriesTree_->setCurrentIndex(QModelIndex());
  if (widget->type_ != TAB_FEED) {
    feedsTreeView_->setCurrentIndex(QModelIndex());
    feedProperties_->setEnabled(false);
  }

  if (index == TAB_WIDGET_PERMANENT) {
    feedsWidgetVisibleAct_->setChecked(showFeedsTabPermanent_);
    slotVisibledFeedsWidget();
  } else {
    if (hideFeedsOpenTab_) {
      feedsWidgetVisibleAct_->setChecked(false);
      slotVisibledFeedsWidget();
    }
  }

  stackedWidget_->setCurrentIndex(index);

  if (!updateCurrentTab_) return;

  if ((tabBar_->closingTabState_ == TabBar::CloseTabIdle) && (currentNewsTab->type_ < TAB_WEB)) {
    setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_, FeedReadSwitchingTab, currentNewsTab);

    currentNewsTab->newsHeader_->saveStateColumns(this, currentNewsTab);
    settings_->setValue("NewsTabSplitterState",
                        currentNewsTab->newsTabWidgetSplitter_->saveState());
  }

  if (widget->type_ == TAB_FEED) {
    if (widget->feedId_ == 0)
      widget->hide();
    createNewsTab(index);

    QModelIndex feedIndex = feedsTreeModel_->getIndexById(widget->feedId_);
    feedsTreeView_->setCurrentIndex(feedIndex);
    feedProperties_->setEnabled(feedIndex.isValid());

    setFeedsFilter(feedsFilterGroup_->checkedAction(), false);

    slotUpdateNews();
    if (widget->isVisible())
      newsView_->setFocus();
    else
      feedsTreeView_->setFocus();

    statusUnread_->setVisible(widget->feedId_);
    statusAll_->setVisible(widget->feedId_);
  } else if (widget->type_ == TAB_WEB) {
    statusUnread_->setVisible(false);
    statusAll_->setVisible(false);
    currentNewsTab = widget;
    currentNewsTab->setSettings();
    currentNewsTab->retranslateStrings();
    currentNewsTab->webView_->setFocus();
  } else if (widget->type_ == TAB_DOWNLOADS) {
    statusUnread_->setVisible(false);
    statusAll_->setVisible(false);
    downloadManager_->show();
    currentNewsTab = widget;
    currentNewsTab->retranslateStrings();
  } else {
    QList<QTreeWidgetItem *> treeItems;
    if (widget->type_ == TAB_CAT_LABEL) {
      treeItems = categoriesTree_->findItems(QString::number(widget->labelId_),
                                                 Qt::MatchFixedString|Qt::MatchRecursive,
                                                 2);
    } else {
      treeItems = categoriesTree_->findItems(QString::number(widget->type_),
                                                 Qt::MatchFixedString,
                                                 1);
    }
    categoriesTree_->setCurrentItem(treeItems.at(0));

    createNewsTab(index);
    slotUpdateNews();
    newsView_->setFocus();

    int unreadCount = 0;
    int allCount = widget->newsModel_->rowCount();
    QString countStr = categoriesTree_->currentItem()->text(4);
    if (!countStr.isEmpty()) {
      countStr.remove(QRegExp("[()]"));
      switch (widget->type_) {
      case TAB_CAT_UNREAD:
        unreadCount = countStr.toInt();
        break;
      case TAB_CAT_STAR: case TAB_CAT_LABEL:
        unreadCount = countStr.section("/", 0, 0).toInt();
        break;
      }
    }
    statusUnread_->setText(QString(" " + tr("Unread: %1") + " ").arg(unreadCount));
    statusAll_->setText(QString(" " + tr("All: %1") + " ").arg(allCount));

    statusUnread_->setVisible(widget->type_ != TAB_CAT_DEL);
    statusAll_->setVisible(true);
  }

  setTextTitle(widget->newsTitleLabel_->toolTip(), widget);
}

/** @brief Process tab moving
 *----------------------------------------------------------------------------*/
void RSSListing::slotTabMoved(int fromIndex, int toIndex)
{
  stackedWidget_->insertWidget(toIndex, stackedWidget_->widget(fromIndex));
}

/** @brief Manage displaying columns in feed tree
 *---------------------------------------------------------------------------*/
void RSSListing::feedsColumnVisible(QAction *action)
{
  int idx = action->data().toInt();
  if (action->isChecked())
    feedsTreeView_->showColumn(idx);
  else
    feedsTreeView_->hideColumn(idx);
}

/** @brief Set browser position
 *---------------------------------------------------------------------------*/
void RSSListing::setBrowserPosition(QAction *action)
{
  browserPosition_ = action->data().toInt();
  currentNewsTab->setBrowserPosition();
}

/** @brief Create tab with browser only (without news list)
 *---------------------------------------------------------------------------*/
QWebPage *RSSListing::createWebTab()
{
  NewsTabWidget *widget = new NewsTabWidget(this, TAB_WEB);
  int indexTab = addTab(widget);

  widget->setTextTab(tr("Loading..."));
  widget->setSettings();
  widget->retranslateStrings();

  if (openNewsTab_ == NEW_TAB_FOREGROUND) {
    currentNewsTab = widget;
    emit signalSetCurrentTab(indexTab);
  }

  openNewsTab_ = 0;

  return widget->webView_->page();
}
// ----------------------------------------------------------------------------
void RSSListing::creatFeedTab(int feedId, int feedParId)
{
  QSqlQuery q;
  q.exec(QString("SELECT text, image, currentNews, xmlUrl FROM feeds WHERE id=='%1'").
         arg(feedId));

  if (q.next()) {
    NewsTabWidget *widget = new NewsTabWidget(this, TAB_FEED, feedId, feedParId);
    addTab(widget);
    widget->setSettings();
    widget->retranslateStrings();
    widget->setBrowserPosition();

    bool isFeed = true;
    if (q.value(3).toString().isEmpty())
      isFeed = false;

    // Set icon and title for tab
    QPixmap iconTab;
    QByteArray byteArray = q.value(1).toByteArray();
    if (!isFeed) {
      iconTab.load(":/images/folder");
    } else {
      if (byteArray.isNull() || defaultIconFeeds_) {
        iconTab.load(":/images/feed");
      } else if (isFeed) {
        iconTab.loadFromData(QByteArray::fromBase64(byteArray));
      }
    }
    widget->newsIconTitle_->setPixmap(iconTab);
    widget->setTextTab(q.value(0).toString());

    QString feedIdFilter;
    if (feedsTreeModel_->isFolder(feedsTreeModel_->getIndexById(feedId))) {
      feedIdFilter = QString("(%1) AND ").arg(getIdFeedsString(feedId));
    } else {
      feedIdFilter = QString("feedId=%1 AND ").arg(feedId);
    }

    if (newsFilterGroup_->checkedAction()->objectName() == "filterNewsAll_") {
      feedIdFilter.append("deleted = 0");
    } else if (newsFilterGroup_->checkedAction()->objectName() == "filterNewsNew_") {
      feedIdFilter.append(QString("new = 1 AND deleted = 0"));
    } else if (newsFilterGroup_->checkedAction()->objectName() == "filterNewsUnread_") {
      feedIdFilter.append(QString("read < 2 AND deleted = 0"));
    } else if (newsFilterGroup_->checkedAction()->objectName() == "filterNewsStar_") {
      feedIdFilter.append(QString("starred = 1 AND deleted = 0"));
    } else if (newsFilterGroup_->checkedAction()->objectName() == "filterNewsNotStarred_") {
      feedIdFilter.append(QString("starred = 0 AND deleted = 0"));
    } else if (newsFilterGroup_->checkedAction()->objectName() == "filterNewsUnreadStar_") {
      feedIdFilter.append(QString("(read < 2 OR starred = 1) AND deleted = 0"));
    } else if (newsFilterGroup_->checkedAction()->objectName() == "filterNewsLastDay_") {
      feedIdFilter.append(QString("(published >= datetime('now', '-1 day')) AND deleted = 0"));
    } else if (newsFilterGroup_->checkedAction()->objectName() == "filterNewsLastWeek_") {
      feedIdFilter.append(QString("(published >= datetime('now', '-7 day')) AND deleted = 0"));
    }
    widget->newsModel_->setFilter(feedIdFilter);

    if (widget->newsModel_->rowCount() != 0) {
      while (widget->newsModel_->canFetchMore())
        widget->newsModel_->fetchMore();
    }

    // focus feed has displayed before
    int newsRow = -1;
    int newsId = widget->newsModel_->index(newsRow, widget->newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();
    if (openingFeedAction_ == 0) {
      QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("id"));
      QModelIndexList indexList = newsModel_->match(index, Qt::EditRole, newsId);
      if (indexList.count()) newsRow = indexList.first().row();
    } else if (openingFeedAction_ == 1) newsRow = 0;

    widget->newsView_->setCurrentIndex(widget->newsModel_->index(newsRow, widget->newsModel_->fieldIndex("title")));
    if (newsRow == -1) widget->newsView_->verticalScrollBar()->setValue(newsRow);

    if ((openingFeedAction_ < 2) && openNewsWebViewOn_) {
      widget->slotNewsViewSelected(widget->newsModel_->index(newsRow, widget->newsModel_->fieldIndex("title")));
    } else {
      widget->slotNewsViewSelected(widget->newsModel_->index(-1, widget->newsModel_->fieldIndex("title")));
      QSqlQuery q;
      QString qStr = QString("UPDATE feeds SET currentNews='%1' WHERE id=='%2'").
          arg(newsId).arg(feedId);
      q.exec(qStr);
    }
  }
}

/** @brief Open news using Enter key
 *---------------------------------------------------------------------------*/
void RSSListing::slotOpenNewsWebView()
{
  if (!newsView_->hasFocus()) return;
  currentNewsTab->slotNewsViewSelected(newsView_->currentIndex());
}
// ----------------------------------------------------------------------------
void RSSListing::slotNewsUpPressed()
{
  currentNewsTab->slotNewsUpPressed();
}
// ----------------------------------------------------------------------------
void RSSListing::slotNewsDownPressed()
{
  currentNewsTab->slotNewsDownPressed();
}
// ----------------------------------------------------------------------------
void RSSListing::slotNewsPageUpPressed()
{
  currentNewsTab->slotNewsPageUpPressed();
}
// ----------------------------------------------------------------------------
void RSSListing::slotNewsPageDownPressed()
{
  currentNewsTab->slotNewsPageDownPressed();
}
// ----------------------------------------------------------------------------
void RSSListing::markNewsRead()
{
  currentNewsTab->markNewsRead();
}
// ----------------------------------------------------------------------------
void RSSListing::markAllNewsRead()
{
  currentNewsTab->markAllNewsRead();
}
// ----------------------------------------------------------------------------
void RSSListing::markNewsStar()
{
  currentNewsTab->markNewsStar();
}
// ----------------------------------------------------------------------------
void RSSListing::deleteNews()
{
  currentNewsTab->deleteNews();
}
// ----------------------------------------------------------------------------
void RSSListing::deleteAllNewsList()
{
  currentNewsTab->deleteAllNewsList();
}
// ----------------------------------------------------------------------------
void RSSListing::restoreNews()
{
  currentNewsTab->restoreNews();
}
// ----------------------------------------------------------------------------
void RSSListing::openInBrowserNews()
{
  currentNewsTab->openInBrowserNews();
}
// ----------------------------------------------------------------------------
void RSSListing::openInExternalBrowserNews()
{
  currentNewsTab->openInExternalBrowserNews();
}
// ----------------------------------------------------------------------------
void RSSListing::slotOpenNewsNewTab()
{
  openNewsTab_ = NEW_TAB_FOREGROUND;
  currentNewsTab->openNewsNewTab();
}
// ----------------------------------------------------------------------------
void RSSListing::slotOpenNewsBackgroundTab()
{
  openNewsTab_ = NEW_TAB_BACKGROUND;
  currentNewsTab->openNewsNewTab();
}

/** @brief Copy news URL-link
 *----------------------------------------------------------------------------*/
void RSSListing::slotCopyLinkNews()
{
  currentNewsTab->slotCopyLinkNews();
}

/** @brief Reload full model
 * @details Performs: reload model, reset proxy model, restore focus
 *---------------------------------------------------------------------------*/
void RSSListing::feedsModelReload(bool checkFilter)
{
  if (checkFilter && feedsTreeModel_->filter().isEmpty()) {
    feedsTreeView_->viewport()->update();
#ifdef HAVE_QT5
    feedsTreeView_->header()->setSectionResizeMode(feedsTreeModel_->proxyColumnByOriginal("unread"), QHeaderView::ResizeToContents);
    feedsTreeView_->header()->setSectionResizeMode(feedsTreeModel_->proxyColumnByOriginal("undeleteCount"), QHeaderView::ResizeToContents);
    feedsTreeView_->header()->setSectionResizeMode(feedsTreeModel_->proxyColumnByOriginal("updated"), QHeaderView::ResizeToContents);
#else
    feedsTreeView_->header()->setResizeMode(feedsTreeModel_->proxyColumnByOriginal("unread"), QHeaderView::ResizeToContents);
    feedsTreeView_->header()->setResizeMode(feedsTreeModel_->proxyColumnByOriginal("undeleteCount"), QHeaderView::ResizeToContents);
    feedsTreeView_->header()->setResizeMode(feedsTreeModel_->proxyColumnByOriginal("updated"), QHeaderView::ResizeToContents);
#endif
    return;
  }

  int topRow = feedsTreeView_->verticalScrollBar()->value();

  int feedId = feedsTreeModel_->getIdByIndex(feedsTreeView_->currentIndex());

  feedsTreeModel_->refresh();
  expandNodes();

  QModelIndex feedIndex = feedsTreeModel_->getIndexById(feedId);
  feedsTreeView_->selectIdEn_ = false;
  feedsTreeView_->setCurrentIndex(feedIndex);
  feedsTreeView_->verticalScrollBar()->setValue(topRow);
}
// ----------------------------------------------------------------------------
void RSSListing::setCurrentTab(int index, bool updateCurrentTab)
{
  updateCurrentTab_ = updateCurrentTab;
  tabBar_->setCurrentIndex(index);
  updateCurrentTab_ = true;
}

/** @brief Set focus to search field (CTRL+F)
 *---------------------------------------------------------------------------*/
void RSSListing::findText()
{
  if (currentNewsTab->type_ < TAB_WEB)
    currentNewsTab->findText_->setFocus();
}

/** @brief Show notification on inbox news
 *---------------------------------------------------------------------------*/
void RSSListing::showNotification()
{
  if (idFeedList_.isEmpty() || isActiveWindow() || !showNotifyOn_) {
    idFeedList_.clear();
    cntNewNewsList_.clear();
    return;
  }

  if (fullscreenModeNotify_) {
#if defined(Q_OS_WIN)
    HWND hWnd = GetForegroundWindow();
    RECT appBounds;
    RECT rc;
    GetWindowRect(GetDesktopWindow(), &rc);

    if((hWnd != GetDesktopWindow())
   #ifdef HAVE_QT5
       && (hWnd != GetShellWindow())
   #endif
       ) {
      GetWindowRect(hWnd, &appBounds);
      if ((rc.top == appBounds.top) && (rc.bottom == appBounds.bottom) &&
          (rc.left == appBounds.left) && (rc.right == appBounds.right)) {
        return;
      }
    }
#endif
  }

  if (notificationWidget) delete notificationWidget;
  notificationWidget = new NotificationWidget(idFeedList_, cntNewNewsList_, this);

  idFeedList_.clear();
  cntNewNewsList_.clear();

  connect(notificationWidget, SIGNAL(signalShow()), this, SLOT(slotShowWindows()));
  connect(notificationWidget, SIGNAL(signalDelete()),
          this, SLOT(deleteNotification()));
  connect(notificationWidget, SIGNAL(signalOpenNews(int, int)),
          this, SLOT(slotOpenNew(int, int)));
  connect(notificationWidget, SIGNAL(signalOpenExternalBrowser(QUrl)),
          this, SLOT(slotOpenNewBrowser(QUrl)));
  connect(notificationWidget, SIGNAL(signalMarkRead(int, int, int)),
          this, SLOT(slotMarkReadNewsInNotification(int, int, int)));

  notificationWidget->show();
}

/** @brief Delete notification widget
 *---------------------------------------------------------------------------*/
void RSSListing::deleteNotification()
{
  notificationWidget->deleteLater();
  notificationWidget = NULL;
}

/** @brief Show news on click in notification window
 *---------------------------------------------------------------------------*/
void RSSListing::slotOpenNew(int feedId, int newsId)
{
  deleteNotification();

  openingFeedAction_ = 0;
  openNewsWebViewOn_ = true;

  QSqlQuery q;
  QString qStr = QString("UPDATE feeds SET currentNews='%1' WHERE id=='%2'").arg(newsId).arg(feedId);
  q.exec(qStr);

  QModelIndex feedIndex = feedsTreeModel_->getIndexById(feedId);
  feedsTreeModel_->setData(feedsTreeModel_->indexSibling(feedIndex, "currentNews"),
                           newsId);

  feedsTreeView_->setCurrentIndex(feedIndex);
  slotFeedClicked(feedIndex);

  openingFeedAction_ = settings_->value("/Settings/openingFeedAction", 0).toInt();
  openNewsWebViewOn_ = settings_->value("/Settings/openNewsWebViewOn", true).toBool();
  QModelIndex index = newsView_->currentIndex();
  slotShowWindows();
  newsView_->setCurrentIndex(index);
}

/** @brief Open news in external browser on click in notification window
 *---------------------------------------------------------------------------*/
void RSSListing::slotOpenNewBrowser(const QUrl &url)
{
  currentNewsTab->openUrl(url);
}

void RSSListing::slotMarkReadNewsInNotification(int feedId, int newsId, int read)
{
  QSqlQuery q;
  bool showNews = false;
  if (currentNewsTab->type_ < TAB_WEB) {
    int cnt = newsModel_->rowCount();
    for (int i = 0; i < cnt; ++i) {
      if (newsId == newsModel_->index(i, newsModel_->fieldIndex("id")).data().toInt()) {
        if (read == 1) {
          if (newsModel_->index(i, newsModel_->fieldIndex("new")).data(Qt::EditRole).toInt() == 1) {
            newsModel_->setData(
                  newsModel_->index(i, newsModel_->fieldIndex("new")),
                  0);
            q.exec(QString("UPDATE news SET new=0 WHERE id=='%1'").arg(newsId));
          }
          if (newsModel_->index(i, newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() == 0) {
            newsModel_->setData(
                  newsModel_->index(i, newsModel_->fieldIndex("read")),
                  1);
            q.exec(QString("UPDATE news SET read=1 WHERE id=='%1'").arg(newsId));
          }
        } else {
          if (newsModel_->index(i, newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() != 0) {
            newsModel_->setData(
                  newsModel_->index(i, newsModel_->fieldIndex("read")),
                  0);
            q.exec(QString("UPDATE news SET read=0 WHERE id=='%1'").arg(newsId));
          }
        }

        newsView_->viewport()->update();
        showNews = true;
        break;
      }
    }
  }

  if (!showNews) {
    q.exec(QString("UPDATE news SET new=0, read='%1' WHERE id=='%2'").
           arg(read).arg(newsId));
  }

  slotUpdateStatus(feedId);
  recountCategoryCounts();
}

// ----------------------------------------------------------------------------
void RSSListing::slotFindFeeds(QString)
{
  if (!findFeedsWidget_->isVisible()) return;

  setFeedsFilter(feedsFilterGroup_->checkedAction(), false);
}
// ----------------------------------------------------------------------------
void RSSListing::slotSelectFind()
{
  slotFindFeeds(findFeeds_->text());
}
// ----------------------------------------------------------------------------
void RSSListing::findFeedVisible(bool visible)
{
  findFeedsWidget_->setVisible(visible);
  if (visible) {
    findFeeds_->setFocus();
  } else {
    findFeeds_->clear();
    // Call filter explicitly, because invisible widget don't calls it
    setFeedsFilter(feedsFilterGroup_->checkedAction(), false);
  }
}

/** @brief Totally remove news
 *---------------------------------------------------------------------------*/
void RSSListing::cleanUp()
{
  CleanUpWizard *cleanUpWizard = new CleanUpWizard(this);
  cleanUpWizard->restoreGeometry(settings_->value("CleanUpWizard/geometry").toByteArray());

  cleanUpWizard->exec();
  settings_->setValue("CleanUpWizard/geometry", cleanUpWizard->saveGeometry());

  delete cleanUpWizard;
}

/** @brief Delete news from the feed by criteria
 *---------------------------------------------------------------------------*/
void RSSListing::cleanUpShutdown()
{
  QSqlQuery q;
  QString qStr;

  db_.transaction();

  q.exec("UPDATE news SET new=0 WHERE new==1");
  q.exec("UPDATE news SET read=2 WHERE read==1");
  q.exec("UPDATE feeds SET newCount=0 WHERE newCount!=0");

  if (cleanupOnShutdown_) {
    QList<int> feedsIdList;
    QList<int> foldersIdList;
    q.exec("SELECT id, xmlUrl FROM feeds");
    while (q.next()) {
      if (q.value(1).toString().isEmpty()) {
        foldersIdList << q.value(0).toInt();
      }
      else {
        feedsIdList << q.value(0).toInt();
      }
    }

    // Run Cleanup for all feeds, except categories
    foreach (int feedId, feedsIdList) {
      int countDelNews = 0;
      int countAllNews = 0;

      qStr = QString("SELECT undeleteCount FROM feeds WHERE id=='%1'").arg(feedId);
      q.exec(qStr);
      if (q.next()) countAllNews = q.value(0).toInt();

      QString qStr1 = QString("UPDATE news SET description='', content='', received='', "
                           "author_name='', author_uri='', author_email='', "
                           "category='', new='', read='', starred='', label='', "
                           "deleteDate='', feedParentId='', deleted=2");

      qStr = QString("SELECT id, received FROM news WHERE feedId=='%1' AND deleted == 0").
          arg(feedId);
      if (neverUnreadCleanUp_) qStr.append(" AND read!=0");
      if (neverStarCleanUp_) qStr.append(" AND starred==0");
      if (neverLabelCleanUp_) qStr.append(" AND (label=='' OR label==',' OR label IS NULL)");
      q.exec(qStr);
      while (q.next()) {
        int newsId = q.value(0).toInt();

        if (newsCleanUpOn_ && (countDelNews < (countAllNews - maxNewsCleanUp_))) {
          qStr = QString("%1 WHERE id=='%2'").arg(qStr1).arg(newsId);
          QSqlQuery qt;
          qt.exec(qStr);
          countDelNews++;
          continue;
        }

        QDateTime dateTime = QDateTime::fromString(
              q.value(1).toString(),
              Qt::ISODate);
        if (dayCleanUpOn_ &&
            (dateTime.daysTo(QDateTime::currentDateTime()) > maxDayCleanUp_)) {
          qStr = QString("%1 WHERE id=='%2'").arg(qStr1).arg(newsId);
          QSqlQuery qt;
          qt.exec(qStr);
          countDelNews++;
          continue;
        }

        if (readCleanUp_) {
          qStr = QString("%1 WHERE read!=0 AND id=='%2'").arg(qStr1).arg(newsId);
          QSqlQuery qt;
          qt.exec(qStr);
          countDelNews++;
        }
      }

      int undeleteCount = 0;
      qStr = QString("SELECT count(id) FROM news WHERE feedId=='%1' AND deleted==0").
          arg(feedId);
      q.exec(qStr);
      if (q.next()) undeleteCount = q.value(0).toInt();

      int unreadCount = 0;
      qStr = QString("SELECT count(read) FROM news WHERE feedId=='%1' AND read==0 AND deleted==0").
          arg(feedId);
      q.exec(qStr);
      if (q.next()) unreadCount = q.value(0).toInt();

      qStr = QString("UPDATE feeds SET unread='%1', undeleteCount='%2' WHERE id=='%3'").
          arg(unreadCount).arg(undeleteCount).arg(feedId);
      q.exec(qStr);
    }

    // Run categories recount, because cleanup may change counts
    foreach (int folderIdStart, foldersIdList) {
      if (folderIdStart < 1) continue;

      int folderId = folderIdStart;
      // Process all parents
      while (0 < folderId) {
        int unreadCount = -1;
        int undeleteCount = -1;

        // Calculate sum of all feeds with same parent
        qStr = QString("SELECT sum(unread), sum(undeleteCount) FROM feeds "
                       "WHERE parentId=='%1'").arg(folderId);
        q.exec(qStr);
        if (q.next()) {
          unreadCount   = q.value(0).toInt();
          undeleteCount = q.value(1).toInt();
        }

        if (unreadCount != -1) {
          qStr = QString("UPDATE feeds SET unread='%1', undeleteCount='%2' WHERE id=='%3'").
              arg(unreadCount).arg(undeleteCount).arg(folderId);
          q.exec(qStr);
        }

        // go to next parent's parent
        qStr = QString("SELECT parentId FROM feeds WHERE id=='%1'").arg(folderId);
        folderId = 0;
        q.exec(qStr);
        if (q.next()) folderId = q.value(0).toInt();
      }
    }

    if (cleanUpDeleted_) {
      q.exec("UPDATE news SET description='', content='', received='', "
             "author_name='', author_uri='', author_email='', "
             "category='', new='', read='', starred='', label='', "
             "deleteDate='', feedParentId='', deleted=2 WHERE deleted==1");
    }
  }

  db_.commit();

  if (cleanupOnShutdown_ && optimizeDB_) {
    q.exec("VACUUM");
  }
  q.finish();
}

/** @brief Zooming in browser
 *---------------------------------------------------------------------------*/
void RSSListing::browserZoom(QAction *action)
{
  if (currentNewsTab->type_ == TAB_DOWNLOADS) return;

  if (action->objectName() == "zoomInAct") {
    currentNewsTab->webView_->setZoomFactor(currentNewsTab->webView_->zoomFactor()+0.1);
  } else if (action->objectName() == "zoomOutAct") {
    if (currentNewsTab->webView_->zoomFactor() > 0.1)
      currentNewsTab->webView_->setZoomFactor(currentNewsTab->webView_->zoomFactor()-0.1);
  } else {
    currentNewsTab->webView_->setZoomFactor(1);
  }
}

/** @brief Call default e-mail application to report the problem
 *---------------------------------------------------------------------------*/
void RSSListing::slotReportProblem()
{
  QDesktopServices::openUrl(QUrl("https://code.google.com/p/quite-rss/issues/list"));
}

/** @brief Print browser page
 *---------------------------------------------------------------------------*/
void RSSListing::slotPrint()
{
  if (currentNewsTab->type_ == TAB_DOWNLOADS) return;

  QPrinter printer;
  printer.setDocName(tr("Web Page"));
  QPrintDialog *printDlg = new QPrintDialog(&printer);
  connect(printDlg, SIGNAL(accepted(QPrinter*)), currentNewsTab->webView_, SLOT(print(QPrinter*)));
  printDlg->exec();
  delete printDlg;
}

/** @brief Call print preview dialog
 *---------------------------------------------------------------------------*/
void RSSListing::slotPrintPreview()
{
  if (currentNewsTab->type_ == TAB_DOWNLOADS) return;

  QPrinter printer;
  printer.setDocName(tr("Web Page"));
  QPrintPreviewDialog *prevDlg = new QPrintPreviewDialog(&printer);
  prevDlg->setWindowFlags(prevDlg->windowFlags() | Qt::WindowMaximizeButtonHint);
  prevDlg->resize(650, 800);
  connect(prevDlg, SIGNAL(paintRequested(QPrinter*)), currentNewsTab->webView_, SLOT(print(QPrinter*)));
  prevDlg->exec();
  delete prevDlg;
}
// ----------------------------------------------------------------------------
void RSSListing::setFullScreen()
{
  if (!isFullScreen()) {
    // hide menu & toolbars
    mainToolbar_->hide();
    menuBar()->hide();
#ifdef HAVE_X11
    show();
    raise();
    setWindowState(windowState() | Qt::WindowFullScreen);
#else
    setWindowState(windowState() | Qt::WindowFullScreen);
#endif
  } else {
    menuBar()->show();
    if (mainToolbarToggle_->isChecked())
      mainToolbar_->show();
    setWindowState(windowState() & ~Qt::WindowFullScreen);
  }
}
// ----------------------------------------------------------------------------
void RSSListing::setStayOnTop()
{
  int state = windowState();

  if (stayOnTopAct_->isChecked())
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
  else
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);

  if ((state & Qt::WindowFullScreen) || (state & Qt::WindowMaximized)) {
    restoreGeometry(settings_->value("GeometryState").toByteArray());
  }
  setWindowState((Qt::WindowState)state);
  show();
}

/** @brief Move index after drag&drop operations
 * @param indexWhat index that is moving
 * @param indexWhere index, where to move
 *---------------------------------------------------------------------------*/
void RSSListing::slotMoveIndex(QModelIndex &indexWhat, QModelIndex &indexWhere, int how)
{
  feedsTreeView_->setCursor(Qt::WaitCursor);

  int feedIdWhat = feedsTreeModel_->getIdByIndex(indexWhat);
  int feedParIdWhat = feedsTreeModel_->getParidByIndex(indexWhat);
  int feedIdWhere = feedsTreeModel_->getIdByIndex(indexWhere);
  int feedParIdWhere = feedsTreeModel_->getParidByIndex(indexWhere);

  // Repair rowToParent
  QSqlQuery q;
  if (how == 2) {
    // Move to another folder
    QList<int> idList;
    q.exec(QString("SELECT id FROM feeds WHERE parentId='%1' ORDER BY rowToParent").
           arg(feedParIdWhat));
    while (q.next()) {
      if (feedIdWhat != q.value(0).toInt())
        idList << q.value(0).toInt();
    }
    for (int i = 0; i < idList.count(); i++) {
      q.exec(QString("UPDATE feeds SET rowToParent='%1' WHERE id=='%2'").
             arg(i).arg(idList.at(i)));
    }

    int rowToParent = 0;
    q.exec(QString("SELECT count(id) FROM feeds WHERE parentId='%1'").
           arg(feedIdWhere));
    if (q.next()) rowToParent = q.value(0).toInt();

    q.exec(QString("UPDATE feeds SET parentId='%1', rowToParent='%2' WHERE id=='%3'").
           arg(feedIdWhere).arg(rowToParent).arg(feedIdWhat));

    QList<int> categoriesList;
    categoriesList << feedParIdWhat << feedIdWhere;
    recountFeedCategories(categoriesList);

    feedParIdWhere = feedIdWhere;
  } else if (feedParIdWhat == feedParIdWhere) {
    // Move inside folder
    QList<int> idList;
    q.exec(QString("SELECT id FROM feeds WHERE parentId='%1' ORDER BY rowToParent").
           arg(feedParIdWhat));
    while (q.next()) {
      idList << q.value(0).toInt();
    }

    int rowWhat = feedsTreeModel_->dataField(indexWhat, "rowToParent").toInt();
    int rowWhere = feedsTreeModel_->dataField(indexWhere, "rowToParent").toInt();
    if ((rowWhat < rowWhere) && (how != 1)) rowWhere--;
    else if (how == 1) rowWhere++;
    idList.insert(rowWhere, idList.takeAt(rowWhat));

    for (int i = 0; i < idList.count(); i++) {
      q.exec(QString("UPDATE feeds SET rowToParent='%1' WHERE id=='%2'").
             arg(i).arg(idList.at(i)));
    }
  } else {
    // Move in another folder beside feeds
    QList<int> idList;
    q.exec(QString("SELECT id FROM feeds WHERE parentId='%1' ORDER BY rowToParent").
           arg(feedParIdWhat));
    while (q.next()) {
      if (feedIdWhat != q.value(0).toInt())
        idList << q.value(0).toInt();
    }
    for (int i = 0; i < idList.count(); i++) {
      q.exec(QString("UPDATE feeds SET rowToParent='%1' WHERE id=='%2'").
             arg(i).arg(idList.at(i)));
    }

    //

    idList.clear();
    q.exec(QString("SELECT id FROM feeds WHERE parentId='%1' ORDER BY rowToParent").
           arg(feedParIdWhere));
    while (q.next()) {
      idList << q.value(0).toInt();
    }

    int rowWhere = feedsTreeModel_->dataField(indexWhere, "rowToParent").toInt();
    if (how == 1) rowWhere++;
    idList.insert(rowWhere, feedIdWhat);

    for (int i = 0; i < idList.count(); i++) {
      q.exec(QString("UPDATE feeds SET rowToParent='%1' WHERE id=='%2'").
             arg(i).arg(idList.at(i)));
    }

    q.exec(QString("UPDATE feeds SET parentId='%1' WHERE id=='%2'").
           arg(feedParIdWhere).arg(feedIdWhat));

    QList<int> categoriesList;
    categoriesList << feedParIdWhat << feedParIdWhere;
    recountFeedCategories(categoriesList);
  }

  feedsTreeModel_->refresh();
  expandNodes();

  QModelIndex feedIndex = feedsTreeModel_->getIndexById(feedIdWhat);
  feedsTreeView_->setCurrentIndex(feedIndex);

  feedsTreeView_->setCursor(Qt::ArrowCursor);
}

/** @brief Process clicks in feeds tree
 * @param item Item that was clicked
 *---------------------------------------------------------------------------*/
void RSSListing::slotCategoriesClicked(QTreeWidgetItem *item, int, bool createTab)
{
  if (stackedWidget_->count() && currentNewsTab->type_ < TAB_WEB) {
    currentNewsTab->newsHeader_->saveStateColumns(this, currentNewsTab);
    settings_->setValue("NewsTabSplitterState",
                        currentNewsTab->newsTabWidgetSplitter_->saveState());
  }

  int type = item->text(1).toInt();

  int indexTab = -1;
  if (!createTab) {
    for (int i = 0; i < stackedWidget_->count(); i++) {
      NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
      if (widget->type_ == type) {
        if (widget->type_ == TAB_CAT_LABEL) {
          if (widget->labelId_ == item->text(2).toInt()) {
            indexTab = i;
            break;
          }
        }
        else {
          indexTab = i;
          break;
        }
      }
    }
  }

  if (indexTab == -1) {
    if (createTab) {
      setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_,
                  FeedReadSwitchingTab, currentNewsTab);

      NewsTabWidget *widget = new NewsTabWidget(this, type);
      indexTab = addTab(widget);
      createNewsTab(indexTab);
    }
    else {
      feedsTreeView_->setCurrentIndex(QModelIndex());
      feedProperties_->setEnabled(false);

      if (tabBar_->currentIndex() != TAB_WIDGET_PERMANENT) {
        setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_,
                    FeedReadSwitchingTab, currentNewsTab);

        QModelIndex curIndex = categoriesTree_->currentIndex();
        updateCurrentTab_ = false;
        tabBar_->setCurrentIndex(TAB_WIDGET_PERMANENT);
        updateCurrentTab_ = true;
        categoriesTree_->setCurrentIndex(curIndex);

        currentNewsTab = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);
        newsModel_ = currentNewsTab->newsModel_;
        newsView_ = currentNewsTab->newsView_;
      } else {
        setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_,
                    FeedReadSwitchingFeed, currentNewsTab);
      }

      currentNewsTab->type_ = type;
      currentNewsTab->feedId_ = -1;
      currentNewsTab->feedParId_ = -1;
      currentNewsTab->setSettings(false);
      currentNewsTab->setVisible(true);
    }

    // Set icon and title of current tab
    currentNewsTab->newsIconTitle_->setPixmap(item->icon(0).pixmap(16,16));
    currentNewsTab->setTextTab(item->text(0));

    currentNewsTab->labelId_ = item->text(2).toInt();

    switch (type) {
    case TAB_CAT_UNREAD:
      currentNewsTab->categoryFilterStr_ = "feedId > 0 AND deleted = 0 AND read < 2";
      break;
    case TAB_CAT_STAR:
      currentNewsTab->categoryFilterStr_ = "feedId > 0 AND deleted = 0 AND starred = 1";
      break;
    case TAB_CAT_DEL:
      currentNewsTab->categoryFilterStr_ = "feedId > 0 AND deleted = 1";
      break;
    case TAB_CAT_LABEL:
      if (currentNewsTab->labelId_ != 0) {
        currentNewsTab->categoryFilterStr_ =
            QString("feedId > 0 AND deleted = 0 AND label LIKE '%,%1,%'").
            arg(currentNewsTab->labelId_);
      } else {
        currentNewsTab->categoryFilterStr_ =
            QString("feedId > 0 AND deleted = 0 AND label!='' AND label!=','");
      }
      break;
    }
    newsModel_->setFilter(currentNewsTab->categoryFilterStr_);

    if (newsModel_->rowCount() != 0) {
      while (newsModel_->canFetchMore())
        newsModel_->fetchMore();
    }
    if (type == TAB_CAT_DEL){
      currentNewsTab->newsHeader_->setSortIndicator(newsModel_->fieldIndex("deleteDate"),
                                                    Qt::DescendingOrder);
    } else {
      if ((currentNewsTab->newsHeader_->sortIndicatorSection() == newsModel_->fieldIndex("read")) ||
          currentNewsTab->newsHeader_->sortIndicatorSection() == newsModel_->fieldIndex("starred")) {
        currentNewsTab->slotSort(currentNewsTab->newsHeader_->sortIndicatorSection(),
                                 currentNewsTab->newsHeader_->sortIndicatorOrder());
      }
    }

    // Search previous displayed news of the feed
    int newsRow = -1;
    if (openingFeedAction_ == 0) {
      int newsIdCur = item->text(3).toInt();
      QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("id"));
      QModelIndexList indexList = newsModel_->match(index, Qt::EditRole, newsIdCur);

      if (!indexList.isEmpty()) newsRow = indexList.first().row();
    } else if (openingFeedAction_ == 1) {
      newsRow = 0;
    } else if (openingFeedAction_ == 3) {
      QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("read"));
      QModelIndexList indexList;
      if (newsView_->header()->sortIndicatorOrder() == Qt::DescendingOrder)
        indexList = newsModel_->match(index, Qt::EditRole, 0, -1);
      else
        indexList = newsModel_->match(index, Qt::EditRole, 0);

      if (!indexList.isEmpty()) newsRow = indexList.last().row();
    }

    // Display previous displayed news of the feed
    newsView_->setCurrentIndex(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
    if (newsRow == -1) newsView_->verticalScrollBar()->setValue(newsRow);

    if ((openingFeedAction_ != 2) && openNewsWebViewOn_) {
      currentNewsTab->slotNewsViewSelected(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
    } else {
      currentNewsTab->slotNewsViewSelected(newsModel_->index(-1, newsModel_->fieldIndex("title")));
    }

    if (createTab)
      emit signalSetCurrentTab(indexTab);
  } else {
    emit signalSetCurrentTab(indexTab, true);
  }

  int unreadCount = 0;
  int allCount = currentNewsTab->newsModel_->rowCount();
  QString countStr = categoriesTree_->currentItem()->text(4);
  if (!countStr.isEmpty()) {
    countStr.remove(QRegExp("[()]"));
    switch (currentNewsTab->type_) {
    case TAB_CAT_UNREAD:
      unreadCount = countStr.toInt();
      break;
    case TAB_CAT_STAR: case TAB_CAT_LABEL:
      unreadCount = countStr.section("/", 0, 0).toInt();
      break;
    }
  }
  statusUnread_->setText(QString(" " + tr("Unread: %1") + " ").arg(unreadCount));
  statusAll_->setText(QString(" " + tr("All: %1") + " ").arg(allCount));

  statusUnread_->setVisible(currentNewsTab->type_ != TAB_CAT_DEL);
  statusAll_->setVisible(true);
}

void RSSListing::clearDeleted()
{
  QSqlQuery q;
  q.exec("UPDATE news SET description='', content='', received='', "
         "author_name='', author_uri='', author_email='', "
         "category='', new='', read='', starred='', label='', "
         "deleteDate='', feedParentId='', deleted=2 WHERE deleted==1");

  if (currentNewsTab->type_ == TAB_CAT_DEL) {
    currentNewsTab->newsModel_->select();
    currentNewsTab->slotNewsViewSelected(QModelIndex());
  }

  recountCategoryCounts();
}

/** @brief Show/Hide categories tree
 *---------------------------------------------------------------------------*/
void RSSListing::showNewsCategoriesTree()
{
  if (categoriesTree_->isHidden()) {
    showCategoriesButton_->setIcon(QIcon(":/images/images/panel_hide.png"));
    showCategoriesButton_->setToolTip(tr("Hide Categories"));
    categoriesTree_->show();
    feedsSplitter_->restoreState(feedsWidgetSplitterState_);
    recountCategoryCounts();
  } else {
    feedsWidgetSplitterState_ = feedsSplitter_->saveState();
    showCategoriesButton_->setIcon(QIcon(":/images/images/panel_show.png"));
    showCategoriesButton_->setToolTip(tr("Show Categories"));
    categoriesTree_->hide();
    QList <int> sizes;
    sizes << height() << 20;
    feedsSplitter_->setSizes(sizes);
  }
}

/** @brief Move splitter between feeds tree and categories tree
 *---------------------------------------------------------------------------*/
void RSSListing::feedsSplitterMoved(int pos, int)
{
  if (categoriesTree_->isHidden()) {
    int height = pos + categoriesPanel_->height() + 2;
    if (height < feedsSplitter_->height()) {
      showCategoriesButton_->setIcon(QIcon(":/images/images/panel_hide.png"));
      showCategoriesButton_->setToolTip(tr("Hide Categories"));
      categoriesTree_->show();
      recountCategoryCounts();
    }
  }
}

/** @brief Set specified label for news
 *---------------------------------------------------------------------------*/
void RSSListing::setLabelNews(QAction *action)
{
  if (currentNewsTab->type_ >= TAB_WEB) return;

  newsLabelAction_->setIcon(action->icon());
  newsLabelAction_->setToolTip(action->text());
  newsLabelAction_->setData(action->data());

  currentNewsTab->setLabelNews(action->data().toInt());
}

/** @brief Set last chosen label for news
 *---------------------------------------------------------------------------*/
void RSSListing::setDefaultLabelNews()
{
  if (currentNewsTab->type_ >= TAB_WEB) return;

  currentNewsTab->setLabelNews(newsLabelAction_->data().toInt());
}

/** @brief Get Label that belong to current news
 *---------------------------------------------------------------------------*/
void RSSListing::getLabelNews()
{
  for (int i = 0; i < newsLabelGroup_->actions().count(); i++) {
    newsLabelGroup_->actions().at(i)->setChecked(false);
  }

  if (currentNewsTab->type_ >= TAB_WEB) return;

  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(
        newsModel_->fieldIndex("label"));
  if (!indexes.count()) return;

  if (indexes.count() == 1) {
    QModelIndex index = indexes.at(0);
    QStringList strLabelIdList = index.data(Qt::EditRole).toString().split(",", QString::SkipEmptyParts);
    foreach (QString strLabelId, strLabelIdList) {
      for (int i = 0; i < newsLabelGroup_->actions().count(); i++) {
        if (newsLabelGroup_->actions().at(i)->data().toString() == strLabelId)
          newsLabelGroup_->actions().at(i)->setChecked(true);
      }
    }
  } else {
    for (int i = 0; i < newsLabelGroup_->actions().count(); i++) {
      bool check = false;
      QString strLabelId = newsLabelGroup_->actions().at(i)->data().toString();
      for (int y = indexes.count()-1; y >= 0; --y) {
        QModelIndex index = indexes.at(y);
        QString strIdLabels = index.data(Qt::EditRole).toString();
        if (!strIdLabels.contains(QString(",%1,").arg(strLabelId))) {
          check = false;
          break;
        }
        check = true;
      }
      newsLabelGroup_->actions().at(i)->setChecked(check);
    }
  }
}

/** @brief Add tab widget to tabs stack widget
 *---------------------------------------------------------------------------*/
int RSSListing::addTab(NewsTabWidget *widget)
{
  int indexTab;
  if (openNewTabNextToActive_) {
    if (stackedWidget_->count()) tabBar_->insertTab(tabBar_->currentIndex()+1, "");
    indexTab = stackedWidget_->insertWidget(stackedWidget_->currentIndex()+1, widget);
  }
  else {
    if (stackedWidget_->count()) tabBar_->addTab("");
    indexTab = stackedWidget_->addWidget(widget);
  }
  tabBar_->setTabButton(indexTab,
                        QTabBar::LeftSide,
                        widget->newsTitleLabel_);
  return indexTab;
}

/** @brief Request authentification
 *---------------------------------------------------------------------------*/
void RSSListing::slotAuthentication(QNetworkReply *reply, QAuthenticator *auth)
{
  AuthenticationDialog *authenticationDialog =
      new AuthenticationDialog(this, reply->url(), auth);

  if (!authenticationDialog->save_->isChecked())
    authenticationDialog->exec();

  delete authenticationDialog;
}
// ----------------------------------------------------------------------------
void RSSListing::reduceNewsList()
{
  currentNewsTab->reduceNewsList();
}
// ----------------------------------------------------------------------------
void RSSListing::increaseNewsList()
{
  currentNewsTab->increaseNewsList();
}

/** @brief Save browser current page to file
 *---------------------------------------------------------------------------*/
void RSSListing::slotSavePageAs()
{
  if (currentNewsTab->type_ == TAB_DOWNLOADS) return;

  QString fileName = currentNewsTab->webView_->title();
  if (fileName == "news_descriptions") {
    int row = currentNewsTab->newsView_->currentIndex().row();
    fileName = currentNewsTab->newsModel_->record(row).field("title").value().toString();
  }

  fileName = fileName.trimmed();
  fileName = fileName.replace(QRegExp("[:\"]"), "_");
  fileName = QDir::toNativeSeparators(QDir::homePath() + "/" + fileName);
  fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                          fileName,
                                          tr("HTML-Files (*.html)")+ ";;" +
                                          tr("Text files (*.txt)"));
  if (fileName.isNull()) return;

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly)) {
    statusBar()->showMessage(tr("Save As: can't open a file"), 3000);
    return;
  }
  QFileInfo fileInfo(fileName);
  if (fileInfo.suffix() == "txt") {
    file.write(currentNewsTab->webView_->page()->mainFrame()->toPlainText().toUtf8());
  } else {
    QString html = currentNewsTab->webView_->page()->mainFrame()->toHtml();
    QTextCodec *codec = QTextCodec::codecForHtml(html.toUtf8(),
                                                 QTextCodec::codecForName("UTF-8"));
    file.write(codec->fromUnicode(html));
  }
  file.close();
}

/** @brief Get user login adn password form DB
 * @param url Site URL
 * @param auth Enable authorization flag
 *---------------------------------------------------------------------------*/
QString RSSListing::getUserInfo(QUrl url, int auth)
{
  QString userInfo;
  if (auth == 1) {
    QSqlQuery q;
    q.prepare("SELECT username, password FROM passwords WHERE server=?");
    q.addBindValue(url.host());
    q.exec();
    if (q.next()) {
      userInfo = QString("%1:%2").arg(q.value(0).toString()).
          arg(QString::fromUtf8(QByteArray::fromBase64(q.value(1).toByteArray())));
    }
  }
  return userInfo;
}

/** @brief Restore last deleted news
 *---------------------------------------------------------------------------*/
void RSSListing::restoreLastNews()
{
  QSqlQuery q;
  q.exec("SELECT id, feedId FROM news WHERE deleted=1 AND deleteDate!='' ORDER BY deleteDate DESC");
  if (q.next()) {
    QModelIndex curIndex = newsView_->currentIndex();
    int newsIdCur = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();

    int newsId = q.value(0).toInt();
    int feedId = q.value(1).toInt();
    q.exec(QString("UPDATE news SET deleted=0, deleteDate='' WHERE id=='%1'").
           arg(newsId));

    newsModel_->select();

    while (newsModel_->canFetchMore())
      newsModel_->fetchMore();

    QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("id"));
    QModelIndexList indexList = newsModel_->match(index, Qt::EditRole, newsIdCur);
    if (indexList.count()) {
      int newsRow = indexList.first().row();
      newsView_->setCurrentIndex(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
    }
    slotUpdateStatus(feedId);
    recountCategoryCounts();
  }
}

/** @brief Switch to next unread news
 *---------------------------------------------------------------------------*/
void RSSListing::nextUnreadNews()
{
  if (currentNewsTab->type_ >= TAB_WEB) return;
  newsView_->clearSelection();

  int newsRow = currentNewsTab->findUnreadNews(true);

  if (newsRow == -1) {
    if (currentNewsTab->type_ != TAB_FEED) return;

    QModelIndex indexPrevUnread =
        feedsTreeView_->indexNextUnread(feedsTreeView_->currentIndex(), 1);
    if (!indexPrevUnread.isValid()) {
      QModelIndex index =
          feedsTreeModel_->index(-1, feedsTreeView_->columnIndex("text"));
      indexPrevUnread = feedsTreeView_->indexNextUnread(index, 1);
    }
    if (indexPrevUnread.isValid()) {
      if (changeBehaviorActionNUN_)
        openingFeedAction_ = 4;
      else
        openingFeedAction_ = 3;
      feedsTreeView_->setCurrentIndex(indexPrevUnread);
      slotFeedClicked(indexPrevUnread);
      openingFeedAction_ = settings_->value("/Settings/openingFeedAction", 0).toInt();
    }
    newsView_->setCurrentIndex(newsView_->currentIndex());
    return;
  }

  int value = newsView_->verticalScrollBar()->value();
  int pageStep = newsView_->verticalScrollBar()->pageStep();
  if (newsRow > (value + pageStep/2))
    newsView_->verticalScrollBar()->setValue(newsRow - pageStep/2);

  QModelIndex index = newsModel_->index(newsRow, newsModel_->fieldIndex("title"));
  newsView_->setCurrentIndex(index);
  currentNewsTab->slotNewsViewSelected(index);
}

/** @brief Switch to previous unread news
 *---------------------------------------------------------------------------*/
void RSSListing::prevUnreadNews()
{
  if (currentNewsTab->type_ >= TAB_WEB) return;
  newsView_->clearSelection();

  int newsRow = currentNewsTab->findUnreadNews(false);

  int newsRowCur = newsView_->currentIndex().row();
  if ((newsRow >= newsRowCur) || (newsRow == -1)) {
    if (currentNewsTab->type_ != TAB_FEED) return;

    QModelIndex indexNextUnread =
        feedsTreeView_->indexNextUnread(feedsTreeView_->currentIndex(), 2);
    if (indexNextUnread.isValid()) {
      openingFeedAction_ = 3;
      feedsTreeView_->setCurrentIndex(indexNextUnread);
      slotFeedClicked(indexNextUnread);
      openingFeedAction_ = settings_->value("/Settings/openingFeedAction", 0).toInt();
    }
    newsView_->setCurrentIndex(newsView_->currentIndex());
    return;
  }

  int value = newsView_->verticalScrollBar()->value();
  int pageStep = newsView_->verticalScrollBar()->pageStep();
  if (newsRow < (value + pageStep/2))
    newsView_->verticalScrollBar()->setValue(newsRow - pageStep/2);

  QModelIndex index = newsModel_->index(newsRow, newsModel_->fieldIndex("title"));
  newsView_->setCurrentIndex(index);
  currentNewsTab->slotNewsViewSelected(index);
}

/** @brief Get feeds ids list of folder \a idFolder
 *---------------------------------------------------------------------------*/
QList<int> RSSListing::getIdFeedsInList(int idFolder)
{
  QList<int> idList;
  if (idFolder <= 0) return idList;

  QSqlQuery q;
  QQueue<int> parentIds;
  parentIds.enqueue(idFolder);
  while (!parentIds.empty()) {
    int parentId = parentIds.dequeue();
    QString qStr = QString("SELECT id, xmlUrl FROM feeds WHERE parentId='%1'").
        arg(parentId);
    q.exec(qStr);
    while (q.next()) {
      int feedId = q.value(0).toInt();
      if (!q.value(1).toString().isEmpty())
        idList << feedId;
      if (q.value(1).toString().isEmpty())
        parentIds.enqueue(feedId);
    }
  }
  return idList;
}

/** @brief Get feeds ids list string of folder \a idFolder
 *---------------------------------------------------------------------------*/
QString RSSListing::getIdFeedsString(int idFolder, int idException)
{
  QList<int> idList = getIdFeedsInList(idFolder);
  if (idList.count()) {
    QString str;
    foreach (int id, idList) {
      if (id == idException) continue;
      if (!str.isEmpty()) str.append(" OR ");
      str.append(QString("feedId=%1").arg(id));
    }
    return str;
  } else {
    return QString("feedId=-1");
  }
}

/** @brief Set application title
 *---------------------------------------------------------------------------*/
void RSSListing::setTextTitle(const QString &text, NewsTabWidget *widget)
{
  if (currentNewsTab != widget) return;

  if (text.isEmpty()) setWindowTitle("QuiteRSS");
  else setWindowTitle(QString("%1 - QuiteRSS").arg(text));
}

/** @brief Enable|Disable indent in feeds tree
 *---------------------------------------------------------------------------*/
void RSSListing::slotIndentationFeedsTree()
{
  feedsTreeView_->setRootIsDecorated(indentationFeedsTreeAct_->isChecked());
}

// ----------------------------------------------------------------------------
void RSSListing::customizeMainToolbar()
{
  showCustomizeToolbarDlg(customizeMainToolbarAct2_);
}
// ----------------------------------------------------------------------------
void RSSListing::showCustomizeToolbarDlg(QAction *action)
{
  QToolBar *toolbar = mainToolbar_;
  if (action->objectName() == "customizeFeedsToolbarAct") {
    toolbar = feedsToolBar_;
  } else if (action->objectName() == "customizeNewsToolbarAct") {
    if (currentNewsTab->type_ == TAB_DOWNLOADS) return;
    toolbar = currentNewsTab->newsToolBar_;
  }

  CustomizeToolbarDialog *toolbarDlg = new CustomizeToolbarDialog(this, toolbar);

  toolbarDlg->exec();

  delete toolbarDlg;
}

/** @brief Process news sharing
 *---------------------------------------------------------------------------*/
void RSSListing::slotShareNews(QAction *action)
{
  currentNewsTab->slotShareNews(action);
}
// ----------------------------------------------------------------------------
void RSSListing::showMenuShareNews()
{
  if (mainToolbar_->widgetForAction(shareMenuAct_)) {
    QWidget *widget = mainToolbar_->widgetForAction(shareMenuAct_);
    if (widget->underMouse()) {
      shareMenu_->popup(widget->mapToGlobal(QPoint(0, mainToolbar_->height()-1)));
    }
  }
  if (feedsToolBar_->widgetForAction(shareMenuAct_)) {
    QWidget *widget = feedsToolBar_->widgetForAction(shareMenuAct_);
    if (widget->underMouse()) {
      shareMenu_->popup(widget->mapToGlobal(QPoint(0, feedsToolBar_->height()-1)));
    }
  }
  if (currentNewsTab->newsToolBar_->widgetForAction(shareMenuAct_)) {
    QWidget *widget = currentNewsTab->newsToolBar_->widgetForAction(shareMenuAct_);
    if (widget->underMouse()) {
      shareMenu_->popup(widget->mapToGlobal(QPoint(0, currentNewsTab->newsToolBar_->height()-1)));
    }
  }
}

/** @brief Open feed home page in external browser
 *---------------------------------------------------------------------------*/
void RSSListing::slotOpenHomeFeed()
{
  QModelIndex index = feedsTreeView_->currentIndex();
  if (!index.isValid()) return;

  QString homePage = feedsTreeModel_->dataField(index, "htmlUrl").toString();
  QDesktopServices::openUrl(homePage);
}

/** @brief Sort feed and folders by title
 *---------------------------------------------------------------------------*/
void RSSListing::sortedByTitleFeedsTree()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<int> parentIdsPotential;
  parentIdsPotential << 0;
  while (!parentIdsPotential.empty()) {
    int parentId = parentIdsPotential.takeFirst();

    // Search children of parent <parentId>
    QSqlQuery q;
    q.prepare(QString("SELECT id, xmlUrl FROM feeds WHERE parentId=? ORDER BY text"));
    q.addBindValue(parentId);
    q.exec();

    // assing each child his <rowToParent>
    // ... store it in prospactive parent list
    int rowToParent = 0;
    while (q.next()) {
      int parentIdNew = q.value(0).toInt();
      QString xmlUrl = q.value(1).toString();

      QSqlQuery q2;
      q2.prepare("UPDATE feeds SET rowToParent=? WHERE id=?");
      q2.addBindValue(rowToParent);
      q2.addBindValue(parentIdNew);
      q2.exec();

      if (xmlUrl.isEmpty())
        parentIdsPotential << parentIdNew;
      ++rowToParent;
    }
  }

  feedsModelReload();
  QApplication::restoreOverrideCursor();
}

/** @brief Set user style sheet for browser
 * @param filePath Filepath of user style
 * @return URL-link to user style
 *---------------------------------------------------------------------------*/
QUrl RSSListing::userStyleSheet(const QString &filePath) const
{
  QString userStyle = "html{background-color:white;}";

  QFile file(filePath);
  if (!filePath.isEmpty() && file.open(QFile::ReadOnly)) {
    QString fileData = QString::fromUtf8(file.readAll());
    fileData.remove(QLatin1Char('\n'));
    userStyle.append(fileData);
    file.close();
  }

  const QString &encodedStyle = userStyle.toLatin1().toBase64();
  const QString &dataString = QString("data:text/css;charset=utf-8;base64,%1").arg(encodedStyle);

  return QUrl(dataString);
}
// ----------------------------------------------------------------------------
bool RSSListing::addFeedInQueue(const QString &feedUrl, const QDateTime &date, int auth)
{
  int feedUrlIndex = feedUrlList_.indexOf(feedUrl);
  if (feedUrlIndex > -1) {
    return false;
  } else {
    feedUrlList_.append(feedUrl);
    updateFeedsCount_ = updateFeedsCount_ + 2;
    QString userInfo = getUserInfo(feedUrl, auth);
    emit signalRequestUrl(feedUrl, date, userInfo);
    return true;
  }
}
// ----------------------------------------------------------------------------
void RSSListing::showNewsMenu()
{
  newsSortByMenu_->setEnabled(currentNewsTab->type_ < TAB_WEB);
}
// ----------------------------------------------------------------------------
void RSSListing::showNewsSortByMenu()
{
  if (currentNewsTab->type_ >= TAB_WEB) return;

  QListIterator<QAction *> iter(newsSortByColumnGroup_->actions());
  while (iter.hasNext()) {
    QAction *nextAction = iter.next();
    delete nextAction;
  }

  int section = currentNewsTab->newsHeader_->sortIndicatorSection();
  iter = currentNewsTab->newsHeader_->viewMenu_->actions();
  while (iter.hasNext()) {
    QAction *nextAction = iter.next();
    QAction *newsSortByColumnAct = new QAction(this);
    newsSortByColumnAct->setCheckable(true);
    newsSortByColumnAct->setText(nextAction->text());
    newsSortByColumnAct->setData(nextAction->data());
    if (nextAction->data().toInt() == section) {
      newsSortByColumnAct->setChecked(true);
    }
    newsSortByColumnGroup_->addAction(newsSortByColumnAct);
  }
  newsSortByMenu_->insertActions(newsSortByMenu_->actions().at(0),
                                 newsSortByColumnGroup_->actions());

  if (currentNewsTab->newsHeader_->sortIndicatorOrder() == Qt::AscendingOrder) {
    newsSortOrderGroup_->actions().at(0)->setChecked(true);
  } else {
    newsSortOrderGroup_->actions().at(1)->setChecked(true);
  }
}
// ----------------------------------------------------------------------------
void RSSListing::setNewsSortByColumn()
{
  if (currentNewsTab->type_ >= TAB_WEB) return;

  int lIdx = newsSortByColumnGroup_->checkedAction()->data().toInt();
  if (newsSortOrderGroup_->actions().at(0)->isChecked()) {
    currentNewsTab->newsHeader_->setSortIndicator(lIdx, Qt::AscendingOrder);
  } else {
    currentNewsTab->newsHeader_->setSortIndicator(lIdx, Qt::DescendingOrder);
  }
}
// ----------------------------------------------------------------------------
void RSSListing::slotPrevFolder()
{
  QModelIndex indexBefore = feedsTreeView_->currentIndex();
  QModelIndex indexAfter;

  // Set to bottom folder, if there's no above folder
  if (!indexBefore.isValid())
    indexAfter = feedsTreeView_->lastFolderInFolder(QModelIndex());
  else
    indexAfter = feedsTreeView_->indexPreviousFolder(indexBefore);

  // there's no "upper" index
  if (!indexAfter.isValid()) return;

  feedsTreeView_->setCurrentIndex(indexAfter);
  slotFeedClicked(indexAfter);
}
// ----------------------------------------------------------------------------
void RSSListing::slotNextFolder()
{
  QModelIndex indexBefore = feedsTreeView_->currentIndex();
  QModelIndex indexAfter;

  // Set to top index, if there's no below index
  if (!indexBefore.isValid())
    indexAfter = feedsTreeView_->firstFolderInFolder(QModelIndex());
  else
    indexAfter = feedsTreeView_->indexNextFolder(indexBefore);

  // there's no "downer" index
  if (!indexAfter.isValid()) return;

  feedsTreeView_->setCurrentIndex(indexAfter);
  slotFeedClicked(indexAfter);
}
// ----------------------------------------------------------------------------
void RSSListing::slotExpandFolder()
{
  QModelIndex index = feedsTreeView_->currentIndex();
  if (!feedsTreeModel_->isFolder(index)) {
    index = feedsTreeModel_->parent(index);
  }
  feedsTreeView_->setExpanded(index, !feedsTreeView_->isExpanded(index));
}
// ----------------------------------------------------------------------------
void RSSListing::showDownloadManager(bool activate)
{
  int indexTab = -1;
  NewsTabWidget *widget;
  for (int i = 0; i < stackedWidget_->count(); i++) {
    widget = (NewsTabWidget*)stackedWidget_->widget(i);
    if (widget->type_ == TAB_DOWNLOADS) {
      indexTab = i;
      break;
    }
  }

  if (indexTab == -1) {
    widget = new NewsTabWidget(this, TAB_DOWNLOADS);
    indexTab = addTab(widget);
    QPixmap iconTab;
    iconTab.load(":/images/download");
    widget->newsIconTitle_->setPixmap(iconTab);
    widget->retranslateStrings();
  }

  if (activate) {
    currentNewsTab = widget;
    currentNewsTab->setTextTab(tr("Downloads"));
    statusUnread_->setVisible(false);
    statusAll_->setVisible(false);
    downloadManager_->show();
    emit signalSetCurrentTab(indexTab);
  } else {
    widget->setTextTab(tr("Downloads"));
  }
}
// ----------------------------------------------------------------------------
void RSSListing::updateInfoDownloads(const QString &text)
{
  NewsTabWidget *widget;
  for (int i = 0; i < stackedWidget_->count(); i++) {
    widget = (NewsTabWidget*)stackedWidget_->widget(i);
    if (widget->type_ == TAB_DOWNLOADS) {
      widget->setTextTab(QString("%1 %2").arg(tr("Downloads")).arg(text));
      break;
    }
  }
}

bool RSSListing::removePath(const QString &path)
{
    bool result = true;
    QFileInfo info(path);
    if (info.isDir()) {
        QDir dir(path);
        foreach (const QString &entry, dir.entryList(QDir::AllDirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot)) {
            result &= removePath(dir.absoluteFilePath(entry));
        }
        if (!info.dir().rmdir(info.fileName()))
            return false;
    } else {
        result = QFile::remove(path);
    }
    return result;
}
