#ifndef NEWSTABWIDGET_H
#define NEWSTABWIDGET_H

#include <QtGui>
#include <QtSql>
#include <QtWebKit>

#include "feedsmodel.h"
#include "feedsview.h"
#include "newsheader.h"
#include "newsmodel.h"
#include "newsview.h"
#include "webview.h"

#define TOP_POSITION    0
#define BOTTOM_POSITION 1
#define RIGHT_POSITION  2
#define LEFT_POSITION   3

class NewsTabWidget : public QWidget
{
  Q_OBJECT
private:
  void createNewsList();
  void createMenuNews();
  void createWebWidget();

  QWidget *newsWidget_;
  QMenu *newsContextMenu_;

  FeedsModel *feedsModel_;
  FeedsView *feedsView_;

  QFrame *lineWebWidget;
  QWidget *webWidget_;
  QLabel *webPanelTitle_;
  QLabel *webPanelTitleLabel_;
  QLabel *webPanelAuthorLabel_;
  QLabel *webPanelAuthor_;
  QWidget *webControlPanel_;
  QProgressBar *webViewProgress_;
  QLabel *webViewProgressLabel_;

  QAction *webHomePageAct_;
  QAction *webExternalBrowserAct_;

  QTimer *markNewsReadTimer_;

  int currentNewsIdOld;
  int currentFeedIdOld;

public:
  explicit NewsTabWidget(int feedId, QWidget *parent);
  void retranslateStrings();
  void setSettings();
  void setBrowserPosition();

  void updateWebView(QModelIndex index);

  int feedId_;

  NewsModel *newsModel_;
  NewsView *newsView_;
  NewsHeader *newsHeader_;
  QToolBar *newsToolBar_;
  QSplitter *newsTabWidgetSplitter_;

  WebView *webView_;
  QWidget *webPanel_;

  QLabel *newsIconTitle_;
  QLabel *newsTextTitle_;
  QWidget *newsTitleLabel_;
  QToolButton *closeButton_;

signals:
  void signalWebViewSetContent(QString content);

public slots:
  void slotNewsViewClicked(QModelIndex index);
  void slotNewsViewSelected(QModelIndex index, bool clicked = false);
  void slotNewsViewDoubleClicked(QModelIndex index);
  void slotNewsMiddleClicked(QModelIndex index);
  void slotNewsUpPressed();
  void slotNewsDownPressed();
  void slotNewsHomePressed();
  void slotNewsEndPressed();
  void markAllNewsRead(bool openFeed = true);

private slots:
  void showContextMenuNews(const QPoint &p);
  void slotSetItemRead(QModelIndex index, int read);
  void slotSetItemStar(QModelIndex index, int starred);
  void slotReadTimer();
  void markNewsRead();
  void markNewsStar();
  void deleteNews();

  void slotWebViewSetContent(QString content);
  void slotWebTitleLinkClicked(QString urlStr);
  void webHomePage();
  void openPageInExternalBrowser();
  void slotLinkClicked(QUrl url);
  void slotLinkHovered(const QString &link, const QString &, const QString &);
  void slotSetValue(int value);
  void slotLoadStarted();
  void slotLoadFinished(bool ok);
  void slotOpenNewsWebView();
  void openInBrowserNews();
  void openInExternalBrowserNews();

  void slotTabClose();
  void webTitleChanged(QString title);

};

#endif // NEWSTABWIDGET_H
