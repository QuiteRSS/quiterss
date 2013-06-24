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
#ifndef RSSLISTING_H
#define RSSLISTING_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QtSql>
#include <QtWebKit>
#include <QNetworkProxy>
#include <QNetworkDiskCache>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QSound>

#include "categoriestreewidget.h"
#include "cookiejar.h"
#include "dbmemfilethread.h"
#include "downloadmanager.h"
#include "faviconthread.h"
#include "feedstreemodel.h"
#include "feedstreeview.h"
#include "findfeed.h"
#include "networkmanager.h"
#include "newsheader.h"
#include "newsmodel.h"
#include "newstabwidget.h"
#include "newsview.h"
#include "notifications.h"
#include "parsethread.h"
#include "tabbar.h"
#include "updateappdialog.h"
#include "updatedelayer.h"
#include "updatethread.h"
#include "webview.h"

#define NEW_TAB_FOREGROUND 1
#define NEW_TAB_BACKGROUND 2

enum FeedReedType {
  FeedReadSwitchingFeed,
  FeedReadClosingTab,
  FeedReadPlaceToTray,
  FeedReadTypeCount,
  FeedReadSwitchingTab
};

class RSSListing : public QMainWindow
{
  Q_OBJECT
public:
  RSSListing(QSettings *settings, const QString &appDataDirPath,
             const QString &dataDirPath, QWidget *parent = 0);
  ~RSSListing();

  static bool removePath(const QString &path);

  bool showSplashScreen_;
  bool showTrayIcon_;
  bool startingTray_;
  QSystemTrayIcon *traySystem;
  void restoreFeedsOnStartUp();
  void expandNodes();
  QList<int> getIdFeedsInList(int idFolder);
  QString getIdFeedsString(int idFolder, int idException = -1);
  void recountCategoryCounts();

  void setToolBarStyle(const QString &styleStr);
  void setToolBarIconSize(const QString &iconSizeStr);

  QSettings *settings_;
  QString appDataDirPath_;
  QString dataDirPath_;
  QString lastFeedPath_;
  QSqlDatabase db_;
  FeedsTreeModel *feedsTreeModel_;
  FeedsTreeView *feedsTreeView_;
  CategoriesTreeWidget *categoriesTree_;
#define TAB_WIDGET_PERMANENT 0
  QStackedWidget *stackedWidget_;
  DownloadManager *downloadManager_;

  bool showFeedsTabPermanent_;

  NewsTabWidget *currentNewsTab;

  QAction *newsToolbarToggle_;
  QAction *browserToolbarToggle_;
  QAction *categoriesPanelToggle_;
  QAction *newsFilter_;
  QAction *openDescriptionNewsAct_;
  QAction *openInBrowserAct_;
  QAction *openInExternalBrowserAct_;
  QAction *openNewsNewTabAct_;
  QAction *openNewsBackgroundTabAct_;
  QAction *markNewsRead_;
  QAction *markAllNewsRead_;
  QAction *markStarAct_;
  QAction *updateFeedAct_;
  QAction *deleteNewsAct_;
  QAction *deleteAllNewsAct_;
  QAction *newsKeyUpAct_;
  QAction *newsKeyDownAct_;
  QAction *prevUnreadNewsAct_;
  QAction *nextUnreadNewsAct_;
  QAction *autoLoadImagesToggle_;
  QAction *printAct_;
  QAction *printPreviewAct_;
  QAction *savePageAsAct_;
  QAction *restoreNewsAct_;
  QAction *restoreLastNewsAct_;
  QAction *newsLabelAction_;
  QAction *newsLabelMenuAction_;
  QAction *findTextAct_;
  QAction *openHomeFeedAct_;
  QAction *shareMenuAct_;
  QAction *emailShareAct_;
  QAction *evernoteShareAct_;
  QAction *gplusShareAct_;
  QAction *facebookShareAct_;
  QAction *livejournalShareAct_;
  QAction *pocketShareAct_;
  QAction *twitterShareAct_;
  QAction *vkShareAct_;
  QAction *copyLinkAct_;
  QAction *nextFolderAct_;
  QAction *prevFolderAct_;
  QAction *expandFolderAct_;
  QAction *closeTabAct_;
  QAction *closeOtherTabsAct_;
  QAction *closeAllTabsAct_;

  QActionGroup *newsFilterGroup_;
  QActionGroup *newsLabelGroup_;
  QActionGroup *shareGroup_;

  QMenu *newsLabelMenu_;
  QMenu *shareMenu_;
  QMenu *newsSortByMenu_;

  QString newsListFontFamily_;
  int newsListFontSize_;
  QString newsTitleFontFamily_;
  int newsTitleFontSize_;
  QString newsTextFontFamily_;
  int newsTextFontSize_;
  QString notificationFontFamily_;
  int notificationFontSize_;
  int browserMinFontSize_;
  int browserMinLogFontSize_;
  QString newsListTextColor_;
  QString newsListBackgroundColor_;
  QString focusedNewsTextColor_;
  QString focusedNewsBGColor_;
  QString linkColor_;
  QString titleColor_;
  QString dateColor_;
  QString authorColor_;
  QString newsTitleBackgroundColor_;
  QString newsBackgroundColor_;
  QString formatDate_;
  QString formatTime_;

  bool markNewsReadOn_;
  bool markCurNewsRead_;
  int  markNewsReadTime_;
  bool markPrevNewsRead_;
  bool markReadSwitchingFeed_;
  bool markReadClosingTab_;
  bool markReadMinimize_;
  bool showDescriptionNews_;
  bool alternatingRowColorsNews_;
  bool simplifiedDateTime_;
  bool notDeleteStarred_;
  bool notDeleteLabeled_;
  bool markIdenticalNewsRead_;

  bool autoLoadImages_;
  bool openLinkInBackground_;
  bool openingLink_;  //!< Flag - link is being opened
  bool openLinkInBackgroundEmbedded_;

  int externalBrowserOn_;
  QString externalBrowser_;
  bool javaScriptEnable_;
  bool pluginsEnable_;
  int maxPagesInCache_;
  QString downloadLocation_;
  bool askDownloadLocation_;

  int browserPosition_;

  QString newsFilterStr;

  int oldState;

  QNetworkDiskCache *diskCache_;
  CookieJar *cookieJar_;
  NetworkManager *networkManager_;

  bool hideFeedsOpenTab_;
  bool showToggleFeedsTree_;
  bool defaultIconFeeds_;

  int openNewsTab_;

  QStringList c2fWhitelist_;
  bool c2fEnabled_;
  QString userStyleBrowser_;

  int positionNotify_;
  int countShowNewsNotify_;
  int widthTitleNewsNotify_;
  int timeShowNewsNotify_;

public slots:
  void addFeed();
  void addFolder();
  void deleteItemFeedsTree();
  void slotImportFeeds();
  void slotExportFeeds();
  void slotFeedClicked(QModelIndex index);
  void slotFeedSelected(QModelIndex index, bool createTab = false);
  void slotGetFeed();
  void slotGetAllFeeds();
  void showOptionDlg();
  void receiveMessage(const QString&);
  void slotPlaceToTray();
  void slotActivationTray(QSystemTrayIcon::ActivationReason reason);
  void slotShowWindows(bool trayClick = false);
  void slotClose();
  void slotCloseApp();
  void myEmptyWorkingSet();
  void getUrlDone(const int &result, const QString &feedUrlStr,
                  const QByteArray &data, const QDateTime &dtReply);
  void slotUpdateFeed(const QString &feedUrl, const bool &changed, int newCount);
  void slotUpdateFeedDelayed(const QString &feedUrl, const bool &changed, int newCount);
  void slotFeedCountsUpdate(FeedCountStruct counts);
  void slotUpdateNews();
  void slotUpdateStatus(int feedId, bool changed = true);
  void setNewsFilter(QAction*, bool clicked = true);
  void slotCloseTab(int index);
  QWebPage *createWebTab();
  void setAutoLoadImages(bool set = true);
  void slotAuthentication(QNetworkReply *reply, QAuthenticator *auth);
  void feedsModelReload(bool checkFilter = false);

signals:
  void signalPlaceToTray();
  void signalCloseApp();
  void signalRequestUrl(const QString &urlString, const QDateTime &date, const QString &userInfo);
  void xmlReadyParse(const QByteArray &data, const QString &feedUrlStr,
                     const QDateTime &dtReply);
  void faviconRequestUrl(const QString &urlString, const QString &feedUrl);
  void signalIconFeedReady(const QString &feedUrl, const QByteArray &faviconData);
  void signalSetCurrentTab(int index, bool updateTab = false);
  void signalShowNotification();
  void signalRefreshInfoTray();
  void signalNextUpdate();
  void signalRecountCategoryCounts();

protected:
  bool eventFilter(QObject *obj, QEvent *ev);
  virtual void closeEvent(QCloseEvent*);
  virtual void changeEvent(QEvent*);

private slots:
  void slotTimerLinkOpening();
  void slotProgressBarUpdate();
  void slotVisibledFeedsWidget();
  void updateIconToolBarNull(bool feedsWidgetVisible);
  void setFeedRead(int type, int feedId, FeedReedType feedReadType,
                   NewsTabWidget *widgetTab = 0, int idException = -1);
  void markFeedRead();
  void setFeedsFilter(QAction*, bool clicked = true);
  void slotRecountCategoryCounts();

  void slotShowAboutDlg();

  void showContextMenuFeed(const QPoint & pos);
  void slotFeedsFilter();
  void slotNewsFilter();
  void slotUpdateFeedsTimer();
  bool addFeedInQueue(const QString &feedUrl);
  void slotShowUpdateAppDlg();
  void showContextMenuToolBar(const QPoint &pos);
  void showFeedPropertiesDlg();
  void slotFeedMenuShow();
  void markAllFeedsRead();
  void markAllFeedsOld();
  void slotIconFeedPreparing(const QString &feedUrl, const QByteArray &byteArray);
  void slotIconFeedUpdate(int feedId, int feedParId, const QByteArray &faviconData);
  void slotCommitDataRequest(QSessionManager&);
  void showNewsFiltersDlg(bool newFilter = false);
  void showFilterRulesDlg();
  void slotUpdateAppCheck();
  void slotNewVersion(const QString &newVersion);
  void slotFeedUpPressed();
  void slotFeedDownPressed();
  void slotFeedHomePressed();
  void slotFeedEndPressed();
  void slotFeedPrevious();
  void slotFeedNext();
  void setStyleApp(QAction*);
  void slotSwitchFocus();
  void slotSwitchPrevFocus();
  void slotOpenFeedNewTab();
  void slotOpenCategoryNewTab();
  void slotTabCurrentChanged(int index);
  void slotTabMoved(int fromIndex, int toIndex);
  void feedsColumnVisible(QAction *action);
  void setBrowserPosition(QAction *action);
  void slotOpenNewsWebView();

  void slotNewsUpPressed();
  void slotNewsDownPressed();
  void markNewsRead();
  void markAllNewsRead();
  void markNewsStar();
  void deleteNews();
  void deleteAllNewsList();
  void restoreNews();
  void openInBrowserNews();
  void openInExternalBrowserNews();
  void slotOpenNewsNewTab();
  void slotOpenNewsBackgroundTab();
  void slotCopyLinkNews();
  void setCurrentTab(int index, bool updateCurrentTab = false);
  void findText();

  void showNotification();
  void deleteNotification();
  void slotOpenNew(int feedId, int feedParId, int newsId);
  void slotOpenNewBrowser(const QUrl &url);

  void slotFindFeeds(QString);
  void slotSelectFind();
  void findFeedVisible(bool visible);

  void browserZoom(QAction*);
  void slotReportProblem();

  void slotPrint();
  void slotPrintPreview();

  void slotSavePageAs();

  void setFullScreen();
  void setStayOnTop();

  void slotMoveIndex(QModelIndex &indexWhat,QModelIndex &indexWhere, int how);

  void slotRefreshInfoTray();

  void slotCategoriesClicked(QTreeWidgetItem *item, int, bool createTab = false);
  void clearDeleted();
  void showNewsCategoriesTree();
  void feedsSplitterMoved(int pos, int);

  void setLabelNews(QAction *action);
  void setDefaultLabelNews();
  void getLabelNews();

  void setTextTitle(const QString &text, NewsTabWidget *widget);

  void lockMainToolbar(bool lock);
  void hideMainToolbar();

  void reduceNewsList();
  void increaseNewsList();

  void restoreLastNews();

  void nextUnreadNews();
  void prevUnreadNews();

  void slotIndentationFeedsTree();

  void customizeMainToolbar();
  void showCustomizeToolbarDlg(QAction *action);

  void slotShareNews(QAction *action);
  void showMenuShareNews();

  void slotOpenHomeFeed();
  void sortedByTitleFeedsTree();

  void showNewsMenu();
  void showNewsSortByMenu();
  void setNewsSortByColumn();

  void slotNextFolder();
  void slotPrevFolder();
  void slotExpandFolder();

  void showDownloadManager(bool activate = true);
  void updateInfoDownloads(const QString &text);

  void cleanUp();
  void cleanUpShutdown();

private:
  UpdateThread *persistentUpdateThread_;
  ParseThread *persistentParseThread_;
  QNetworkProxy networkProxy_;
  UpdateDelayer *updateDelayer_;

  void setProxy(const QNetworkProxy proxy);
  void showProgressBar(int addToMaximum);
  void createFeedsWidget();
  void createNewsTab(int index);
  void createToolBarNull();
  void createActions();
  void createShortcut();
  void loadActionShortcuts();
  void saveActionShortcuts();
  void createMenu();
  void createToolBar();
  void readSettings ();
  void writeSettings();
  void createTrayMenu();
  void createStatusBar();
  void createTray();
  void createTabBar();
  void loadSettingsFeeds();
  void appInstallTranslator();
  void retranslateStrings();
  void playSoundNewNews();
  void recountFeedCounts(int feedId, bool update = true);
  void recountFeedCategories(const QList<int> &categoriesList);
  void creatFeedTab(int feedId, int feedParId);
  QString getUserInfo(QUrl url, int auth);
  QUrl userStyleSheet(const QString &filePath) const;
  void initUpdateFeeds();

  int addTab(NewsTabWidget *widget);

  QString dbFileName_;
  NewsModel *newsModel_;
  TabBar *tabBar_;
  QSplitter *mainSplitter_;
  QWidget *feedsPanel_;
  QFrame *feedsWidget_;

  QList<QAction *> listActions_;
  QStringList listDefaultShortcut_;

  bool updateCheckEnabled_;

  bool closeApp_;

  QAction *addAct_;
  QAction *addFeedAct_;
  QAction *addFolderAct_;
  QAction *deleteFeedAct_;
  QAction *importFeedsAct_;
  QAction *exportFeedsAct_;
  QAction *mainToolbarToggle_;
  QAction *feedsToolbarToggle_;
  QAction *toolBarLockAct_;
  QAction *toolBarHideAct_;
  QAction *customizeMainToolbarAct_;
  QAction *customizeMainToolbarAct2_;
  QAction *customizeFeedsToolbarAct_;
  QAction *customizeNewsToolbarAct_;
  QAction *systemStyle_;
  QAction *system2Style_;
  QAction *greenStyle_;
  QAction *orangeStyle_;
  QAction *purpleStyle_;
  QAction *pinkStyle_;
  QAction *grayStyle_;
  QAction *topBrowserPositionAct_;
  QAction *bottomBrowserPositionAct_;
  QAction *rightBrowserPositionAct_;
  QAction *leftBrowserPositionAct_;
  QAction *showCleanUpWizardAct_;
  QAction *showDownloadManagerAct_;
  QAction *setNewsFiltersAct_;
  QAction *setFilterNewsAct_;
  QAction *optionsAct_;
  QAction *updateAllFeedsAct_;
  QAction *markAllFeedsRead_;
  QAction *indentationFeedsTreeAct_;
  QAction *sortedByTitleFeedsTreeAct_;
  QAction *collapseAllFoldersAct_;
  QAction *expandAllFoldersAct_;
  QAction *exitAct_;
  QAction *feedsFilter_;
  QAction *filterFeedsAll_;
  QAction *filterFeedsNew_;
  QAction *filterFeedsUnread_;
  QAction *filterFeedsStarred_;

  QAction *filterNewsAll_;
  QAction *filterNewsNew_;
  QAction *filterNewsUnread_;
  QAction *filterNewsStar_;
  QAction *filterNewsNotStarred_;
  QAction *filterNewsUnreadStar_;
  QAction *filterNewsLastDay_;
  QAction *filterNewsLastWeek_;
  QAction *aboutAct_;
  QAction *updateAppAct_;
  QAction *reportProblemAct_;

  QAction *markFeedRead_;
  QAction *feedProperties_;
  QAction *showWindowAct_;
  QAction *feedKeyUpAct_;
  QAction *feedKeyDownAct_;
  QAction *switchFocusAct_;
  QAction *switchFocusPrevAct_;
  QAction *feedsWidgetVisibleAct_;
  QAction *openFeedNewTabAct_;
  QAction *placeToTrayAct_;

  QAction *zoomInAct_;
  QAction *zoomOutAct_;
  QAction *zoomTo100Act_;

  QActionGroup *customizeToolbarGroup_;
  QActionGroup *styleGroup_;
  QActionGroup *browserPositionGroup_;
  QActionGroup *feedsFilterGroup_;
  QActionGroup *browserZoomGroup_;
  QActionGroup *newsSortByColumnGroup_;
  QActionGroup *newsSortOrderGroup_;

  QAction *feedsFilterAction_; //!< Filter name storage, needed to enable last used filter
  QAction *newsFilterAction_;  //!< Filter name storege, needed to enable last used filter
  QString mainNewsFilter_;

  QAction *showUnreadCount_;
  QAction *showUndeleteCount_;
  QAction *showLastUpdated_;
  QActionGroup *feedsColumnsGroup_;

  QAction *findFeedAct_;
  QAction *fullScreenAct_;
  QAction *stayOnTopAct_;

  QAction *nextTabAct_;
  QAction *prevTabAct_;

  QAction *reduceNewsListAct_;
  QAction *increaseNewsListAct_;

  QMenu *fileMenu_;
  QMenu *newMenu_;
  QMenu *editMenu_;
  QMenu *viewMenu_;
  QMenu *toolbarsMenu_;
  QMenu *customizeToolbarMenu_;
  QMenu *styleMenu_;
  QMenu *browserPositionMenu_;
  QMenu *feedMenu_;
  QMenu *newsMenu_;
  QMenu *browserMenu_;
  QMenu *toolsMenu_;
  QMenu *helpMenu_;
  QMenu *trayMenu_;
  QMenu *newsContextMenu_;
  QMenu *feedsFilterMenu_;
  QMenu *newsFilterMenu_;
  QMenu *feedsColumnsMenu_;
  QMenu *browserZoomMenu_;

  QToolBar *mainToolbar_;
  QToolBar *feedsToolBar_;

  NewsView *newsView_;

  QProgressBar *progressBar_;
  QLabel *statusUnread_;
  QLabel *statusAll_;

  QPushButton *pushButtonNull_;

  QTimer *updateFeedsTimer_;
  int updateIntervalSec_;
  int updateTimeCount_;
  bool updateFeedsStartUp_;
  bool updateFeedsEnable_;
  int  updateFeedsInterval_;
  int  updateFeedsIntervalType_;
  QList<QString> feedUrlList_;
  QMap<int,int> updateFeedsIntervalSec_;
  QMap<int,int> updateFeedsTimeCount_;

  QTranslator *translator_;
  QString langFileName_;

  bool minimizingTray_;
  bool closingTray_;
  bool singleClickTray_;
  bool clearStatusNew_;
  bool emptyWorking_;
  int behaviorIconTray_;

  int openingFeedAction_;
  bool openNewsWebViewOn_;

  FaviconThread *faviconThread_;

  DBMemFileThread *dbMemFileThread_;

  bool soundNewNews_;
  QString soundNotifyPath_;
  bool playSoundNewNews_;
  bool showNotifyOn_;
  bool fullscreenModeNotify_;
  bool onlySelectedFeeds_;

  UpdateAppDialog *updateAppDialog_;

  bool cleanupOnShutdown_;
  int maxDayCleanUp_;
  int maxNewsCleanUp_;
  bool dayCleanUpOn_;
  bool newsCleanUpOn_;
  bool readCleanUp_;
  bool neverUnreadCleanUp_;
  bool neverStarCleanUp_;
  bool neverLabelCleanUp_;
  bool cleanUpDeleted_;
  bool optimizeDB_;

  int updateFeedsCount_;
  QList<int> idFeedList_;
  QList<int> cntNewNewsList_;

  bool reopenFeedStartup_;
  bool openNewTabNextToActive_;

  bool updateCurrentTab_;

  NotificationWidget *notificationWidget;

  FindFeed *findFeeds_;
  QWidget *findFeedsWidget_;

  int feedIdOld_;

  bool storeDBMemory_;
  bool storeDBMemoryT_;

  int  openingLinkTimeout_;  //!< During this time we'll trying swithing back to apllication
  QTimer timerLinkOpening_;

  QWidget *categoriesPanel_;
  QLabel *categoriesLabel_;
  QToolButton *showCategoriesButton_;
  QWidget *categoriesWidget_;
  QSplitter *feedsSplitter_;
  QByteArray feedsWidgetSplitterState_;

  QElapsedTimer timer_;

  qint64 activationStateChangedTime_;

  bool importFeedStart_;

  bool changeBehaviorActionNUN_;

  bool recountCategoryCountsOn_;

  QString diskCacheDirPathDefault_;

};

#endif

