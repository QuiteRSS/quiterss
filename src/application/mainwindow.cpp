/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2021 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "mainwindow.h"

#include "common.h"
#include "mainapplication.h"
#include "database.h"
#include "aboutdialog.h"
#include "adblockmanager.h"
#include "adblockicon.h"
#include "addfeedwizard.h"
#include "addfolderdialog.h"
#include "cleanupwizard.h"
#include "customizetoolbardialog.h"
#include "feedpropertiesdialog.h"
#include "filterrulesdialog.h"
#include "newsfiltersdialog.h"
#include "webpage.h"
#include "settings.h"

#if defined(Q_OS_WIN)
#include <windows.h>
#include <psapi.h>
#endif
#include <QMainWindow>
#include <QStatusBar>
#include <qzregexp.h>

// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , isMinimizeToTray_(true)
  , currentNewsTab(NULL)
  , isOpeningLink_(false)
  , openNewsTab_(0)
  , feedsFilterAction_(NULL)
  , newsFilterAction_(NULL)
  , newsView_(NULL)
  , updateTimeCount_(0)
#if defined(HAVE_QT5) || defined(HAVE_PHONON)
  , mediaPlayer_(NULL)
#endif
  , updateAppDialog_(NULL)
  , notificationWidget(NULL)
  , feedIdOld_(-2)
  , isStartImportFeed_(false)
  , recountCategoryCountsOn_(false)
  , optionsDialog_(NULL)
{
  setObjectName("mainWindow");
  setWindowTitle("QuiteRSS");
  setContextMenuPolicy(Qt::CustomContextMenu);

  if (mainApp->analytics())
    mainApp->analytics()->sendScreenview("MainWindow");

  db_ = QSqlDatabase::database();

  createFeedsWidget();
  createToolBarNull();

  createActions();
  createShortcut();
  createMenu();
  createToolBar();

  createStatusBar();
  createTray();

  createTabBarWidget();
  createCentralWidget();

  loadSettingsFeeds();

  setStyleSheet("QMainWindow::separator { width: 1px; }");

  loadSettings();

  addOurFeed();

  initUpdateFeeds();

  QTimer::singleShot(5000, this, SLOT(slotUpdateAppCheck()));

  connect(this, SIGNAL(signalShowNotification(bool)),
          SLOT(showNotification(bool)), Qt::QueuedConnection);
  connect(this, SIGNAL(signalPlaySoundNewNews()),
          SLOT(slotPlaySoundNewNews()), Qt::QueuedConnection);

  connect(&timerLinkOpening_, SIGNAL(timeout()),
          this, SLOT(slotTimerLinkOpening()));

  connect(mainApp->downloadManager(), SIGNAL(signalShowDownloads(bool)),
          this, SLOT(showDownloadManager(bool)));
  connect(mainApp->downloadManager(), SIGNAL(signalUpdateInfo(QString)),
          this, SLOT(updateInfoDownloads(QString)));

  connect(&timerTrayOpenNotify, SIGNAL(timeout()), this, SLOT(slotTrayOpenNotifyTimer()));
  timerTrayOpenNotify.setSingleShot(true);

  retranslateStrings();

  installEventFilter(this);
}

// ---------------------------------------------------------------------------
MainWindow::~MainWindow()
{

}

// ---------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
  if (mainApp->isClosing())
    return;

  if (closingTray_ && showTrayIcon_) {
    event->ignore();
    isOpeningLink_ = false;

    oldState = windowState();
    emit signalPlaceToTray();
  } else {
#if defined(Q_OS_MAC)
    event->ignore();
#endif
    quitApp();
  }
}

/** @brief Process quit application
 *---------------------------------------------------------------------------*/
void MainWindow::quitApp()
{
  mainApp->setClosing();
  isMinimizeToTray_ = true;
  disconnect(this);
  if (updateAppDialog_) {
    updateAppDialog_->disconnectObjects();
    updateAppDialog_->deleteLater();
  }
  if (optionsDialog_) {
    optionsDialog_->close();
  }

  mainApp->showClosingWidget();

  hide();
  traySystem->hide();

  saveSettings();

  for (int i = 0; i < stackedWidget_->count(); i++) {
    NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
    widget->disconnectObjects();
  }

  mainApp->networkManager()->disconnect(mainApp->networkManager());
  mainApp->updateFeeds()->disconnectObjects();
  emit signalQuitApp();
}

// ---------------------------------------------------------------------------
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
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
  } else if (obj == categoriesTree_) {
    if (event->type() == QEvent::Show) {
      recountCategoryCounts();
    }
  }
  else if (event->type() == QEvent::WindowActivate) {
    activationStateChangedTime_ = QDateTime::currentMSecsSinceEpoch();
  }
  // Process  open link in browser in background
  else if (event->type() == QEvent::WindowDeactivate) {
    if (isOpeningLink_ && openLinkInBackground_) {
      isOpeningLink_ = false;
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
  } else if (event->type() == QEvent::Hide) {
    if (minimizingTray_  && showTrayIcon_ && !isMinimizeToTray_) {
      emit signalPlaceToTray();
    }
  }

  // pass the event on to the parent class
  return QMainWindow::eventFilter(obj, event);
}

/** @brief Process send link to external browser in background
 *---------------------------------------------------------------------------*/
void MainWindow::slotTimerLinkOpening()
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
void MainWindow::changeEvent(QEvent *event)
{
  if (event->type() == QEvent::WindowStateChange) {
    isOpeningLink_ = false;
    if (isMinimized()) {
      oldState = ((QWindowStateChangeEvent*)event)->oldState();
    } else {
      oldState = windowState();
    }
  } else if (event->type() == QEvent::ActivationChange) {
    if (isActiveWindow() && (behaviorIconTray_ == CHANGE_ICON_TRAY)) {
      traySystem->setIcon(QIcon(":/images/quiterss128"));
    }
  } else if (event->type() == QEvent::LanguageChange) {
    retranslateStrings();
  }
  QMainWindow::changeEvent(event);
}

/** @brief Process placing to tray event
 *---------------------------------------------------------------------------*/
void MainWindow::slotPlaceToTray()
{
  isMinimizeToTray_ = true;
  hide();

  if (emptyWorking_)
    QTimer::singleShot(10000, this, SLOT(myEmptyWorkingSet()));
  if (markReadMinimize_)
    setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_, FeedReadPlaceToTray, currentNewsTab);
  if (clearStatusNew_)
    QTimer::singleShot(0, this, SIGNAL(signalMarkAllFeedsOld()));
  clearNotification(true);

  saveSettings();

  mainApp->updateFeeds()->saveMemoryDatabase();

  isMinimizeToTray_ = false;
}

/** @brief Process tray event
 *---------------------------------------------------------------------------*/
void MainWindow::slotActivationTray(QSystemTrayIcon::ActivationReason reason)
{
  bool activated = false;

  switch (reason) {
  case QSystemTrayIcon::Unknown:
    break;
  case QSystemTrayIcon::Context:
    trayMenu_->activateWindow();
    break;

  case QSystemTrayIcon::DoubleClick:
  {
    if (!singleClickTray_)
    {
      timerTrayOpenNotify.stop();

      if ((QDateTime::currentMSecsSinceEpoch() - activationStateChangedTime_ < 300) || isActiveWindow())
      {
        activated = true;
      }

      showWindows(activated);
    }

    break;
  }

  case QSystemTrayIcon::Trigger:
  {
    if (singleClickTray_)
    {
      qint64 lastActiveChangeTime = QDateTime::currentMSecsSinceEpoch() - activationStateChangedTime_;

      if (isActiveWindow() ? (lastActiveChangeTime >= 200) : (lastActiveChangeTime < 200))
      {
        activated = true;
      }

      showWindows(activated);
    }
    else
    {
      if (notificationWidget == NULL)
      {
        if (!timerTrayOpenNotify.isActive())
        {
          timerTrayOpenNotify.start(400);
        }
      }
      else
      {
        delete notificationWidget;
        notificationWidget = NULL;
      }
    }

    break;
  }

  case QSystemTrayIcon::MiddleClick:
    break;
  }
}
void MainWindow::slotTrayOpenNotifyTimer()
{
  emit signalShowNotification(true);
}

/** @brief Show window on event
 *---------------------------------------------------------------------------*/
void MainWindow::showWindows(bool trayClick)
{
  if (!trayClick || isHidden()) {
    if (oldState & Qt::WindowFullScreen) {
      show();
    } else if (oldState & Qt::WindowMaximized) {
      showMaximized();
    } else {
      showNormal();
      Settings settings;
      restoreGeometry(settings.value("GeometryState").toByteArray());
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
void MainWindow::createFeedsWidget()
{
  feedsModel_ = new FeedsModel(this);

  feedsProxyModel_ = new FeedsProxyModel(feedsModel_);
  feedsProxyModel_->setSourceModel(feedsModel_);

  feedsView_ = new FeedsView(this);
  feedsView_->setModel(feedsProxyModel_);
  feedsView_->setSourceModel(feedsModel_);
  feedsModel_->setView(feedsView_);

  for (int i = 0; i < feedsView_->model()->columnCount(); ++i)
    feedsView_->hideColumn(i);
  feedsView_->showColumn(feedsView_->columnIndex("text"));
#ifdef HAVE_QT5
  feedsView_->header()->setSectionResizeMode(feedsView_->columnIndex("text"), QHeaderView::Stretch);
  feedsView_->header()->setSectionResizeMode(feedsView_->columnIndex("unread"), QHeaderView::ResizeToContents);
  feedsView_->header()->setSectionResizeMode(feedsView_->columnIndex("undeleteCount"), QHeaderView::ResizeToContents);
  feedsView_->header()->setSectionResizeMode(feedsView_->columnIndex("updated"), QHeaderView::ResizeToContents);
#else
  feedsView_->header()->setResizeMode(feedsView_->columnIndex("text"), QHeaderView::Stretch);
  feedsView_->header()->setResizeMode(feedsView_->columnIndex("unread"), QHeaderView::ResizeToContents);
  feedsView_->header()->setResizeMode(feedsView_->columnIndex("undeleteCount"), QHeaderView::ResizeToContents);
  feedsView_->header()->setResizeMode(feedsView_->columnIndex("updated"), QHeaderView::ResizeToContents);
#endif

  feedsToolBar_ = new QToolBar(this);
  feedsToolBar_->setObjectName("feedsToolBar");
  feedsToolBar_->setStyleSheet("QToolBar { border: none; padding: 0px; }");

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

  showCategoriesButton_ = new ToolButton(this);
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
  feedsSplitter_->addWidget(feedsView_);
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

  connect(feedsView_, SIGNAL(pressed(QModelIndex)),
          this, SLOT(slotFeedClicked(QModelIndex)));
  connect(feedsView_, SIGNAL(signalMiddleClicked()),
          this, SLOT(slotOpenFeedNewTab()));
  connect(feedsView_, SIGNAL(signalDoubleClicked()),
          this, SLOT(slotGetFeed()));
  connect(feedsView_, SIGNAL(pressKeyUp()), this, SLOT(slotFeedUpPressed()));
  connect(feedsView_, SIGNAL(pressKeyDown()), this, SLOT(slotFeedDownPressed()));
  connect(feedsView_, SIGNAL(pressKeyHome()), this, SLOT(slotFeedHomePressed()));
  connect(feedsView_, SIGNAL(pressKeyEnd()), this, SLOT(slotFeedEndPressed()));
  connect(feedsView_, SIGNAL(pressKeyPageUp()), this, SLOT(slotFeedPageUpPressed()));
  connect(feedsView_, SIGNAL(pressKeyPageDown()), this, SLOT(slotFeedPageDownPressed()));
  connect(feedsView_, SIGNAL(signalDropped(QModelIndex,int)),
          this, SLOT(slotMoveIndex(QModelIndex,int)));
  connect(feedsView_, SIGNAL(customContextMenuRequested(QPoint)),
          this, SLOT(showContextMenuFeed(const QPoint &)));

  connect(findFeeds_, SIGNAL(textChanged(QString)),
          this, SLOT(slotFindFeeds(QString)));
  connect(findFeeds_, SIGNAL(signalSelectFind()),
          this, SLOT(slotSelectFind()));
  connect(findFeeds_, SIGNAL(returnPressed()),
          this, SLOT(slotSelectFind()));
  connect(findFeeds_, SIGNAL(signalVisible(bool)),
          this, SLOT(findFeedVisible(bool)));

  connect(categoriesTree_, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
          this, SLOT(slotCategoriesClicked(QTreeWidgetItem*,int)));
  connect(categoriesTree_, SIGNAL(signalMiddleClicked()),
          this, SLOT(slotOpenCategoryNewTab()));
  connect(categoriesTree_, SIGNAL(signalClearDeleted()),
          this, SLOT(clearDeleted()));
  connect(categoriesTree_, SIGNAL(signalMarkRead(QTreeWidgetItem*)),
          this, SLOT(slotMarkReadCategory(QTreeWidgetItem*)));
  connect(categoriesTree_, SIGNAL(signalViewAllYoutubeVideos()),
          this, SLOT(viewAllYoutubeVideos()));
  connect(showCategoriesButton_, SIGNAL(clicked()),
          this, SLOT(showNewsCategoriesTree()));
  connect(feedsSplitter_, SIGNAL(splitterMoved(int,int)),
          this, SLOT(feedsSplitterMoved(int,int)));

  categoriesLabel_->installEventFilter(this);
  categoriesTree_->installEventFilter(this);
}
// ---------------------------------------------------------------------------
void MainWindow::createToolBarNull()
{
  pushButtonNull_ = new QPushButton(this);
  pushButtonNull_->setObjectName("pushButtonNull");
  pushButtonNull_->setIcon(QIcon(":/images/images/triangleR.png"));
  pushButtonNull_->setFixedWidth(6);
  pushButtonNull_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  pushButtonNull_->setFocusPolicy(Qt::NoFocus);
}
// ---------------------------------------------------------------------------
void MainWindow::createNewsTab(int index)
{
  currentNewsTab = (NewsTabWidget*)stackedWidget_->widget(index);
  currentNewsTab->setSettings();
  currentNewsTab->retranslateStrings();
  currentNewsTab->setBrowserPosition();

  newsModel_ = currentNewsTab->newsModel_;
  newsView_ = currentNewsTab->newsView_;
}
// ---------------------------------------------------------------------------
void MainWindow::createStatusBar()
{
#if defined(HAVE_X11) || defined(Q_OS_MAC)
  statusBar()->setStyleSheet(QString("QStatusBar::item {border-right: 1px solid %1;"
                                     "margin: 1px;}").
                             arg(qApp->palette().color(QPalette::Dark).name()));
#endif

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

  QToolButton *stopUpdateButton_ = new QToolButton(progressBar_);
  stopUpdateButton_->setFocusPolicy(Qt::NoFocus);
  stopUpdateButton_->setToolButtonStyle(Qt::ToolButtonIconOnly);
  stopUpdateButton_->setFixedSize(15, 15);
  stopUpdateButton_->setCursor(Qt::ArrowCursor);
  stopUpdateButton_->setDefaultAction(stopUpdateAct_);
  stopUpdateButton_->setStyleSheet(
        "QToolButton { border: none; padding: 0px; background: none; }"
        "QToolButton:hover { background: rgba(150, 150, 150, 60) }"
        );
  stopUpdateButton_->move(progressBar_->rect().right() - stopUpdateButton_->sizeHint().width(),
                          progressBar_->rect().top());


  adblockIcon_ = new AdBlockIcon(this);

  QToolButton *loadImagesButton = new QToolButton(this);
  loadImagesButton->setFocusPolicy(Qt::NoFocus);
  loadImagesButton->setIconSize(QSize(16,16));
  loadImagesButton->setDefaultAction(autoLoadImagesToggle_);
  loadImagesButton->setStyleSheet("QToolButton { border: none; padding: 0px; background: none; }");

  QToolButton *fullScreenButton = new QToolButton(this);
  fullScreenButton->setFocusPolicy(Qt::NoFocus);
  loadImagesButton->setIconSize(QSize(16,16));
  fullScreenButton->setDefaultAction(fullScreenAct_);
  fullScreenButton->setStyleSheet("QToolButton { border: none; padding: 0px; background: none; }");
  statusBar()->installEventFilter(this);

  statusBar()->addPermanentWidget(progressBar_);
  statusUnread_ = new QLabel(this);
  statusUnread_->hide();
  statusBar()->addPermanentWidget(statusUnread_);
  statusAll_ = new QLabel(this);
  statusAll_->hide();
  statusBar()->addPermanentWidget(statusAll_);
  statusBar()->addPermanentWidget(adblockIcon_);
  statusBar()->addPermanentWidget(loadImagesButton);
  statusBar()->addPermanentWidget(fullScreenButton);
  statusBar()->setVisible(true);
}
// ---------------------------------------------------------------------------
void MainWindow::createTray()
{
  traySystem = new QSystemTrayIcon(QIcon(":/images/quiterss128"), this);
  traySystem->setToolTip("QuiteRSS");

#ifndef Q_OS_MAC
  connect(traySystem,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this, SLOT(slotActivationTray(QSystemTrayIcon::ActivationReason)));
  connect(this, SIGNAL(signalPlaceToTray()),
          this, SLOT(slotPlaceToTray()), Qt::QueuedConnection);
#endif

  createTrayMenu();
}

/** @brief Create tabbar widget
 *---------------------------------------------------------------------------*/
void MainWindow::createTabBarWidget()
{
  mainMenuButton_ = new ToolButton(this);
  mainMenuButton_->setObjectName("mainMenuButton");
  mainMenuButton_->setIcon(QIcon(":/images/menu"));

  tabBar_ = new TabBar();

  QHBoxLayout *tabBarLayout = new QHBoxLayout();
  tabBarLayout->setContentsMargins(0, 0, 0, 0);
  tabBarLayout->setSpacing(0);
  tabBarLayout->addWidget(mainMenuButton_);
  tabBarLayout->addSpacing(5);
  tabBarLayout->addWidget(tabBar_, 1);

  tabBarWidget_ = new QWidget(this);
  tabBarWidget_->setObjectName("tabBarWidget");
  tabBarWidget_->setMinimumHeight(1);
  tabBarWidget_->setLayout(tabBarLayout);

  connect(mainMenuButton_, SIGNAL(clicked()), this, SLOT(showMainMenu()));
  connect(tabBar_, SIGNAL(closeTab(int)),
          this, SLOT(slotCloseTab(int)));
  connect(tabBar_, SIGNAL(currentChanged(int)),
          this, SLOT(slotTabCurrentChanged(int)), Qt::QueuedConnection);
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

void MainWindow::createCentralWidget()
{
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
        arg(feedsPanel_->palette().window().color().name()).
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
  mainLayout->addWidget(tabBarWidget_);
  mainLayout->addLayout(mainLayout1, 1);

  centralWidget_ = new QWidget(this);
  centralWidget_->setLayout(mainLayout);

  setCentralWidget(centralWidget_);
}

/** @brief Create actions for main menu and toolbar
 *---------------------------------------------------------------------------*/
void MainWindow::createActions()
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

  addFeedTrayAct_ = new QAction(this);
  addFeedTrayAct_->setIcon(QIcon(":/images/feed"));
  connect(addFeedTrayAct_, SIGNAL(triggered()), this, SLOT(addFeed()));

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

  createBackupAct_ = new QAction(this);
  createBackupAct_->setObjectName("createBackupAct");
  createBackupAct_->setIcon(QIcon(":/images/backup"));
  this->addAction(createBackupAct_);
  connect(createBackupAct_, SIGNAL(triggered()), this, SLOT(createBackup()));

  showMenuBarAct_ = new QAction(this);
  showMenuBarAct_->setCheckable(true);
  connect(showMenuBarAct_, SIGNAL(triggered()), this, SLOT(showMenuBar()));

  exitAct_ = new QAction(this);
  exitAct_->setObjectName("exitAct");
  this->addAction(exitAct_);
  connect(exitAct_, SIGNAL(triggered()), this, SLOT(quitApp()));

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
  statusBarToggle_ = new QAction(this);
  statusBarToggle_->setCheckable(true);

  connect(feedsToolbarToggle_, SIGNAL(toggled(bool)),
          feedsPanel_, SLOT(setVisible(bool)));
  connect(categoriesPanelToggle_, SIGNAL(toggled(bool)),
          categoriesWidget_, SLOT(setVisible(bool)));
  connect(statusBarToggle_, SIGNAL(toggled(bool)),
          statusBar(), SLOT(setVisible(bool)));

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

  classicLayoutAct_ = new QAction(this);
  classicLayoutAct_->setObjectName("classicLayoutAct_");
  classicLayoutAct_->setIcon(QIcon(":/images/layout_classic"));
  classicLayoutAct_->setCheckable(true);
  classicLayoutAct_->setData(0);
  newspaperLayoutAct_ = new QAction(this);
  newspaperLayoutAct_->setObjectName("newspaperLayoutAct_");
  newspaperLayoutAct_->setIcon(QIcon(":/images/layout_newspaper"));
  newspaperLayoutAct_->setCheckable(true);
  newspaperLayoutAct_->setData(1);
  layoutToggle_ = new QAction(this);
  layoutToggle_->setObjectName("layoutToggle");
  layoutToggle_->setIcon(QIcon(":/images/layout_classic"));
  this->addAction(layoutToggle_);
  connect(layoutToggle_, SIGNAL(triggered()),
          this, SLOT(setNewsLayout()));

  systemStyle_ = new QAction(this);
  systemStyle_->setObjectName("systemStyle_");
  systemStyle_->setCheckable(true);
  system2Style_ = new QAction(this);
  system2Style_->setObjectName("system2Style_");
  system2Style_->setCheckable(true);
  darkStyle_ = new QAction(this);
  darkStyle_->setObjectName("darkStyle_");
  darkStyle_->setCheckable(true);
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

  savePageAsDescriptAct_ = new QAction(this);
  savePageAsDescriptAct_->setObjectName("savePageAsDescriptAct");
  savePageAsDescriptAct_->setIcon(QIcon(":/images/save_as"));
  this->addAction(savePageAsDescriptAct_);
  connect(savePageAsDescriptAct_, SIGNAL(triggered()),
          this, SLOT(slotSavePageAsDescript()));

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

  stopUpdateAct_ = new QAction(this);
  stopUpdateAct_->setObjectName("stopUpdateAct");
  stopUpdateAct_->setIcon(QIcon(":/images/close"));
  this->addAction(stopUpdateAct_);
  connect(stopUpdateAct_, SIGNAL(triggered()), this, SLOT(slotStopUpdate()));

  markAllFeedsRead_ = new QAction(this);
  markAllFeedsRead_->setObjectName("markAllFeedRead");
  markAllFeedsRead_->setIcon(QIcon(":/images/markReadAll"));
  this->addAction(markAllFeedsRead_);
  connect(markAllFeedsRead_, SIGNAL(triggered()), this, SIGNAL(signalMarkAllFeedsRead()));

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
          feedsView_, SLOT(collapseAll()));

  expandAllFoldersAct_ = new QAction(this);
  expandAllFoldersAct_->setObjectName("expandAllFolderAct");
  expandAllFoldersAct_->setIcon(QIcon(":/images/bulletPlus"));
  this->addAction(expandAllFoldersAct_);
  connect(expandAllFoldersAct_, SIGNAL(triggered()),
          feedsView_, SLOT(expandAll()));

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
  showCleanUpWizardAct_->setIcon(QIcon(":/images/cleanup"));
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
  filterFeedsError_ = new QAction(this);
  filterFeedsError_->setObjectName("filterFeedsError_");
  filterFeedsError_->setCheckable(true);

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

  webWidgetVisibleAct_ = new QAction(this);
  webWidgetVisibleAct_->setObjectName("webWidgetVisibleAct");
  webWidgetVisibleAct_->setCheckable(true);
  this->addAction(webWidgetVisibleAct_);

  showUnreadCount_ = new QAction(this);
  showUnreadCount_->setData(feedsView_->columnIndex("unread"));
  showUnreadCount_->setCheckable(true);
  showUndeleteCount_ = new QAction(this);
  showUndeleteCount_->setData(feedsView_->columnIndex("undeleteCount"));
  showUndeleteCount_->setCheckable(true);
  showLastUpdated_ = new QAction(this);
  showLastUpdated_->setData(feedsView_->columnIndex("updated"));
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

  showLabelsMenuAct_ = new QAction(this);
  showLabelsMenuAct_->setObjectName("showLabelsMenuAct");
  this->addAction(showLabelsMenuAct_);
  connect(showLabelsMenuAct_, SIGNAL(triggered()),
          this, SLOT(slotShowLabelsMenu()));

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

  pageUpWebViewAct_ = new QAction(this);
  pageUpWebViewAct_->setObjectName("pageUpWebViewAct");
  this->addAction(pageUpWebViewAct_);
  connect(pageUpWebViewAct_, SIGNAL(triggered()),
          this, SLOT(slotPageUpWebView()));
  pageDownWebViewAct_ = new QAction(this);
  pageDownWebViewAct_->setObjectName("pageDownWebViewAct");
  this->addAction(pageDownWebViewAct_);
  connect(pageDownWebViewAct_, SIGNAL(triggered()),
          this, SLOT(slotPageDownWebView()));

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

  settingPageLabelsAct_ = new QAction(this);
  settingPageLabelsAct_->setObjectName("settingPageLabelsAct");
  this->addAction(settingPageLabelsAct_);
  connect(settingPageLabelsAct_, SIGNAL(triggered()), this, SLOT(showSettingPageLabels()));

  backWebPageAct_ = new QAction(this);
  backWebPageAct_->setObjectName("backWebPageAct");
  forwardWebPageAct_ = new QAction(this);
  forwardWebPageAct_->setObjectName("forwardWebPageAct");
  reloadWebPageAct_ = new QAction(this);
  reloadWebPageAct_->setObjectName("reloadWebPageAct");

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
  evernoteShareAct_->setIcon(QIcon(":/share/images/share/evernote.png"));
  shareGroup_->addAction(evernoteShareAct_);

  facebookShareAct_ = new QAction(this);
  facebookShareAct_->setObjectName("facebookShareAct");
  facebookShareAct_->setText("Facebook");
  facebookShareAct_->setIcon(QIcon(":/share/images/share/facebook.png"));
  shareGroup_->addAction(facebookShareAct_);

  livejournalShareAct_ = new QAction(this);
  livejournalShareAct_->setObjectName("livejournalShareAct");
  livejournalShareAct_->setText("LiveJournal");
  livejournalShareAct_->setIcon(QIcon(":/share/images/share/livejournal.png"));
  shareGroup_->addAction(livejournalShareAct_);

  pocketShareAct_ = new QAction(this);
  pocketShareAct_->setObjectName("pocketShareAct");
  pocketShareAct_->setText("Pocket");
  pocketShareAct_->setIcon(QIcon(":/share/images/share/pocket.png"));
  shareGroup_->addAction(pocketShareAct_);

  twitterShareAct_ = new QAction(this);
  twitterShareAct_->setObjectName("twitterShareAct");
  twitterShareAct_->setText("Twitter");
  twitterShareAct_->setIcon(QIcon(":/share/images/share/twitter.png"));
  shareGroup_->addAction(twitterShareAct_);

  vkShareAct_ = new QAction(this);
  vkShareAct_->setObjectName("vkShareAct");
  vkShareAct_->setText("VK");
  vkShareAct_->setIcon(QIcon(":/share/images/share/vk.png"));
  shareGroup_->addAction(vkShareAct_);

  linkedinShareAct_ = new QAction(this);
  linkedinShareAct_->setObjectName("linkedinShareAct");
  linkedinShareAct_->setText("LinkedIn");
  linkedinShareAct_->setIcon(QIcon(":/share/images/share/linkedin.png"));
  shareGroup_->addAction(linkedinShareAct_);

  bloggerShareAct_ = new QAction(this);
  bloggerShareAct_->setObjectName("bloggerShareAct");
  bloggerShareAct_->setText("Blogger");
  bloggerShareAct_->setIcon(QIcon(":/share/images/share/blogger.png"));
  shareGroup_->addAction(bloggerShareAct_);

  printfriendlyShareAct_ = new QAction(this);
  printfriendlyShareAct_->setObjectName("printfriendlyShareAct");
  printfriendlyShareAct_->setText("PrintFriendly");
  printfriendlyShareAct_->setIcon(QIcon(":/share/images/share/printfriendly.png"));
  shareGroup_->addAction(printfriendlyShareAct_);

  instapaperShareAct_ = new QAction(this);
  instapaperShareAct_->setObjectName("instapaperShareAct");
  instapaperShareAct_->setText("Instapaper");
  instapaperShareAct_->setIcon(QIcon(":/share/images/share/instapaper.png"));
  shareGroup_->addAction(instapaperShareAct_);

  redditShareAct_ = new QAction(this);
  redditShareAct_->setObjectName("redditShareAct");
  redditShareAct_->setText("Reddit");
  redditShareAct_->setIcon(QIcon(":/share/images/share/reddit.ico"));
  shareGroup_->addAction(redditShareAct_);

  hackerNewsShareAct_ = new QAction(this);
  hackerNewsShareAct_->setObjectName("hackerNewsShareAct");
  hackerNewsShareAct_->setText("HackerNews");
  hackerNewsShareAct_->setIcon(QIcon(":/share/images/share/hackernews.png"));
  shareGroup_->addAction(hackerNewsShareAct_);

  telegramShareAct_ = new QAction(this);
  telegramShareAct_->setObjectName("telegramShareAct");
  telegramShareAct_->setText("Telegram");
  telegramShareAct_->setIcon(QIcon(":/share/images/share/telegram.png"));
  shareGroup_->addAction(telegramShareAct_);

  viberShareAct_ = new QAction(this);
  viberShareAct_->setObjectName("viberShareAct");
  viberShareAct_->setText("Viber");
  viberShareAct_->setIcon(QIcon(":/share/images/share/viber.png"));
  shareGroup_->addAction(viberShareAct_);

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
void MainWindow::createShortcut()
{
  addFeedAct_->setShortcut(QKeySequence(QKeySequence::New));
  listActions_.append(addFeedAct_);
  addFolderAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_N));
  listActions_.append(addFolderAct_);
  listActions_.append(deleteFeedAct_);
  listActions_.append(createBackupAct_);
  exitAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));  // standart on other OS
  listActions_.append(exitAct_);
  updateFeedAct_->setShortcut(QKeySequence(Qt::Key_F5));
  listActions_.append(updateFeedAct_);
  updateAllFeedsAct_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F5));
  listActions_.append(updateAllFeedsAct_);
  listActions_.append(stopUpdateAct_);
  listActions_.append(openHomeFeedAct_);
  listActions_.append(showDownloadManagerAct_);
  listActions_.append(showCleanUpWizardAct_);
  listActions_.append(setNewsFiltersAct_);
  listActions_.append(setFilterNewsAct_);
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
  listActions_.append(createBackupAct_);
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
  listActions_.append(webWidgetVisibleAct_);

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
  listActions_.append(savePageAsDescriptAct_);

  fullScreenAct_->setShortcut(QKeySequence(Qt::Key_F11));
  listActions_.append(fullScreenAct_);

  stayOnTopAct_->setShortcut(QKeySequence(Qt::Key_F10));
  listActions_.append(stayOnTopAct_);

  listActions_.append(layoutToggle_);

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

  listActions_.append(findFeedAct_);

  listActions_.append(copyLinkAct_);

  listActions_.append(backWebPageAct_);
  listActions_.append(forwardWebPageAct_);
  listActions_.append(reloadWebPageAct_);
  listActions_.append(pageUpWebViewAct_);
  listActions_.append(pageDownWebViewAct_);

  listActions_.append(shareGroup_->actions());

  // Actions for labels do add at the end
  listActions_.append(settingPageLabelsAct_);
  listActions_.append(showLabelsMenuAct_);
  listActions_.append(newsLabelGroup_->actions());

  loadActionShortcuts();
}
// ---------------------------------------------------------------------------
void MainWindow::loadActionShortcuts()
{
  Settings settings;
  settings.beginGroup("/Shortcuts");

  QListIterator<QAction *> iter(listActions_);
  while (iter.hasNext()) {
    QAction *pAction = iter.next();
    if (pAction->objectName().isEmpty())
      continue;

    listDefaultShortcut_.append(pAction->shortcut().toString());

    const QString& sKey = '/' + pAction->objectName();
    const QString& sValue = settings.value('/' + sKey, pAction->shortcut().toString()).toString();
    pAction->setShortcut(QKeySequence(sValue));
  }

  settings.endGroup();
}
// ---------------------------------------------------------------------------
void MainWindow::saveActionShortcuts()
{
  Settings settings;
  settings.beginGroup("/Shortcuts/");

  QListIterator<QAction *> iter(listActions_);
  while (iter.hasNext()) {
    QAction *pAction = iter.next();
    if (pAction->objectName().isEmpty())
      continue;

    const QString& sKey = '/' + pAction->objectName();
    const QString& sValue = QString(pAction->shortcut().toString());
    settings.setValue(sKey, sValue);
  }

  settings.endGroup();
}
// ---------------------------------------------------------------------------
void MainWindow::createMenu()
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
  fileMenu_->addAction(createBackupAct_);
  fileMenu_->addSeparator();
#ifndef Q_OS_MAC
  fileMenu_->addAction(showMenuBarAct_);
  fileMenu_->addSeparator();
#endif
  fileMenu_->addAction(exitAct_);

  toolbarsMenu_ = new QMenu(this);
  toolbarsMenu_->addAction(feedsWidgetVisibleAct_);
  toolbarsMenu_->addAction(webWidgetVisibleAct_);
  toolbarsMenu_->addAction(mainToolbarToggle_);
  toolbarsMenu_->addAction(feedsToolbarToggle_);
  toolbarsMenu_->addAction(newsToolbarToggle_);
  toolbarsMenu_->addAction(browserToolbarToggle_);
  toolbarsMenu_->addAction(categoriesPanelToggle_);
  toolbarsMenu_->addAction(statusBarToggle_);

  customizeToolbarGroup_ = new QActionGroup(this);
  customizeToolbarGroup_->addAction(customizeMainToolbarAct_);
  customizeToolbarGroup_->addAction(customizeFeedsToolbarAct_);
  customizeToolbarGroup_->addAction(customizeNewsToolbarAct_);
  customizeToolbarMenu_ = new QMenu(this);
  customizeToolbarMenu_->addActions(customizeToolbarGroup_->actions());

  layoutGroup_ = new QActionGroup(this);
  layoutGroup_->addAction(classicLayoutAct_);
  layoutGroup_->addAction(newspaperLayoutAct_);
  layoutMenu_ = new QMenu(this);
  layoutMenu_->addActions(layoutGroup_->actions());
  layoutToggle_->setMenu(layoutMenu_);

  styleGroup_ = new QActionGroup(this);
  styleGroup_->addAction(systemStyle_);
  styleGroup_->addAction(system2Style_);
  styleGroup_->addAction(darkStyle_);
  styleGroup_->addAction(greenStyle_);
  styleGroup_->addAction(orangeStyle_);
  styleGroup_->addAction(purpleStyle_);
  styleGroup_->addAction(pinkStyle_);
  styleGroup_->addAction(grayStyle_);

  styleMenu_ = new QMenu(this);
  styleMenu_->addActions(styleGroup_->actions());

  browserPositionGroup_ = new QActionGroup(this);
  browserPositionGroup_->addAction(topBrowserPositionAct_);
  browserPositionGroup_->addAction(bottomBrowserPositionAct_);
  browserPositionGroup_->addAction(rightBrowserPositionAct_);
  browserPositionGroup_->addAction(leftBrowserPositionAct_);

  browserPositionMenu_ = new QMenu(this);
  browserPositionMenu_->addActions(browserPositionGroup_->actions());

  viewMenu_  = new QMenu(this);
  viewMenu_->addMenu(toolbarsMenu_);
  viewMenu_->addMenu(customizeToolbarMenu_);
  viewMenu_->addSeparator();
  viewMenu_->addMenu(layoutMenu_);
  viewMenu_->addMenu(browserPositionMenu_);
  viewMenu_->addMenu(styleMenu_);
  viewMenu_->addSeparator();
  viewMenu_->addAction(stayOnTopAct_);
  viewMenu_->addAction(fullScreenAct_);

  feedsFilterGroup_ = new QActionGroup(this);
  feedsFilterGroup_->setExclusive(true);
  feedsFilterGroup_->addAction(filterFeedsAll_);
  feedsFilterGroup_->addAction(filterFeedsNew_);
  feedsFilterGroup_->addAction(filterFeedsUnread_);
  feedsFilterGroup_->addAction(filterFeedsStarred_);
  feedsFilterGroup_->addAction(filterFeedsError_);

  feedsFilterMenu_ = new QMenu(this);
  feedsFilterMenu_->addActions(feedsFilterGroup_->actions());
  feedsFilterMenu_->insertSeparator(filterFeedsNew_);

  feedsFilter_->setMenu(feedsFilterMenu_);

  feedsColumnsGroup_ = new QActionGroup(this);
  feedsColumnsGroup_->setExclusive(false);
  feedsColumnsGroup_->addAction(showUnreadCount_);
  feedsColumnsGroup_->addAction(showUndeleteCount_);
  feedsColumnsGroup_->addAction(showLastUpdated_);

  feedsColumnsMenu_ = new QMenu(this);
  feedsColumnsMenu_->addActions(feedsColumnsGroup_->actions());

  feedMenu_ = new QMenu(this);
  feedMenu_->addAction(updateFeedAct_);
  feedMenu_->addAction(updateAllFeedsAct_);
  feedMenu_->addSeparator();
  feedMenu_->addAction(markFeedRead_);
  feedMenu_->addAction(markAllFeedsRead_);
  feedMenu_->addSeparator();
  feedMenu_->addAction(feedsFilter_);
  feedMenu_->addMenu(feedsColumnsMenu_);
  feedMenu_->addAction(sortedByTitleFeedsTreeAct_);
  feedMenu_->addAction(indentationFeedsTreeAct_);
  feedMenu_->addSeparator();
  feedMenu_->addAction(deleteFeedAct_);
  feedMenu_->addSeparator();
  feedMenu_->addAction(feedProperties_);
  feedMenu_->addSeparator();

  newsLabelMenu_ = new QMenu(this);
  newsLabelMenu_->addActions(newsLabelGroup_->actions());
  newsLabelMenuAction_ = new QAction(this);
  newsLabelMenuAction_->setIcon(QIcon(":/images/label_3"));
  newsLabelAction_->setMenu(newsLabelMenu_);
  newsLabelMenuAction_->setMenu(newsLabelMenu_);

  shareMenu_ = new QMenu(this);
  shareMenu_->addActions(shareGroup_->actions());
  shareMenuAct_ = new QAction(this);
  shareMenuAct_->setObjectName("shareMenuAct");
  shareMenuAct_->setIcon(QIcon(":/images/images/share.png"));
  shareMenuAct_->setMenu(shareMenu_);
  this->addAction(shareMenuAct_);

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

  newsFilterMenu_ = new QMenu(this);
  newsFilterMenu_->addActions(newsFilterGroup_->actions());
  newsFilterMenu_->insertSeparator(filterNewsNew_);
  newsFilterMenu_->insertSeparator(filterNewsLastDay_);

  newsFilter_->setMenu(newsFilterMenu_);

  newsSortByMenu_ = new QMenu(this);
  newsSortByMenu_->addSeparator();
  newsSortByMenu_->addActions(newsSortOrderGroup_->actions());

  newsMenu_ = new QMenu(this);
  newsMenu_->addAction(markNewsRead_);
  newsMenu_->addAction(markAllNewsRead_);
  newsMenu_->addSeparator();
  newsMenu_->addAction(markStarAct_);
  newsMenu_->addAction(newsLabelMenuAction_);
  newsMenu_->addAction(shareMenuAct_);
  newsMenu_->addSeparator();
  newsMenu_->addAction(newsFilter_);
  newsMenu_->addMenu(newsSortByMenu_);
  newsMenu_->addSeparator();
  newsMenu_->addAction(deleteNewsAct_);
  newsMenu_->addAction(deleteAllNewsAct_);

  browserZoomGroup_ = new QActionGroup(this);
  browserZoomGroup_->addAction(zoomInAct_);
  browserZoomGroup_->addAction(zoomOutAct_);
  browserZoomGroup_->addAction(zoomTo100Act_);

  browserZoomMenu_ = new QMenu(this);
  browserZoomMenu_->setIcon(QIcon(":/images/zoom"));
  browserZoomMenu_->addActions(browserZoomGroup_->actions());
  browserZoomMenu_->insertSeparator(zoomTo100Act_);

  browserMenu_ = new QMenu(this);
  browserMenu_->addAction(autoLoadImagesToggle_);
  browserMenu_->addMenu(browserZoomMenu_);
  browserMenu_->addSeparator();
  browserMenu_->addAction(printAct_);
  browserMenu_->addAction(printPreviewAct_);
  browserMenu_->addSeparator();
  browserMenu_->addAction(savePageAsAct_);
  browserMenu_->addSeparator();
  browserMenu_->addAction(tr("&AdBlock"), AdBlockManager::instance(), SLOT(showDialog()));

  toolsMenu_ = new QMenu(this);
  toolsMenu_->addAction(showDownloadManagerAct_);
  toolsMenu_->addSeparator();
  toolsMenu_->addAction(showCleanUpWizardAct_);
  toolsMenu_->addAction(setNewsFiltersAct_);
  toolsMenu_->addSeparator();
  toolsMenu_->addAction(optionsAct_);

  helpMenu_ = new QMenu(this);
  helpMenu_->addAction(updateAppAct_);
  helpMenu_->addSeparator();
  helpMenu_->addAction(reportProblemAct_);
  helpMenu_->addAction(aboutAct_);

#ifndef Q_OS_MAC
  mainMenu_ = new QMenu(this);
  mainMenu_->addAction(addAct_);
  mainMenu_->addSeparator();
  mainMenu_->addAction(importFeedsAct_);
  mainMenu_->addAction(exportFeedsAct_);
  mainMenu_->addSeparator();
  mainMenu_->addAction(createBackupAct_);
  mainMenu_->addSeparator();
  mainMenu_->addMenu(viewMenu_);
  mainMenu_->addMenu(feedMenu_);
  mainMenu_->addMenu(newsMenu_);
  mainMenu_->addMenu(browserMenu_);
  mainMenu_->addSeparator();
  mainMenu_->addMenu(toolsMenu_);
  mainMenu_->addSeparator();
  mainMenu_->addMenu(helpMenu_);
  mainMenu_->addSeparator();
  mainMenu_->addAction(showMenuBarAct_);
  mainMenu_->addSeparator();
  mainMenu_->addAction(exitAct_);
#endif

  menuBar()->addMenu(fileMenu_);
  menuBar()->addMenu(viewMenu_);
  menuBar()->addMenu(feedMenu_);
  menuBar()->addMenu(newsMenu_);
  menuBar()->addMenu(browserMenu_);
  menuBar()->addMenu(toolsMenu_);
  menuBar()->addMenu(helpMenu_);

  connect(customizeToolbarGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(showCustomizeToolbarDlg(QAction*)));
  connect(styleGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(setStyleApp(QAction*)));
  connect(browserPositionGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(setBrowserPosition(QAction*)));
  connect(feedsFilterGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(setFeedsFilter()));
  connect(feedsFilter_, SIGNAL(triggered()), this, SLOT(slotFeedsFilter()));
  connect(feedsColumnsGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(feedsColumnVisible(QAction*)));
  connect(feedMenu_, SIGNAL(aboutToShow()), this, SLOT(slotFeedMenuShow()));
  connect(newsLabelMenu_, SIGNAL(aboutToShow()), this, SLOT(getLabelNews()));
  connect(shareMenuAct_, SIGNAL(triggered()), this, SLOT(showMenuShareNews()));
  connect(newsFilterGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(setNewsFilter(QAction*)));
  connect(newsFilter_, SIGNAL(triggered()), this, SLOT(slotNewsFilter()));
  connect(newsSortByMenu_, SIGNAL(aboutToShow()),
          this, SLOT(showNewsSortByMenu()));
  connect(newsMenu_, SIGNAL(aboutToShow()), this, SLOT(showNewsMenu()));
  connect(browserZoomGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(browserZoom(QAction*)));
}
// ---------------------------------------------------------------------------
void MainWindow::createToolBar()
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

/** @brief Load settings from ini-file
 *---------------------------------------------------------------------------*/
void MainWindow::loadSettings()
{
  Settings settings;
  settings.beginGroup("Settings");

  showSplashScreen_ = settings.value("showSplashScreen", true).toBool();
  reopenFeedStartup_ = settings.value("reopenFeedStartup", true).toBool();
  openNewTabNextToActive_ = settings.value("openNewTabNextToActive", false).toBool();

  showTrayIcon_ = settings.value("showTrayIcon", true).toBool();
#ifndef Q_OS_MAC
  startingTray_ = settings.value("startingTray", false).toBool();
  minimizingTray_ = settings.value("minimizingTray", true).toBool();
  closingTray_ = settings.value("closingTray", false).toBool();
#else
  startingTray_ = false;
  minimizingTray_ = false;
  closingTray_ = false;
#endif
  singleClickTray_ = settings.value("singleClickTray", false).toBool();
  clearStatusNew_ = settings.value("clearStatusNew", true).toBool();
  emptyWorking_ = settings.value("emptyWorking", true).toBool();

  QString fontFamily = settings.value("feedsFontFamily", qApp->font().family()).toString();
  int fontSize = settings.value("feedsFontSize", qApp->font().pointSize()).toInt();
  feedsView_->setFont(QFont(fontFamily, fontSize));
  feedsModel_->font_ = feedsView_->font();
  categoriesTree_->setFont(QFont(fontFamily, fontSize));

  newsListFontFamily_ = settings.value("newsFontFamily", qApp->font().family()).toString();
  newsListFontSize_ = settings.value("newsFontSize", qApp->font().pointSize()).toInt();
  newsTitleFontFamily_ = settings.value("newsTitleFontFamily", qApp->font().family()).toString();
  newsTitleFontSize_ = settings.value("newsTitleFontSize", qApp->font().pointSize()+2).toInt();
  newsTextFontFamily_ = settings.value("newsTextFontFamily", qApp->font().family()).toString();
  newsTextFontSize_ = settings.value("newsTextFontSize", qApp->font().pointSize()+2).toInt();
  notificationFontFamily_ = settings.value("notificationFontFamily", qApp->font().family()).toString();
  notificationFontSize_ = settings.value("notificationFontSize", qApp->font().pointSize()).toInt();

  QString browserStandardFont = settings.value(
        "browserStandardFont", QWebSettings::globalSettings()->fontFamily(QWebSettings::StandardFont)).toString();
  QString browserFixedFont = settings.value(
        "browserFixedFont", QWebSettings::globalSettings()->fontFamily(QWebSettings::FixedFont)).toString();
  QString browserSerifFont = settings.value(
        "browserSerifFont", QWebSettings::globalSettings()->fontFamily(QWebSettings::SerifFont)).toString();
  QString browserSansSerifFont = settings.value(
        "browserSansSerifFont", QWebSettings::globalSettings()->fontFamily(QWebSettings::SansSerifFont)).toString();
  QString browserCursiveFont = settings.value(
        "browserCursiveFont", QWebSettings::globalSettings()->fontFamily(QWebSettings::CursiveFont)).toString();
  QString browserFantasyFont = settings.value(
        "browserFantasyFont", QWebSettings::globalSettings()->fontFamily(QWebSettings::FantasyFont)).toString();
  int browserDefaultFontSize = settings.value(
        "browserDefaultFontSize", QWebSettings::globalSettings()->fontSize(QWebSettings::DefaultFontSize)).toInt();
  int browserFixedFontSize = settings.value(
        "browserFixedFontSize", QWebSettings::globalSettings()->fontSize(QWebSettings::DefaultFixedFontSize)).toInt();
  int browserMinFontSize = settings.value(
        "browserMinFontSize", QWebSettings::globalSettings()->fontSize(QWebSettings::MinimumFontSize)).toInt();
  int browserMinLogFontSize = settings.value(
        "browserMinLogFontSize", QWebSettings::globalSettings()->fontSize(QWebSettings::MinimumLogicalFontSize)).toInt();

  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::StandardFont, browserStandardFont);
  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::FixedFont, browserFixedFont);
  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::SerifFont, browserSerifFont);
  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::SansSerifFont, browserSansSerifFont);
  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::CursiveFont, browserCursiveFont);
  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::FantasyFont, browserFantasyFont);
  QWebSettings::globalSettings()->setFontSize(
        QWebSettings::DefaultFontSize, browserDefaultFontSize);
  QWebSettings::globalSettings()->setFontSize(
        QWebSettings::DefaultFixedFontSize, browserFixedFontSize);
  QWebSettings::globalSettings()->setFontSize(
        QWebSettings::MinimumFontSize, browserMinFontSize);
  QWebSettings::globalSettings()->setFontSize(
        QWebSettings::MinimumLogicalFontSize, browserMinLogFontSize);

  updateFeedsEnable_ = settings.value("autoUpdatefeeds", false).toBool();
  updateFeedsInterval_ = settings.value("autoUpdatefeedsTime", 10).toInt();
  updateFeedsIntervalType_ = settings.value("autoUpdatefeedsInterval", 0).toInt();

  openingFeedAction_ = settings.value("openingFeedAction", 0).toInt();
  openNewsWebViewOn_ = settings.value("openNewsWebViewOn", true).toBool();

  markNewsReadOn_ = settings.value("markNewsReadOn", true).toBool();
  markCurNewsRead_ = settings.value("markCurNewsRead", true).toBool();
  markNewsReadTime_ = settings.value("markNewsReadTime", 0).toInt();
  markPrevNewsRead_= settings.value("markPrevNewsRead", false).toBool();
  markReadSwitchingFeed_ = settings.value("markReadSwitchingFeed", false).toBool();
  markReadClosingTab_ = settings.value("markReadClosingTab", false).toBool();
  markReadMinimize_ = settings.value("markReadMinimize", false).toBool();

  showDescriptionNews_ = settings.value("showDescriptionNews", true).toBool();

  formatDate_ = settings.value("formatData", "dd.MM.yy").toString();
  formatTime_ = settings.value("formatTime", "hh:mm").toString();
  feedsModel_->formatDate_ = formatDate_;
  feedsModel_->formatTime_ = formatTime_;

  alternatingRowColorsNews_ = settings.value("alternatingColorsNews", false).toBool();
  changeBehaviorActionNUN_ = settings.value("changeBehaviorActionNUN", false).toBool();
  simplifiedDateTime_ = settings.value("simplifiedDateTime", true).toBool();
  notDeleteStarred_ = settings.value("notDeleteStarred", false).toBool();
  notDeleteLabeled_ = settings.value("notDeleteLabeled", false).toBool();
  markIdenticalNewsRead_ = settings.value("markIdenticalNewsRead", true).toBool();
  avoidOldNews_ = settings.value("avoidOldNews", false).toBool();
  avoidedOldNewsDate_ = settings.value("avoidedOldNewsDate").toDate();

  mainNewsFilter_ = settings.value("mainNewsFilter", "filterNewsAll_").toString();

  cleanupOnShutdown_ = settings.value("cleanupOnShutdown", true).toBool();
  maxDayCleanUp_ = settings.value("maxDayClearUp", 30).toInt();
  maxNewsCleanUp_ = settings.value("maxNewsClearUp", 200).toInt();
  dayCleanUpOn_ = settings.value("dayClearUpOn", true).toBool();
  newsCleanUpOn_ = settings.value("newsClearUpOn", true).toBool();
  readCleanUp_ = settings.value("readClearUp", false).toBool();
  neverUnreadCleanUp_ = settings.value("neverUnreadClearUp", true).toBool();
  neverStarCleanUp_ = settings.value("neverStarClearUp", true).toBool();
  neverLabelCleanUp_ = settings.value("neverLabelClearUp", true).toBool();
  cleanUpDeleted_ = settings.value("cleanUpDeleted", false).toBool();
  optimizeDB_ = settings.value("optimizeDB", false).toBool();

  externalBrowserOn_ = settings.value("externalBrowserOn", 0).toInt();
  externalBrowser_ = settings.value("externalBrowser", "").toString();
  javaScriptEnable_ = settings.value("javaScriptEnable", true).toBool();
  pluginsEnable_ = settings.value("pluginsEnable", true).toBool();
  maxPagesInCache_ = settings.value("maxPagesInCache", 3).toInt();
  downloadLocation_ = settings.value("downloadLocation", "").toString();
  askDownloadLocation_ = settings.value("askDownloadLocation", true).toBool();
  defaultZoomPages_ = settings.value("defaultZoomPages", 100).toInt();
  autoLoadImages_ = settings.value("autoLoadImages", true).toBool();

  QWebSettings::globalSettings()->setAttribute(
        QWebSettings::JavascriptEnabled, javaScriptEnable_);
  QWebSettings::globalSettings()->setAttribute(
        QWebSettings::PluginsEnabled, pluginsEnable_);
  QWebSettings::globalSettings()->setMaximumPagesInCache(maxPagesInCache_);
#ifdef WEBKIT_ALPHA
  QWebSettings::globalSettings()->setAttribute(
        QWebSettings::ErrorPageEnabled, false);
#endif
  QWebSettings::globalSettings()->setOfflineStorageDefaultQuota(0);
  QWebSettings::globalSettings()->setOfflineStoragePath(mainApp->dataDir());

  soundNewNews_ = settings.value("soundNewNews", true).toBool();
  soundNotifyPath_ = settings.value("soundNotifyPath", mainApp->soundNotifyDefaultFile()).toString();
  showNotifyOn_ = settings.value("showNotifyOn", true).toBool();
  screenNotify_ = settings.value("screenNotify", 0).toInt();
  positionNotify_ = settings.value("positionNotify", 3).toInt();
  transparencyNotify_ = settings.value("transparencyNotify", 60).toInt();
  countShowNewsNotify_ = settings.value("countShowNewsNotify", 10).toInt();
  widthTitleNewsNotify_ = settings.value("widthTitleNewsNotify", 300).toInt();
  timeShowNewsNotify_ = settings.value("timeShowNewsNotify", 10).toInt();
  fullscreenModeNotify_ = settings.value("fullscreenModeNotify", true).toBool();
  showNotifyInactiveApp_ = settings.value("showNotifyInactiveApp", true).toBool();
  onlySelectedFeeds_ = settings.value("onlySelectedFeeds", false).toBool();
  showTitlesFeedsNotify_ = settings.value("showTitlesFeedsNotify", true).toBool();
  showIconFeedNotify_ = settings.value("showIconFeedNotify", false).toBool();
  showButtonMarkAllNotify_ = settings.value("showButtonMarkAllNotify", true).toBool();
  showButtonMarkReadNotify_ = settings.value("showButtonMarkReadNotify", true).toBool();
  showButtonExBrowserNotify_ = settings.value("showButtonExBrowserNotify", true).toBool();
  showButtonDeleteNotify_ = settings.value("showButtonDeleteNotify", true).toBool();
  closeNotify_ = settings.value("closeNotify", true).toBool();

  toolBarLockAct_->setChecked(settings.value("mainToolbarLock", true).toBool());
  lockMainToolbar(toolBarLockAct_->isChecked());

  mainToolbarToggle_->setChecked(settings.value("mainToolbarShow2", false).toBool());
  feedsToolbarToggle_->setChecked(settings.value("feedsToolbarShow2", true).toBool());
  newsToolbarToggle_->setChecked(settings.value("newsToolbarShow", true).toBool());
  browserToolbarToggle_->setChecked(settings.value("browserToolbarShow", true).toBool());
  categoriesPanelToggle_->setChecked(settings.value("categoriesPanelShow", true).toBool());
  categoriesWidget_->setVisible(categoriesPanelToggle_->isChecked());
  statusBarToggle_->setChecked(settings.value("statusBarShow", true).toBool());

  QString str = settings.value("mainToolBar",
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

  str = settings.value("feedsToolBar2",
                               "newAct,Separator,updateAllFeedsAct,markFeedRead,"
                               "Separator,feedsFilter,findFeedAct").toString();

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

  setToolBarStyle(settings.value("toolBarStyle", "toolBarStyleTuI_").toString());
  QString iconStr = settings.value("toolBarIconSize", "toolBarIconNormal_").toString();
  setToolBarIconSize(mainToolbar_, iconStr);
  iconStr = settings.value("feedsToolBarIconSize", "toolBarIconSmall_").toString();
  setToolBarIconSize(feedsToolBar_, iconStr);

  str = settings.value("styleApplication", "defaultStyle_").toString();
  QList<QAction*> listActions = styleGroup_->actions();
  foreach(QAction *action, listActions) {
    if (action->objectName() == str) {
      action->setChecked(true);
      break;
    }
  }

  showUnreadCount_->setChecked(settings.value("showUnreadCount", true).toBool());
  showUndeleteCount_->setChecked(settings.value("showUndeleteCount", false).toBool());
  showLastUpdated_->setChecked(settings.value("showLastUpdated", false).toBool());
  feedsColumnVisible(showUnreadCount_);
  feedsColumnVisible(showUndeleteCount_);
  feedsColumnVisible(showLastUpdated_);

  indentationFeedsTreeAct_->setChecked(settings.value("indentationFeedsTree", true).toBool());
  slotIndentationFeedsTree();

  newsLayout_ = settings.value("newsLayout", 0).toInt();
  switch (newsLayout_) {
  case 1:
    newspaperLayoutAct_->setChecked(true);
    layoutToggle_->setIcon(QIcon(":/images/layout_newspaper"));
    break;
  default:
    classicLayoutAct_->setChecked(true);
    layoutToggle_->setIcon(QIcon(":/images/layout_classic"));
  }
  connect(layoutGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(setNewsLayout(QAction*)));

  browserPosition_ = settings.value("browserPosition", BOTTOM_POSITION).toInt();
  switch (browserPosition_) {
  case TOP_POSITION:   topBrowserPositionAct_->setChecked(true); break;
  case RIGHT_POSITION: rightBrowserPositionAct_->setChecked(true); break;
  case LEFT_POSITION:  leftBrowserPositionAct_->setChecked(true); break;
  default: bottomBrowserPositionAct_->setChecked(true);
  }

  openLinkInBackground_ = settings.value("openLinkInBackground", true).toBool();
  openLinkInBackgroundEmbedded_ = settings.value("openLinkInBackgroundEmbedded", true).toBool();
  openingLinkTimeout_ = settings.value("openingLinkTimeout", 1000).toInt();

  stayOnTopAct_->setChecked(settings.value("stayOnTop", false).toBool());
  if (stayOnTopAct_->isChecked())
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
  else
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);

  hideFeedsOpenTab_ = settings.value("hideFeedsOpenTab", false).toBool();
  showToggleFeedsTree_ = settings.value("showToggleFeedsTree", true).toBool();
  pushButtonNull_->setVisible(showToggleFeedsTree_);

  defaultIconFeeds_ = settings.value("defaultIconFeeds", false).toBool();
  feedsModel_->defaultIconFeeds_ = defaultIconFeeds_;
  feedsView_->autocollapseFolder_ =
      settings.value("autocollapseFolder", false).toBool();

#ifndef Q_OS_MAC
  showMenuBarAct_->setChecked(settings.value("showMenuBar", false).toBool());
#else
  showMenuBarAct_->setChecked(true);
#endif

  settings.endGroup();

  settings.beginGroup("Color");
  QString windowTextColor = qApp->palette().brush(QPalette::WindowText).color().name();
  QString linkTextColor = qApp->palette().brush(QPalette::Link).color().name();
  feedsModel_->textColor_ = settings.value("feedsListTextColor", windowTextColor).toString();
  feedsModel_->backgroundColor_ = settings.value("feedsListBackgroundColor", "").toString();
  feedsView_->setStyleSheet(QString("#feedsView_ {background: %1;}").arg(feedsModel_->backgroundColor_));
  newsListTextColor_ = settings.value("newsListTextColor", windowTextColor).toString();
  newsListBackgroundColor_ = settings.value("newsListBackgroundColor", "").toString();
  newNewsTextColor_ = settings.value("newNewsTextColor", windowTextColor).toString();
  unreadNewsTextColor_ = settings.value("unreadNewsTextColor", windowTextColor).toString();
  focusedNewsTextColor_ = settings.value("focusedNewsTextColor", windowTextColor).toString();
  focusedNewsBGColor_ = settings.value("focusedNewsBGColor", "").toString();
  linkColor_ = settings.value("linkColor", "#0066CC").toString();
  titleColor_ = settings.value("titleColor", "#0066CC").toString();
  dateColor_ = settings.value("dateColor", "#666666").toString();
  authorColor_ = settings.value("authorColor", "#666666").toString();
  newsTextColor_ = settings.value("newsTextColor", "#000000").toString();
  newsTitleBackgroundColor_ = settings.value("newsTitleBackgroundColor", "#FFFFFF").toString();
  newsBackgroundColor_ = settings.value("newsBackgroundColor", "#FFFFFF").toString();
  feedsModel_->feedWithNewNewsColor_ = settings.value("feedWithNewNewsColor", linkTextColor).toString();
  feedsModel_->countNewsUnreadColor_ = settings.value("countNewsUnreadColor", linkTextColor).toString();
  feedsModel_->focusedFeedTextColor_ = settings.value("focusedFeedTextColor", windowTextColor).toString();
  feedsModel_->focusedFeedBGColor_ = settings.value("focusedFeedBGColor", "").toString();
  feedsModel_->feedDisabledUpdateColor_ = settings.value("feedDisabledUpdateColor", "#999999").toString();
  alternatingRowColors_ = settings.value("alternatingRowColors", qApp->palette().color(QPalette::AlternateBase).name()).toString();
  notifierTextColor_ = settings.value("notifierTextColor", windowTextColor).toString();
  notifierBackgroundColor_ = settings.value("notifierBackgroundColor", "#FFFFFF").toString();
  settings.endGroup();

  resize(800, 600);
  restoreGeometry(settings.value("GeometryState").toByteArray());
  restoreState(settings.value("ToolBarsState").toByteArray());

  if (!mainToolbarToggle_->isChecked())
    mainToolbar_->hide();
  if (!feedsToolbarToggle_->isChecked())
    feedsPanel_->hide();
  if (!statusBarToggle_->isChecked())
    statusBar()->hide();

  mainSplitter_->restoreState(settings.value("MainSplitterState").toByteArray());
  feedsWidgetVisibleAct_->setChecked(settings.value("FeedsWidgetVisible", true).toBool());
  slotVisibledFeedsWidget();
  webWidgetVisibleAct_->setChecked(settings.value("WebWidgetVisible", true).toBool());

  feedsWidgetSplitterState_ = settings.value("FeedsWidgetSplitterState").toByteArray();
  bool showCategories = settings.value("NewsCategoriesTreeVisible", true).toBool();
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
  bool expandCategories = settings.value("categoriesTreeExpanded", true).toBool();
  if (expandCategories)
    categoriesTree_->expandAll();

  showMenuBar();

  adblockIcon_->setEnabled(settings.value("AdBlock/enabled", true).toBool());
}

/** @brief Save settings in ini-file
 *---------------------------------------------------------------------------*/
void MainWindow::saveSettings()
{
  Settings settings;
  settings.beginGroup("Settings");

  settings.setValue("showSplashScreen", showSplashScreen_);
  settings.setValue("reopenFeedStartup", reopenFeedStartup_);
  settings.setValue("openNewTabNextToActive", openNewTabNextToActive_);

  settings.setValue("createLastFeed", mainApp->isSaveDataLastFeed());

  settings.setValue("showTrayIcon", showTrayIcon_);
  settings.setValue("startingTray", startingTray_);
  settings.setValue("minimizingTray", minimizingTray_);
  settings.setValue("closingTray", closingTray_);
  settings.setValue("behaviorIconTray", behaviorIconTray_);
  settings.setValue("singleClickTray", singleClickTray_);
  settings.setValue("clearStatusNew", clearStatusNew_);
  settings.setValue("emptyWorking", emptyWorking_);

  settings.setValue("langFileName", mainApp->language());

  QString fontFamily = feedsView_->font().family();
  settings.setValue("feedsFontFamily", fontFamily);
  int fontSize = feedsView_->font().pointSize();
  settings.setValue("feedsFontSize", fontSize);

  settings.setValue("newsFontFamily", newsListFontFamily_);
  settings.setValue("newsFontSize", newsListFontSize_);
  settings.setValue("newsTitleFontFamily", newsTitleFontFamily_);
  settings.setValue("newsTitleFontSize", newsTitleFontSize_);
  settings.setValue("newsTextFontFamily", newsTextFontFamily_);
  settings.setValue("newsTextFontSize", newsTextFontSize_);
  settings.setValue("notificationFontFamily", notificationFontFamily_);
  settings.setValue("notificationFontSize", notificationFontSize_);

  settings.setValue("autoUpdatefeeds", updateFeedsEnable_);
  settings.setValue("autoUpdatefeedsTime", updateFeedsInterval_);
  settings.setValue("autoUpdatefeedsInterval", updateFeedsIntervalType_);

  settings.setValue("openingFeedAction", openingFeedAction_);
  settings.setValue("openNewsWebViewOn", openNewsWebViewOn_);

  settings.setValue("markNewsReadOn", markNewsReadOn_);
  settings.setValue("markCurNewsRead", markCurNewsRead_);
  settings.setValue("markNewsReadTime", markNewsReadTime_);
  settings.setValue("markPrevNewsRead", markPrevNewsRead_);
  settings.setValue("markReadSwitchingFeed", markReadSwitchingFeed_);
  settings.setValue("markReadClosingTab", markReadClosingTab_);
  settings.setValue("markReadMinimize", markReadMinimize_);

  settings.setValue("showDescriptionNews", showDescriptionNews_);

  settings.setValue("formatData", formatDate_);
  settings.setValue("formatTime", formatTime_);

  settings.setValue("alternatingColorsNews", alternatingRowColorsNews_);
  settings.setValue("changeBehaviorActionNUN", changeBehaviorActionNUN_);
  settings.setValue("simplifiedDateTime", simplifiedDateTime_);
  settings.setValue("notDeleteStarred", notDeleteStarred_);
  settings.setValue("notDeleteLabeled", notDeleteLabeled_);
  settings.setValue("markIdenticalNewsRead", markIdenticalNewsRead_);
  settings.setValue("avoidOldNews", avoidOldNews_);
  settings.setValue("avoidedOldNewsDate", avoidedOldNewsDate_);

  settings.setValue("mainNewsFilter", mainNewsFilter_);

  settings.setValue("cleanupOnShutdown", cleanupOnShutdown_);
  settings.setValue("maxDayClearUp", maxDayCleanUp_);
  settings.setValue("maxNewsClearUp", maxNewsCleanUp_);
  settings.setValue("dayClearUpOn", dayCleanUpOn_);
  settings.setValue("newsClearUpOn", newsCleanUpOn_);
  settings.setValue("readClearUp", readCleanUp_);
  settings.setValue("neverUnreadClearUp", neverUnreadCleanUp_);
  settings.setValue("neverStarClearUp", neverStarCleanUp_);
  settings.setValue("neverLabelClearUp", neverLabelCleanUp_);
  settings.setValue("cleanUpDeleted", cleanUpDeleted_);
  settings.setValue("optimizeDB", optimizeDB_);

  settings.setValue("externalBrowserOn", externalBrowserOn_);
  settings.setValue("externalBrowser", externalBrowser_);
  settings.setValue("javaScriptEnable", javaScriptEnable_);
  settings.setValue("pluginsEnable", pluginsEnable_);
  settings.setValue("maxPagesInCache", maxPagesInCache_);
  settings.setValue("downloadLocation", downloadLocation_);
  settings.setValue("askDownloadLocation", askDownloadLocation_);
  settings.setValue("defaultZoomPages", defaultZoomPages_);
  settings.setValue("autoLoadImages", autoLoadImages_);

  settings.setValue("soundNewNews", soundNewNews_);
  settings.setValue("soundNotifyPath", soundNotifyPath_);
  settings.setValue("showNotifyOn", showNotifyOn_);
  settings.setValue("screenNotify", screenNotify_);
  settings.setValue("positionNotify", positionNotify_);
  settings.setValue("transparencyNotify", transparencyNotify_);
  settings.setValue("countShowNewsNotify", countShowNewsNotify_);
  settings.setValue("widthTitleNewsNotify", widthTitleNewsNotify_);
  settings.setValue("timeShowNewsNotify", timeShowNewsNotify_);
  settings.setValue("fullscreenModeNotify", fullscreenModeNotify_);
  settings.setValue("showNotifyInactiveApp", showNotifyInactiveApp_);
  settings.setValue("onlySelectedFeeds", onlySelectedFeeds_);
  settings.setValue("showTitlesFeedsNotify", showTitlesFeedsNotify_);
  settings.setValue("showIconFeedNotify", showIconFeedNotify_);
  settings.setValue("showButtonMarkAllNotify", showButtonMarkAllNotify_);
  settings.setValue("showButtonMarkReadNotify", showButtonMarkReadNotify_);
  settings.setValue("showButtonExBrowserNotify", showButtonExBrowserNotify_);
  settings.setValue("showButtonDeleteNotify", showButtonDeleteNotify_);
  settings.setValue("closeNotify", closeNotify_);

  settings.setValue("mainToolbarLock", toolBarLockAct_->isChecked());

  settings.setValue("mainToolbarShow2", mainToolbarToggle_->isChecked());
  settings.setValue("feedsToolbarShow2", feedsToolbarToggle_->isChecked());
  settings.setValue("newsToolbarShow", newsToolbarToggle_->isChecked());
  settings.setValue("browserToolbarShow", browserToolbarToggle_->isChecked());
  settings.setValue("categoriesPanelShow", categoriesPanelToggle_->isChecked());
  settings.setValue("statusBarShow", statusBarToggle_->isChecked());

  settings.setValue("styleApplication",
                    styleGroup_->checkedAction()->objectName());

  settings.setValue("showUnreadCount", showUnreadCount_->isChecked());
  settings.setValue("showUndeleteCount", showUndeleteCount_->isChecked());
  settings.setValue("showLastUpdated", showLastUpdated_->isChecked());

  settings.setValue("indentationFeedsTree", indentationFeedsTreeAct_->isChecked());

  settings.setValue("newsLayout", newsLayout_);
  settings.setValue("browserPosition", browserPosition_);

  settings.setValue("openLinkInBackground", openLinkInBackground_);
  settings.setValue("openLinkInBackgroundEmbedded", openLinkInBackgroundEmbedded_);
  settings.setValue("openingLinkTimeout", openingLinkTimeout_);

  settings.setValue("stayOnTop", stayOnTopAct_->isChecked());

  settings.setValue("hideFeedsOpenTab", hideFeedsOpenTab_);
  settings.setValue("showToggleFeedsTree", showToggleFeedsTree_);

  settings.setValue("defaultIconFeeds", defaultIconFeeds_);
  settings.setValue("autocollapseFolder", feedsView_->autocollapseFolder_);

  settings.setValue("showMenuBar", showMenuBarAct_->isChecked());

  settings.endGroup();

  settings.beginGroup("Color");
  settings.setValue("feedsListTextColor", feedsModel_->textColor_);
  settings.setValue("feedsListBackgroundColor", feedsModel_->backgroundColor_);
  settings.setValue("newsListTextColor", newsListTextColor_);
  settings.setValue("newsListBackgroundColor", newsListBackgroundColor_);
  settings.setValue("newNewsTextColor", newNewsTextColor_);
  settings.setValue("unreadNewsTextColor", unreadNewsTextColor_);
  settings.setValue("focusedNewsTextColor", focusedNewsTextColor_);
  settings.setValue("focusedNewsBGColor", focusedNewsBGColor_);
  settings.setValue("linkColor", linkColor_);
  settings.setValue("titleColor", titleColor_);
  settings.setValue("dateColor", dateColor_);
  settings.setValue("authorColor", authorColor_);
  settings.setValue("newsTextColor", newsTextColor_);
  settings.setValue("newsTitleBackgroundColor", newsTitleBackgroundColor_);
  settings.setValue("newsBackgroundColor", newsBackgroundColor_);
  settings.setValue("feedWithNewNewsColor", feedsModel_->feedWithNewNewsColor_);
  settings.setValue("countNewsUnreadColor", feedsModel_->countNewsUnreadColor_);
  settings.setValue("focusedFeedTextColor", feedsModel_->focusedFeedTextColor_);
  settings.setValue("focusedFeedBGColor", feedsModel_->focusedFeedBGColor_);
  settings.setValue("feedDisabledUpdateColor", feedsModel_->feedDisabledUpdateColor_);
  settings.setValue("alternatingRowColors", alternatingRowColors_);
  settings.setValue("notifierTextColor", notifierTextColor_);
  settings.setValue("notifierBackgroundColor", notifierBackgroundColor_);
  settings.endGroup();

  settings.setValue("GeometryState", saveGeometry());
  settings.setValue("ToolBarsState", saveState());

  settings.setValue("MainSplitterState", mainSplitter_->saveState());
  settings.setValue("FeedsWidgetVisible", showFeedsTabPermanent_);
  settings.setValue("WebWidgetVisible", webWidgetVisibleAct_->isChecked());

  bool newsCategoriesTreeVisible = true;
  if (categoriesWidget_->height() <= (categoriesPanel_->height()+2)) {
    newsCategoriesTreeVisible = false;
    settings.setValue("FeedsWidgetSplitterState", feedsWidgetSplitterState_);
  } else {
    settings.setValue("FeedsWidgetSplitterState", feedsSplitter_->saveState());
  }
  settings.setValue("NewsCategoriesTreeVisible", newsCategoriesTreeVisible);
  settings.setValue("categoriesTreeExpanded", categoriesTree_->topLevelItem(CategoriesTreeWidget::LabelsItem)->isExpanded());

  if (stackedWidget_->count()) {
    NewsTabWidget *widget;
    if (currentNewsTab->type_ < NewsTabWidget::TabTypeWeb)
      widget = currentNewsTab;
    else
      widget = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);

    widget->newsHeader_->saveStateColumns(widget);
    settings.setValue("NewsTabSplitterState",
                      widget->newsTabWidgetSplitter_->saveState());
  }

  NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);
  settings.setValue("feedSettings/currentId", widget->feedId_);
  settings.setValue("feedSettings/filterName",
                    feedsFilterGroup_->checkedAction()->objectName());
  settings.setValue("newsSettings/filterName",
                    newsFilterGroup_->checkedAction()->objectName());

  mainApp->cookieJar()->saveCookies();
  mainApp->c2fSaveSettings();
  AdBlockManager::instance()->save();
}

void MainWindow::showMainMenu()
{
#ifndef Q_OS_MAC
  mainMenu_->popup(mainMenuButton_->mapToGlobal(QPoint(0, mainMenuButton_->height())));
#endif
}

/** @brief Add feed to feed list
 *---------------------------------------------------------------------------*/
void MainWindow::addFeed()
{
  int curFolderId = 0;
  QPersistentModelIndex curIndex = feedsView_->selectIndex();
  if (feedsModel_->isFolder(curIndex)) {
    curFolderId = feedsModel_->idByIndex(curIndex);
  } else {
    curFolderId = feedsModel_->paridByIndex(curIndex);
  }

  AddFeedWizard *addFeedWizard = new AddFeedWizard(this, curFolderId);
  addFeedWizard->show();

  int result = addFeedWizard->exec();
  if (result == QDialog::Rejected) {
    delete addFeedWizard;
    return;
  }

  emit faviconRequestUrl(addFeedWizard->htmlUrlString_, addFeedWizard->feedUrlString_);

  QList<int> categoriesList;
  categoriesList << addFeedWizard->feedParentId_;
  recountFeedCategories(categoriesList);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  feedsView_->setCurrentIndex(QModelIndex());
  feedsModelReload();
  QModelIndex index = feedsProxyModel_->mapFromSource(addFeedWizard->feedId_);
  feedsView_->selectIdEn_ = true;
  feedsView_->setCurrentIndex(index);
  slotFeedClicked(index);
  QApplication::restoreOverrideCursor();
  slotUpdateFeed(addFeedWizard->feedId_, true, addFeedWizard->newCount_, false);

  delete addFeedWizard;
}

/** @brief Add folder to feed list
 *---------------------------------------------------------------------------*/
void MainWindow::addFolder()
{
  int curFolderId = 0;
  QPersistentModelIndex curIndex = feedsView_->selectIndex();
  if (feedsModel_->isFolder(curIndex)) {
    curFolderId = feedsModel_->idByIndex(curIndex);
  } else {
    curFolderId = feedsModel_->paridByIndex(curIndex);
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
void MainWindow::deleteItemFeedsTree()
{
  if (!feedsView_->selectIndex().isValid()) return;

  QMessageBox msgBox(this);
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle(tr("Confirm Delete"));
  msgBox.setText(tr("Are you sure to delete selected elements?"));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::No);

  if (msgBox.exec() == QMessageBox::No) return;

  QModelIndex currentIndex = feedsProxyModel_->mapToSource(feedsView_->currentIndex());
  int feedIdCur = feedsModel_->idByIndex(currentIndex);

  QModelIndexList indexList = feedsView_->selectionModel()->selectedRows(0);
  if (indexList.count() <= 1) {
    indexList.clear();
    indexList.append(feedsProxyModel_->mapFromSource(feedsView_->selectIndex()));
  }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<int> idList;
  QList<int> parentIdList;
  for (int i = indexList.count()-1; i >= 0; --i) {
    QModelIndex index = feedsProxyModel_->mapToSource(indexList[i]);
    if (feedsModel_->isFolder(index)) {
      idList.append(feedsModel_->dataField(index, "id").toInt());
      int parentId = feedsModel_->dataField(index, "parentId").toInt();
      if (!parentIdList.contains(parentId)) {
        parentIdList.append(parentId);
      }
      indexList.removeAt(i);
    }
  }
  for (int i = indexList.count()-1; i >= 0; --i) {
    QModelIndex index = feedsProxyModel_->mapToSource(indexList[i]);
    int parentId = feedsModel_->dataField(index, "parentId").toInt();
    if (!idList.contains(parentId)) {
      idList.append(feedsModel_->dataField(index, "id").toInt());
      int parentId = feedsModel_->dataField(index, "parentId").toInt();
      if (!parentIdList.contains(parentId)) {
        parentIdList.append(parentId);
      }
    }
    indexList.removeAt(i);
  }

  db_.transaction();
  QSqlQuery q;
  QString idStr;
  QString feedIdStr;
  foreach (int feedId, idList) {
    if (idStr.isEmpty()) {
      idStr.append(QString("id='%1'").arg(feedId));
      feedIdStr.append(QString("feedId='%1'").arg(feedId));
    } else {
      idStr.append(QString(" OR id='%1'").arg(feedId));
      feedIdStr.append(QString(" OR feedId='%1'").arg(feedId));
    }
    QQueue<int> parentIds;
    parentIds.enqueue(feedId);
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
  }
  q.exec(QString("DELETE FROM feeds WHERE %1").arg(idStr));
  q.exec(QString("DELETE FROM news WHERE %1").arg(feedIdStr));
  db_.commit();

  // Correction row
  foreach (int parentId, parentIdList) {
    QList<int> idCorrectList;
    q.exec(QString("SELECT id FROM feeds WHERE parentId='%1' ORDER BY rowToParent").
           arg(parentId));
    while (q.next()) {
      idCorrectList << q.value(0).toInt();
    }
    for (int i = 0; i < idCorrectList.count(); i++) {
      q.exec(QString("UPDATE feeds SET rowToParent='%1' WHERE id=='%2'").
             arg(i).arg(idCorrectList.at(i)));
    }
  }

  recountFeedCategories(parentIdList);
  feedsModelReload();
  currentIndex = feedsProxyModel_->mapFromSource(feedIdCur);
  feedsView_->setCurrentIndex(currentIndex);
  slotFeedClicked(currentIndex);

  QApplication::restoreOverrideCursor();
}

/** @brief Import feeds from OPML-file
 *
 * Calls open file system dialog with filter *.opml.
 * Adds all feeds to DB include hierarchy, ignore duplicate feeds
 *---------------------------------------------------------------------------*/
void MainWindow::slotImportFeeds()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select OPML-File"),
                                                  QDir::homePath(),
                                                  QString(tr("OPML-Files (*.%1 *.%2)"))
                                                  .arg("opml").arg("xml"));

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

  QByteArray xmlData = file.readAll();
  file.close();

  isStartImportFeed_ = true;
  emit signalImportFeeds(xmlData);
}

/** @brief Export feeds to OPML-file
 *---------------------------------------------------------------------------*/
void MainWindow::slotExportFeeds()
{
  QString fileName = QFileDialog::getSaveFileName(this, tr("Select OPML-File"),
                                                  QDir::homePath(),
                                                  QString(tr("OPML-Files (*.%1)"))
                                                  .arg("opml"));

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
  FeedsModel exportTreeModel(this);
  QTreeView exportTreeView;
  exportTreeView.setModel(&exportTreeModel);
  exportTreeModel.setView(feedsView_);
  exportTreeView.expandAll();

  QModelIndex index = exportTreeModel.index(0, 0);
  QStack<int> parentIdsStack;
  parentIdsStack.push(0);
  while (index.isValid()) {
    int feedId = exportTreeModel.idByIndex(index);
    int feedParId = exportTreeModel.paridByIndex(index);

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
// ----------------------------------------------------------------------------
void MainWindow::slotFeedsViewportUpdate()
{
  feedsView_->viewport()->update();
#ifdef HAVE_QT5
  feedsView_->header()->setSectionResizeMode(feedsView_->columnIndex("unread"), QHeaderView::ResizeToContents);
  feedsView_->header()->setSectionResizeMode(feedsView_->columnIndex("undeleteCount"), QHeaderView::ResizeToContents);
  feedsView_->header()->setSectionResizeMode(feedsView_->columnIndex("updated"), QHeaderView::ResizeToContents);
#else
  feedsView_->header()->setResizeMode(feedsView_->columnIndex("unread"), QHeaderView::ResizeToContents);
  feedsView_->header()->setResizeMode(feedsView_->columnIndex("undeleteCount"), QHeaderView::ResizeToContents);
  feedsView_->header()->setResizeMode(feedsView_->columnIndex("updated"), QHeaderView::ResizeToContents);
#endif
}
// ----------------------------------------------------------------------------
void MainWindow::slotFeedCountsUpdate(FeedCountStruct counts)
{
  QModelIndex index = feedsModel_->indexById(counts.feedId);
  if (index.isValid()) {
    QModelIndex indexUnread   = feedsModel_->indexSibling(index, "unread");
    QModelIndex indexNew      = feedsModel_->indexSibling(index, "newCount");
    QModelIndex indexUndelete = feedsModel_->indexSibling(index, "undeleteCount");
    feedsModel_->setData(indexUnread, counts.unreadCount);
    feedsModel_->setData(indexNew, counts.newCount);
    feedsModel_->setData(indexUndelete, counts.undeleteCount);

    if (!counts.updated.isEmpty()) {
      QModelIndex indexUpdated  = feedsModel_->indexSibling(index, "updated");
      feedsModel_->setData(indexUpdated, counts.updated);
    }

    if (!counts.lastBuildDate.isEmpty()) {
      QModelIndex indexLastBuildDate = feedsModel_->indexSibling(index, "lastBuildDate");
      feedsModel_->setData(indexLastBuildDate, counts.lastBuildDate);
    }

    if (!counts.htmlUrl.isEmpty()) {
      QModelIndex indexHtmlUrl = feedsModel_->indexSibling(index, "htmlUrl");
      feedsModel_->setData(indexHtmlUrl, counts.htmlUrl);
    }

    if (!counts.title.isEmpty()) {
      QModelIndex indexTitle = feedsModel_->indexSibling(index, "title");
      feedsModel_->setData(indexTitle, counts.title);
    }
  }

  if (isStartImportFeed_ && !counts.xmlUrl.isEmpty()) {
    emit faviconRequestUrl(counts.htmlUrl, counts.xmlUrl);
  }
}

/** @brief Recalculate counters for specified categories
 * @details Processing DB data. Model "reselect()" needed.
 * @param categoriesList - categories identifiers list for processing
 *---------------------------------------------------------------------------*/
void MainWindow::recountFeedCategories(const QList<int> &categoriesList)
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
void MainWindow::recountCategoryCounts()
{
  if (recountCategoryCountsOn_) return;

  if (!categoriesTree_->isVisible() || !stackedWidget_->count()) return;

  recountCategoryCountsOn_ = true;
  emit signalRecountCategoryCounts();
}

/** @brief Process recalculating categories counters
 *----------------------------------------------------------------------------*/
void MainWindow::slotRecountCategoryCounts(QList<int> deletedList, QList<int> starredList,
                                           QList<int> readList, QStringList labelList)
{
  int allStarredCount = 0;
  int unreadStarredCount = 0;
  int deletedCount = 0;
  QMap<int,int> allCountList;
  QMap<int,int> unreadCountList;
  int allLabelCount = 0;
  int unreadLabelCount = 0;
  QFont font;

  QTreeWidgetItem *labelTreeItem = categoriesTree_->topLevelItem(CategoriesTreeWidget::LabelsItem);
  for (int i = 0; i < labelTreeItem->childCount(); i++) {
    int id = labelTreeItem->child(i)->text(2).toInt();
    allCountList.insert(id, 0);
    unreadCountList.insert(id, 0);
  }

  for (int i = 0; i < deletedList.count(); ++i) {
    if (deletedList.at(i) == 0) {
      if (starredList.at(i) == 1) {
        allStarredCount++;
        if (readList.at(i) == 0)
          unreadStarredCount++;
      }
      QString idString = labelList.at(i);
      if (!idString.isEmpty() && idString != ",") {
        QStringList idList = idString.split(",", QString::SkipEmptyParts);
        foreach (QString idStr, idList) {
          int id = idStr.toInt();
          if (allCountList.contains(id)) {
            allCountList[id]++;
            if (readList.at(i) == 0)
              unreadCountList[id]++;
          }
        }
      }
    } else if (deletedList.at(i) == 1) {
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
    font = labelTreeItem->child(i)->font(0);
    if (unreadCountList[id])
      font.setBold(true);
    else
      font.setBold(false);
    labelTreeItem->child(i)->setFont(0, font);

    unreadLabelCount = unreadLabelCount + unreadCountList[id];
    allLabelCount = allLabelCount + allCountList[id];
  }

  QString countStr;
  if (!unreadStarredCount && !allStarredCount)
    countStr = "";
  else
    countStr = QString("(%1/%2)").arg(unreadStarredCount).arg(allStarredCount);
  categoriesTree_->topLevelItem(CategoriesTreeWidget::StarredItem)->setText(4, countStr);
  font = categoriesTree_->topLevelItem(CategoriesTreeWidget::StarredItem)->font(0);
  if (unreadStarredCount)
    font.setBold(true);
  else
    font.setBold(false);
  categoriesTree_->topLevelItem(CategoriesTreeWidget::StarredItem)->setFont(0, font);

  if (!deletedCount)
    countStr = "";
  else
    countStr = QString("(%1)").arg(deletedCount);
  categoriesTree_->topLevelItem(CategoriesTreeWidget::DeletedItem)->setText(4, countStr);

  if (!unreadLabelCount && !allLabelCount)
    countStr = "";
  else
    countStr = QString("(%1/%2)").arg(unreadLabelCount).arg(allLabelCount);
  categoriesTree_->topLevelItem(CategoriesTreeWidget::LabelsItem)->setText(4, countStr);
  font = categoriesTree_->topLevelItem(CategoriesTreeWidget::LabelsItem)->font(0);
  if (unreadLabelCount)
    font.setBold(true);
  else
    font.setBold(false);
  categoriesTree_->topLevelItem(CategoriesTreeWidget::LabelsItem)->setFont(0, font);

  NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(stackedWidget_->currentIndex());
  if ((widget->type_ > NewsTabWidget::TabTypeFeed) && (widget->type_ < NewsTabWidget::TabTypeWeb)
      && categoriesTree_->currentIndex().isValid()) {
    int unreadCount = widget->getUnreadCount(categoriesTree_->currentItem()->text(4));
    int allCount = widget->newsModel_->rowCount();
    statusUnread_->setText(QString(" " + tr("Unread: %1") + " ").arg(unreadCount));
    statusAll_->setText(QString(" " + tr("All: %1") + " ").arg(allCount));
  }

  recountCategoryCountsOn_ = false;
}

/** @brief Update feed view
 *
 * Slot is called by UpdateDelayer after some delay
 * @param feedId Feed identifier to update
 * @param changed Flag indicating that feed is updated indeed
 *---------------------------------------------------------------------------*/
void MainWindow::slotUpdateFeed(int feedId, bool changed, int newCount, bool finish)
{
  if (finish) {
    emit signalShowNotification();
    progressBar_->hide();
    progressBar_->setMaximum(0);
    progressBar_->setValue(0);
    isStartImportFeed_ = false;
  }

  if (!changed) {
    emit signalNextUpdate(finish);
    return;
  }

  // Action after new news has arrived: tray, sound
  if (!isActiveWindow() && (newCount > 0) &&
      (behaviorIconTray_ == CHANGE_ICON_TRAY)) {
    traySystem->setIcon(QIcon(":/images/quiterss128_NewNews"));
  }
  emit signalRefreshInfoTray();
  if (newCount > 0)
    emit signalPlaySoundNewNews();

  // Manage notifications
  bool showNotify = true;

  if (showNotifyInactiveApp_)
  {
    showNotify = !isActiveWindow();
  }

  if (!showNotify)
  {
    clearNotification();
  }

  if (newCount > 0)
  {
    bool bAddRecentNews = !onlySelectedFeeds_ || idFeedsNotifyList_.contains(feedId);
    bool bAddNewNews = bAddRecentNews && showNotify;

    if (bAddNewNews)
    {
      int newFeedIdIndex = newNews.idFeedList_.indexOf(feedId);

      if (-1 < newFeedIdIndex)
      {
        newNews.cntNewsList_[newFeedIdIndex] = newCount;
      }
      else
      {
        newNews.idFeedList_.append(feedId);
        newNews.cntNewsList_.append(newCount);
      }
    }

    if (bAddRecentNews)
    {
      int recentFeedIdIndex = recentNews.idFeedList_.indexOf(feedId);

      if (-1 < recentFeedIdIndex)
      {
        recentNews.cntNewsList_[recentFeedIdIndex] += newCount;
      }
      else
      {
        recentNews.idFeedList_.append(feedId);
        recentNews.cntNewsList_.append(newCount);
      }
    }
  }

  recountCategoryCounts();

  emit signalNextUpdate(finish);
}

/** @brief Process updating news list
 *---------------------------------------------------------------------------*/
void MainWindow::slotUpdateNews(int refresh)
{
  int newsId = newsModel_->index(
        newsView_->currentIndex().row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

  newsModel_->select();

  if (newsModel_->rowCount() != 0) {
    while (newsModel_->canFetchMore())
      newsModel_->fetchMore();
  }

  currentNewsTab->loadNewspaper(refresh);

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
void MainWindow::slotFeedClicked(QModelIndex index)
{
  if (feedsView_->selectionModel()->selectedRows(0).count() > 1) return;

  int feedIdCur = feedsModel_->idByIndex(feedsProxyModel_->mapToSource(index));

  if (stackedWidget_->count() && currentNewsTab->type_ < NewsTabWidget::TabTypeWeb) {
    currentNewsTab->newsHeader_->saveStateColumns(currentNewsTab);
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
      QModelIndex currentIndex = feedsProxyModel_->mapFromSource(feedIdCur);
      feedsView_->setCurrentIndex(currentIndex);

      currentNewsTab = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);
      newsModel_ = currentNewsTab->newsModel_;
      newsView_ = currentNewsTab->newsView_;
    } else {
      if (stackedWidget_->count() && currentNewsTab->type_ != NewsTabWidget::TabTypeFeed) {
        setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_, FeedReadSwitchingFeed, currentNewsTab);
      } else {
        // Mark previous feed Read while switching to another feed
        setFeedRead(NewsTabWidget::TabTypeFeed, feedIdOld_, FeedReadSwitchingFeed, 0, feedIdCur);
      }

      categoriesTree_->setCurrentIndex(QModelIndex());
    }

    slotFeedSelected(feedsModel_->indexById(feedIdCur));
    feedsView_->repaint();
  } else if (indexTab != -1) {
    tabBar_->setCurrentIndex(indexTab);
  }
  feedIdOld_ = feedIdCur;
}

/** @brief Process feed choosing
 *---------------------------------------------------------------------------*/
void MainWindow::slotFeedSelected(QModelIndex index, bool createTab)
{
  int feedId = feedsModel_->idByIndex(index);
  int feedParId = feedsModel_->paridByIndex(index);

  // Open or create feed tab
  if (!stackedWidget_->count() || createTab) {
    NewsTabWidget *widget = new NewsTabWidget(this, NewsTabWidget::TabTypeFeed, feedId, feedParId);
    int indexTab = addTab(widget);

    createNewsTab(indexTab);

    if (indexTab == 0)
      currentNewsTab->closeButton_->setVisible(false);
    if (!index.isValid())
      currentNewsTab->setVisible(false);

    emit signalSetCurrentTab(indexTab);
  } else {
    currentNewsTab->type_ = NewsTabWidget::TabTypeFeed;
    currentNewsTab->feedId_ = feedId;
    currentNewsTab->feedParId_ = feedParId;
    currentNewsTab->setSettings(true, false);
    currentNewsTab->setVisible(index.isValid());
  }
  statusUnread_->setVisible(index.isValid());
  statusAll_->setVisible(index.isValid());

  // Set icon for tab has opened
  bool isFeed = (index.isValid() && feedsModel_->isFolder(index)) ? false : true;

  QPixmap iconTab;
  QByteArray byteArray = feedsModel_->dataField(index, "image").toByteArray();
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
  currentNewsTab->setTextTab(feedsModel_->dataField(index, "text").toString());

  feedProperties_->setEnabled(index.isValid());

  setNewsFilter(newsFilterGroup_->checkedAction(), false);

  // Search feed news that displayed before
  int newsRow = -1;
  if (openingFeedAction_ == 0) {
    QModelIndex feedIndex = feedsModel_->indexById(feedId);
    int newsIdCur = feedsModel_->dataField(feedIndex, "currentNews").toInt();
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

  // Focus feed news that displayed before
  newsView_->setCurrentIndex(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
  if (newsRow == -1) newsView_->verticalScrollBar()->setValue(newsRow);

  if ((openingFeedAction_ != 2) && openNewsWebViewOn_) {
    currentNewsTab->slotNewsViewSelected(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
  } else {
    currentNewsTab->slotNewsViewSelected(newsModel_->index(-1, newsModel_->fieldIndex("title")));
    int newsId = newsModel_->index(newsRow, newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();
    QString qStr = QString("UPDATE feeds SET currentNews='%1' WHERE id=='%2'").arg(newsId).arg(feedId);
    mainApp->sqlQueryExec(qStr);
    QModelIndex feedIndex = feedsModel_->indexById(feedId);
    feedsModel_->setData(feedsModel_->indexSibling(feedIndex, "currentNews"), newsId);
  }
}

// ----------------------------------------------------------------------------
void MainWindow::showOptionDlg(int index)
{
  static int pageIndex = 0;
  Settings settings;

  if (index != -1) pageIndex = index;

  if (optionsDialog_) {
    optionsDialog_->activateWindow();
    return;
  }

  optionsDialog_ = new OptionsDialog(this);

  settings.beginGroup("Settings");
  bool updateFeedsStartUp = settings.value("autoUpdatefeedsStartUp", false).toBool();
  settings.endGroup();

  optionsDialog_->showSplashScreen_->setChecked(showSplashScreen_);
  optionsDialog_->reopenFeedStartup_->setChecked(reopenFeedStartup_);
  optionsDialog_->openNewTabNextToActive_->setChecked(openNewTabNextToActive_);
  optionsDialog_->hideFeedsOpenTab_->setChecked(hideFeedsOpenTab_);
  optionsDialog_->showToggleFeedsTree_->setChecked(showToggleFeedsTree_);
  optionsDialog_->defaultIconFeeds_->setChecked(defaultIconFeeds_);
  optionsDialog_->autocollapseFolder_->setChecked(feedsView_->autocollapseFolder_);
  bool showCloseButtonTab = settings.value("Settings/showCloseButtonTab", true).toBool();
  optionsDialog_->showCloseButtonTab_->setChecked(showCloseButtonTab);

  bool updateCheckEnabled = settings.value("Settings/updateCheckEnabled", true).toBool();
  optionsDialog_->updateCheckEnabled_->setChecked(updateCheckEnabled);
  bool statisticsEnabled = settings.value("Settings/statisticsEnabled2", true).toBool();
  optionsDialog_->statisticsEnabled_->setChecked(statisticsEnabled);

  bool storeDBMemory_ = settings.value("Settings/storeDBMemory", true).toBool();
  optionsDialog_->storeDBMemory_->setChecked(storeDBMemory_);
  int saveDBMemFileInterval = settings.value("Settings/saveDBMemFileInterval", 30).toInt();
  optionsDialog_->saveDBMemFileInterval_->setValue(saveDBMemFileInterval);

  optionsDialog_->showTrayIconBox_->setChecked(showTrayIcon_);
  optionsDialog_->startingTray_->setChecked(startingTray_);
  optionsDialog_->minimizingTray_->setChecked(minimizingTray_);
  optionsDialog_->closingTray_->setChecked(closingTray_);
  optionsDialog_->setBehaviorIconTray(behaviorIconTray_);
  optionsDialog_->singleClickTray_->setChecked(singleClickTray_);
  optionsDialog_->clearStatusNew_->setChecked(clearStatusNew_);
  optionsDialog_->emptyWorking_->setChecked(emptyWorking_);

  optionsDialog_->setProxy(mainApp->networkProxy());

  int timeoutRequest = settings.value("Settings/timeoutRequest", 15).toInt();
  int numberRequests = settings.value("Settings/numberRequest", 10).toInt();
  int numberRepeats = settings.value("Settings/numberRepeats", 2).toInt();
  optionsDialog_->timeoutRequest_->setValue(timeoutRequest);
  optionsDialog_->numberRequests_->setValue(numberRequests);
  optionsDialog_->numberRepeats_->setValue(numberRepeats);

  optionsDialog_->embeddedBrowserOn_->setChecked(externalBrowserOn_ <= 0);
  optionsDialog_->externalBrowserOn_->setChecked(externalBrowserOn_ >= 1);
  optionsDialog_->defaultExternalBrowserOn_->setChecked((externalBrowserOn_ == 0) ||
                                                        (externalBrowserOn_ == 1));
  optionsDialog_->otherExternalBrowserOn_->setChecked((externalBrowserOn_ == -1) ||
                                                      (externalBrowserOn_ == 2));
  optionsDialog_->otherExternalBrowserEdit_->setText(externalBrowser_);
  optionsDialog_->autoLoadImages_->setChecked(autoLoadImages_);
  optionsDialog_->javaScriptEnable_->setChecked(javaScriptEnable_);
  optionsDialog_->pluginsEnable_->setChecked(pluginsEnable_);
  optionsDialog_->defaultZoomPages_->setValue(defaultZoomPages_);
  optionsDialog_->openLinkInBackground_->setChecked(openLinkInBackground_);
  optionsDialog_->openLinkInBackgroundEmbedded_->setChecked(openLinkInBackgroundEmbedded_);
  optionsDialog_->maxPagesInCache_->setValue(maxPagesInCache_);

  settings.beginGroup("Settings");

  QString userStyleBrowser = settings.value("userStyleBrowser", QString()).toString();
  optionsDialog_->userStyleBrowserEdit_->setText(userStyleBrowser);

  bool useDiskCache = settings.value("useDiskCache", true).toBool();
  optionsDialog_->diskCacheOn_->setChecked(useDiskCache);
  QString diskCacheDir = settings.value("dirDiskCache", mainApp->cacheDefaultDir()).toString();
  if (diskCacheDir.isEmpty()) diskCacheDir = mainApp->cacheDefaultDir();
  optionsDialog_->dirDiskCacheEdit_->setText(diskCacheDir);
  int maxDiskCache = settings.value("maxDiskCache", 50).toInt();
  optionsDialog_->maxDiskCache_->setValue(maxDiskCache);

  settings.endGroup();

  UseCookies useCookies = mainApp->cookieJar()->useCookies();
  optionsDialog_->saveCookies_->setChecked(useCookies == SaveCookies);
  optionsDialog_->deleteCookiesOnClose_->setChecked(useCookies == DeleteCookiesOnClose);
  optionsDialog_->blockCookies_->setChecked(useCookies == BlockCookies);

  optionsDialog_->downloadLocationEdit_->setText(downloadLocation_);
  optionsDialog_->askDownloadLocation_->setChecked(askDownloadLocation_);

  optionsDialog_->updateFeedsStartUp_->setChecked(updateFeedsStartUp);
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
  optionsDialog_->avoidedOldNewsDateOn_->setChecked(avoidOldNews_);

  if (!avoidedOldNewsDate_.isNull() && avoidedOldNewsDate_.isValid()) {
    optionsDialog_->avoidedOldNewsDate_->setSelectedDate(avoidedOldNewsDate_);
  } else {
    optionsDialog_->avoidedOldNewsDate_->setSelectedDate(QDate::currentDate());
  }

  for (int i = 0; i < optionsDialog_->mainNewsFilter_->count(); i++) {
    if (optionsDialog_->mainNewsFilter_->itemData(i).toString() == mainNewsFilter_) {
      optionsDialog_->mainNewsFilter_->setCurrentIndex(i);
      break;
    }
  }

  QString styleSheetNews = settings.value("Settings/styleSheetNews",
                                          mainApp->styleSheetNewsDefaultFile()).toString();
  optionsDialog_->styleSheetNewsEdit_->setText(styleSheetNews);

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

  optionsDialog_->soundNotifyBox_->setChecked(soundNewNews_);
  optionsDialog_->editSoundNotifer_->setText(soundNotifyPath_);
  optionsDialog_->showNotifyOn_->setChecked(showNotifyOn_);
  optionsDialog_->screenNotify_->setCurrentIndex(screenNotify_);
  optionsDialog_->positionNotify_->setCurrentIndex(positionNotify_);
  optionsDialog_->transparencyNotify_->setValue(transparencyNotify_);
  optionsDialog_->countShowNewsNotify_->setValue(countShowNewsNotify_);
  optionsDialog_->widthTitleNewsNotify_->setValue(widthTitleNewsNotify_);
  optionsDialog_->timeShowNewsNotify_->setValue(timeShowNewsNotify_);
  optionsDialog_->fullscreenModeNotify_->setChecked(fullscreenModeNotify_);
  optionsDialog_->showNotifyInactiveApp_->setChecked(showNotifyInactiveApp_);
  optionsDialog_->onlySelectedFeeds_->setChecked(onlySelectedFeeds_);
  optionsDialog_->showTitlesFeedsNotify_->setChecked(showTitlesFeedsNotify_);
  optionsDialog_->showIconFeedNotify_->setChecked(showIconFeedNotify_);
  optionsDialog_->showButtonMarkAllNotify_->setChecked(showButtonMarkAllNotify_);
  optionsDialog_->showButtonMarkReadNotify_->setChecked(showButtonMarkReadNotify_);
  optionsDialog_->showButtonExBrowserNotify_->setChecked(showButtonExBrowserNotify_);
  optionsDialog_->showButtonDeleteNotify_->setChecked(showButtonDeleteNotify_);
  optionsDialog_->closeNotify_->setChecked(closeNotify_);

  optionsDialog_->setLanguage(mainApp->language());

  QString strFont = QString("%1, %2").
      arg(feedsView_->font().family()).
      arg(feedsView_->font().pointSize());
  optionsDialog_->fontsTree_->topLevelItem(0)->setText(2, strFont);
  strFont = QString("%1, %2").arg(newsListFontFamily_).arg(newsListFontSize_);
  optionsDialog_->fontsTree_->topLevelItem(1)->setText(2, strFont);
  strFont = QString("%1, %2").arg(newsTitleFontFamily_).arg(newsTitleFontSize_);
  optionsDialog_->fontsTree_->topLevelItem(2)->setText(2, strFont);
  strFont = QString("%1, %2").arg(newsTextFontFamily_).arg(newsTextFontSize_);
  optionsDialog_->fontsTree_->topLevelItem(3)->setText(2, strFont);
  strFont = QString("%1, %2").arg(notificationFontFamily_).arg(notificationFontSize_);
  optionsDialog_->fontsTree_->topLevelItem(4)->setText(2, strFont);

  settings.beginGroup("Settings");
  QString browserStandardFont = settings.value(
        "browserStandardFont", QWebSettings::globalSettings()->fontFamily(QWebSettings::StandardFont)).toString();
  QString browserFixedFont = settings.value(
        "browserFixedFont", QWebSettings::globalSettings()->fontFamily(QWebSettings::FixedFont)).toString();
  QString browserSerifFont = settings.value(
        "browserSerifFont", QWebSettings::globalSettings()->fontFamily(QWebSettings::SerifFont)).toString();
  QString browserSansSerifFont = settings.value(
        "browserSansSerifFont", QWebSettings::globalSettings()->fontFamily(QWebSettings::SansSerifFont)).toString();
  QString browserCursiveFont = settings.value(
        "browserCursiveFont", QWebSettings::globalSettings()->fontFamily(QWebSettings::CursiveFont)).toString();
  QString browserFantasyFont = settings.value(
        "browserFantasyFont", QWebSettings::globalSettings()->fontFamily(QWebSettings::FantasyFont)).toString();
  int browserDefaultFontSize = settings.value(
        "browserDefaultFontSize", QWebSettings::globalSettings()->fontSize(QWebSettings::DefaultFontSize)).toInt();
  int browserFixedFontSize = settings.value(
        "browserFixedFontSize", QWebSettings::globalSettings()->fontSize(QWebSettings::DefaultFixedFontSize)).toInt();
  int browserMinFontSize = settings.value(
        "browserMinFontSize", QWebSettings::globalSettings()->fontSize(QWebSettings::MinimumFontSize)).toInt();
  int browserMinLogFontSize = settings.value(
        "browserMinLogFontSize", QWebSettings::globalSettings()->fontSize(QWebSettings::MinimumLogicalFontSize)).toInt();
  settings.endGroup();

  optionsDialog_->browserStandardFont_->setCurrentFont(QFont(browserStandardFont));
  optionsDialog_->browserFixedFont_->setCurrentFont(QFont(browserFixedFont));
  optionsDialog_->browserSerifFont_->setCurrentFont(QFont(browserSerifFont));
  optionsDialog_->browserSansSerifFont_->setCurrentFont(QFont(browserSansSerifFont));
  optionsDialog_->browserCursiveFont_->setCurrentFont(QFont(browserCursiveFont));
  optionsDialog_->browserFantasyFont_->setCurrentFont(QFont(browserFantasyFont));
  optionsDialog_->browserDefaultFontSize_->setValue(browserDefaultFontSize);
  optionsDialog_->browserFixedFontSize_->setValue(browserFixedFontSize);
  optionsDialog_->browserMinFontSize_->setValue(browserMinFontSize);
  optionsDialog_->browserMinLogFontSize_->setValue(browserMinLogFontSize);

  QPixmap pixmapColor(14, 14);
  pixmapColor.fill(feedsModel_->textColor_);
  optionsDialog_->colorsTree_->topLevelItem(0)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(0)->setText(1, feedsModel_->textColor_);
  if (feedsModel_->backgroundColor_.isEmpty())
    pixmapColor.fill(QColor(0, 0, 0, 0));
  else
    pixmapColor.fill(feedsModel_->backgroundColor_);
  optionsDialog_->colorsTree_->topLevelItem(1)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(1)->setText(1, feedsModel_->backgroundColor_);
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
  pixmapColor.fill(newsTextColor_);
  optionsDialog_->colorsTree_->topLevelItem(10)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(10)->setText(1, newsTextColor_);
  pixmapColor.fill(newsTitleBackgroundColor_);
  optionsDialog_->colorsTree_->topLevelItem(11)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(11)->setText(1, newsTitleBackgroundColor_);
  pixmapColor.fill(newsBackgroundColor_);
  optionsDialog_->colorsTree_->topLevelItem(12)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(12)->setText(1, newsBackgroundColor_);
  pixmapColor.fill(feedsModel_->feedWithNewNewsColor_);
  optionsDialog_->colorsTree_->topLevelItem(13)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(13)->setText(1, feedsModel_->feedWithNewNewsColor_);
  pixmapColor.fill(feedsModel_->countNewsUnreadColor_);
  optionsDialog_->colorsTree_->topLevelItem(14)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(14)->setText(1, feedsModel_->countNewsUnreadColor_);

  pixmapColor.fill(newNewsTextColor_);
  optionsDialog_->colorsTree_->topLevelItem(15)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(15)->setText(1, newNewsTextColor_);
  pixmapColor.fill(unreadNewsTextColor_);
  optionsDialog_->colorsTree_->topLevelItem(16)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(16)->setText(1, unreadNewsTextColor_);

  pixmapColor.fill(feedsModel_->focusedFeedTextColor_);
  optionsDialog_->colorsTree_->topLevelItem(17)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(17)->setText(1, feedsModel_->focusedFeedTextColor_);
  if (feedsModel_->focusedFeedBGColor_.isEmpty())
    pixmapColor.fill(QColor(0, 0, 0, 0));
  else
    pixmapColor.fill(feedsModel_->focusedFeedBGColor_);
  optionsDialog_->colorsTree_->topLevelItem(18)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(18)->setText(1, feedsModel_->focusedFeedBGColor_);
  pixmapColor.fill(feedsModel_->feedDisabledUpdateColor_);
  optionsDialog_->colorsTree_->topLevelItem(19)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(19)->setText(1, feedsModel_->feedDisabledUpdateColor_);

  pixmapColor.fill(alternatingRowColors_);
  optionsDialog_->colorsTree_->topLevelItem(20)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(20)->setText(1, alternatingRowColors_);

  pixmapColor.fill(notifierTextColor_);
  optionsDialog_->colorsTree_->topLevelItem(21)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(21)->setText(1, notifierTextColor_);
  pixmapColor.fill(notifierBackgroundColor_);
  optionsDialog_->colorsTree_->topLevelItem(22)->setIcon(0, pixmapColor);
  optionsDialog_->colorsTree_->topLevelItem(22)->setText(1, notifierBackgroundColor_);

  NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);
  backWebPageAct_->setText(widget->webView_->page()->action(QWebPage::Back)->text());
  backWebPageAct_->setToolTip(widget->webView_->page()->action(QWebPage::Back)->toolTip() + " " + tr("(Browser)"));
  backWebPageAct_->setIcon(widget->webView_->page()->action(QWebPage::Back)->icon());
  backWebPageAct_->setShortcut(widget->webView_->page()->action(QWebPage::Back)->shortcut());

  forwardWebPageAct_->setText(widget->webView_->page()->action(QWebPage::Forward)->text());
  forwardWebPageAct_->setToolTip(widget->webView_->page()->action(QWebPage::Forward)->toolTip() + " " + tr("(Browser)"));
  forwardWebPageAct_->setIcon(widget->webView_->page()->action(QWebPage::Forward)->icon());
  forwardWebPageAct_->setShortcut(widget->webView_->page()->action(QWebPage::Forward)->shortcut());

  reloadWebPageAct_->setText(widget->webView_->page()->action(QWebPage::Reload)->text());
  reloadWebPageAct_->setToolTip(widget->webView_->page()->action(QWebPage::Reload)->toolTip() + " " + tr("(Browser)"));
  reloadWebPageAct_->setIcon(widget->webView_->page()->action(QWebPage::Reload)->icon());
  reloadWebPageAct_->setShortcut(widget->webView_->page()->action(QWebPage::Reload)->shortcut());

  optionsDialog_->loadActionShortcut(listActions_, &listDefaultShortcut_);

  // Display setting dialog

  optionsDialog_->setCurrentItem(pageIndex);
  int result = optionsDialog_->exec();
  pageIndex = optionsDialog_->currentIndex();

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
    QTreeWidgetItem *labelTreeItem = categoriesTree_->topLevelItem(CategoriesTreeWidget::LabelsItem);
    while (labelTreeItem->childCount()) {
      labelTreeItem->removeChild(labelTreeItem->child(0));
    }

    bool closeTab = true;
    int indexTab = -1;
    int tabLabelId = -1;
    for (int i = 0; i < stackedWidget_->count(); i++) {
      NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
      if (widget->type_ == NewsTabWidget::TabTypeLabel) {
        indexTab = i;
        tabLabelId = widget->labelId_;
        break;
      }
    }

    QSqlQuery q;
    q.exec("SELECT id, name, image, currentNews, num, color_bg, color_text FROM labels ORDER BY num");
    while (q.next()) {
      int idLabel = q.value(0).toInt();
      QString nameLabel = q.value(1).toString();
      QByteArray byteArray = q.value(2).toByteArray();
      QString currentNews = q.value(3).toString();
      QPixmap imageLabel;
      if (!byteArray.isNull())
        imageLabel.loadFromData(byteArray);
      QStringList dataItem;
      dataItem << nameLabel << QString::number(NewsTabWidget::TabTypeLabel)
               << QString::number(idLabel) << currentNews;
      QTreeWidgetItem *childItem = new QTreeWidgetItem(dataItem);
      childItem->setIcon(0, QIcon(imageLabel));
      childItem->setData(0, CategoriesTreeWidget::ImageRole, q.value(2));
      childItem->setData(0, CategoriesTreeWidget::NumRole, q.value(4));
      childItem->setData(0, CategoriesTreeWidget::colorBgRole, q.value(5));
      childItem->setData(0, CategoriesTreeWidget::colorTextRole, q.value(6));
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
      slotUpdateNews(NewsTabWidget::RefreshWithPos);
    }
  }

  showSplashScreen_ = optionsDialog_->showSplashScreen_->isChecked();
  reopenFeedStartup_ = optionsDialog_->reopenFeedStartup_->isChecked();
  openNewTabNextToActive_ = optionsDialog_->openNewTabNextToActive_->isChecked();
  hideFeedsOpenTab_ = optionsDialog_->hideFeedsOpenTab_->isChecked();
  showToggleFeedsTree_ = optionsDialog_->showToggleFeedsTree_->isChecked();
  defaultIconFeeds_ = optionsDialog_->defaultIconFeeds_->isChecked();
  feedsModel_->defaultIconFeeds_ = defaultIconFeeds_;
  feedsView_->autocollapseFolder_ = optionsDialog_->autocollapseFolder_->isChecked();

  showCloseButtonTab = optionsDialog_->showCloseButtonTab_->isChecked();
  settings.setValue("Settings/showCloseButtonTab", showCloseButtonTab);

  pushButtonNull_->setVisible(showToggleFeedsTree_);

  updateCheckEnabled = optionsDialog_->updateCheckEnabled_->isChecked();
  settings.setValue("Settings/updateCheckEnabled", updateCheckEnabled);
  statisticsEnabled = optionsDialog_->statisticsEnabled_->isChecked();
  settings.setValue("Settings/statisticsEnabled2", statisticsEnabled);

  storeDBMemory_ = optionsDialog_->storeDBMemory_->isChecked();
  settings.setValue("Settings/storeDBMemory", storeDBMemory_);
  if (saveDBMemFileInterval != optionsDialog_->saveDBMemFileInterval_->value()) {
    saveDBMemFileInterval = optionsDialog_->saveDBMemFileInterval_->value();
    settings.setValue("Settings/saveDBMemFileInterval", saveDBMemFileInterval);
    mainApp->updateFeeds()->startSaveTimer();
  }

  showTrayIcon_ = optionsDialog_->showTrayIconBox_->isChecked();
  startingTray_ = optionsDialog_->startingTray_->isChecked();
  minimizingTray_ = optionsDialog_->minimizingTray_->isChecked();
  closingTray_ = optionsDialog_->closingTray_->isChecked();
  behaviorIconTray_ = optionsDialog_->behaviorIconTray();
  if (behaviorIconTray_ > CHANGE_ICON_TRAY) {
    emit signalRefreshInfoTray();
  } else {
    traySystem->setIcon(QIcon(":/images/quiterss128"));
  }
  singleClickTray_ = optionsDialog_->singleClickTray_->isChecked();
  clearStatusNew_ = optionsDialog_->clearStatusNew_->isChecked();
  emptyWorking_ = optionsDialog_->emptyWorking_->isChecked();
  if (showTrayIcon_) traySystem->show();
  else traySystem->hide();

  mainApp->proxySaveSettings(optionsDialog_->proxy());

  timeoutRequest = optionsDialog_->timeoutRequest_->value();
  numberRequests = optionsDialog_->numberRequests_->value();
  numberRepeats = optionsDialog_->numberRepeats_->value();
  settings.setValue("Settings/timeoutRequest", timeoutRequest);
  settings.setValue("Settings/numberRequest", numberRequests);
  settings.setValue("Settings/numberRepeats", numberRepeats);

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
  autoLoadImages_ = optionsDialog_->autoLoadImages_->isChecked();
  javaScriptEnable_ = optionsDialog_->javaScriptEnable_->isChecked();
  pluginsEnable_ = optionsDialog_->pluginsEnable_->isChecked();
  openLinkInBackground_ = optionsDialog_->openLinkInBackground_->isChecked();
  openLinkInBackgroundEmbedded_ = optionsDialog_->openLinkInBackgroundEmbedded_->isChecked();
  maxPagesInCache_ = optionsDialog_->maxPagesInCache_->value();
  defaultZoomPages_ = optionsDialog_->defaultZoomPages_->value();

  QWebSettings::globalSettings()->setAttribute(
        QWebSettings::JavascriptEnabled, javaScriptEnable_);
  QWebSettings::globalSettings()->setAttribute(
        QWebSettings::PluginsEnabled, pluginsEnable_);
  QWebSettings::globalSettings()->setMaximumPagesInCache(maxPagesInCache_);

  settings.beginGroup("Settings");

  userStyleBrowser = optionsDialog_->userStyleBrowserEdit_->text();
  settings.setValue("userStyleBrowser", userStyleBrowser);

  useDiskCache = optionsDialog_->diskCacheOn_->isChecked();
  settings.setValue("useDiskCache", useDiskCache);
  maxDiskCache = optionsDialog_->maxDiskCache_->value();
  settings.setValue("maxDiskCache", maxDiskCache);

  if (diskCacheDir != optionsDialog_->dirDiskCacheEdit_->text()) {
    Common::removePath(diskCacheDir);
  }
  diskCacheDir = optionsDialog_->dirDiskCacheEdit_->text();
  if (diskCacheDir.isEmpty()) diskCacheDir = mainApp->cacheDefaultDir();
  settings.setValue("dirDiskCache", diskCacheDir);

  settings.endGroup();

  mainApp->setDiskCache();

  useCookies = SaveCookies;
  if (optionsDialog_->deleteCookiesOnClose_->isChecked())
    useCookies = DeleteCookiesOnClose;
  else if (optionsDialog_->blockCookies_->isChecked())
    useCookies = BlockCookies;
  mainApp->cookieJar()->setUseCookies(useCookies);

  downloadLocation_ = optionsDialog_->downloadLocationEdit_->text();
  askDownloadLocation_ = optionsDialog_->askDownloadLocation_->isChecked();

  updateFeedsStartUp = optionsDialog_->updateFeedsStartUp_->isChecked();
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
  feedsModel_->formatDate_ = formatDate_;
  formatTime_ = optionsDialog_->formatTime_->itemData(
        optionsDialog_->formatTime_->currentIndex()).toString();
  feedsModel_->formatTime_ = formatTime_;

  alternatingRowColorsNews_ = optionsDialog_->alternatingRowColorsNews_->isChecked();
  changeBehaviorActionNUN_ = optionsDialog_->changeBehaviorActionNUN_->isChecked();
  simplifiedDateTime_ = optionsDialog_->simplifiedDateTime_->isChecked();
  notDeleteStarred_ = optionsDialog_->notDeleteStarred_->isChecked();
  notDeleteLabeled_ = optionsDialog_->notDeleteLabeled_->isChecked();
  markIdenticalNewsRead_ = optionsDialog_->markIdenticalNewsRead_->isChecked();
  avoidOldNews_ = optionsDialog_->avoidedOldNewsDateOn_->isChecked();

  if (!optionsDialog_->avoidedOldNewsDate_->selectedDate().isNull() && optionsDialog_->avoidedOldNewsDate_->selectedDate().isValid()) {
    avoidedOldNewsDate_ = optionsDialog_->avoidedOldNewsDate_->selectedDate();
  } else {
    avoidedOldNewsDate_ = QDate::currentDate();
  }

  mainNewsFilter_ = optionsDialog_->mainNewsFilter_->itemData(
        optionsDialog_->mainNewsFilter_->currentIndex()).toString();

  styleSheetNews = optionsDialog_->styleSheetNewsEdit_->text();
  if (styleSheetNews.isEmpty()) styleSheetNews = mainApp->styleSheetNewsDefaultFile();
  settings.setValue("Settings/styleSheetNews", styleSheetNews);

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

  soundNewNews_ = optionsDialog_->soundNotifyBox_->isChecked();
  soundNotifyPath_ = optionsDialog_->editSoundNotifer_->text();
  showNotifyOn_ = optionsDialog_->showNotifyOn_->isChecked();
  screenNotify_ = optionsDialog_->screenNotify_->currentIndex();
  positionNotify_ = optionsDialog_->positionNotify_->currentIndex();
  transparencyNotify_ = optionsDialog_->transparencyNotify_->value();
  countShowNewsNotify_ = optionsDialog_->countShowNewsNotify_->value();
  widthTitleNewsNotify_ = optionsDialog_->widthTitleNewsNotify_->value();
  timeShowNewsNotify_ = optionsDialog_->timeShowNewsNotify_->value();
  fullscreenModeNotify_ = optionsDialog_->fullscreenModeNotify_->isChecked();
  showNotifyInactiveApp_ = optionsDialog_->showNotifyInactiveApp_->isChecked();
  onlySelectedFeeds_ = optionsDialog_->onlySelectedFeeds_->isChecked();
  showTitlesFeedsNotify_ = optionsDialog_->showTitlesFeedsNotify_->isChecked();
  showIconFeedNotify_ = optionsDialog_->showIconFeedNotify_->isChecked();
  showButtonMarkAllNotify_ = optionsDialog_->showButtonMarkAllNotify_->isChecked();
  showButtonMarkReadNotify_ = optionsDialog_->showButtonMarkReadNotify_->isChecked();
  showButtonExBrowserNotify_ = optionsDialog_->showButtonExBrowserNotify_->isChecked();
  showButtonDeleteNotify_ = optionsDialog_->showButtonDeleteNotify_->isChecked();
  closeNotify_ = optionsDialog_->closeNotify_->isChecked();

  mainApp->setLanguage(optionsDialog_->language());
  mainApp->setTranslateApplication();

  QFont font = feedsView_->font();
  font.setFamily(
        optionsDialog_->fontsTree_->topLevelItem(0)->text(2).section(", ", 0, 0));
  font.setPointSize(
        optionsDialog_->fontsTree_->topLevelItem(0)->text(2).section(", ", 1).toInt());
  feedsView_->setFont(font);
  feedsModel_->font_ = font;
  categoriesTree_->setFont(font);

  newsListFontFamily_ = optionsDialog_->fontsTree_->topLevelItem(1)->text(2).section(", ", 0, 0);
  newsListFontSize_ = optionsDialog_->fontsTree_->topLevelItem(1)->text(2).section(", ", 1).toInt();
  newsTitleFontFamily_ = optionsDialog_->fontsTree_->topLevelItem(2)->text(2).section(", ", 0, 0);
  newsTitleFontSize_ = optionsDialog_->fontsTree_->topLevelItem(2)->text(2).section(", ", 1).toInt();
  newsTextFontFamily_ = optionsDialog_->fontsTree_->topLevelItem(3)->text(2).section(", ", 0, 0);
  newsTextFontSize_ = optionsDialog_->fontsTree_->topLevelItem(3)->text(2).section(", ", 1).toInt();
  notificationFontFamily_ = optionsDialog_->fontsTree_->topLevelItem(4)->text(2).section(", ", 0, 0);
  notificationFontSize_ = optionsDialog_->fontsTree_->topLevelItem(4)->text(2).section(", ", 1).toInt();

  browserStandardFont = optionsDialog_->browserStandardFont_->currentFont().family();
  browserFixedFont = optionsDialog_->browserFixedFont_->currentFont().family();
  browserSerifFont = optionsDialog_->browserSerifFont_->currentFont().family();
  browserSansSerifFont = optionsDialog_->browserSansSerifFont_->currentFont().family();
  browserCursiveFont = optionsDialog_->browserCursiveFont_->currentFont().family();
  browserFantasyFont = optionsDialog_->browserFantasyFont_->currentFont().family();
  browserDefaultFontSize = optionsDialog_->browserDefaultFontSize_->value();
  browserFixedFontSize = optionsDialog_->browserFixedFontSize_->value();
  browserMinFontSize = optionsDialog_->browserMinFontSize_->value();
  browserMinLogFontSize = optionsDialog_->browserMinLogFontSize_->value();

  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::StandardFont, browserStandardFont);
  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::FixedFont, browserFixedFont);
  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::SerifFont, browserSerifFont);
  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::SansSerifFont, browserSansSerifFont);
  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::CursiveFont, browserCursiveFont);
  QWebSettings::globalSettings()->setFontFamily(
        QWebSettings::FantasyFont, browserFantasyFont);
  QWebSettings::globalSettings()->setFontSize(
        QWebSettings::DefaultFontSize, browserDefaultFontSize);
  QWebSettings::globalSettings()->setFontSize(
        QWebSettings::DefaultFixedFontSize, browserFixedFontSize);
  QWebSettings::globalSettings()->setFontSize(
        QWebSettings::MinimumFontSize, browserMinFontSize);
  QWebSettings::globalSettings()->setFontSize(
        QWebSettings::MinimumLogicalFontSize, browserMinLogFontSize);

  settings.beginGroup("Settings");
  settings.setValue("browserStandardFont", browserStandardFont);
  settings.setValue("browserFixedFont", browserFixedFont);
  settings.setValue("browserSerifFont", browserSerifFont);
  settings.setValue("browserSansSerifFont", browserSansSerifFont);
  settings.setValue("browserCursiveFont", browserCursiveFont);
  settings.setValue("browserFantasyFont", browserFantasyFont);
  settings.setValue("browserDefaultFontSize", browserDefaultFontSize);
  settings.setValue("browserFixedFontSize", browserFixedFontSize);
  settings.setValue("browserMinFontSize", browserMinFontSize);
  settings.setValue("browserMinLogFontSize", browserMinLogFontSize);
  settings.endGroup();

  feedsModel_->textColor_ = optionsDialog_->colorsTree_->topLevelItem(0)->text(1);
  feedsModel_->backgroundColor_ = optionsDialog_->colorsTree_->topLevelItem(1)->text(1);
  feedsView_->setStyleSheet(QString("#feedsView_ {background: %1;}").arg(feedsModel_->backgroundColor_));
  newsListTextColor_ = optionsDialog_->colorsTree_->topLevelItem(2)->text(1);
  newsListBackgroundColor_ = optionsDialog_->colorsTree_->topLevelItem(3)->text(1);
  focusedNewsTextColor_ = optionsDialog_->colorsTree_->topLevelItem(4)->text(1);
  focusedNewsBGColor_ = optionsDialog_->colorsTree_->topLevelItem(5)->text(1);
  linkColor_ = optionsDialog_->colorsTree_->topLevelItem(6)->text(1);
  titleColor_ = optionsDialog_->colorsTree_->topLevelItem(7)->text(1);
  dateColor_ = optionsDialog_->colorsTree_->topLevelItem(8)->text(1);
  authorColor_ = optionsDialog_->colorsTree_->topLevelItem(9)->text(1);
  newsTextColor_ = optionsDialog_->colorsTree_->topLevelItem(10)->text(1);
  newsTitleBackgroundColor_ = optionsDialog_->colorsTree_->topLevelItem(11)->text(1);
  newsBackgroundColor_ = optionsDialog_->colorsTree_->topLevelItem(12)->text(1);
  feedsModel_->feedWithNewNewsColor_ = optionsDialog_->colorsTree_->topLevelItem(13)->text(1);
  feedsModel_->countNewsUnreadColor_ = optionsDialog_->colorsTree_->topLevelItem(14)->text(1);
  newNewsTextColor_ = optionsDialog_->colorsTree_->topLevelItem(15)->text(1);
  unreadNewsTextColor_ = optionsDialog_->colorsTree_->topLevelItem(16)->text(1);
  feedsModel_->focusedFeedTextColor_ = optionsDialog_->colorsTree_->topLevelItem(17)->text(1);
  feedsModel_->focusedFeedBGColor_ = optionsDialog_->colorsTree_->topLevelItem(18)->text(1);
  feedsModel_->feedDisabledUpdateColor_ = optionsDialog_->colorsTree_->topLevelItem(19)->text(1);
  alternatingRowColors_ = optionsDialog_->colorsTree_->topLevelItem(20)->text(1);
  notifierTextColor_ = optionsDialog_->colorsTree_->topLevelItem(21)->text(1);
  notifierBackgroundColor_ = optionsDialog_->colorsTree_->topLevelItem(22)->text(1);

  delete optionsDialog_;
  optionsDialog_ = NULL;

  settings.beginGroup("Settings");
  settings.setValue("autoUpdatefeedsStartUp", updateFeedsStartUp);
  settings.endGroup();

  saveSettings();
  saveActionShortcuts();
  mainApp->reloadUserStyleBrowser();

  if (currentNewsTab != NULL) {
    if (currentNewsTab->type_ < NewsTabWidget::TabTypeWeb)
      currentNewsTab->newsHeader_->saveStateColumns(currentNewsTab);
    currentNewsTab->setSettings(false);
  }
}

void MainWindow::showSettingPageLabels()
{
  showOptionDlg(5);
}

// ----------------------------------------------------------------------------
void MainWindow::createTrayMenu()
{
  trayMenu_ = new QMenu(this);
  showWindowAct_ = new QAction(this);
  connect(showWindowAct_, SIGNAL(triggered()), this, SLOT(showWindows()));
  QFont font_ = showWindowAct_->font();
  font_.setBold(true);
  showWindowAct_->setFont(font_);
  trayMenu_->addAction(showWindowAct_);
  trayMenu_->addAction(addFeedTrayAct_);
  trayMenu_->addAction(updateAllFeedsAct_);
  trayMenu_->addAction(markAllFeedsRead_);
  trayMenu_->addSeparator();

  trayMenu_->addAction(optionsAct_);
  trayMenu_->addSeparator();

  trayMenu_->addAction(exitAct_);
  traySystem->setContextMenu(trayMenu_);
}

/** @brief Free memory working set in Windows
 *---------------------------------------------------------------------------*/
void MainWindow::myEmptyWorkingSet()
{
#if defined(Q_OS_WIN)
  if (isHidden())
    EmptyWorkingSet(GetCurrentProcess());
#endif
}
// ----------------------------------------------------------------------------
void MainWindow::initUpdateFeeds()
{
  QSqlQuery q;

  if (onlySelectedFeeds_) {
    q.exec("SELECT feedId FROM feeds_ex WHERE value=1 AND name='showNotification'");
    while (q.next()) {
      idFeedsNotifyList_.append(q.value(0).toInt());
    }
  }

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
          this, SLOT(slotGetFeedsTimer()));
  updateFeedsTimer_->start(1000);
}
// ----------------------------------------------------------------------------
void MainWindow::slotGetFeedsTimer()
{
  if (updateFeedsEnable_) {
    updateTimeCount_++;
    if (updateTimeCount_ >= updateIntervalSec_) {
      updateTimeCount_ = 0;

      emit signalGetAllFeedsTimer();
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

      emit signalGetFeedTimer(feedId);
    }
  }
}
/** @brief Process update feed action
 *---------------------------------------------------------------------------*/
void MainWindow::slotGetFeed()
{
  QModelIndexList indexList = feedsView_->selectionModel()->selectedRows(0);
  if (indexList.count() <= 1) {
    indexList.clear();
    indexList.append(feedsProxyModel_->mapFromSource(feedsView_->selectIndex()));
  }
  QList<int> idList;
  foreach (QModelIndex indexProxy, indexList) {
    QModelIndex index = feedsProxyModel_->mapToSource(indexProxy);
    if (feedsModel_->isFolder(index)) {
      QList<int> list = UpdateObject::getIdFeedsInList(db_, feedsModel_->dataField(index, "id").toInt());
      foreach (int idFeed, list) {
        if (!idList.contains(idFeed)) {
          idList.append(idFeed);
          index = feedsModel_->indexById(idFeed);
          if (!feedsModel_->dataField(index, "disableUpdate").toBool()) {
            emit signalGetFeed(feedsModel_->dataField(index, "id").toInt(),
                               feedsModel_->dataField(index, "xmlUrl").toString(),
                               feedsModel_->dataField(index, "lastBuildDate").toDateTime(),
                               feedsModel_->dataField(index, "authentication").toInt());
          }
        }
      }

    } else {
      int idFeed = feedsModel_->dataField(index, "id").toInt();
      if (!idList.contains(idFeed)) {
        idList.append(idFeed);
        emit signalGetFeed(feedsModel_->dataField(index, "id").toInt(),
                           feedsModel_->dataField(index, "xmlUrl").toString(),
                           feedsModel_->dataField(index, "lastBuildDate").toDateTime(),
                           feedsModel_->dataField(index, "authentication").toInt());
      }
    }
  }
}

/** @brief Process update all feeds action
 *---------------------------------------------------------------------------*/
void MainWindow::slotGetAllFeeds()
{
  emit signalGetAllFeeds();
}

void MainWindow::slotStopUpdate()
{
  progressBar_->hide();
  emit signalStopUpdate();
}

/** @brief Show update progress bar after feed update has started
 *---------------------------------------------------------------------------*/
void MainWindow::showProgressBar(int maximum)
{
  if (maximum == 0) {
    isStartImportFeed_ = false;
    return;
  }

  Settings settings;
  settings.setValue("Flags/updatingFeeds", true);

  playSoundNewNews_ = false;

  progressBar_->setMaximum(maximum);
  progressBar_->show();
}
void MainWindow::slotSetValue(int value)
{
  if (progressBar_->isVisible())
    progressBar_->setValue(progressBar_->maximum() - value);
}
void MainWindow::showMessageStatusBar(QString message, int timeout)
{
  statusBar()->showMessage(message, timeout);
}
void MainWindow::slotCountsStatusBar(int unreadCount, int allCount)
{
  statusUnread_->setText(QString(" " + tr("Unread: %1") + " ").arg(unreadCount));
  statusAll_->setText(QString(" " + tr("All: %1") + " ").arg(allCount));
}
// ----------------------------------------------------------------------------
void MainWindow::slotVisibledFeedsWidget()
{
  if (tabBar_->currentIndex() == TAB_WIDGET_PERMANENT) {
    showFeedsTabPermanent_ = feedsWidgetVisibleAct_->isChecked();
  }

  feedsWidget_->setVisible(feedsWidgetVisibleAct_->isChecked());
  updateIconToolBarNull(feedsWidgetVisibleAct_->isChecked());
}
// ----------------------------------------------------------------------------
void MainWindow::updateIconToolBarNull(bool feedsWidgetVisible)
{
  if (feedsWidgetVisible)
    pushButtonNull_->setIcon(QIcon(":/images/images/triangleR.png"));
  else
    pushButtonNull_->setIcon(QIcon(":/images/images/triangleL.png"));
}

/** @brief Update status of current feed or feed of current tab
 *---------------------------------------------------------------------------*/
void MainWindow::slotUpdateStatus(int feedId, bool changed)
{
  emit signalUpdateStatus(feedId, changed);
}

/** @brief Set filter for viewing feeds and categories
 * @param pAct Filter mode
 * @param clicked Flag to call function after user click or from programm code
 *    true  - user click
 *    false - programm call
 *----------------------------------------------------------------------------*/
void MainWindow::setFeedsFilter(bool clicked)
{
  QList<int> idList;
  static bool setFilter = false;

  if (setFilter) return;
  setFilter = true;

  QAction* filterAct = feedsFilterGroup_->checkedAction();

  if (filterAct->objectName() == "filterFeedsNew_") {
    QModelIndex index = feedsProxyModel_->mapToSource(feedsView_->currentIndex());
    int newCount = feedsModel_->dataField(index, "newCount").toInt();
    if (!(clicked && !newCount)) {
      while (index.isValid()) {
        idList << feedsModel_->idByIndex(index);
        index = index.parent();
      }
    }
  } else if (filterAct->objectName() == "filterFeedsUnread_") {
    QModelIndex index = feedsProxyModel_->mapToSource(feedsView_->currentIndex());
    int unRead = feedsModel_->dataField(index, "unread").toInt();
    if (!(clicked && !unRead)) {
      while (index.isValid()) {
        idList << feedsModel_->idByIndex(index);
        index = index.parent();
      }
    }
  } else if (filterAct->objectName() == "filterFeedsStarred_") {
    QSqlQuery q;
    q.exec(QString("SELECT id, parentId FROM feeds WHERE label LIKE '%starred%'"));
    while (q.next()) {
      idList.append(q.value(0).toInt());
      int parentId = q.value(1).toInt();
      if (!idList.contains(parentId)) {
        while (parentId) {
          if (!idList.contains(parentId))
            idList.append(parentId);
          else break;
          QSqlQuery q1;
          q1.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(parentId));
          if (q1.next()) parentId = q1.value(0).toInt();
        }
      }
    }
  } else if (filterAct->objectName() == "filterFeedsError_") {
    QSqlQuery q;
    q.exec(QString("SELECT id, parentId FROM feeds WHERE status!=0 AND status!=''"));
    while (q.next()) {
      idList.append(q.value(0).toInt());
      int parentId = q.value(1).toInt();
      if (!idList.contains(parentId)) {
        while (parentId) {
          if (!idList.contains(parentId))
            idList.append(parentId);
          else break;
          QSqlQuery q1;
          q1.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(parentId));
          if (q1.next()) parentId = q1.value(0).toInt();
        }
      }
    }
  }

  if ((filterAct->objectName() == "filterFeedsNew_") ||
      (filterAct->objectName() == "filterFeedsUnread_")) {
    for (int i = 0; i < stackedWidget_->count(); i++) {
      if ((i == 0) && clicked && (stackedWidget_->currentIndex() == 0)) continue;
      NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
      if (!idList.contains(widget->feedId_)) {
        idList.append(widget->feedId_);
        QModelIndex index = feedsModel_->indexById(widget->feedId_).parent();
        while (index.isValid()) {
          int id = feedsModel_->idByIndex(index);
          if (!idList.contains(id))
            idList.append(id);
          index = index.parent();
        }
      }
    }
  }

  // Set filter
  feedsProxyModel_->setFilter(filterAct->objectName(), idList,
                              findFeeds_->findGroup_->checkedAction()->objectName(),
                              findFeeds_->text());

  feedsView_->restoreExpanded();

  feedsView_->clearSelection();
  feedsView_->setCurrentIndex(feedsView_->currentIndex());

  if (clicked && (tabBar_->currentIndex() == TAB_WIDGET_PERMANENT)) {
    slotFeedClicked(feedsView_->currentIndex());
  }

  if (filterAct->objectName() == "filterFeedsAll_") feedsFilter_->setIcon(QIcon(":/images/filterOff"));
  else feedsFilter_->setIcon(QIcon(":/images/filterOn"));

  // Store filter to enable it as "last used filter"
  if (filterAct->objectName() != "filterFeedsAll_")
    feedsFilterAction_ = filterAct;

  setFilter = false;
}

/** @brief Set filter for news list
 *---------------------------------------------------------------------------*/
void MainWindow::setNewsFilter(QAction* pAct, bool clicked)
{
  if (currentNewsTab == NULL) return;
  if (currentNewsTab->type_ >= NewsTabWidget::TabTypeWeb) {
    filterNewsAll_->setChecked(true);
    return;
  }

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
    mainApp->sqlQueryExec(qStr);
  }

  // Create filter for category or for feed
  if (feedsModel_->isFolder(feedsModel_->indexById(feedId))) {
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
  QString objectName = currentNewsTab->findText_->findGroup_->checkedAction()->objectName();
  if (objectName != "findInBrowserAct") {
    QString findText = currentNewsTab->findText_->text();
    if (!findText.isEmpty()) {
      findText = findText.replace("'", "''").toUpper();
      if (objectName == "findTitleAct") {
        filterStr.append(
              QString(" AND UPPER(title) LIKE '%%1%'").arg(findText));
      } else if (objectName == "findAuthorAct") {
        filterStr.append(
              QString(" AND UPPER(author_name) LIKE '%%1%'").arg(findText));
      } else if (objectName == "findCategoryAct") {
        filterStr.append(
              QString(" AND UPPER(category) LIKE '%%1%'").arg(findText));
      } else if (objectName == "findContentAct") {
        filterStr.append(
              QString(" AND (UPPER(content) LIKE '%%1%' OR UPPER(description) LIKE '%%1%')").
              arg(findText));
      } else if (objectName == "findLinkAct") {
        filterStr.append(
              QString(" AND link_href LIKE '%%1%'").
              arg(findText));
      } else {
        filterStr.append(
              QString(" AND (UPPER(title) LIKE '%%1%' OR UPPER(author_name) LIKE '%%1%' "
                      "OR UPPER(category) LIKE '%%1%' OR UPPER(content) LIKE '%%1%' "
                      "OR UPPER(description) LIKE '%%1%')").
              arg(findText));
      }
    }
  }

  newsModel_->setFilter(filterStr);
  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  if ((currentNewsTab->newsHeader_->sortIndicatorSection() == newsModel_->fieldIndex("read")) ||
      currentNewsTab->newsHeader_->sortIndicatorSection() == newsModel_->fieldIndex("starred")) {
    currentNewsTab->slotSort(currentNewsTab->newsHeader_->sortIndicatorSection(),
                             currentNewsTab->newsHeader_->sortIndicatorOrder());
  }

  currentNewsTab->loadNewspaper();

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

  // Store filter to enable it as "last used filter"
  if (pAct->objectName() != mainNewsFilter_)
    newsFilterAction_ = pAct;
}

/** @brief Mark feed Read while clicking on unfocused one
 *---------------------------------------------------------------------------*/
void MainWindow::setFeedRead(int type, int feedId, FeedReedType feedReadType,
                             NewsTabWidget *widgetTab, int idException)
{
  if ((type >= NewsTabWidget::TabTypeWeb) || (type == NewsTabWidget::TabTypeDel))
    return;

  if ((type == NewsTabWidget::TabTypeFeed) && (feedReadType != FeedReadSwitchingTab)) {
    if (feedId <= -1) return;

    QList<int> idNewsList;
    if (((feedReadType == FeedReadSwitchingFeed) && markReadSwitchingFeed_) ||
        ((feedReadType == FeedReadClosingTab) && markReadClosingTab_) ||
        ((feedReadType == FeedReadPlaceToTray) && markReadMinimize_)) {
      emit signalSetFeedRead(feedReadType, feedId, idException, idNewsList);
    } else {
      emit signalSetFeedRead(feedReadType, feedId, idException, idNewsList);
    }
  } else if (widgetTab) {
    int cnt = widgetTab->newsModel_->rowCount();
    if (cnt == 0) return;

    QList<int> idNewsList;
    for (int i = cnt-1; i >= 0; --i) {
      int newsId = widgetTab->newsModel_->index(i, widgetTab->newsModel_->fieldIndex("id")).data().toInt();
      idNewsList.append(newsId);
    }
    for (int i = 0; i < stackedWidget_->count(); i++) {
      NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
      if ((widget->type_ < NewsTabWidget::TabTypeWeb) &&
          !((feedReadType == FeedReadSwitchingFeed) && (i == TAB_WIDGET_PERMANENT))) {
        int row = widget->newsView_->currentIndex().row();
        int newsId = widget->newsModel_->index(row, widget->newsModel_->fieldIndex("id")).data().toInt();
        idNewsList.removeOne(newsId);
      }
    }
    emit signalSetFeedRead(FeedReadSwitchingTab, feedId, idException, idNewsList);
  }
}

/** @brief Mark feed read
 *---------------------------------------------------------------------------*/
void MainWindow::markFeedRead()
{
  bool openFeed = false;
  bool isFolder = false;

  QModelIndexList indexList = feedsView_->selectionModel()->selectedRows(0);
  if (indexList.count() <= 1) {
    indexList.clear();
    indexList.append(feedsProxyModel_->mapFromSource(feedsView_->selectIndex()));
  }
  QList<int> idList;
  for (int i = indexList.count()-1; i >= 0; --i) {
    QModelIndex index = feedsProxyModel_->mapToSource(indexList[i]);
    if (feedsModel_->isFolder(index)) {
      idList.append(feedsModel_->dataField(index, "id").toInt());
      indexList.removeAt(i);
    }
  }
  for (int i = indexList.count()-1; i >= 0; --i) {
    QModelIndex index = feedsProxyModel_->mapToSource(indexList[i]);
    int parentId = feedsModel_->dataField(index, "parentId").toInt();
    if (!idList.contains(parentId)) {
      idList.append(feedsModel_->dataField(index, "id").toInt());
    }
    indexList.removeAt(i);
  }
  foreach (int id, idList) {
    bool openFeedT = false;
    QModelIndex index = feedsModel_->indexById(id);
    int parentId = feedsModel_->dataField(index, "parentId").toInt();
    if (currentNewsTab->feedId_ == id) {
      openFeedT = true;
      openFeed = true;
    }
    if (currentNewsTab->feedParId_ == id) {
      openFeed = true;
    }
    if (currentNewsTab->feedId_ == parentId) {
      openFeed = true;
      isFolder = true;
    }
    if (feedsModel_->isFolder(index)) {
      if (currentNewsTab->feedId_ == id) {
        isFolder = true;
      }
      QList<int> list = UpdateObject::getIdFeedsInList(db_, id);
      foreach (int id1, list) {
        QModelIndex index1 = feedsModel_->indexById(id1);
        QModelIndex indexUnread = feedsModel_->indexSibling(index1, "unread");
        QModelIndex indexNew    = feedsModel_->indexSibling(index1, "newCount");
        feedsModel_->setData(indexUnread, 0);
        feedsModel_->setData(indexNew, 0);

        if (!openFeed) {
          int parentId1 = feedsModel_->dataField(index1, "parentId").toInt();
          if ((currentNewsTab->feedId_ == id1) || (currentNewsTab->feedId_ == parentId1)) {
            openFeed = true;
          }
        }
      }
    }

    QModelIndex indexUnread = feedsModel_->indexSibling(index, "unread");
    QModelIndex indexNew    = feedsModel_->indexSibling(index, "newCount");
    feedsModel_->setData(indexUnread, 0);
    feedsModel_->setData(indexNew, 0);
    emit signalMarkFeedRead(id, feedsModel_->isFolder(index), openFeedT);
  }

  // Update feed view with focus
  if (openFeed) {
    if ((tabBar_->currentIndex() == TAB_WIDGET_PERMANENT) && !isFolder) {
      signalRefreshNewsView(1);
    } else {
      signalRefreshNewsView(0);
      recountCategoryCounts();
      emit signalSetFeedsFilter(true);
    }
  }
  // Update feeds view without focus
  else {
    recountCategoryCounts();
    emit signalSetFeedsFilter();
  }
}

/** @brief Refresh news view (After mark all feeds or one feed read)
 *---------------------------------------------------------------------------*/
void MainWindow::slotRefreshNewsView(int nextUnread)
{
  if (nextUnread == 1) {
    feedsView_->clearSelection();
    QModelIndex indexNextUnread =
        feedsView_->indexNextUnread(feedsView_->currentIndex());
    feedsView_->setCurrentIndex(indexNextUnread);
    slotFeedClicked(indexNextUnread);
  } else if ((tabBar_->currentIndex() == TAB_WIDGET_PERMANENT) && (nextUnread == -1)) {
    QModelIndex index = feedsProxyModel_->index(-1, "text");
    feedsView_->setCurrentIndex(index);
    slotFeedClicked(index);
  } else {
    int currentRow = newsView_->currentIndex().row();

    newsModel_->select();

    while (newsModel_->canFetchMore())
      newsModel_->fetchMore();

    currentNewsTab->loadNewspaper(NewsTabWidget::RefreshWithPos);

    newsView_->setCurrentIndex(newsModel_->index(currentRow, newsModel_->fieldIndex("title")));
  }
}

// ----------------------------------------------------------------------------
void MainWindow::slotShowAboutDlg()
{
  AboutDialog *aboutDialog = new AboutDialog(mainApp->language(), this);
  aboutDialog->exec();
  delete aboutDialog;
}

/** @brief Call context menu of the feeds tree
 *----------------------------------------------------------------------------*/
void MainWindow::showContextMenuFeed(const QPoint &pos)
{
  slotFeedMenuShow();

  QModelIndex index = feedsView_->indexAt(pos);
  if (index.isValid()) {
    QRect rectText = feedsView_->visualRect(index);
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

      menu.exec(feedsView_->viewport()->mapToGlobal(pos));
    }
  } else {
    QMenu menu;
    menu.addAction(addAct_);

    menu.exec(feedsView_->viewport()->mapToGlobal(pos));
  }

  index = feedsProxyModel_->mapToSource(feedsView_->currentIndex());
  feedsView_->selectId_ = feedsModel_->idByIndex(index);

  feedProperties_->setEnabled(feedsView_->selectIndex().isValid());
}
// ----------------------------------------------------------------------------
void MainWindow::slotFeedMenuShow()
{
  feedProperties_->setEnabled(feedsView_->selectIndex().isValid());
}
// ----------------------------------------------------------------------------
void MainWindow::loadSettingsFeeds()
{
  markCurNewsRead_ = false;
  Settings settings;
  behaviorIconTray_ = settings.value("Settings/behaviorIconTray", NEW_COUNT_ICON_TRAY).toInt();

  QString filterName = settings.value("feedSettings/filterName", "filterFeedsAll_").toString();
  QList<QAction*> listActions = feedsFilterGroup_->actions();
  foreach(QAction *action, listActions) {
    if (action->objectName() == filterName) {
      action->setChecked(true);
      break;
    }
  }
  filterName = settings.value("newsSettings/filterName", "filterNewsAll_").toString();
  listActions = newsFilterGroup_->actions();
  foreach(QAction *action, listActions) {
    if (action->objectName() == filterName) {
      action->setChecked(true);
      break;
    }
  }

  setFeedsFilter(false);
}

/** @brief Restore feeds state on application startup
 *---------------------------------------------------------------------------*/
void MainWindow::restoreFeedsOnStartUp()
{
  qApp->processEvents();

  //* Restore current feed
  QModelIndex feedIndex = QModelIndex();
  if (reopenFeedStartup_) {
    Settings settings;
    int feedId = settings.value("feedSettings/currentId", 0).toInt();
    feedIndex = feedsProxyModel_->mapFromSource(feedId);
  }
  feedsView_->setCurrentIndex(feedIndex);
  updateCurrentTab_ = false;
  slotFeedClicked(feedIndex);
  currentNewsTab->webView_->setFocus();
  updateCurrentTab_ = true;

  slotUpdateStatus(-1, false);
  recountCategoryCounts();

  // Open feeds in tabs
  QSqlQuery q;
  q.exec(QString("SELECT id, parentId FROM feeds WHERE displayOnStartup=1"));
  while (q.next()) {
    creatFeedTab(q.value(0).toInt(), q.value(1).toInt());
  }
}
// ----------------------------------------------------------------------------
void MainWindow::slotFeedsFilter()
{
  if (feedsFilterGroup_->checkedAction()->objectName() == "filterFeedsAll_") {
    if (feedsFilterAction_ != NULL) {
      feedsFilterAction_->setChecked(true);
      setFeedsFilter();
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
    setFeedsFilter();
  }
}
// ----------------------------------------------------------------------------
void MainWindow::slotNewsFilter()
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
void MainWindow::slotShowUpdateAppDlg()
{
  UpdateAppDialog *updateAppDialog = new UpdateAppDialog(mainApp->language(), this);
  updateAppDialog->activateWindow();
  updateAppDialog->exec();
  delete updateAppDialog;
}
// ----------------------------------------------------------------------------
void MainWindow::retranslateStrings()
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

  mainMenuButton_->setToolTip(tr("Menu"));

  addAct_->setText(tr("&Add"));
  addAct_->setToolTip(tr("Add New Feed"));

  addFeedAct_->setText(tr("&Feed..."));
  addFeedAct_->setToolTip(tr("Add New Feed"));

  addFeedTrayAct_->setText(tr("Add Feed..."));

  addFolderAct_->setText(tr("F&older..."));
  addFolderAct_->setToolTip(tr("Add New Folder"));

  openFeedNewTabAct_->setText(tr("Open in New Tab"));

  deleteFeedAct_->setText(tr("&Delete..."));
  deleteFeedAct_->setToolTip(tr("Delete Selected Feed"));

  importFeedsAct_->setText(tr("&Import Feeds..."));
  importFeedsAct_->setToolTip(tr("Import Feeds from OPML File"));

  exportFeedsAct_->setText(tr("&Export Feeds..."));
  exportFeedsAct_->setToolTip(tr("Export Feeds to OPML File"));

  createBackupAct_->setText(tr("&Create Backup..."));
  showMenuBarAct_->setText(tr("S&how Menu Bar"));

  exitAct_->setText(tr("E&xit"));

  updateFeedAct_->setText(tr("Update Feed"));
  updateFeedAct_->setToolTip(tr("Update Current Feed"));

  updateAllFeedsAct_->setText(tr("Update All"));
  updateAllFeedsAct_->setToolTip(tr("Update All Feeds"));

  stopUpdateAct_->setText(tr("Stop Update Feeds"));
  stopUpdateAct_->setToolTip(tr("Stop Update Feeds"));

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
  filterFeedsError_->setText(tr("Show Not Working Feeds"));

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
  openDescriptionNewsAct_->setToolTip(tr("Open News Description"));
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
  copyLinkAct_->setToolTip(tr("Copy News Link"));

  restoreLastNewsAct_->setText(tr("Restore last deleted news"));

  markFeedRead_->setText(tr("Mark Read"));
  markFeedRead_->setToolTip(tr("Mark Feed Read"));
  feedProperties_->setText(tr("Properties"));
  feedProperties_->setToolTip(tr("Properties"));

  fileMenu_->setTitle(tr("&File"));
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

  layoutMenu_->setTitle(tr("Layout"));
  classicLayoutAct_->setText(tr("Classic"));
  newspaperLayoutAct_->setText(tr("Newspaper"));
  layoutToggle_->setText(tr("Layout"));

  styleMenu_->setTitle(tr("Application Style"));
  systemStyle_->setText(tr("System"));
  system2Style_->setText(tr("System2"));
  darkStyle_->setText(tr("Dark"));
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
  newsKeyPageUpAct_->setText(tr("News Page Up"));
  newsKeyPageDownAct_->setText(tr("News Page Down"));

  nextUnreadNewsAct_->setText(tr("Next Unread News"));
  prevUnreadNewsAct_->setText(tr("Previous Unread News"));

  switchFocusAct_->setText(tr("Switch Focus to Next Panel"));
  switchFocusAct_->setToolTip(
        tr("Switch Focus to Next Panel (Tree Feeds, List News, Browser)"));
  switchFocusPrevAct_->setText(tr("Switch Focus to Previous Panel"));
  switchFocusPrevAct_->setToolTip(
        tr("Switch Focus to Previous Panel (Tree Feeds, Browser, List News)"));

  feedsWidgetVisibleAct_->setText(tr("Tree Feeds"));
  feedsWidgetVisibleAct_->setToolTip(tr("Show/Hide Tree Feeds"));
  webWidgetVisibleAct_->setText(tr("Browser"));
  webWidgetVisibleAct_->setToolTip(tr("Show/Hide Browser"));

  placeToTrayAct_->setText(tr("Minimize to Tray"));
  placeToTrayAct_->setToolTip(tr("Minimize Application to Tray"));

  feedsColumnsMenu_->setTitle(tr("Columns"));
  showUnreadCount_->setText(tr("Count Unread News"));
  showUndeleteCount_->setText(tr("Count All News"));
  showLastUpdated_->setText(tr("Last Update"));

  indentationFeedsTreeAct_->setText(tr("Show Indentation"));

  findFeedAct_->setText(tr("Search Feed"));
  findFeedAct_->setToolTip(tr("Search Feed"));

  browserZoomMenu_->setTitle(tr("Zoom"));
  zoomInAct_->setText(tr("Zoom In"));
  zoomInAct_->setToolTip(tr("Zoom in in browser"));
  zoomOutAct_->setText(tr("Zoom Out"));
  zoomOutAct_->setToolTip(tr("Zoom out in browser"));
  zoomTo100Act_->setText(tr("100%"));
  zoomTo100Act_->setToolTip(tr("Reset browser zoom"));

  printAct_->setText(tr("Print..."));
  printAct_->setToolTip(tr("Print Web Page"));
  printPreviewAct_->setText(tr("Print Preview..."));
  printPreviewAct_->setToolTip(tr("Preview Web Page"));

  pageUpWebViewAct_->setText(tr("Page up (Browser)"));
  pageDownWebViewAct_->setText(tr("Page down (Browser)"));

  savePageAsAct_->setText(tr("Save As..."));
  savePageAsAct_->setToolTip(tr("Save Page As..."));
  savePageAsDescriptAct_->setText(tr("Save page in database"));
  savePageAsDescriptAct_->setToolTip(tr("Save page in database instead of news description"));

  toolbarsMenu_->setTitle(tr("Show/Hide"));
  mainToolbarToggle_->setText(tr("Main Toolbar"));
  feedsToolbarToggle_->setText(tr("Feeds Toolbar"));
  newsToolbarToggle_->setText(tr("News Toolbar"));
  browserToolbarToggle_->setText(tr("Browser Toolbar"));
  categoriesPanelToggle_->setText(tr("Panel Categories"));
  statusBarToggle_->setText(tr("Status Bar"));

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
  showLabelsMenuAct_->setText(tr("Show labels menu"));

  closeTabAct_->setText(tr("Close Tab"));
  closeOtherTabsAct_->setText(tr("Close Other Tabs"));
  closeAllTabsAct_->setText(tr("Close All Tabs"));
  nextTabAct_->setText(tr("Switch to next tab"));
  prevTabAct_->setText(tr("Switch to previous tab"));

  categoriesTree_->topLevelItem(CategoriesTreeWidget::UnreadItem)->setText(0, tr("Unread"));
  categoriesTree_->topLevelItem(CategoriesTreeWidget::StarredItem)->setText(0, tr("Starred"));
  categoriesTree_->topLevelItem(CategoriesTreeWidget::DeletedItem)->setText(0, tr("Deleted"));
  categoriesTree_->topLevelItem(CategoriesTreeWidget::LabelsItem)->setText(0, tr("Labels"));

  reduceNewsListAct_->setText(tr("Decrease news list/increase browser"));
  increaseNewsListAct_->setText(tr("Increase news list/decrease browser"));

  findTextAct_->setText(tr("Find"));

  openHomeFeedAct_->setText(tr("Open Homepage Feed"));
  sortedByTitleFeedsTreeAct_->setText(tr("Sort by Name"));
  collapseAllFoldersAct_->setText(tr("Collapse All Folders"));
  expandAllFoldersAct_->setText(tr("Expand All Folders"));
  nextFolderAct_->setText(tr("Next Folder"));
  prevFolderAct_->setText(tr("Previous Folder"));
  expandFolderAct_->setText(tr("Expand Folder"));

  settingPageLabelsAct_->setText(tr("Setting Page: Labels"));

  shareMenuAct_->setText(tr("Share"));

  newsSortByMenu_->setTitle(tr("Sort By"));
  newsSortOrderGroup_->actions().at(0)->setText(tr("Ascending"));
  newsSortOrderGroup_->actions().at(1)->setText(tr("Descending"));

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
        if (widget->type_ == NewsTabWidget::TabTypeLabel) {
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
  mainApp->downloadManager()->retranslateStrings();
  adblockIcon_->retranslateStrings();
  QApplication::translate("AdBlockCustomList", "Custom Rules");


  if ((mainApp->language() == "ar") || (mainApp->language() == "fa")) {
    QApplication::setLayoutDirection(Qt::RightToLeft);
    mainMenuButton_->setStyleSheet("#mainMenuButton { border: none; padding: 0px 5px 0px 0px; }");
  } else {
    QApplication::setLayoutDirection(Qt::LeftToRight);
    mainMenuButton_->setStyleSheet("#mainMenuButton { border: none; padding: 0px 0px 0px 5px; }");
  }

  /** Hack ltr <-> rtl**/
  int indexTab = tabBar_->addTab("");
  tabBar_->removeTab(indexTab);
}
// ----------------------------------------------------------------------------
void MainWindow::setToolBarStyle(const QString &styleStr)
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
void MainWindow::setToolBarIconSize(QToolBar *toolbar, const QString &iconSizeStr)
{
  if (iconSizeStr == "toolBarIconBig_") {
    toolbar->setIconSize(QSize(32, 32));
  } else if (iconSizeStr == "toolBarIconSmall_") {
    toolbar->setIconSize(QSize(18, 18));
  } else {
    toolbar->setIconSize(QSize(24, 24));
  }
}

/** @brief Call toolbar context menu
 *----------------------------------------------------------------------------*/
void MainWindow::showContextMenuToolBar(const QPoint &pos)
{
  QMenu menu;
  menu.addAction(customizeMainToolbarAct2_);
  menu.addSeparator();
  menu.addAction(toolBarLockAct_);
  menu.addAction(toolBarHideAct_);

  menu.exec(mainToolbar_->mapToGlobal(pos));
}
// ----------------------------------------------------------------------------
void MainWindow::lockMainToolbar(bool lock)
{
  mainToolbar_->setMovable(!lock);
}
// ----------------------------------------------------------------------------
void MainWindow::hideMainToolbar()
{
  mainToolbarToggle_->setChecked(false);
  mainToolbar_->hide();
}
// ----------------------------------------------------------------------------
void MainWindow::showFeedPropertiesDlg()
{
  if (!feedsView_->selectIndex().isValid()) {
    feedProperties_->setEnabled(false);
    return;
  }

  QPersistentModelIndex index = feedsView_->selectIndex();
  int feedId = feedsModel_->idByIndex(index);
  bool isFeed = (index.isValid() && feedsModel_->isFolder(index)) ? false : true;

  FeedPropertiesDialog *feedPropertiesDialog = new FeedPropertiesDialog(isFeed, this);

  FEED_PROPERTIES properties;
  FEED_PROPERTIES properties_tmp;

  QByteArray byteArray = feedsModel_->dataField(index, "image").toByteArray();
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
              feedsModel_->dataField(index, "text").toString() +
              "'");
  feedPropertiesDialog->setWindowTitle(str);

  properties.general.text =
      feedsModel_->dataField(index, "text").toString();
  properties.general.title =
      feedsModel_->dataField(index, "title").toString();
  properties.general.url =
      feedsModel_->dataField(index, "xmlUrl").toString();
  properties.general.homepage =
      feedsModel_->dataField(index, "htmlUrl").toString();
  properties.general.displayOnStartup =
      feedsModel_->dataField(index, "displayOnStartup").toInt();
  properties.display.displayEmbeddedImages =
      feedsModel_->dataField(index, "displayEmbeddedImages").toInt();
  properties.display.javaScriptEnable =
      feedsModel_->dataField(index, "javaScriptEnable").toInt();
  if (feedsModel_->dataField(index, "displayNews").toString().isEmpty())
    properties.display.displayNews = !showDescriptionNews_;
  else
    properties.display.displayNews =
        feedsModel_->dataField(index, "displayNews").toInt();
  properties.display.layoutDirection =
      feedsModel_->dataField(index, "layoutDirection").toInt();

  properties.general.disableUpdate =
      feedsModel_->dataField(index, "disableUpdate").toBool();

  if (feedsModel_->dataField(index, "updateIntervalEnable").isNull() ||
      (feedsModel_->dataField(index, "updateIntervalEnable").toInt() == -1)) {
    properties.general.updateEnable = updateFeedsEnable_;
    properties.general.updateInterval = updateFeedsInterval_;
    properties.general.intervalType = updateFeedsIntervalType_;
  } else {
    properties.general.updateEnable =
        feedsModel_->dataField(index, "updateIntervalEnable").toBool();
    properties.general.updateInterval =
        feedsModel_->dataField(index, "updateInterval").toInt();
    properties.general.intervalType =
        feedsModel_->dataField(index, "updateIntervalType").toInt();
  }

  if (feedsModel_->dataField(index, "label").toString().contains("starred"))
    properties.general.starred = true;
  else
    properties.general.starred = false;

  properties.general.duplicateNewsMode =
      feedsModel_->dataField(index, "duplicateNewsMode").toBool();

  properties.general.addSingleNewsAnyDateOn =
      feedsModel_->dataField(index, "addSingleNewsAnyDateOn").toBool();

  properties.general.avoidedOldSingleNewsDateOn =
      feedsModel_->dataField(index, "avoidedOldSingleNewsDateOn").toBool();

  properties.general.avoidedOldSingleNewsDate =
      feedsModel_->dataField(index, "avoidedOldSingleNewsDate").toDate();

  Settings settings;
  settings.beginGroup("NewsHeader");
  QString indexColumnsStr = settings.value("columns").toString();
  QStringList indexColumnsList = indexColumnsStr.split(",", QString::SkipEmptyParts);
  foreach (QString indexStr, indexColumnsList) {
    properties.columnDefault.columns.append(indexStr.toInt());
  }
  NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);
  int sortBy = settings.value("sortBy", widget->newsModel_->fieldIndex("published")).toInt();
  properties.columnDefault.sortBy = sortBy;
  int sortType = settings.value("sortOrder", Qt::DescendingOrder).toInt();
  properties.columnDefault.sortType = sortType;
  settings.endGroup();

  if (feedsModel_->dataField(index, "columns").toString().isEmpty()) {
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
    indexColumnsStr = feedsModel_->dataField(index, "columns").toString();
    indexColumnsList = indexColumnsStr.split(",", QString::SkipEmptyParts);
    foreach (QString indexStr, indexColumnsList) {
      properties.column.columns.append(indexStr.toInt());
    }
    properties.column.sortBy = feedsModel_->dataField(index, "sort").toInt();
    properties.column.sortType = feedsModel_->dataField(index, "sortType").toInt();
  }

  properties.authentication.on = false;
  if (feedsModel_->dataField(index, "authentication").toInt() == 1) {
    properties.authentication.on = true;
  }
  QUrl url(feedsModel_->dataField(index, "xmlUrl").toString());
  QSqlQuery q;
  q.prepare("SELECT username, password FROM passwords WHERE server=?");
  q.addBindValue(url.host());
  q.exec();
  if (q.next()) {
    properties.authentication.user = q.value(0).toString();
    properties.authentication.pass = QString::fromUtf8(QByteArray::fromBase64(q.value(1).toByteArray()));
  }

  properties.status.feedStatus = feedsModel_->dataField(index, "status").toString();

  QDateTime dtLocalTime = QDateTime::currentDateTime();
  QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
  int nTimeShift = dtLocalTime.secsTo(dtUTC);

  QDateTime dt = QDateTime::fromString(
        feedsModel_->dataField(index, "created").toString(),
        Qt::ISODate);
  properties.status.createdTime = dt.addSecs(nTimeShift);

  dt = QDateTime::fromString(
        feedsModel_->dataField(index, "updated").toString(),
        Qt::ISODate);
  properties.status.lastUpdate = dt.addSecs(nTimeShift);

  dt = QDateTime::fromString(
        feedsModel_->dataField(index, "lastBuildDate").toString(),
        Qt::ISODate);
  properties.status.lastBuildDate = dt.addSecs(nTimeShift);

  properties.status.undeleteCount = feedsModel_->dataField(index, "undeleteCount").toInt();
  properties.status.newCount      = feedsModel_->dataField(index, "newCount").toInt();
  properties.status.unreadCount   = feedsModel_->dataField(index, "unread").toInt();
  properties.status.description   = feedsModel_->dataField(index, "description").toString();

  properties.status.feedsCount = 0;
  if (!isFeed) {
    QQueue<int> parentIds;
    parentIds.enqueue(feedId);
    while (!parentIds.empty()) {
      int parentId = parentIds.dequeue();
      q.exec(QString("SELECT id, xmlUrl FROM feeds WHERE parentId='%1'").arg(parentId));
      while (q.next()) {
        QString xmlUrl = q.value(1).toString();
        if (xmlUrl.isEmpty())
          parentIds.enqueue(q.value(0).toInt());
        else
          properties.status.feedsCount++;
      }
    }
  }

  feedPropertiesDialog->setFeedProperties(properties);
  properties_tmp = properties;

  int result = feedPropertiesDialog->exec();
  if (result == QDialog::Rejected) {
    delete feedPropertiesDialog;
    return;
  }

  if (!mainApp->storeDBMemory())
    db_.transaction();

  properties = feedPropertiesDialog->getFeedProperties();
  delete feedPropertiesDialog;

  index = feedsModel_->indexById(feedId);

  q.prepare("UPDATE feeds SET text = ?, xmlUrl = ?, displayOnStartup = ?, "
            "displayEmbeddedImages = ?, displayNews = ?, layoutDirection = ?, "
            "label = ?, duplicateNewsMode = ?, addSingleNewsAnyDateOn = ?, avoidedOldSingleNewsDateOn = ?, avoidedOldSingleNewsDate = ?,"
            " authentication = ?, disableUpdate = ?, "
            "javaScriptEnable = ? WHERE id == ?");
  q.addBindValue(properties.general.text);
  q.addBindValue(properties.general.url);
  q.addBindValue(properties.general.displayOnStartup);
  q.addBindValue(properties.display.displayEmbeddedImages);
  q.addBindValue(properties.display.displayNews);
  q.addBindValue(properties.display.layoutDirection);
  if (properties.general.starred)
    q.addBindValue("starred");
  else
    q.addBindValue("");
  q.addBindValue(properties.general.duplicateNewsMode ? 1 : 0);
  q.addBindValue(properties.general.addSingleNewsAnyDateOn ? 1 : 0);
  q.addBindValue(properties.general.avoidedOldSingleNewsDateOn ? 1 : 0);
  q.addBindValue(properties.general.avoidedOldSingleNewsDate);
  q.addBindValue(properties.authentication.on ? 1 : 0);
  q.addBindValue(properties.general.disableUpdate ? 1 : 0);
  q.addBindValue(properties.display.javaScriptEnable);
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

  QModelIndex indexColumns = feedsModel_->indexSibling(index, "columns");
  QModelIndex indexSort = feedsModel_->indexSibling(index, "sort");
  QModelIndex indexSortType = feedsModel_->indexSibling(index, "sortType");
  feedsModel_->setData(indexColumns, indexColumnsStr);
  feedsModel_->setData(indexSort, properties.column.sortBy);
  feedsModel_->setData(indexSortType, properties.column.sortType);

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

        QPersistentModelIndex index1 = feedsModel_->indexById(id);
        indexColumns = feedsModel_->indexSibling(index1, "columns");
        indexSort = feedsModel_->indexSibling(index1, "sort");
        indexSortType = feedsModel_->indexSibling(index1, "sortType");
        feedsModel_->setData(indexColumns, indexColumnsStr);
        feedsModel_->setData(indexSort, properties.column.sortBy);
        feedsModel_->setData(indexSortType, properties.column.sortType);

        if (currentNewsTab->feedId_ == id)
          currentNewsTab->newsHeader_->setColumns(index1);

        if (xmlUrl.isEmpty())
          parentIds.enqueue(id);
      }
    }
  }

  if (currentNewsTab->feedId_ == feedId)
    currentNewsTab->newsHeader_->setColumns(index);


  if (!(!feedsModel_->dataField(index, "authentication").toInt() && !properties.authentication.on)) {
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

  QPersistentModelIndex indexText    = feedsModel_->indexSibling(index, "text");
  QPersistentModelIndex indexUrl     = feedsModel_->indexSibling(index, "xmlUrl");
  QPersistentModelIndex indexStartup = feedsModel_->indexSibling(index, "displayOnStartup");
  QModelIndex indexImages  = feedsModel_->indexSibling(index, "displayEmbeddedImages");
  QModelIndex indexNews    = feedsModel_->indexSibling(index, "displayNews");
  QModelIndex indexRTL     = feedsModel_->indexSibling(index, "layoutDirection");
  QModelIndex indexLabel   = feedsModel_->indexSibling(index, "label");
  QModelIndex indexDuplicate = feedsModel_->indexSibling(index, "duplicateNewsMode");
  QModelIndex indexaddSingleNewsAnyDateOn = feedsModel_->indexSibling(index, "addSingleNewsAnyDateOn");
  QModelIndex indexavoidedOldSingleNewsDateOn = feedsModel_->indexSibling(index, "avoidedOldSingleNewsDateOn");
  QModelIndex indexavoidedOldSingleNewsDate = feedsModel_->indexSibling(index, "avoidedOldSingleNewsDate");
  QModelIndex indexAuthentication = feedsModel_->indexSibling(index, "authentication");
  QModelIndex indexDisableUpdate = feedsModel_->indexSibling(index, "disableUpdate");
  QModelIndex indexJavaScript = feedsModel_->indexSibling(index, "javaScriptEnable");
  feedsModel_->setData(indexText, properties.general.text);
  feedsModel_->setData(indexUrl, properties.general.url);
  feedsModel_->setData(indexStartup, properties.general.displayOnStartup);
  feedsModel_->setData(indexImages, properties.display.displayEmbeddedImages);
  feedsModel_->setData(indexNews, properties.display.displayNews);
  feedsModel_->setData(indexRTL, properties.display.layoutDirection);
  feedsModel_->setData(indexLabel, properties.general.starred ? "starred" : "");
  feedsModel_->setData(indexDuplicate, properties.general.duplicateNewsMode ? 1 : 0);
  feedsModel_->setData(indexaddSingleNewsAnyDateOn, properties.general.addSingleNewsAnyDateOn ? 1 : 0);
  feedsModel_->setData(indexavoidedOldSingleNewsDateOn, properties.general.avoidedOldSingleNewsDateOn ? 1 : 0);
  feedsModel_->setData(indexavoidedOldSingleNewsDate, properties.general.avoidedOldSingleNewsDate);
  feedsModel_->setData(indexAuthentication, properties.authentication.on ? 1 : 0);
  feedsModel_->setData(indexDisableUpdate, properties.general.disableUpdate ? 1 : 0);
  feedsModel_->setData(indexJavaScript, properties.display.javaScriptEnable);

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

    QPersistentModelIndex indexUpdateEnable   = feedsModel_->indexSibling(index, "updateIntervalEnable");
    QPersistentModelIndex indexUpdateInterval = feedsModel_->indexSibling(index, "updateInterval");
    QPersistentModelIndex indexIntervalType   = feedsModel_->indexSibling(index, "updateIntervalType");
    feedsModel_->setData(indexUpdateEnable, properties.general.updateEnable ? 1 : 0);
    feedsModel_->setData(indexUpdateInterval, properties.general.updateInterval);
    feedsModel_->setData(indexIntervalType, properties.general.intervalType);

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

          QPersistentModelIndex index1 = feedsModel_->indexById(id);
          indexUpdateEnable   = feedsModel_->indexSibling(index1, "updateIntervalEnable");
          indexUpdateInterval = feedsModel_->indexSibling(index1, "updateInterval");
          indexIntervalType   = feedsModel_->indexSibling(index1, "updateIntervalType");
          feedsModel_->setData(indexUpdateEnable, properties.general.updateEnable ? 1 : 0);
          feedsModel_->setData(indexUpdateInterval, properties.general.updateInterval);
          feedsModel_->setData(indexIntervalType, properties.general.intervalType);

          if (!xmlUrl.isEmpty()) {
            if (properties.general.updateEnable) {
              updateFeedsIntervalSec_.insert(id, updateInterval);
              updateFeedsTimeCount_.insert(id, 0);
            } else {
              updateFeedsIntervalSec_.remove(id);
              updateFeedsTimeCount_.remove(id);
            }
          } else {
            parentIds.enqueue(id);
          }
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

    QPersistentModelIndex indexUpdateEnable = feedsModel_->indexSibling(index, "updateIntervalEnable");
    feedsModel_->setData(indexUpdateEnable, "-1");

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

  if ((properties.display.layoutDirection  != properties_tmp.display.layoutDirection) &&
      !isFeed) {
    QQueue<int> parentIds;
    parentIds.enqueue(feedId);
    while (!parentIds.empty()) {
      int parentId = parentIds.dequeue();
      q.exec(QString("SELECT id, xmlUrl FROM feeds WHERE parentId='%1'").arg(parentId));
      while (q.next()) {
        int id = q.value(0).toInt();
        QString xmlUrl = q.value(1).toString();

        QSqlQuery q1;
        q1.prepare("UPDATE feeds SET layoutDirection = ? WHERE id == ?");
        q1.addBindValue(properties.display.layoutDirection);
        q1.addBindValue(id);
        q1.exec();

        QPersistentModelIndex index1 = feedsModel_->indexById(id);
        indexRTL = feedsModel_->indexSibling(index1, "layoutDirection");
        feedsModel_->setData(indexRTL, properties.display.layoutDirection);

        for (int i = 0; i < stackedWidget_->count(); i++) {
          NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
          if (widget->feedId_ == id) {
            widget->setSettings();
          }
        }

        if (xmlUrl.isEmpty())
          parentIds.enqueue(id);
      }
    }
  }

  for (int i = 0; i < stackedWidget_->count(); i++) {
    NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
    if (widget->feedId_ == feedId) {
      if (properties.general.text != properties_tmp.general.text) {
        widget->setTextTab(properties.general.text);
      }
      if ((properties.display.layoutDirection != properties_tmp.display.layoutDirection) ||
          (properties.display.javaScriptEnable != properties_tmp.display.javaScriptEnable)) {
        widget->setSettings();
      }
    }
  }

  if ((properties.general.disableUpdate != properties_tmp.general.disableUpdate) &&
      !isFeed) {
    QQueue<int> parentIds;
    parentIds.enqueue(feedId);
    while (!parentIds.empty()) {
      int parentId = parentIds.dequeue();
      q.exec(QString("SELECT id, xmlUrl FROM feeds WHERE parentId='%1'").arg(parentId));
      while (q.next()) {
        int id = q.value(0).toInt();
        QString xmlUrl = q.value(1).toString();

        QSqlQuery q1;
        q1.prepare("UPDATE feeds SET disableUpdate = ? WHERE id == ?");
        q1.addBindValue(properties.general.disableUpdate ? 1 : 0);
        q1.addBindValue(id);
        q1.exec();

        QPersistentModelIndex index1 = feedsModel_->indexById(id);
        indexDisableUpdate = feedsModel_->indexSibling(index1, "disableUpdate");
        feedsModel_->setData(indexDisableUpdate, properties.general.disableUpdate ? 1 : 0);

        if (xmlUrl.isEmpty())
          parentIds.enqueue(id);
      }
    }
  }

  if (!mainApp->storeDBMemory())
    db_.commit();
}

/** @brief Update tray information: icon and tooltip text
 *---------------------------------------------------------------------------*/
void MainWindow::slotRefreshInfoTray(int newCount, int unreadCount)
{
  if (!unreadCount)
    categoriesTree_->topLevelItem(CategoriesTreeWidget::UnreadItem)->setText(4, "");
  else
    categoriesTree_->topLevelItem(CategoriesTreeWidget::UnreadItem)->setText(4, QString("(%1)").arg(unreadCount));
  QFont font = categoriesTree_->topLevelItem(CategoriesTreeWidget::UnreadItem)->font(0);
  if (unreadCount)
    font.setBold(true);
  else
    font.setBold(false);
  categoriesTree_->topLevelItem(CategoriesTreeWidget::UnreadItem)->setFont(0, font);

  if (!showTrayIcon_) return;

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
          font.setPixelSize(60);
          trayCountStr = QString::number(trayCount);
        } else {
          font.setPixelSize(86);
          trayCountStr = "#";
        }
      } else {
        font.setBold(true);
        font.setPixelSize(90);
        trayCountStr = QString::number(trayCount);
      }

      // Draw icon, text above it, and set this icon to tray icon
      QPixmap icon(128, 128);
      icon.fill(Qt::transparent);
      QPainter trayPainter;
      trayPainter.begin(&icon);
      trayPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
      trayPainter.setRenderHint(QPainter::TextAntialiasing, true);
      QRect rectangle(0, 0, 128, 128);
      QLinearGradient gradient(rectangle.bottomLeft(), rectangle.topLeft());
      QColor color("#117C04");
      gradient.setColorAt(0, color.lighter());
      gradient.setColorAt(0.5, color);
      gradient.setColorAt(1, color.lighter());
      trayPainter.setBrush(gradient);
      trayPainter.drawRoundedRect(rectangle, 20, 20);
      trayPainter.setFont(font);
      trayPainter.setPen("#FFFFFF");
      trayPainter.drawText(rectangle, Qt::AlignVCenter | Qt::AlignHCenter,
                           trayCountStr);
      trayPainter.end();
      traySystem->setIcon(icon);
    }
    // Draw icon without number
    else {
      traySystem->setIcon(QIcon(":/images/quiterss128"));
    }
  }
}

/** @brief Prepare feed icon for storing in DB
 *---------------------------------------------------------------------------*/
void MainWindow::slotIconFeedPreparing(QString feedUrl, QByteArray byteArray,
                                       QString format)
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
void MainWindow::slotIconFeedUpdate(int feedId, QByteArray faviconData)
{
  QModelIndex index = feedsModel_->indexById(feedId);
  if (index.isValid()) {
    QModelIndex indexImage = feedsModel_->indexSibling(index, "image");
    feedsModel_->setData(indexImage, faviconData.toBase64());
    feedsView_->viewport()->update();
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
  if (currentNewsTab->type_ < NewsTabWidget::TabTypeWeb)
    currentNewsTab->newsView_->viewport()->update();
}
// ----------------------------------------------------------------------------
void MainWindow::slotPlaySound(const QString &path)
{
  QString soundPath = mainApp->absolutePath(path);

  if (!QFile::exists(soundPath)) {
    qWarning() << QString("Error playing sound: %1").arg(soundPath);
    return;
  }

  bool playing = false;
  Settings settings;
  bool useMediaPlayer = settings.value("Settings/useMediaPlayer", true).toBool();

  if (useMediaPlayer) {
#ifdef HAVE_QT5
    if (mediaPlayer_ == NULL) {
      playlist_ = new QMediaPlaylist(this);
      mediaPlayer_ = new QMediaPlayer(this);
      mediaPlayer_->setPlaylist(playlist_);
      connect(mediaPlayer_, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
              this, SLOT(mediaStatusChanged(QMediaPlayer::MediaStatus)));
      connect(mediaPlayer_, SIGNAL(error(QMediaPlayer::Error)),
              this, SLOT(mediaError(QMediaPlayer::Error)));
    }

    playlist_->addMedia(QUrl::fromLocalFile(soundPath));
    if (playlist_->currentIndex() == -1) {
      playlist_->setCurrentIndex(1);
      mediaPlayer_->play();
    }

    playing = true;
#else
#ifdef HAVE_PHONON
    if (mediaPlayer_ == NULL) {
      mediaPlayer_ = new Phonon::MediaObject(this);
      audioOutput_ = new Phonon::AudioOutput(Phonon::MusicCategory, this);
      Phonon::createPath(mediaPlayer_, audioOutput_);
      connect(mediaPlayer_, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
              this, SLOT(mediaStateChanged(Phonon::State,Phonon::State)));
    }

    if (mediaPlayer_->state() == Phonon::ErrorState)
      mediaPlayer_->clear();

    if ((mediaPlayer_->state() != Phonon::PausedState) &&
        (mediaPlayer_->state() != Phonon::StoppedState)) {
      mediaPlayer_->enqueue(soundPath);
    }
    else {
      mediaPlayer_->setCurrentSource(soundPath);
    }
    mediaPlayer_->play();

    playing = true;
#endif
#endif
  }

  if (!playing) {
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
    QSound::play(soundPath);
#else
    QProcess::startDetached(QString("play %1").arg(soundPath));
#endif
  }
}

#ifdef HAVE_QT5
void MainWindow::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
  if (status == QMediaPlayer::EndOfMedia) {
    playlist_->removeMedia(0);
  }
}

void MainWindow::mediaError(QMediaPlayer::Error error)
{
  QTextCodec *codec = QTextCodec::codecForLocale();
  qCritical() << QString("Error Media: %1 - %2").
                 arg(error).
                 arg(codec->toUnicode(mediaPlayer_->errorString().toUtf8()));
}
#endif

#ifdef HAVE_PHONON
void MainWindow::mediaStateChanged(Phonon::State newstate, Phonon::State)
{
  if (newstate == Phonon::ErrorState) {
    QTextCodec *codec = QTextCodec::codecForLocale();
    qCritical() << QString("Error Phonon: %1 - %2").
                   arg(mediaPlayer_->errorType()).
                   arg(codec->toUnicode(mediaPlayer_->errorString().toUtf8()));
  }
}
#endif

void MainWindow::slotPlaySoundNewNews()
{
  if (!playSoundNewNews_ && soundNewNews_) {
    slotPlaySound(soundNotifyPath_);
    playSoundNewNews_ = true;
  }
}

// ----------------------------------------------------------------------------
void MainWindow::showNewsFiltersDlg(bool newFilter)
{
  NewsFiltersDialog *newsFiltersDialog = new NewsFiltersDialog(this);
  if (newFilter) {
    newsFiltersDialog->filtersTree_->setCurrentItem(
          newsFiltersDialog->filtersTree_->topLevelItem(
            newsFiltersDialog->filtersTree_->topLevelItemCount()-1));
  }

  newsFiltersDialog->exec();

  delete newsFiltersDialog;
}
// ----------------------------------------------------------------------------
void MainWindow::showFilterRulesDlg()
{
  if (!feedsView_->selectIndex().isValid()) return;

  int feedId = feedsView_->selectId_;

  FilterRulesDialog *filterRulesDialog = new FilterRulesDialog(
        this, -1, feedId);

  QModelIndex index = feedsModel_->indexById(feedId);
  QString text = feedsModel_->dataField(index, "text").toString();
  filterRulesDialog->filterName_->setText(QString("'%1'").arg(text));

  int result = filterRulesDialog->exec();
  if (result == QDialog::Rejected) {
    delete filterRulesDialog;
    return;
  }

  delete filterRulesDialog;

  showNewsFiltersDlg(true);
}
// ----------------------------------------------------------------------------
void MainWindow::slotUpdateAppCheck()
{
  updateAppDialog_ = new UpdateAppDialog(mainApp->language(), this, false);
  connect(updateAppDialog_, SIGNAL(signalNewVersion(QString)),
          this, SLOT(slotNewVersion(QString)), Qt::QueuedConnection);
}
// ----------------------------------------------------------------------------
void MainWindow::slotNewVersion(const QString &newVersion)
{
  updateAppDialog_->disconnectObjects();
  delete updateAppDialog_;
  updateAppDialog_ = NULL;

  if (!newVersion.isEmpty()) {
    traySystem->showMessage(tr("Check for updates"),
                            tr("A new version of QuiteRSS..."));
    connect(traySystem, SIGNAL(messageClicked()),
            this, SLOT(slotShowUpdateAppDlg()));
  }
}

/** @brief Process Key_Up in feeds tree
 *---------------------------------------------------------------------------*/
void MainWindow::slotFeedUpPressed()
{
  QModelIndex indexBefore = feedsView_->currentIndex();
  QModelIndex indexAfter;

  // Jump to bottom in case of the most top index
  if (!indexBefore.isValid())
    indexAfter = feedsProxyModel_->index(feedsProxyModel_->rowCount()-1, "text");
  else
    indexAfter = feedsView_->indexAbove(indexBefore);

  // There is no "upper" index
  if (!indexAfter.isValid()) return;

  feedsView_->setCurrentIndex(indexAfter);
  slotFeedClicked(indexAfter);
}

/** @brief Process Key_Down in feeds tree
 *---------------------------------------------------------------------------*/
void MainWindow::slotFeedDownPressed()
{
  QModelIndex indexBefore = feedsView_->currentIndex();
  QModelIndex indexAfter;

  // Jump to top in case of the most bottom index
  if (!indexBefore.isValid())
    indexAfter = feedsProxyModel_->index(0, "text");
  else
    indexAfter = feedsView_->indexBelow(indexBefore);

  // There is no "downer" index
  if (!indexAfter.isValid()) return;

  feedsView_->setCurrentIndex(indexAfter);
  slotFeedClicked(indexAfter);
}

/** @brief Process previous feed shortcut
 *---------------------------------------------------------------------------*/
void MainWindow::slotFeedPrevious()
{
  QModelIndex indexBefore = feedsView_->currentIndex();
  if (!indexBefore.isValid())
    indexBefore = feedsProxyModel_->index(feedsProxyModel_->rowCount()-1, feedsView_->columnIndex("text"));

  QModelIndex indexAfter = feedsView_->indexPrevious(indexBefore);

  // There is no "upper" index
  if (!indexAfter.isValid()) return;
  feedsView_->clearSelection();

  feedsView_->setCurrentIndex(indexAfter);
  slotFeedClicked(indexAfter);
}

/** @brief Process next feed shortcut
 *---------------------------------------------------------------------------*/
void MainWindow::slotFeedNext()
{
  QModelIndex indexBefore = feedsView_->currentIndex();
  if (!indexBefore.isValid())
    indexBefore = feedsProxyModel_->index(0, feedsView_->columnIndex("text"));

  QModelIndex indexAfter = feedsView_->indexNext(indexBefore);

  // There is no "downer" index
  if (!indexAfter.isValid()) return;
  feedsView_->clearSelection();

  feedsView_->setCurrentIndex(indexAfter);
  slotFeedClicked(indexAfter);
}

/** @brief Process Key_Home in feeds tree
 *---------------------------------------------------------------------------*/
void MainWindow::slotFeedHomePressed()
{
  QModelIndex index = feedsProxyModel_->index(0, "text");
  feedsView_->setCurrentIndex(index);
  slotFeedClicked(index);
}

/** @brief Process Key_End in feeds tree
 *---------------------------------------------------------------------------*/
void MainWindow::slotFeedEndPressed()
{
  QModelIndex index = feedsProxyModel_->index(feedsProxyModel_->rowCount()-1, "text");
  feedsView_->setCurrentIndex(index);
  slotFeedClicked(index);
}

/** @brief Process pressing PageUp-key in feeds tree
 *----------------------------------------------------------------------------*/
void MainWindow::slotFeedPageUpPressed()
{
  int row = 0;
  QModelIndex index = feedsView_->currentIndex();
  if (index.isValid())
    row = index.row() - feedsView_->verticalScrollBar()->pageStep();
  if (row < 0)
    row = 0;
  index = feedsProxyModel_->index(row, "text");

  feedsView_->setCurrentIndex(index);
  slotFeedClicked(index);
}

/** @brief Process pressing PageDown-key in feeds tree
 *----------------------------------------------------------------------------*/
void MainWindow::slotFeedPageDownPressed()
{
  int row = 0;
  QModelIndex index = feedsView_->currentIndex();
  if (index.isValid())
    row = index.row() + feedsView_->verticalScrollBar()->pageStep();
  if (row >= feedsProxyModel_->rowCount())
    row = feedsProxyModel_->rowCount()-1;
  index = feedsProxyModel_->index(row, "text");

  feedsView_->setCurrentIndex(index);
  slotFeedClicked(index);
}

/** @brief Set application style
 *---------------------------------------------------------------------------*/
void MainWindow::setStyleApp(QAction *pAct)
{
  Settings settings;

  settings.setValue("Settings/styleApplication", pAct->objectName());

  QString fileName(mainApp->resourcesDir());
  if (pAct->objectName() == "systemStyle_") {
    fileName.append("/style/system.qss");
  } else if (pAct->objectName() == "system2Style_") {
    fileName.append("/style/system2.qss");
  } else if (pAct->objectName() == "darkStyle_") {
    fileName.append("/style/dark.qss");
  } else if (pAct->objectName() == "orangeStyle_") {
    fileName.append("/style/orange.qss");
  } else if (pAct->objectName() == "purpleStyle_") {
    fileName.append("/style/purple.qss");
  } else if (pAct->objectName() == "pinkStyle_") {
    fileName.append("/style/pink.qss");
  } else if (pAct->objectName() == "grayStyle_") {
    fileName.append("/style/gray.qss");
  } else {
    fileName.append("/style/green.qss");
  }

  QString userStyleBrowser = "";
  if (pAct->objectName() == "darkStyle_") {
    userStyleBrowser = mainApp->styleSheetWebDarkFile();
    feedsModel_->textColor_ = "#e1e0e1";
    newsListTextColor_ = "#e1e0e1";
    newsListBackgroundColor_ = "#464546";
    newNewsTextColor_ = "#e1e0e1";
    unreadNewsTextColor_ = "#e1e0e1";
    newsBackgroundColor_ = "#464546";
    newsTitleBackgroundColor_ = "#464546";
    titleColor_ = "#e1e0e1";
    newsTextColor_ = "#e1e0e1";
    dateColor_ = "#a5a5a5";
    authorColor_ = "#a5a5a5";
    notifierTextColor_ = "#e1e0e1";
    notifierBackgroundColor_ = "#464546";
    transparencyNotify_ = 40;
    alternatingRowColors_ = "#3a393a";
  } else {
    QString windowTextColor = qApp->palette().brush(QPalette::WindowText).color().name();
    feedsModel_->textColor_ = windowTextColor;
    newsListTextColor_ = windowTextColor;
    newsListBackgroundColor_ = "";
    newNewsTextColor_ = windowTextColor;
    unreadNewsTextColor_ = windowTextColor;
    newsBackgroundColor_ = "#FFFFFF";
    newsTitleBackgroundColor_ = "#FFFFFF";
    titleColor_ = "#0066CC";
    newsTextColor_ = "#000000";
    dateColor_ = "#666666";
    authorColor_ = "#666666";
    notifierTextColor_ = windowTextColor;
    notifierBackgroundColor_ = "#FFFFFF";
    transparencyNotify_ = 60;
    alternatingRowColors_ = qApp->palette().color(QPalette::AlternateBase).name();
  }

  settings.beginGroup("Settings");
  settings.setValue("userStyleBrowser", userStyleBrowser);
  settings.setValue("transparencyNotify", transparencyNotify_);
  settings.endGroup();
  settings.beginGroup("Color");
  settings.setValue("feedsListTextColor", feedsModel_->textColor_);
  settings.setValue("newsListTextColor", newsListTextColor_);
  settings.setValue("newsListBackgroundColor", newsListBackgroundColor_);
  settings.setValue("newNewsTextColor", newNewsTextColor_);
  settings.setValue("unreadNewsTextColor", unreadNewsTextColor_);
  settings.setValue("titleColor", titleColor_);
  settings.setValue("newsTextColor", newsTextColor_);
  settings.setValue("newsTitleBackgroundColor", newsTitleBackgroundColor_);
  settings.setValue("newsBackgroundColor", newsBackgroundColor_);
  settings.setValue("dateColor", dateColor_);
  settings.setValue("authorColor", authorColor_);
  settings.setValue("notifierTextColor", notifierTextColor_);
  settings.setValue("notifierBackgroundColor", notifierBackgroundColor_);
  settings.setValue("alternatingRowColors", alternatingRowColors_);
  settings.endGroup();

  QFile file(fileName);
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
        arg(feedsPanel_->palette().window().color().name()).
        arg(qApp->palette().color(QPalette::Dark).name()));

  mainApp->reloadUserStyleBrowser();
  if (currentNewsTab != NULL) {
    if (currentNewsTab->type_ < NewsTabWidget::TabTypeWeb)
      currentNewsTab->newsHeader_->saveStateColumns(currentNewsTab);
    currentNewsTab->setSettings(false);
  }
}

/** Switch focus forward between feed tree, news list and browser
 *---------------------------------------------------------------------------*/
void MainWindow::slotSwitchFocus()
{
  if (feedsView_->hasFocus()) {
    newsView_->setFocus();
  } else if (newsView_->hasFocus()) {
    currentNewsTab->webView_->setFocus();
  } else {
    feedsView_->setFocus();
  }
}

/** @brief Switch focus backward between feed tree, news list and browser
 *---------------------------------------------------------------------------*/
void MainWindow::slotSwitchPrevFocus()
{
  if (feedsView_->hasFocus()) {
    currentNewsTab->webView_->setFocus();
  } else if (currentNewsTab->webView_->hasFocus()) {
    newsView_->setFocus();
  } else {
    feedsView_->setFocus();
  }
}

/** @brief Open feed in a new tab
 *---------------------------------------------------------------------------*/
void MainWindow::slotOpenFeedNewTab()
{
  if (stackedWidget_->count() && currentNewsTab->type_ < NewsTabWidget::TabTypeWeb) {
    setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_, FeedReadSwitchingTab, currentNewsTab);
    currentNewsTab->newsHeader_->saveStateColumns(currentNewsTab);
    Settings settings;
    settings.setValue("NewsTabSplitterState", currentNewsTab->newsTabWidgetSplitter_->saveState());
  }

  QModelIndexList indexList = feedsView_->selectionModel()->selectedRows(0);
  if (indexList.count() <= 1) {
    indexList.clear();
    indexList.append(feedsProxyModel_->mapFromSource(feedsView_->selectIndex()));
  }
  QModelIndex index = indexList.takeFirst();
  feedsView_->selectIdEn_ = false;
  feedsView_->setCurrentIndex(index);
  slotFeedSelected(feedsProxyModel_->mapToSource(index), true);

  foreach (QModelIndex indexProxy, indexList) {
    QModelIndex index = feedsProxyModel_->mapToSource(indexProxy);
    creatFeedTab(feedsModel_->dataField(index, "id").toInt(),
                 feedsModel_->dataField(index, "parentId").toInt());
  }
}

void MainWindow::slotOpenCategoryNewTab()
{
  slotCategoriesClicked(categoriesTree_->currentItem(), 0, true);
}

/** @brief Close tab with \a index
 *---------------------------------------------------------------------------*/
void MainWindow::slotCloseTab(int index)
{
  if (index != 0) {
    NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(index);

    setFeedRead(widget->type_, widget->feedId_, FeedReadClosingTab, widget);

    stackedWidget_->removeWidget(widget);
    tabBar_->removeTab(index);
    widget->disconnectObjects();
    widget->newsTitleLabel_->deleteLater();
    widget->deleteLater();
  }
}

/** @brief Switch to tab with index \a index
 *---------------------------------------------------------------------------*/
void MainWindow::slotTabCurrentChanged(int index)
{
  if (!stackedWidget_->count()) return;
  if (tabBar_->closingTabState_ == TabBar::CloseTabOtherIndex) return;

  NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(index);

  if ((widget->type_ == NewsTabWidget::TabTypeFeed) || (widget->type_ >= NewsTabWidget::TabTypeWeb))
    categoriesTree_->setCurrentIndex(QModelIndex());
  if (widget->type_ != NewsTabWidget::TabTypeFeed) {
    feedsView_->setCurrentIndex(QModelIndex());
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

  if ((tabBar_->closingTabState_ == TabBar::CloseTabIdle) && (currentNewsTab->type_ < NewsTabWidget::TabTypeWeb)) {
    setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_, FeedReadSwitchingTab, currentNewsTab);

    currentNewsTab->newsHeader_->saveStateColumns(currentNewsTab);
    Settings settings;
    settings.setValue("NewsTabSplitterState", currentNewsTab->newsTabWidgetSplitter_->saveState());
  }

  if (widget->type_ == NewsTabWidget::TabTypeFeed) {
    if (widget->feedId_ == 0)
      widget->hide();
    currentNewsTab = (NewsTabWidget*)stackedWidget_->widget(index);
    currentNewsTab->setSettings(false);
    currentNewsTab->retranslateStrings();
    currentNewsTab->setBrowserPosition();

    newsModel_ = currentNewsTab->newsModel_;
    newsView_ = currentNewsTab->newsView_;

    QModelIndex feedIndex = feedsProxyModel_->mapFromSource(feedsModel_->indexById(widget->feedId_));
    feedsView_->setCurrentIndex(feedIndex);
    feedProperties_->setEnabled(feedIndex.isValid());

    setFeedsFilter(false);

    slotUpdateNews(NewsTabWidget::RefreshWithPos);
    if (widget->isVisible())
      newsView_->setFocus();
    else
      feedsView_->setFocus();

    statusUnread_->setVisible(widget->feedId_);
    statusAll_->setVisible(widget->feedId_);
  } else if (widget->type_ == NewsTabWidget::TabTypeWeb) {
    statusUnread_->setVisible(false);
    statusAll_->setVisible(false);
    currentNewsTab = widget;
    currentNewsTab->setSettings(false);
    currentNewsTab->retranslateStrings();
    currentNewsTab->webView_->setFocus();
  } else if (widget->type_ == NewsTabWidget::TabTypeDownloads) {
    statusUnread_->setVisible(false);
    statusAll_->setVisible(false);
    mainApp->downloadManager()->show();
    currentNewsTab = widget;
    currentNewsTab->retranslateStrings();
  } else {
    QList<QTreeWidgetItem *> treeItems;
    if (widget->type_ == NewsTabWidget::TabTypeLabel) {
      treeItems = categoriesTree_->findItems(QString::number(widget->labelId_),
                                             Qt::MatchFixedString|Qt::MatchRecursive,
                                             2);
    } else {
      treeItems = categoriesTree_->findItems(QString::number(widget->type_),
                                             Qt::MatchFixedString,
                                             1);
    }
    categoriesTree_->setCurrentItem(treeItems.at(0));

    currentNewsTab = (NewsTabWidget*)stackedWidget_->widget(index);
    currentNewsTab->setSettings(false);
    currentNewsTab->retranslateStrings();
    currentNewsTab->setBrowserPosition();

    newsModel_ = currentNewsTab->newsModel_;
    newsView_ = currentNewsTab->newsView_;

    slotUpdateNews(NewsTabWidget::RefreshWithPos);
    newsView_->setFocus();

    int unreadCount = widget->getUnreadCount(categoriesTree_->currentItem()->text(4));
    int allCount = widget->newsModel_->rowCount();
    statusUnread_->setText(QString(" " + tr("Unread: %1") + " ").arg(unreadCount));
    statusAll_->setText(QString(" " + tr("All: %1") + " ").arg(allCount));

    statusUnread_->setVisible(widget->type_ != NewsTabWidget::TabTypeDel);
    statusAll_->setVisible(true);
  }

  setTextTitle(widget->newsTitleLabel_->toolTip(), widget);
}

/** @brief Process tab moving
 *----------------------------------------------------------------------------*/
void MainWindow::slotTabMoved(int fromIndex, int toIndex)
{
  stackedWidget_->insertWidget(toIndex, stackedWidget_->widget(fromIndex));
}

/** @brief Manage displaying columns in feed tree
 *---------------------------------------------------------------------------*/
void MainWindow::feedsColumnVisible(QAction *action)
{
  int idx = action->data().toInt();
  if (action->isChecked())
    feedsView_->showColumn(idx);
  else
    feedsView_->hideColumn(idx);
}

void MainWindow::setNewsLayout(QAction *action)
{
  newsLayout_ = action->data().toInt();
  switch (newsLayout_) {
  case 1:
    layoutToggle_->setIcon(QIcon(":/images/layout_newspaper"));
    break;
  default:
    layoutToggle_->setIcon(QIcon(":/images/layout_classic"));
  }
  currentNewsTab->setNewsLayout();
}

void MainWindow::setNewsLayout()
{
  if ((newsLayout_ + 1) >= layoutGroup_->actions().count())
    layoutGroup_->actions().at(0)->setChecked(true);
  else
    layoutGroup_->actions().at(newsLayout_ + 1)->setChecked(true);
  setNewsLayout(layoutGroup_->checkedAction());
}

/** @brief Set browser position
 *---------------------------------------------------------------------------*/
void MainWindow::setBrowserPosition(QAction *action)
{
  browserPosition_ = action->data().toInt();
  currentNewsTab->setBrowserPosition();
}

/** @brief Create tab with browser only (without news list)
 *---------------------------------------------------------------------------*/
QWebPage *MainWindow::createWebTab(QUrl url)
{
  NewsTabWidget *widget = new NewsTabWidget(this, NewsTabWidget::TabTypeWeb);
  int indexTab = addTab(widget);
  widget->setTextTab(tr("Loading..."));

  if (openNewsTab_ == NEW_TAB_FOREGROUND) {
    currentNewsTab = widget;
    emit signalSetCurrentTab(indexTab);
  }

  widget->setSettings();
  widget->retranslateStrings();

  openNewsTab_ = 0;

  if (!url.isEmpty()) {
      widget->locationBar_->setText(url.toString());
      widget->webView_->load(url);
    }

  return widget->webView_->page();
}
// ----------------------------------------------------------------------------
void MainWindow::creatFeedTab(int feedId, int feedParId)
{
  QSqlQuery q;
  q.exec(QString("SELECT text, image, currentNews, xmlUrl FROM feeds WHERE id=='%1'").
         arg(feedId));

  if (q.next()) {
    NewsTabWidget *widget = new NewsTabWidget(this, NewsTabWidget::TabTypeFeed, feedId, feedParId);
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
    if (feedsModel_->isFolder(feedsModel_->indexById(feedId))) {
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

    currentNewsTab->loadNewspaper();

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
void MainWindow::slotOpenNewsWebView()
{
  if (!newsView_->hasFocus()) return;
  currentNewsTab->slotNewsViewSelected(newsView_->currentIndex());
}
// ----------------------------------------------------------------------------
void MainWindow::slotNewsUpPressed()
{
  currentNewsTab->slotNewsUpPressed();
}
// ----------------------------------------------------------------------------
void MainWindow::slotNewsDownPressed()
{
  currentNewsTab->slotNewsDownPressed();
}
// ----------------------------------------------------------------------------
void MainWindow::slotNewsPageUpPressed()
{
  currentNewsTab->slotNewsPageUpPressed();
}
// ----------------------------------------------------------------------------
void MainWindow::slotNewsPageDownPressed()
{
  currentNewsTab->slotNewsPageDownPressed();
}
// ----------------------------------------------------------------------------
void MainWindow::markNewsRead()
{
  currentNewsTab->markNewsRead();
}
// ----------------------------------------------------------------------------
void MainWindow::markAllNewsRead()
{
  currentNewsTab->markAllNewsRead();
}
// ----------------------------------------------------------------------------
void MainWindow::markNewsStar()
{
  currentNewsTab->markNewsStar();
}
// ----------------------------------------------------------------------------
void MainWindow::deleteNews()
{
  currentNewsTab->deleteNews();
}
// ----------------------------------------------------------------------------
void MainWindow::deleteAllNewsList()
{
  currentNewsTab->deleteAllNewsList();
}
// ----------------------------------------------------------------------------
void MainWindow::restoreNews()
{
  currentNewsTab->restoreNews();
}
// ----------------------------------------------------------------------------
void MainWindow::openInBrowserNews()
{
  currentNewsTab->openInBrowserNews();
}
// ----------------------------------------------------------------------------
void MainWindow::openInExternalBrowserNews()
{
  currentNewsTab->openInExternalBrowserNews();
}
// ----------------------------------------------------------------------------
void MainWindow::slotOpenNewsNewTab()
{
  openNewsTab_ = NEW_TAB_FOREGROUND;
  currentNewsTab->openNewsNewTab();
}
// ----------------------------------------------------------------------------
void MainWindow::slotOpenNewsBackgroundTab()
{
  openNewsTab_ = NEW_TAB_BACKGROUND;
  currentNewsTab->openNewsNewTab();
}

/** @brief Copy news URL-link
 *----------------------------------------------------------------------------*/
void MainWindow::slotCopyLinkNews()
{
  currentNewsTab->slotCopyLinkNews();
}

void MainWindow::slotShowLabelsMenu()
{
  currentNewsTab->showLabelsMenu();
}
// ----------------------------------------------------------------------------
void MainWindow::slotPageUpWebView()
{
  QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_PageUp, Qt::NoModifier);
  QApplication::sendEvent(currentNewsTab->webView_->page(), &keyEvent);
}

void MainWindow::slotPageDownWebView()
{
  QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_PageDown, Qt::NoModifier);
  QApplication::sendEvent(currentNewsTab->webView_->page(), &keyEvent);
}
/** @brief Reload full model
 * @details Performs: reload model, reset proxy model, restore focus
 *---------------------------------------------------------------------------*/
void MainWindow::feedsModelReload(bool checkFilter)
{
  if (checkFilter) {
    if (feedsFilterGroup_->checkedAction()->objectName() != "filterFeedsAll_") {
      setFeedsFilter(false);
    }
    slotFeedsViewportUpdate();
    return;
  }

  int topRow = feedsView_->verticalScrollBar()->value();
  QModelIndex feedIndex = feedsProxyModel_->mapToSource(feedsView_->currentIndex());
  int feedId = feedsModel_->idByIndex(feedIndex);

  feedsView_->refresh();

  feedIndex = feedsProxyModel_->mapFromSource(feedId);
  feedsView_->selectIdEn_ = false;
  feedsView_->setCurrentIndex(feedIndex);
  feedsView_->verticalScrollBar()->setValue(topRow);
}
// ----------------------------------------------------------------------------
void MainWindow::setCurrentTab(int index, bool updateCurrentTab)
{
  updateCurrentTab_ = updateCurrentTab;
  tabBar_->setCurrentIndex(index);
  updateCurrentTab_ = true;
}

/** @brief Set focus to search field (CTRL+F)
 *---------------------------------------------------------------------------*/
void MainWindow::findText()
{
  if (currentNewsTab->type_ < NewsTabWidget::TabTypeWeb) {
    if (!currentNewsTab->findText_->hasFocus())
      currentNewsTab->findText_->setFocus();
    else
      newsView_->setFocus();
  }
}

/** @brief Show notification on inbox news
 *---------------------------------------------------------------------------*/
void MainWindow::showNotification(bool bShowRecentNews/*=false*/)
{
  Settings settings;
  settings.setValue("Flags/updatingFeeds", false);

  bool showNotify = true;

  if (showNotifyInactiveApp_)
  {
    showNotify = !isActiveWindow();
  }

  NewNewsData& curNews = (bShowRecentNews ? recentNews : newNews);

  if (curNews.idFeedList_.isEmpty() || !showNotify || !showNotifyOn_)
  {
    clearNotification();
    return;
  }

  if (fullscreenModeNotify_) {
#if defined(Q_OS_WIN)
    HWND hWnd = GetForegroundWindow();
    RECT appBounds;
    RECT rc;
    GetWindowRect(GetDesktopWindow(), &rc);

    if ((hWnd != GetDesktopWindow())
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

  timerTrayOpenNotify.stop();


  if (notificationWidget != NULL)
  {
    delete notificationWidget;
  }

  notificationWidget = new NotificationWidget(curNews.idFeedList_, curNews.cntNewsList_,
                                              idColorList_, colorList_, this);

  if (!bShowRecentNews)
  {
    clearNotification();
  }

  connect(notificationWidget, SIGNAL(signalShow()), this, SLOT(showWindows()));
  connect(notificationWidget, SIGNAL(signalClose()),
          this, SLOT(deleteNotification()));
  connect(notificationWidget, SIGNAL(signalOpenNews(int, int)),
          this, SLOT(slotOpenNew(int, int)));
  connect(notificationWidget, SIGNAL(signalOpenExternalBrowser(QUrl)),
          this, SLOT(slotOpenNewBrowser(QUrl)));
  connect(notificationWidget, SIGNAL(signalMarkRead(int,int,int)),
          this, SLOT(slotMarkReadNewsInNotification(int,int,int)));
  connect(notificationWidget, SIGNAL(signalDeleteNews(int,int)),
          this, SLOT(slotDeleteNewsInNotification(int,int)));
  connect(notificationWidget, SIGNAL(signalMarkAllRead()),
          this, SLOT(slotMarkAllReadNewsInNotification()));

  notificationWidget->show();
}

/** @brief Delete notification widget
 *---------------------------------------------------------------------------*/
void MainWindow::deleteNotification()
{
  notificationWidget->deleteLater();
  notificationWidget = NULL;
}

void MainWindow::clearNotification(bool bClearRecentNews/*=false*/)
{
  newNews.idFeedList_.clear();
  newNews.cntNewsList_.clear();

  if (bClearRecentNews)
  {
    recentNews.idFeedList_.clear();
    recentNews.cntNewsList_.clear();

    idColorList_.clear();
    colorList_.clear();
  }
}

void MainWindow::slotAddColorList(int id, const QString &color)
{
  if (idColorList_.indexOf(id) == -1)
  {
    idColorList_.append(id);
    colorList_.append(color);
  }
}

/** @brief Show news on click in notification window
 *---------------------------------------------------------------------------*/
void MainWindow::slotOpenNew(int feedId, int newsId)
{
  openingFeedAction_ = 0;
  openNewsWebViewOn_ = true;

  QSqlQuery q;
  q.exec(QString("UPDATE feeds SET currentNews='%1' WHERE id=='%2'").arg(newsId).arg(feedId));

  QModelIndex feedIndex = feedsModel_->indexById(feedId);
  feedsView_->setCurrentIndex(feedsProxyModel_->mapFromSource(feedIndex));
  feedsModel_->setData(feedsModel_->indexSibling(feedIndex, "currentNews"),
                           newsId);

  if (stackedWidget_->count() && currentNewsTab->type_ < NewsTabWidget::TabTypeWeb) {
    currentNewsTab->newsHeader_->saveStateColumns(currentNewsTab);
  }

  // Search open tab containing this feed
  int indexTab = -1;
  for (int i = 0; i < stackedWidget_->count(); i++) {
    NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
    if (widget->feedId_ == feedId) {
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

      currentNewsTab = (NewsTabWidget*)stackedWidget_->widget(TAB_WIDGET_PERMANENT);
      newsModel_ = currentNewsTab->newsModel_;
      newsView_ = currentNewsTab->newsView_;
    } else {
      if (stackedWidget_->count() && currentNewsTab->type_ != NewsTabWidget::TabTypeFeed) {
        setFeedRead(currentNewsTab->type_, currentNewsTab->feedId_, FeedReadSwitchingFeed, currentNewsTab);
      } else {
        // Mark previous feed Read while switching to another feed
        setFeedRead(NewsTabWidget::TabTypeFeed, feedIdOld_, FeedReadSwitchingFeed, 0, feedId);
      }

      categoriesTree_->setCurrentIndex(QModelIndex());
    }
  } else {
    tabBar_->setCurrentIndex(indexTab);
  }
  slotFeedSelected(feedIndex);
  feedsView_->repaint();
  feedIdOld_ = feedId;

  Settings settings;
  openingFeedAction_ = settings.value("/Settings/openingFeedAction", 0).toInt();
  openNewsWebViewOn_ = settings.value("/Settings/openNewsWebViewOn", true).toBool();
  showWindows();
  newsView_->setFocus();
}

/** @brief Open news in external browser on click in notification window
 *---------------------------------------------------------------------------*/
void MainWindow::slotOpenNewBrowser(const QUrl &url)
{
  currentNewsTab->openUrl(url);
}

void MainWindow::slotMarkReadNewsInNotification(int feedId, int newsId, int read)
{
  QSqlQuery q;
  bool showNews = false;
  if (currentNewsTab->type_ < NewsTabWidget::TabTypeWeb) {
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

void MainWindow::slotDeleteNewsInNotification(int feedId, int newsId)
{
  mainApp->sqlQueryExec(QString("UPDATE news SET new=0, read=2, deleted=1, deleteDate='%1' WHERE id=='%2'").
               arg(QDateTime::currentDateTime().toString(Qt::ISODate)).
               arg(newsId));

  if (currentNewsTab->type_ < NewsTabWidget::TabTypeWeb) {
    for (int i = 0; i < newsModel_->rowCount(); ++i) {
      if (newsId == newsModel_->index(i, newsModel_->fieldIndex("id")).data().toInt()) {
        newsModel_->setData(newsModel_->index(i, newsModel_->fieldIndex("new")), 0);
        newsModel_->setData(newsModel_->index(i, newsModel_->fieldIndex("read")), 2);
        newsModel_->setData(newsModel_->index(i, newsModel_->fieldIndex("deleted")), 1);
        newsModel_->setData(newsModel_->index(i, newsModel_->fieldIndex("deleteDate")),
                            QDateTime::currentDateTime().toString(Qt::ISODate));

        newsModel_->submitAll();

        while (newsModel_->canFetchMore())
          newsModel_->fetchMore();

        currentNewsTab->loadNewspaper(NewsTabWidget::RefreshWithPos);

        QModelIndex curIndex;
        if (i == newsModel_->rowCount())
          curIndex = newsModel_->index(i-1, newsModel_->fieldIndex("title"));
        else if (i > newsModel_->rowCount())
          curIndex = newsModel_->index(i-1, newsModel_->fieldIndex("title"));
        else
          curIndex = newsModel_->index(i, newsModel_->fieldIndex("title"));
        newsView_->setCurrentIndex(curIndex);
        currentNewsTab->slotNewsViewSelected(curIndex);
        break;
      }
    }
  }

  slotUpdateStatus(feedId);
  recountCategoryCounts();
}

void MainWindow::slotMarkAllReadNewsInNotification()
{
  if (NotificationWidget *notificationWidget = qobject_cast<NotificationWidget*>(sender())) {
    QList<int> idFeedList = notificationWidget->idFeedList();
    QList<int> idNewsList = notificationWidget->idNewsList();

    if (currentNewsTab->type_ < NewsTabWidget::TabTypeWeb) {
      for (int i = 0; i < newsModel_->rowCount(); ++i) {
        if (idNewsList.contains(newsModel_->index(i, newsModel_->fieldIndex("id")).data().toInt())) {
          newsModel_->setData(
                newsModel_->index(i, newsModel_->fieldIndex("new")), 0);
          newsModel_->setData(
                newsModel_->index(i, newsModel_->fieldIndex("read")), 1);
        }
      }
      newsView_->viewport()->update();
    }

    foreach (int newsId, idNewsList) {
      mainApp->sqlQueryExec(QString("UPDATE news SET new=0, read=1 WHERE id=='%1'").
                   arg(newsId));
    }

    foreach (int feedId, idFeedList) {
      slotUpdateStatus(feedId);
    }
    recountCategoryCounts();
  }
}

// ----------------------------------------------------------------------------
void MainWindow::slotFindFeeds(QString)
{
  if (!findFeedsWidget_->isVisible()) return;

  setFeedsFilter(false);
}
// ----------------------------------------------------------------------------
void MainWindow::slotSelectFind()
{
  slotFindFeeds(findFeeds_->text());
}
// ----------------------------------------------------------------------------
void MainWindow::findFeedVisible(bool visible)
{
  findFeedsWidget_->setVisible(visible);
  if (visible) {
    findFeeds_->setFocus();
  } else {
    findFeedAct_->setChecked(false);
    findFeeds_->clear();
    // Call filter explicitly, because invisible widget don't calls it
    setFeedsFilter(false);
  }
}

/** @brief Totally remove news
 *---------------------------------------------------------------------------*/
void MainWindow::cleanUp()
{
  CleanUpWizard *cleanUpWizard = new CleanUpWizard(this);
  cleanUpWizard->exec();
  delete cleanUpWizard;
}

/** @brief Zooming in browser
 *---------------------------------------------------------------------------*/
void MainWindow::browserZoom(QAction *action)
{
  if (currentNewsTab->type_ == NewsTabWidget::TabTypeDownloads) return;

  if (action->objectName() == "zoomInAct") {
    if (currentNewsTab->webView_->zoomFactor() < 5.0)
      currentNewsTab->webView_->setZoomFactor(currentNewsTab->webView_->zoomFactor()+0.1);
  } else if (action->objectName() == "zoomOutAct") {
    if (currentNewsTab->webView_->zoomFactor() > 0.3)
      currentNewsTab->webView_->setZoomFactor(currentNewsTab->webView_->zoomFactor()-0.1);
  } else {
    currentNewsTab->webView_->setZoomFactor(1);
  }
}

/** @brief Call default e-mail application to report the problem
 *---------------------------------------------------------------------------*/
void MainWindow::slotReportProblem()
{
  QDesktopServices::openUrl(QUrl("https://github.com/QuiteRSS/quiterss/issues"));
}

/** @brief Print browser page
 *---------------------------------------------------------------------------*/
void MainWindow::slotPrint(QWebFrame *frame)
{
  if (currentNewsTab->type_ == NewsTabWidget::TabTypeDownloads) return;

  QPrinter printer;
  printer.setDocName(tr("Web Page"));
  QPrintDialog *printDlg = new QPrintDialog(&printer);
  if (!frame)
    connect(printDlg, SIGNAL(accepted(QPrinter*)), currentNewsTab->webView_, SLOT(print(QPrinter*)));
  else
    connect(printDlg, SIGNAL(accepted(QPrinter*)), frame, SLOT(print(QPrinter*)));
  printDlg->exec();
  printDlg->deleteLater();
}

/** @brief Call print preview dialog
 *---------------------------------------------------------------------------*/
void MainWindow::slotPrintPreview(QWebFrame *frame)
{
  if (currentNewsTab->type_ == NewsTabWidget::TabTypeDownloads) return;

  QPrinter printer;
  printer.setDocName(tr("Web Page"));
  QPrintPreviewDialog *prevDlg = new QPrintPreviewDialog(&printer);
  prevDlg->setWindowFlags(prevDlg->windowFlags() | Qt::WindowMaximizeButtonHint);
  prevDlg->resize(650, 800);
  if (!frame)
    connect(prevDlg, SIGNAL(paintRequested(QPrinter*)), currentNewsTab->webView_, SLOT(print(QPrinter*)));
  else
    connect(prevDlg, SIGNAL(paintRequested(QPrinter*)), frame, SLOT(print(QPrinter*)));
  prevDlg->exec();
  prevDlg->deleteLater();
}
// ----------------------------------------------------------------------------
void MainWindow::setFullScreen()
{
  if (!isFullScreen()) {
    // hide menu
    menuBar()->hide();
#ifdef HAVE_X11
    show();
    raise();
    setWindowState(windowState() | Qt::WindowFullScreen);
#else
    setWindowState(windowState() | Qt::WindowFullScreen);
#endif
  } else {
    if (showMenuBarAct_->isChecked())
      menuBar()->show();
    setWindowState(windowState() & ~Qt::WindowFullScreen);
  }
}
// ----------------------------------------------------------------------------
void MainWindow::setStayOnTop()
{
  isMinimizeToTray_ = true;

  int state = windowState();

  if (stayOnTopAct_->isChecked())
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
  else
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);

  if ((state & Qt::WindowFullScreen) || (state & Qt::WindowMaximized)) {
    Settings settings;
    restoreGeometry(settings.value("GeometryState").toByteArray());
  }
  setWindowState((Qt::WindowState)state);
  show();

  isMinimizeToTray_ = false;
}

void MainWindow::showMenuBar()
{
  if (showMenuBarAct_->isChecked()) {
    mainMenuButton_->hide();
    if (isFullScreen())
      menuBar()->hide();
    else
      menuBar()->show();
  } else {
    mainMenuButton_->show();
    menuBar()->hide();
  }
}

/** @brief Move index after drag&drop operations
 * @param indexWhat index that is moving
 * @param indexWhere index, where to move
 *---------------------------------------------------------------------------*/
void MainWindow::slotMoveIndex(const QModelIndex &indexWhere, int how)
{
  feedsView_->setCursor(Qt::WaitCursor);

  QModelIndexList indexList = feedsView_->selectionModel()->selectedRows(0);
  for (int i = 0; i < indexList.count(); i++) {
    QModelIndex indexWhat = feedsProxyModel_->mapToSource(indexList[i]);
    int feedIdWhat = feedsModel_->idByIndex(indexWhat);
    int feedParIdWhat = feedsModel_->paridByIndex(indexWhat);
    int feedIdWhere = feedsModel_->idByIndex(indexWhere);
    int feedParIdWhere = feedsModel_->paridByIndex(indexWhere);

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
    } else if (feedParIdWhat == feedParIdWhere) {
      // Move inside folder
      QList<int> idList;
      q.exec(QString("SELECT id FROM feeds WHERE parentId='%1' ORDER BY rowToParent").
             arg(feedParIdWhat));
      while (q.next()) {
        idList << q.value(0).toInt();
      }

      int rowWhat = feedsModel_->dataField(indexWhat, "rowToParent").toInt();
      int rowWhere = feedsModel_->dataField(indexWhere, "rowToParent").toInt();
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

      int rowWhere = feedsModel_->dataField(indexWhere, "rowToParent").toInt();
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
  }

  feedsView_->refresh();

  feedsView_->setCurrentIndex(feedsProxyModel_->mapFromSource(feedIdOld_));

  feedsView_->setCursor(Qt::ArrowCursor);
}

/** @brief Process clicks in feeds tree
 * @param item Item that was clicked
 *---------------------------------------------------------------------------*/
void MainWindow::slotCategoriesClicked(QTreeWidgetItem *item, int, bool createTab)
{
  if (stackedWidget_->count() && currentNewsTab->type_ < NewsTabWidget::TabTypeWeb) {
    currentNewsTab->newsHeader_->saveStateColumns(currentNewsTab);
    Settings settings;
    settings.setValue("NewsTabSplitterState", currentNewsTab->newsTabWidgetSplitter_->saveState());
  }

  int type = item->text(1).toInt();
  NewsTabWidget::TabType tabType = static_cast<NewsTabWidget::TabType>(type);

  int indexTab = -1;
  if (!createTab) {
    for (int i = 0; i < stackedWidget_->count(); i++) {
      NewsTabWidget *widget = (NewsTabWidget*)stackedWidget_->widget(i);
      if (widget->type_ == tabType) {
        if (widget->type_ == NewsTabWidget::TabTypeLabel) {
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

      NewsTabWidget *widget = new NewsTabWidget(this, tabType);
      indexTab = addTab(widget);
      createNewsTab(indexTab);
    }
    else {
      feedsView_->setCurrentIndex(QModelIndex());
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

      currentNewsTab->type_ = tabType;
      currentNewsTab->feedId_ = -1;
      currentNewsTab->feedParId_ = -1;
      currentNewsTab->setSettings(true, false);
      currentNewsTab->setVisible(true);
    }

    // Set icon and title of current tab
    currentNewsTab->newsIconTitle_->setPixmap(item->icon(0).pixmap(16,16));
    currentNewsTab->setTextTab(item->text(0));

    currentNewsTab->labelId_ = item->text(2).toInt();

    switch (type) {
    case NewsTabWidget::TabTypeUnread:
      currentNewsTab->categoryFilterStr_ = "feedId > 0 AND deleted = 0 AND read < 2";
      break;
    case NewsTabWidget::TabTypeStar:
      currentNewsTab->categoryFilterStr_ = "feedId > 0 AND deleted = 0 AND starred = 1";
      break;
    case NewsTabWidget::TabTypeDel:
      currentNewsTab->categoryFilterStr_ = "feedId > 0 AND deleted = 1";
      break;
    case NewsTabWidget::TabTypeLabel:
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
    // ... add filter from "search"
    QString filterStr = currentNewsTab->categoryFilterStr_;
    QString objectName = currentNewsTab->findText_->findGroup_->checkedAction()->objectName();
    if (objectName != "findInBrowserAct") {
      QString findText = currentNewsTab->findText_->text();
      if (!findText.isEmpty()) {
        findText = findText.replace("'", "''").toUpper();
        if (objectName == "findTitleAct") {
          filterStr.append(
                QString(" AND UPPER(title) LIKE '%%1%'").arg(findText));
        } else if (objectName == "findAuthorAct") {
          filterStr.append(
                QString(" AND UPPER(author_name) LIKE '%%1%'").arg(findText));
        } else if (objectName == "findCategoryAct") {
          filterStr.append(
                QString(" AND UPPER(category) LIKE '%%1%'").arg(findText));
        } else if (objectName == "findContentAct") {
          filterStr.append(
                QString(" AND (UPPER(content) LIKE '%%1%' OR UPPER(description) LIKE '%%1%')").
                arg(findText));
        } else if (objectName == "findLinkAct") {
          filterStr.append(
                QString(" AND link_href LIKE '%%1%'").
                arg(findText));
        } else {
          filterStr.append(
                QString(" AND (UPPER(title) LIKE '%%1%' OR UPPER(author_name) LIKE '%%1%' "
                        "OR UPPER(category) LIKE '%%1%' OR UPPER(content) LIKE '%%1%' "
                        "OR UPPER(description) LIKE '%%1%')").
                arg(findText));
        }
      }
    }
    newsModel_->setFilter(filterStr);

    if (newsModel_->rowCount() != 0) {
      while (newsModel_->canFetchMore())
        newsModel_->fetchMore();
    }

    if (type == NewsTabWidget::TabTypeDel){
      currentNewsTab->newsHeader_->setSortIndicator(newsModel_->fieldIndex("deleteDate"),
                                                    Qt::DescendingOrder);
    } else {
      if ((currentNewsTab->newsHeader_->sortIndicatorSection() == newsModel_->fieldIndex("read")) ||
          currentNewsTab->newsHeader_->sortIndicatorSection() == newsModel_->fieldIndex("starred")) {
        currentNewsTab->slotSort(currentNewsTab->newsHeader_->sortIndicatorSection(),
                                 currentNewsTab->newsHeader_->sortIndicatorOrder());
      }
    }

    currentNewsTab->loadNewspaper();

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

  int unreadCount = currentNewsTab->getUnreadCount(categoriesTree_->currentItem()->text(4));
  int allCount = currentNewsTab->newsModel_->rowCount();
  statusUnread_->setText(QString(" " + tr("Unread: %1") + " ").arg(unreadCount));
  statusAll_->setText(QString(" " + tr("All: %1") + " ").arg(allCount));

  statusUnread_->setVisible(currentNewsTab->type_ != NewsTabWidget::TabTypeDel);
  statusAll_->setVisible(true);
}

void MainWindow::clearDeleted()
{
  QSqlQuery q;
  q.exec("UPDATE news SET description='', content='', received='', "
         "author_name='', author_uri='', author_email='', "
         "category='', new='', read='', starred='', label='', "
         "deleteDate='', feedParentId='', deleted=2 WHERE deleted==1");

  if (currentNewsTab->type_ == NewsTabWidget::TabTypeDel) {
    currentNewsTab->newsModel_->select();
    currentNewsTab->slotNewsViewSelected(QModelIndex());
  }

  recountCategoryCounts();
}

void MainWindow::slotMarkReadCategory(QTreeWidgetItem *item)
{
  int type = item->text(1).toInt();
  int labelId = item->text(2).toInt();

  if ((currentNewsTab->type_ == type) && (currentNewsTab->labelId_ == labelId)) {
    currentNewsTab->markAllNewsRead();
  }
  else {
    emit signalMarkReadCategory(type, labelId);
    recountCategoryCounts();
  }
}

void MainWindow::viewAllYoutubeVideos()
{
  currentNewsTab->viewAllYoutubeVideos();
}

/** @brief Show/Hide categories tree
 *---------------------------------------------------------------------------*/
void MainWindow::showNewsCategoriesTree()
{
  if (categoriesTree_->isHidden()) {
    showCategoriesButton_->setIcon(QIcon(":/images/images/panel_hide.png"));
    showCategoriesButton_->setToolTip(tr("Hide Categories"));
    categoriesTree_->show();
    feedsSplitter_->restoreState(feedsWidgetSplitterState_);
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
void MainWindow::feedsSplitterMoved(int pos, int)
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
void MainWindow::setLabelNews(QAction *action)
{
  if (currentNewsTab->type_ >= NewsTabWidget::TabTypeWeb) return;

  newsLabelAction_->setIcon(action->icon());
  newsLabelAction_->setToolTip(action->text());
  newsLabelAction_->setData(action->data());

  currentNewsTab->setLabelNews(action->data().toInt());
}

/** @brief Set last chosen label for news
 *---------------------------------------------------------------------------*/
void MainWindow::setDefaultLabelNews()
{
  if (currentNewsTab->type_ >= NewsTabWidget::TabTypeWeb) return;

  currentNewsTab->setLabelNews(newsLabelAction_->data().toInt());
}

/** @brief Get Label that belong to current news
 *---------------------------------------------------------------------------*/
void MainWindow::getLabelNews()
{
  for (int i = 0; i < newsLabelGroup_->actions().count(); i++) {
    newsLabelGroup_->actions().at(i)->setChecked(false);
  }

  if (currentNewsTab->type_ >= NewsTabWidget::TabTypeWeb) return;

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
int MainWindow::addTab(NewsTabWidget *widget)
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

// ----------------------------------------------------------------------------
void MainWindow::reduceNewsList()
{
  currentNewsTab->reduceNewsList();
}
// ----------------------------------------------------------------------------
void MainWindow::increaseNewsList()
{
  currentNewsTab->increaseNewsList();
}

/** @brief Save browser current page to file
 *---------------------------------------------------------------------------*/
void MainWindow::slotSavePageAs()
{
  if (currentNewsTab->type_ == NewsTabWidget::TabTypeDownloads) return;

  QString fileName = currentNewsTab->webView_->title();
  if (newsLayout_ == 0) {
    if (fileName == "news_descriptions") {
      int row = currentNewsTab->newsView_->currentIndex().row();
      fileName = currentNewsTab->newsModel_->dataField(row, "title").toString();
    }
  } else {
    if (currentNewsTab->type_ == NewsTabWidget::TabTypeFeed) {
      QModelIndex feedIndex = feedsView_->currentIndex();
      feedIndex = feedsProxyModel_->mapToSource(feedIndex);
      fileName = feedsModel_->dataField(feedIndex, "text").toString();
    } else {
      fileName = categoriesTree_->currentItem()->text(0);
    }
  }
  QString title = fileName.trimmed();

  fileName = fileName.trimmed();
  fileName = fileName.replace(QzRegExp("[:\"]"), "_");
  fileName = QDir::toNativeSeparators(QDir::homePath() + "/" + fileName);
  fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                          fileName,
                                          QString(tr("HTML-Files (*.%1)") + ";;" + tr("Text files (*.%2)"))
                                          .arg("html").arg("txt"));
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
    QzRegExp reg("news_descriptions", Qt::CaseInsensitive);
    html = html.replace(reg, title);
    reg.setPattern("<img class=\"quiterss-img\"[^>]+\\>");
    html = html.remove(reg);
    QTextCodec *codec = QTextCodec::codecForHtml(html.toUtf8(),
                                                 QTextCodec::codecForName("UTF-8"));
    file.write(codec->fromUnicode(html));
  }
  file.close();
}

void MainWindow::slotSavePageAsDescript()
{
  currentNewsTab->savePageAsDescript();
}

/** @brief Restore last deleted news
 *---------------------------------------------------------------------------*/
void MainWindow::restoreLastNews()
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

    currentNewsTab->loadNewspaper(NewsTabWidget::RefreshWithPos);

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
void MainWindow::nextUnreadNews()
{
  if (currentNewsTab->type_ >= NewsTabWidget::TabTypeWeb) return;
  newsView_->clearSelection();

  int newsRow = currentNewsTab->findUnreadNews(true);

  if (newsRow == -1) {
    if (currentNewsTab->type_ != NewsTabWidget::TabTypeFeed) return;

    QModelIndex indexPrevUnread = QModelIndex();
    if (feedsView_->currentIndex().isValid())
      indexPrevUnread = feedsView_->indexNextUnread(feedsView_->currentIndex(), 1);
    if (!indexPrevUnread.isValid()) {
      indexPrevUnread = feedsView_->indexNextUnread(QModelIndex(), 1);
    }
    if (indexPrevUnread.isValid()) {
      if (changeBehaviorActionNUN_)
        openingFeedAction_ = 4;
      else
        openingFeedAction_ = 3;
      feedsView_->clearSelection();
      feedsView_->setCurrentIndex(indexPrevUnread);
      slotFeedClicked(indexPrevUnread);

      if (tabBar_->currentIndex() != TAB_WIDGET_PERMANENT) {
        QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("read"));
        QModelIndexList indexList;
        if ((newsView_->header()->sortIndicatorOrder() == Qt::DescendingOrder) &&
            (openingFeedAction_ != 4))
          indexList = newsModel_->match(index, Qt::EditRole, 0, -1);
        else
          indexList = newsModel_->match(index, Qt::EditRole, 0);

        if (!indexList.isEmpty()) newsRow = indexList.last().row();

        // Focus feed news that displayed before
        newsView_->setCurrentIndex(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
        if (newsRow == -1) newsView_->verticalScrollBar()->setValue(newsRow);

        if (openNewsWebViewOn_) {
          currentNewsTab->slotNewsViewSelected(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
        }
      }

      Settings settings;
      openingFeedAction_ = settings.value("/Settings/openingFeedAction", 0).toInt();
    }
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
void MainWindow::prevUnreadNews()
{
  if (currentNewsTab->type_ >= NewsTabWidget::TabTypeWeb) return;
  newsView_->clearSelection();

  int newsRow = currentNewsTab->findUnreadNews(false);

  int newsRowCur = newsView_->currentIndex().row();
  if ((newsRow >= newsRowCur) || (newsRow == -1)) {
    if (currentNewsTab->type_ != NewsTabWidget::TabTypeFeed) return;

    QModelIndex indexNextUnread =
        feedsView_->indexNextUnread(feedsView_->currentIndex(), 2);
    if (indexNextUnread.isValid()) {
      openingFeedAction_ = 3;
      feedsView_->clearSelection();
      feedsView_->setCurrentIndex(indexNextUnread);
      slotFeedClicked(indexNextUnread);
      Settings settings;
      openingFeedAction_ = settings.value("/Settings/openingFeedAction", 0).toInt();
    }
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

/** @brief Get feeds ids list string of folder \a idFolder
 *---------------------------------------------------------------------------*/
QString MainWindow::getIdFeedsString(int idFolder, int idException)
{
  QList<int> idList = UpdateObject::getIdFeedsInList(db_, idFolder);
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
void MainWindow::setTextTitle(const QString &text, NewsTabWidget *widget)
{
  if (currentNewsTab != widget) return;

  if (text.isEmpty()) setWindowTitle("QuiteRSS");
  else setWindowTitle(QString("%1 - QuiteRSS").arg(text));
}

/** @brief Enable|Disable indent in feeds tree
 *---------------------------------------------------------------------------*/
void MainWindow::slotIndentationFeedsTree()
{
  feedsView_->setRootIsDecorated(indentationFeedsTreeAct_->isChecked());
}

// ----------------------------------------------------------------------------
void MainWindow::customizeMainToolbar()
{
  showCustomizeToolbarDlg(customizeMainToolbarAct2_);
}
// ----------------------------------------------------------------------------
void MainWindow::showCustomizeToolbarDlg(QAction *action)
{
  QToolBar *toolbar = mainToolbar_;
  if (action->objectName() == "customizeFeedsToolbarAct") {
    toolbar = feedsToolBar_;
  } else if (action->objectName() == "customizeNewsToolbarAct") {
    if (currentNewsTab->type_ == NewsTabWidget::TabTypeWeb) return;
    if (currentNewsTab->type_ == NewsTabWidget::TabTypeDownloads) return;
    toolbar = currentNewsTab->newsToolBar_;
  }

  CustomizeToolbarDialog *toolbarDlg = new CustomizeToolbarDialog(this, toolbar);

  toolbarDlg->exec();

  delete toolbarDlg;
}

/** @brief Process news sharing
 *---------------------------------------------------------------------------*/
void MainWindow::slotShareNews(QAction *action)
{
  currentNewsTab->slotShareNews(action);
}
// ----------------------------------------------------------------------------
void MainWindow::showMenuShareNews()
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
  if (currentNewsTab->type_ < NewsTabWidget::TabTypeWeb) {
    if (currentNewsTab->newsToolBar_->widgetForAction(shareMenuAct_)) {
      QWidget *widget = currentNewsTab->newsToolBar_->widgetForAction(shareMenuAct_);
      if (widget->underMouse()) {
        shareMenu_->popup(widget->mapToGlobal(QPoint(0, currentNewsTab->newsToolBar_->height()-1)));
      }
    }
  }
  if (currentNewsTab->webToolBar_->widgetForAction(shareMenuAct_)) {
    QWidget *widget = currentNewsTab->webToolBar_->widgetForAction(shareMenuAct_);
    if (widget->underMouse()) {
      shareMenu_->popup(widget->mapToGlobal(QPoint(0, currentNewsTab->webToolBar_->height()-1)));
    }
  }
}

/** @brief Open feed home page in external browser
 *---------------------------------------------------------------------------*/
void MainWindow::slotOpenHomeFeed()
{
  QModelIndex index = feedsView_->currentIndex();
  if (!index.isValid()) return;
  index = feedsProxyModel_->mapToSource(index);

  QString homePage = feedsModel_->dataField(index, "htmlUrl").toString();
  QDesktopServices::openUrl(homePage);
}

/** @brief Sort feed and folders by title
 *---------------------------------------------------------------------------*/
void MainWindow::sortedByTitleFeedsTree()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<int> parentIdsPotential;
  parentIdsPotential << 0;
  while (!parentIdsPotential.empty()) {
    int parentId = parentIdsPotential.takeFirst();

    // Search children of parent <parentId>
    QSqlQuery q;
    q.prepare(QString("SELECT id, xmlUrl FROM feeds WHERE parentId=? ORDER BY text COLLATE LOCALE"));
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

// ----------------------------------------------------------------------------
void MainWindow::showNewsMenu()
{
  if (currentNewsTab)
    newsSortByMenu_->setEnabled(currentNewsTab->type_ < NewsTabWidget::TabTypeWeb);
}
// ----------------------------------------------------------------------------
void MainWindow::showNewsSortByMenu()
{
  if (currentNewsTab->type_ >= NewsTabWidget::TabTypeWeb) return;

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
void MainWindow::setNewsSortByColumn()
{
  if (currentNewsTab->type_ >= NewsTabWidget::TabTypeWeb) return;

  int lIdx = newsSortByColumnGroup_->checkedAction()->data().toInt();
  if (newsSortOrderGroup_->actions().at(0)->isChecked()) {
    currentNewsTab->newsHeader_->setSortIndicator(lIdx, Qt::AscendingOrder);
  } else {
    currentNewsTab->newsHeader_->setSortIndicator(lIdx, Qt::DescendingOrder);
  }
}
// ----------------------------------------------------------------------------
void MainWindow::slotPrevFolder()
{
  QModelIndex indexBefore = feedsView_->currentIndex();
  QModelIndex indexAfter;

  // Set to bottom folder, if there's no above folder
  if (!indexBefore.isValid())
    indexAfter = feedsView_->lastFolderInFolder(QModelIndex());
  else
    indexAfter = feedsView_->indexPreviousFolder(indexBefore);

  // there's no "upper" index
  if (!indexAfter.isValid()) return;
  feedsView_->clearSelection();

  feedsView_->setCurrentIndex(indexAfter);
  slotFeedClicked(indexAfter);
}
// ----------------------------------------------------------------------------
void MainWindow::slotNextFolder()
{
  QModelIndex indexBefore = feedsView_->currentIndex();
  QModelIndex indexAfter;

  // Set to top index, if there's no below index
  if (!indexBefore.isValid())
    indexAfter = feedsView_->firstFolderInFolder(QModelIndex());
  else
    indexAfter = feedsView_->indexNextFolder(indexBefore);

  // there's no "downer" index
  if (!indexAfter.isValid()) return;
  feedsView_->clearSelection();

  feedsView_->setCurrentIndex(indexAfter);
  slotFeedClicked(indexAfter);
}
// ----------------------------------------------------------------------------
void MainWindow::slotExpandFolder()
{
  QModelIndex index = feedsView_->currentIndex();
  if (!feedsModel_->isFolder(index)) {
    index = feedsModel_->parent(index);
  }
  feedsView_->setExpanded(index, !feedsView_->isExpanded(index));
}
// ----------------------------------------------------------------------------
void MainWindow::showDownloadManager(bool activate)
{
  int indexTab = -1;
  NewsTabWidget *widget;
  for (int i = 0; i < stackedWidget_->count(); i++) {
    widget = (NewsTabWidget*)stackedWidget_->widget(i);
    if (widget->type_ == NewsTabWidget::TabTypeDownloads) {
      indexTab = i;
      break;
    }
  }

  if (indexTab == -1) {
    widget = new NewsTabWidget(this, NewsTabWidget::TabTypeDownloads);
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
    mainApp->downloadManager()->show();
    emit signalSetCurrentTab(indexTab);
  } else {
    widget->setTextTab(tr("Downloads"));
  }
}
// ----------------------------------------------------------------------------
void MainWindow::updateInfoDownloads(const QString &text)
{
  NewsTabWidget *widget;
  for (int i = 0; i < stackedWidget_->count(); i++) {
    widget = (NewsTabWidget*)stackedWidget_->widget(i);
    if (widget->type_ == NewsTabWidget::TabTypeDownloads) {
      widget->setTextTab(QString("%1 %2").arg(tr("Downloads")).arg(text));
      break;
    }
  }
}

void MainWindow::setStatusFeed(int feedId, QString status)
{
  QModelIndex index = feedsModel_->indexById(feedId);
  if (index.isValid()) {
    QModelIndex indexStatus = feedsModel_->indexSibling(index, "status");
    feedsModel_->setData(indexStatus, status);
    feedsView_->viewport()->update();
  }
}

void MainWindow::addOurFeed()
{
  if (mainApp->dbFileExists()) return;

  QPixmap icon(":/images/quiterss16");
  QByteArray iconData;
  QBuffer buffer(&iconData);
  buffer.open(QIODevice::WriteOnly);
  icon.save(&buffer, "PNG");
  buffer.close();

  QString xmlUrl = "https://quiterss.org/en/rss.xml";
  if (mainApp->language() == "ru")
    xmlUrl = "https://quiterss.org/ru/rss.xml";

  QSqlQuery q;
  q.prepare("INSERT INTO feeds(text, title, xmlUrl, htmlUrl, created, parentId, rowToParent, image) "
            "VALUES(?, ?, ?, ?, ?, ?, ?, ?)");
  q.addBindValue("QuiteRSS");
  q.addBindValue("QuiteRSS");
  q.addBindValue(xmlUrl);
  q.addBindValue("https://quiterss.org");
  q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
  q.addBindValue(0);
  q.addBindValue(0);
  q.addBindValue(iconData.toBase64());
  q.exec();

  feedsModelReload();
}

void MainWindow::createBackup()
{
  QString backupDir(QDir::currentPath());
  Settings settings;
  backupDir = settings.value("Settings/backupDir", backupDir).toString();
  backupDir = QFileDialog::getExistingDirectory(this, tr("Choose Directory"),
                                                backupDir,
                                                QFileDialog::ShowDirsOnly |
                                                QFileDialog::DontResolveSymlinks);
  if (!backupDir.isEmpty()) {
    settings.setValue("Settings/backupDir", backupDir);

    QFileInfo fileInfo;
    QString backupFileName;
    QString timeStr(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));

    fileInfo.setFile(mainApp->dbFileName());
    backupFileName = QString("%1/%2_%3.bak").
        arg(backupDir).
        arg(fileInfo.fileName()).
        arg(timeStr);
    QFile::copy(mainApp->dbFileName(), backupFileName);

    fileInfo.setFile(settings.fileName());
    backupFileName = QString("%1/%2_%3.bak").
        arg(backupDir).
        arg(fileInfo.fileName()).
        arg(timeStr);
    QFile::copy(settings.fileName(), backupFileName);
  }
}

void MainWindow::webViewFullScreen(bool on)
{
  feedsWidget_->setVisible(!on);
  pushButtonNull_->setVisible(!on);
  tabBarWidget_->setVisible(!on);
  currentNewsTab->newsWidget_->setVisible(!on);
  currentNewsTab->webControlPanel_->setVisible(!on);
  pushButtonNull_->setVisible(!on);
  statusBar()->setVisible(!on);
  setFullScreen();
}
