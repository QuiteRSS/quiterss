#ifndef RSSLISTING_H
#define RSSLISTING_H

#include <QtGui>
#include <QtSql>
#include <QtWebKit>
#include <QNetworkProxy>

#include "cookiejar.h"
#include "dbmemfilethread.h"
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
  QList<int> getIdFeedsInList(int idFolder);
  QString getIdFeedsString(int idFolder);

  void setToolBarStyle(const QString &styleStr);
  void setToolBarIconSize(const QString &iconSizeStr);

  QSettings *settings_;
  QSqlDatabase db_;
  FeedsTreeModel *feedsTreeModel_;
  FeedsTreeView *feedsTreeView_;
  QTreeWidget *newsCategoriesTree_;
#define TAB_WIDGET_PERMANENT 0
  QStackedWidget *stackedWidget_;

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
  QAction *facebookShareAct_;
  QAction *livejournalShareAct_;
  QAction *pocketShareAct_;
  QAction *twitterShareAct_;
  QAction *vkShareAct_;

  QActionGroup *newsFilterGroup_;
  QActionGroup *newsLabelGroup_;
  QActionGroup *shareGroup_;

  QMenu *newsLabelMenu_;
  QMenu *shareMenu_;

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

  CookieJar *cookieJar_;
  NetworkManager *networkManager_;

  bool hideFeedsOpenTab_;
  bool defaultIconFeeds_;

  int openNewsTab_;

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
  void getUrlDone(const int &result, const QString &feedUrlStr,
                  const QByteArray &data, const QDateTime &dtReply);
  void slotUpdateFeed(int feedId, const bool &changed, int newCount);
  void slotUpdateFeedDelayed(int feedId, const bool &changed, int newCount);
  void slotFeedCountsUpdate(FeedCountStruct counts);
  void slotUpdateNews();
  void slotUpdateStatus(int feedId, bool changed = true);
  void setNewsFilter(QAction*, bool clicked = true);
  void slotTabCloseRequested(int index);
  QWebPage *createWebTab();
  void setAutoLoadImages(bool set = true);
  void slotAuthentication(QNetworkReply *reply, QAuthenticator *auth);

protected:
  bool eventFilter(QObject *obj, QEvent *ev);
  virtual void closeEvent(QCloseEvent*);
  virtual void changeEvent(QEvent*);

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
  QString getUserInfo(QUrl url, int auth);

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
  QAction *setNewsFiltersAct_;
  QAction *setFilterNewsAct_;
  QAction *optionsAct_;
  QAction *updateAllFeedsAct_;
  QAction *markAllFeedsRead_;
  QAction *indentationFeedsTreeAct_;
  QAction *sortedByTitleFeedsTreeAct_;
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

  QAction *reduceNewsListAct_;
  QAction *increaseNewsListAct_;

  QMenu *fileMenu_;
  QMenu *newMenu_;
  QMenu *editMenu_;
  QMenu *viewMenu_;
  QMenu *toolbarsMenu_;
  QMenu *customizeToolbarMenu_;
  QMenu *mainToolbarMenu_;
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

  QTimer *updateFeedsTimer_;
  bool autoUpdatefeedsStartUp_;
  bool autoUpdatefeeds_;
  int  autoUpdatefeedsTime_;
  int  autoUpdatefeedsInterval_;
  bool updateFeedsStart_;

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
  QTimer timerLinkOpening_;

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

  qint64 activationStateChangedTime_;

  bool importFeedStart_;

private slots:
  void slotTimerLinkOpening();
  void slotProgressBarUpdate();
  void slotVisibledFeedsWidget();
  void updateIconToolBarNull(bool feedsWidgetVisible);
  void setFeedRead(int feedId, FeedReedType feedReadtype);
  void markFeedRead();
  void setFeedsFilter(QAction*, bool clicked = true);
  void feedsModelReload(bool checkFilter = false);

  void slotShowAboutDlg();

  void showContextMenuFeed(const QPoint &);
  void slotFeedsFilter();
  void slotNewsFilter();
  void slotTimerUpdateFeeds();
  void slotShowUpdateAppDlg();
  void showContextMenuToolBar(const QPoint &);
  void slotShowFeedPropertiesDlg();
  void slotFeedMenuShow();
  void slotFeedMenuHide();
  void markAllFeedsRead();
  void markAllFeedsOld();
  void slotIconFeedPreparing(const QString &feedUrl, const QByteArray &byteArray);
  void slotIconFeedUpdate(int feedId, int feedParId, const QByteArray &faviconData);
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
  void slotSwitchPrevFocus();
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

  void slotCategoriesClicked(QTreeWidgetItem *item, int);
  void showNewsCategoriesTree();
  void feedsSplitterMoved(int pos, int);

  void setLabelNews(QAction *action);
  void setDefaultLabelNews();
  void getLabelNews();

  void slotCloseTab();
  void slotNextTab();
  void slotPrevTab();
  void setTextTitle(const QString &text, NewsTabWidget *widget);

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

};

#endif

