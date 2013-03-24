#ifndef NEWSTABWIDGET_H
#define NEWSTABWIDGET_H

#include <QtGui>
#include <QtSql>
#include <QtWebKit>

#include "feedstreemodel.h"
#include "feedstreeview.h"
#include "findtext.h"
#include "newsheader.h"
#include "newsmodel.h"
#include "newsview.h"
#include "webview.h"

class RSSListing;

#define TOP_POSITION    0
#define BOTTOM_POSITION 1
#define RIGHT_POSITION  2
#define LEFT_POSITION   3

#define TAB_FEED       0
#define TAB_WEB        1
#define TAB_CAT_UNREAD 2
#define TAB_CAT_STAR   3
#define TAB_CAT_DEL    4
#define TAB_CAT_LABEL  5

#define RESIZESTEP 25   // Шаг изменения размера списка новостей/браузера

class NewsTabWidget : public QWidget
{
  Q_OBJECT
private:
  void createNewsList();
  void createMenuNews();
  void createWebWidget();

  RSSListing *rsslisting_;
  QSqlDatabase db_;
  QWidget *newsWidget_;
  QMenu *newsContextMenu_;

  FeedsTreeModel *feedsTreeModel_;
  FeedsTreeView *feedsTreeView_;

  QFrame *lineWebWidget;
  QWidget *webWidget_;
  QProgressBar *webViewProgress_;
  QLabel *webViewProgressLabel_;

  QAction *webHomePageAct_;
  QAction *webExternalBrowserAct_;
  QAction *urlExternalBrowserAct_;

  QTimer *markNewsReadTimer_;

  int webDefaultFontSize_;
  int webDefaultFixedFontSize_;

  QMenu *webMenu_;
  QUrl linkUrl_;

  QWidget *newsPanelWidget_;
  bool webToolbarShow_;

public:
  explicit NewsTabWidget(QWidget *parent, int type, int feedId = -1, int feedParId = -1);

  void retranslateStrings();
  void setSettings(bool newTab = true);
  void setBrowserPosition();
  void markNewsRead();
  void markAllNewsRead();
  void markNewsStar();
  void setLabelNews(int labelId);
  void deleteNews();
  void deleteAllNewsList();
  void restoreNews();

  bool openUrl(const QUrl &url);
  void openInBrowserNews();
  void openInExternalBrowserNews();
  void openNewsNewTab();

  void updateWebView(QModelIndex index);

  void hideWebContent();

  void reduceNewsList();
  void increaseNewsList();

  int findUnreadNews(bool next);

  void setTextTab(const QString &text, int width = 114);

  void slotShareNews(QAction *action);

  int type_;
  int feedId_;
  int feedParId_;
  int currentNewsIdOld;
  bool autoLoadImages_;
  int labelId_;
  QString categoryFilterStr_;

  FindTextContent *findText_;

  NewsModel *newsModel_;
  NewsView *newsView_;
  NewsHeader *newsHeader_;
  QToolBar *newsToolBar_;
  QSplitter *newsTabWidgetSplitter_;

  WebView *webView_;
  QWidget *webControlPanel_;

  QLabel *newsIconTitle_;
  QMovie *newsIconMovie_;
  QLabel *newsTextTitle_;
  QWidget *newsTitleLabel_;
  QToolButton *closeButton_;

  QAction *separatorRAct_;

signals:
  void signalSetHtmlWebView(const QString &html = "", const QUrl &baseUrl = QUrl());
  void signalSetTextTab(const QString &text, NewsTabWidget *widget);
  void loadProgress(int);

public slots:
  void slotNewsViewClicked(QModelIndex index);
  void slotNewsViewSelected(QModelIndex index, bool clicked = false);
  void slotNewsViewDoubleClicked(QModelIndex index);
  void slotNewsMiddleClicked(QModelIndex index);
  void slotNewsUpPressed();
  void slotNewsDownPressed();
  void slotNewsHomePressed();
  void slotNewsEndPressed();
  void slotNewsPageUpPressed();
  void slotNewsPageDownPressed();
  void slotSort(int column, int order);

private slots:
  void showContextMenuNews(const QPoint &p);
  void slotSetItemRead(QModelIndex index, int read);
  void slotSetItemStar(QModelIndex index, int starred);
  void slotReadTimer();

  void setHtmlWebView(const QString &html, const QUrl &baseUrl);
  void webHomePage();
  void openPageInExternalBrowser();
  void slotLinkClicked(QUrl url);
  void slotLinkHovered(const QString &link, const QString &str1="", const QString &str2="");
  void slotSetValue(int value);
  void slotLoadStarted();
  void slotLoadFinished(bool);
  void showContextWebPage(const QPoint &p);
  void openUrlInExternalBrowser();

  void slotTabClose();
  void webTitleChanged(QString title);
  void openLink();
  void openLinkInNewTab();

  void slotFindText(const QString& text);
  void slotSelectFind();

  void setWebToolbarVisible(bool show = true, bool checked = true);

  void slotNewslLabelClicked(QModelIndex index);

};

#endif // NEWSTABWIDGET_H
