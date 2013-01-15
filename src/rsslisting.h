#ifndef RSSLISTING_H
#define RSSLISTING_H

#include <QtGui>
#include <QtSql>
#include <QtWebKit>

#include "dbmemfilethread.h"
#include "faviconloader.h"
#include "feedstreemodel.h"
#include "feedstreeview.h"
#include "findfeed.h"
#include "newsheader.h"
#include "newsmodel.h"
#include "newstabwidget.h"
#include "newsview.h"
#include "notifications.h"
#include "parsethread.h"
#include "updateappdialog.h"
#include "updatedelayer.h"
#include "updatethread.h"
#include "webview.h"

class RSSListing : public QMainWindow
{
  Q_OBJECT
public:
  RSSListing(QSettings *settings, QString dataDirPath, QWidget *parent = 0);
  ~RSSListing();
  bool showSplashScreen_;
  bool showTrayIcon_;
  bool startingTray_;
  QSystemTrayIcon *traySystem;
  void restoreFeedsOnStartUp();
  void expandNodes();

  QSettings *settings_;
  QSqlDatabase db_;
  FeedsTreeModel *feedsTreeModel_;
  FeedsTreeView *feedsTreeView_;
  QTreeWidget *newsCategoriesTree_;
#define TAB_WIDGET_PERMANENT 0
  QStackedWidget *stackedWidget_;

  NewsTabWidget *currentNewsTab;

  QAction *newsToolbarToggle_;
  QAction *browserToolbarToggle_;
  QAction *newsFilter_;
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
  QAction *autoLoadImagesToggle_;
  QAction *printAct_;
  QAction *printPreviewAct_;
  QAction *restoreNewsAct_;
  QAction *newsLabelAction_;
  QAction *newsLabelMenuAction_;
  QActionGroup *newsFilterGroup_;
  QActionGroup *newsLabelGroup_;

  QMenu *newsLabelMenu_;

  QString newsFontFamily_;
  int newsFontSize_;
  QString panelNewsFontFamily_;
  int panelNewsFontSize_;
  QString webFontFamily_;
  int webFontSize_;
  QString notificationFontFamily_;
  int notificationFontSize_;
  QString formatDateTime_;

  bool markNewsReadOn_;
  bool markCurNewsRead_;
  int  markNewsReadTime_;
  bool markPrevNewsRead_;
  bool markReadSwitchingFeed_;
  bool markReadClosingTab_;
  bool markReadMinimize_;
  bool showDescriptionNews_;

  bool autoLoadImages_;
  bool openLinkInBackground_;
  bool openingLink_;  //!< флаг открытия ссылки
  bool openLinkInBackgroundEmbedded_;

  int externalBrowserOn_;
  QString externalBrowser_;
  bool javaScriptEnable_;
  bool pluginsEnable_;

  int browserPosition_;

  QString newsFilterStr;

  int oldState;

public slots:
  void addFeed();
  void addFolder();
  void deleteFeed();
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
  void receiveXml(const QByteArray &data, const QUrl &url);
  void getUrlDone(const int &result, const QDateTime &dtReply);
  void slotUpdateFeed(int feedId, const bool &changed, int newCount);
  void slotUpdateFeedDelayed(int feedId, const bool &changed, int newCount);
  void slotUpdateNews();
  void slotUpdateStatus(int feedId, bool changed = true);
  void setNewsFilter(QAction*, bool clicked = true);
  void slotTabCloseRequested(int index);
  QWebPage *createWebTab();
  void setAutoLoadImages(bool set = true);

protected:
  bool eventFilter(QObject *obj, QEvent *ev);
  virtual void closeEvent(QCloseEvent*);
  virtual void changeEvent(QEvent*);
  void timerEvent(QTimerEvent* event);

private:
  UpdateThread *persistentUpdateThread_;
  ParseThread *persistentParseThread_;
  QNetworkProxy networkProxy_;
  UpdateDelayer *updateDelayer_;

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
  void createMenuFeed();
  void createStatusBar();
  void createTray();
  void loadSettingsFeeds();
  void appInstallTranslator();
  void retranslateStrings();
  void playSoundNewNews();
  void feedsCleanUp(QString feedId);
  void recountFeedCounts(int feedId, bool update = true);
  void recountFeedCategories(const QList<int> &categoriesList);
  void creatFeedTab(int feedId, int feedParId);
  void cleanUp();

  int addTab(NewsTabWidget *widget);

  QString dataDirPath_;
  QString lastFeedPath_;

  QString dbFileName_;
  NewsModel *newsModel_;
  QTabBar *tabBar_;
  QSplitter *mainSplitter_;
  QWidget *feedsPanel_;
  QFrame *feedsWidget_;

  QList<QAction *> listActions_;
  QStringList listDefaultShortcut_;

  bool updateCheck_;

  bool closeApp_;

  QAction *addAct_;
  QAction *addFeedAct_;
  QAction *addFolderAct_;
  QAction *deleteFeedAct_;
  QAction *importFeedsAct_;
  QAction *exportFeedsAct_;
  QAction *mainToolbarToggle_;
  QAction *feedsToolbarToggle_;
  QAction *toolBarHide_;
  QAction *toolBarStyleI_;
  QAction *toolBarStyleT_;
  QAction *toolBarStyleTbI_;
  QAction *toolBarStyleTuI_;
  QAction *toolBarIconSmall_;
  QAction *toolBarIconNormal_;
  QAction *toolBarIconBig_;
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
  QAction *setNewsFiltersAct_;
  QAction *setFilterNewsAct_;
  QAction *optionsAct_;
  QAction *updateAllFeedsAct_;
  QAction *markAllFeedsRead_;
  QAction *titleSortFeedsAct_;
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
  QAction *aboutAct_;
  QAction *updateAppAct_;
  QAction *reportProblemAct_;

  QAction *markFeedRead_;
  QAction *feedProperties_;
  QAction *showWindowAct_;
  QAction *feedKeyUpAct_;
  QAction *feedKeyDownAct_;
  QAction *switchFocusAct_;
  QAction *visibleFeedsWidgetAct_;
  QAction *openFeedNewTabAct_;
  QAction *placeToTrayAct_;

  QAction *zoomInAct_;
  QAction *zoomOutAct_;
  QAction *zoomTo100Act_;

  QActionGroup *toolBarStyleGroup_;
  QActionGroup *toolBarIconSizeGroup_;
  QActionGroup *styleGroup_;
  QActionGroup *browserPositionGroup_;
  QActionGroup *feedsFilterGroup_;
  QActionGroup *browserZoomGroup_;

  QAction *feedsFilterAction_; //!< Хранение имени фильтра, необходимое для включения последнего используемого фильтра
  QAction *newsFilterAction_;  //!< Хранение имени фильтра, необходимое для включения последнего используемого фильтра

  QAction *showUnreadCount_;
  QAction *showUndeleteCount_;
  QAction *showLastUpdated_;
  QActionGroup *feedsColumnsGroup_;

  QAction *findFeedAct_;
  QAction *fullScreenAct_;
  QAction *stayOnTopAct_;

  QAction *closeTabAct_;
  QAction *nextTabAct_;
  QAction *prevTabAct_;

  QMenu *fileMenu_;
  QMenu *newMenu_;
  QMenu *editMenu_;
  QMenu *viewMenu_;
  QMenu *toolbarsMenu_;
  QMenu *customizeToolbarMenu_;
  QMenu *mainToolbarMenu_;
  QMenu *toolBarStyleMenu_;
  QMenu *toolBarIconSizeMenu_;
  QMenu *styleMenu_;
  QMenu *browserPositionMenu_;
  QMenu *feedMenu_;
  QMenu *newsMenu_;
  QMenu *browserMenu_;
  QMenu *toolsMenu_;
  QMenu *helpMenu_;
  QMenu *trayMenu_;
  QMenu *newsContextMenu_;
  QMenu *feedContextMenu_;
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

  QByteArray data_;
  QUrl url_;

  QBasicTimer updateFeedsTimer_;
  bool autoUpdatefeedsStartUp_;
  bool autoUpdatefeeds_;
  int  autoUpdatefeedsTime_;
  int  autoUpdatefeedsInterval_;

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

  FaviconLoader *faviconLoader_;

  DBMemFileThread *dbMemFileThread_;
  bool commitDataRequest_;

  bool soundNewNews_;
  QString soundNotifyPath_;
  bool playSoundNewNews_;
  bool showNotifyOn_;
  int countShowNewsNotify_;
  int widthTitleNewsNotify_;
  int timeShowNewsNotify_;
  bool onlySelectedFeeds_;

  UpdateAppDialog *updateAppDialog_;

  int maxDayCleanUp_;
  int maxNewsCleanUp_;
  bool dayCleanUpOn_;
  bool newsCleanUpOn_;
  bool readCleanUp_;
  bool neverUnreadCleanUp_;
  bool neverStarCleanUp_;
  bool neverLabelCleanUp_;

  int updateFeedsCount_;
  QList<int> idFeedList_;
  QList<int> cntNewNewsList_;

  bool reopenFeedStartup_;

  bool updateCurrentTab_;

  NotificationWidget *notificationWidget;

  FindFeed *findFeeds_;
  QWidget *findFeedsWidget_;

  int feedIdOld_;

  bool storeDBMemory_;
  bool storeDBMemoryT_;

  int  openingLinkTimeout_;  //!< в течении этого времени мы будем переключаться обратно в наше приложение
  QBasicTimer timerLinkOpening_;
  int openNewsTab_;

  enum FeedReedType {
    FeedReadTypeSwitchingFeed,
    FeedReadClosingTab,
    FeedReadPlaceToTray,
    FeedReadTypeCount
  };

  QWidget *categoriesPanel_;
  QLabel *categoriesLabel_;
  QToolButton *showCategoriesButton_;
  QWidget *categoriesWidget_;
  QSplitter *feedsSplitter_;
  QByteArray feedsWidgetSplitterState_;

  QElapsedTimer timer_;

private slots:
  void slotProgressBarUpdate();
  void slotVisibledFeedsWidget();
  void updateIconToolBarNull(bool feedsWidgetVisible);
  void setFeedRead(int feedId, FeedReedType feedReadtype);
  void markFeedRead();
  void setFeedsFilter(QAction*, bool clicked = true);
  void feedsModelReload();

  void slotShowAboutDlg();

  void showContextMenuFeed(const QPoint &);
  void slotFeedsFilter();
  void slotNewsFilter();
  void slotTimerUpdateFeeds();
  void slotShowUpdateAppDlg();
  void setToolBarStyle(QAction*);
  void setToolBarIconSize(QAction*);
  void showContextMenuToolBar(const QPoint &);
  void slotShowFeedPropertiesDlg();
  void slotFeedMenuShow();
  void markAllFeedsRead();
  void markAllFeedsOld();
  void slotIconFeedLoad(int feedId, const QByteArray &byteArray, const int &cntQueue);
  void slotCommitDataRequest(QSessionManager&);
  void showNewsFiltersDlg(bool newFilter = false);
  void showFilterRulesDlg();
  void slotUpdateAppCheck();
  void slotNewVersion(QString newVersion);
  void slotFeedUpPressed();
  void slotFeedDownPressed();
  void slotFeedHomePressed();
  void slotFeedEndPressed();
  void slotFeedPrevious();
  void slotFeedNext();
  void setStyleApp(QAction*);
  void slotSwitchFocus();
  void slotOpenFeedNewTab();
  void slotTabCurrentChanged(int index);
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
  void setCurrentTab(int index, bool updateCurrentTab = false);
  void findText();

  void showNotification();
  void deleteNotification();
  void slotOpenNew(int feedId, int feedParId, int newsId);

  void slotFindFeeds(QString);
  void slotSelectFind();
  void findFeedVisible(bool visible);

  void slotSortFeeds();

  void browserZoom(QAction*);
  void slotReportProblem();

  void slotPrint();
  void slotPrintPreview();

  void setFullScreen();
  void setStayOnTop();

  void slotMoveIndex(QModelIndex &indexWhat,QModelIndex &indexWhere);

  void slotRefreshInfoTray();

  void slotCategoriesClicked(QTreeWidgetItem *item, int);
  void showNewsCategoriesTree();
  void feedsSplitterMoved(int pos, int);

  void setLabelNews(QAction *action);
  void setDefaultLabelNews();
  void getLabelNews();

  void slotCloseTab();
  void slotNextTab();
  void slotPrevTab();

  void hideMainToolbar();

signals:
  void signalPlaceToTray();
  void signalCloseApp();
  void startGetUrlTimer();
  void xmlReadyParse(const QByteArray &data, const QUrl &url);
  void signalSetCurrentTab(int index, bool updateTab = false);
  void signalShowNotification();
  void signalRefreshInfoTray();
  void signalNextUpdate();

};

#endif

