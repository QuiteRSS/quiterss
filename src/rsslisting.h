#ifndef RSSLISTING_H
#define RSSLISTING_H

#include <QtGui>
#include <QtSql>
#include <QtWebKit>

#include "feedsmodel.h"
#include "newsheader.h"
#include "newsmodel.h"
#include "parsethread.h"
#include "updatethread.h"
#include "newsview.h"

class RSSListing : public QMainWindow
{
    Q_OBJECT
public:
    RSSListing(QWidget *widget = 0);
    ~RSSListing();
    bool startingTray_;

public slots:
    void addFeed();
    void deleteFeed();
    void slotImportFeeds();
    void slotFeedsTreeClicked(QModelIndex index);
    void slotFeedsTreeSelected(QModelIndex index);
    void slotGetFeed();
    void slotGetAllFeeds();
    void slotNewsViewClicked(QModelIndex index);
    void slotNewsViewDoubleClicked(QModelIndex index);
    void slotFeedsTreeKeyUpDownPressed();
    void slotNewsKeyUpDownPressed();
    void showOptionDlg();
    void receiveMessage(const QString&);
    void slotPlaceToTray();
    void slotActivationTray(QSystemTrayIcon::ActivationReason reason);
    void slotShowWindows();
    void slotClose();
    void slotCloseApp();
    void myEmptyWorkingSet();
    void receiveXml(const QByteArray &data, const QUrl &url);
    void getUrlDone(const int &result, const QDateTime &dtReply);
    void slotUpdateFeed(const QUrl &url);

protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    virtual void closeEvent(QCloseEvent*);
    virtual void changeEvent(QEvent*);
    virtual void showEvent(QShowEvent*);
    void timerEvent(QTimerEvent* event);

private:
    UpdateThread *persistentUpdateThread_;
    ParseThread *persistentParseThread_;
    QNetworkProxy networkProxy_;

    void getFeed(QString urlString, QDateTime date);
    void createActions();
    void createMenu();
    void createToolBar();
    void readSettings ();
    void writeSettings();
    void createTrayMenu();
    void createMenuNews();
    void createMenuFeed();
    void loadSettingsFeeds();
    void updateWebView(QModelIndex index);
    bool sqliteDBMemFile(QSqlDatabase memdb, QString filename, bool save);
    void retranslateStrings();

    QSettings *settings_;
    QString dataDirPath_;

    QSqlDatabase db_;
    QString dbFileName_;
    FeedsModel *feedsModel_;
    NewsModel *newsModel_;

    QAction *addFeedAct_;
    QAction *deleteFeedAct_;
    QAction *importFeedsAct_;
    QAction *toolBarToggle_;
    QAction *toolBarStyleI_;
    QAction *toolBarStyleT_;
    QAction *toolBarStyleTbI_;
    QAction *toolBarStyleTuI_;
    QAction *toolBarIconSmall_;
    QAction *toolBarIconNormal_;
    QAction *toolBarIconBig_;
    QAction *autoLoadImagesToggle_;
    QAction *optionsAct_;
    QAction *updateFeedAct_;
    QAction *updateAllFeedsAct_;
    QAction *exitAct_;
    QAction *markNewsRead_;
    QAction *markAllNewsRead_;
    QAction *markNewsUnread_;
    QAction *feedsFilter_;
    QAction *filterFeedsAll_;
    QAction *filterFeedsNew_;
    QAction *filterFeedsUnread_;
    QAction *newsFilter_;
    QAction *filterNewsAll_;
    QAction *filterNewsNew_;
    QAction *filterNewsUnread_;
    QAction *filterNewsStar_;
    QAction *aboutAct_;
    QAction *updateAppAct_;
    QAction *openInBrowserAct_;
    QAction *markStarAct_;
    QAction *deleteNewsAct_;
    QAction *markFeedRead_;
    QAction *feedProperties_;
    QAction *showWindowAct_;

    QActionGroup *toolBarStyleGroup_;
    QActionGroup *toolBarIconSizeGroup_;
    QActionGroup *feedsFilterGroup_;
    QActionGroup *newsFilterGroup_;

    QMenu *fileMenu_;
    QMenu *editMenu_;
    QMenu *viewMenu_;
    QMenu *toolBarMenu_;
    QMenu *toolBarStyleMenu_;
    QMenu *toolBarIconSizeMenu_;
    QMenu *feedMenu_;
    QMenu *newsMenu_;
    QMenu *toolsMenu_;
    QMenu *helpMenu_;
    QMenu *trayMenu_;
    QMenu *newsContextMenu_;
    QMenu *feedContextMenu_;
    QMenu *feedsFilterMenu_;
    QMenu *newsFilterMenu_;

    QToolBar *toolBar_;
    QToolBar *feedsToolBar_;
    QToolBar *newsToolBar_;

    QDockWidget *feedsDock_;
    Qt::DockWidgetArea feedsDockArea_;
    QDockWidget *newsDock_;
    Qt::DockWidgetArea newsDockArea_;
    QTreeView *feedsView_;
    NewsView *newsView_;
    NewsHeader *newsHeader_;
    QLabel *feedsTitleLabel_;
    QLabel *newsTitleLabel_;

    QLabel *webPanelTitle_;
    QLabel *webPanelTitleLabel_;
    QLabel *webPanelAuthorLabel_;
    QLabel *webPanelAuthor_;
    QWidget *webPanel_;

    QWidget *webWidget_;
    QWebView *webView_;
    QProgressBar *webViewProgress_;

    QSystemTrayIcon *traySystem;
    int oldState;

    QProgressBar *progressBar_;
    QLabel *statusUnread_;
    QLabel *statusAll_;

    QToolBar *toolBarNull_;
    QPushButton *pushButtonNull_;

    QByteArray data_;
    QUrl url_;

    QBasicTimer updateFeedsTimer_;
    bool autoUpdatefeedsStartUp_;
    bool autoUpdatefeeds_;
    int  autoUpdatefeedsTime_;

    QTranslator *translator_;
    QString langFileName_;

    bool autoLoadImages_;

    bool minimizingTray_;
    bool closingTray_;
    bool singleClickTray_;
    bool emptyWorking_;

private slots:
    void slotProgressBarUpdate();
    void slotVisibledFeedsDock();
    void slotDockLocationChanged(Qt::DockWidgetArea area);
    void slotSetItemRead(QModelIndex index, int read);
    void markNewsRead();
    void markAllNewsRead();
    void slotLoadStarted();
    void slotLoadFinished(bool ok);
    void setFeedsFilter(QAction*);
    void setNewsFilter(QAction*);
    void slotFeedsDockLocationChanged(Qt::DockWidgetArea area);
    void slotNewsDockLocationChanged(Qt::DockWidgetArea area);
    void slotUpdateStatus();
    void slotSetAllRead();
    void slotShowAboutDlg();
    void deleteNews();
    void showContextMenuNews(const QPoint &);
    void openInBrowserNews();
    void slotSetItemStar(QModelIndex index, int sticky);
    void markNewsStar();
    void showContextMenuFeed(const QPoint &);
    void slotLinkClicked(QUrl url);
    void setAutoLoadImages();
    void slotFeedsFilter();
    void slotNewsFilter();
    void slotTimerUpdateFeeds();
    void slotShowUpdateAppDlg();
    void setToolBarStyle(QAction*);
    void setToolBarIconSize(QAction*);
    void showContextMenuToolBar(const QPoint &);
    void slotShowFeedPropertiesDlg();
    void slotEditMenuAction();

signals:
    void signalFeedsTreeKeyUpDownPressed();
    void signalFeedKeyUpDownPressed();
    void signalPlaceToTray();
    void signalCloseApp();
    void xmlReadyParse(const QByteArray &data, const QUrl &url);
};

#endif

