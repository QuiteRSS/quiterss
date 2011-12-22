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

public slots:
    void addFeed();
    void deleteFeed();
    void importFeeds();
    void slotFeedsTreeClicked(QModelIndex index);
    void slotGetFeed();
    void slotGetAllFeeds();
    void slotNewsViewClicked(QModelIndex index);
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
    void getUrlDone(const int &result);
    void slotUpdateFeed(const QUrl &url);
    void slotUpdateStatus();

protected:
     bool eventFilter(QObject *obj, QEvent *ev);
     virtual void closeEvent(QCloseEvent*);
     virtual void changeEvent(QEvent*);
     virtual void showEvent(QShowEvent*);

private:
    UpdateThread *persistentUpdateThread_;
    ParseThread *persistentParseThread_;
    QNetworkProxy networkProxy_;

    void getFeed(QString urlString);
    void createActions();
    void createMenu();
    void createToolBar();
    void readSettings ();
    void writeSettings();
    void createTrayMenu();

    QSettings *settings_;

    QSqlDatabase db_;
    FeedsModel *feedsModel_;
    NewsModel *newsModel_;

    QAction *addFeedAct_;
    QAction *deleteFeedAct_;
    QAction *importFeedsAct_;
    QAction *toolBarToggle_;
    QAction *optionsAct_;
    QAction *updateFeedAct_;
    QAction *updateAllFeedsAct_;
    QAction *exitAct_;
    QAction *markNewsRead_;
    QAction *markAllNewsRead_;
    QAction *markNewsUnread_;
    QAction *setProxyAct_;
    QAction *filterFeedsAll_;
    QAction *filterFeedsUnread_;
    QAction *filterNewsAll_;
    QAction *filterNewsUnread_;

    QActionGroup *feedsFilterGroup_;
    QActionGroup *newsFilterGroup_;

    QMenu *fileMenu_;
    QMenu *viewMenu_;
    QMenu *feedMenu_;
    QMenu *newsMenu_;
    QMenu *toolsMenu_;
    QMenu *trayMenu_;
    QToolBar *toolBar_;

    QDockWidget *feedsDock_;
    Qt::DockWidgetArea feedsDockArea_;
    QDockWidget *newsDock_;
    Qt::DockWidgetArea newsDockArea_;
    QTreeView *feedsView_;
    NewsView *newsView_;
    NewsHeader *newsHeader_;

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

private slots:
    void slotSetProxy();
    void slotProgressBarUpdate();
    void slotVisibledFeedsDock();
    void slotDockLocationChanged(Qt::DockWidgetArea area);
    void setItemRead(QModelIndex index, int read);
    void markNewsRead();
    void markAllNewsRead();
    void slotLoadStarted();
    void slotLoadFinished(bool ok);
    void setFeedsFilter(QAction*);
    void setNewsFilter(QAction*);
    void slotFeedsDockLocationChanged(Qt::DockWidgetArea area);
    void slotNewsDockLocationChanged(Qt::DockWidgetArea area);

signals:
    void signalFeedsTreeKeyUpDownPressed();
    void signalFeedKeyUpDownPressed();
    void signalPlaceToTray();
    void signalCloseApp();
    void xmlReadyParse(const QByteArray &data, const QUrl &url);
};

#endif

