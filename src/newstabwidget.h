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

#define TOP_POSITION    0
#define BOTTOM_POSITION 1
#define RIGHT_POSITION  2
#define LEFT_POSITION   3

#define TAB_FEED       0
#define TAB_WEB        1
#define TAB_CAT_DEL    2
#define TAB_CAT_STAR   3
#define TAB_CAT_LABEL  4

class NewsTabWidget : public QWidget
{
  Q_OBJECT
private:
  void createNewsList();
  void createMenuNews();
  void createWebWidget();
  bool openUrl(const QUrl &url);
  void setTitleWebPanel();

  QSqlDatabase *db_;
  QWidget *newsWidget_;
  QMenu *newsContextMenu_;

  FeedsTreeModel *feedsTreeModel_;
  FeedsTreeView *feedsTreeView_;

  QFrame *lineWebWidget;
  QWidget *webWidget_;
  QLabel *webPanelTitle_;
//  QLabel *webPanelTitleLabel_;
  QLabel *webPanelDate_;
//  QLabel *webPanelAuthorLabel_;
  QLabel *webPanelAuthor_;
  QProgressBar *webViewProgress_;
  QLabel *webViewProgressLabel_;
  QString titleString_;
  QString linkString_;

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

  bool pageLoaded_;

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

  void openInBrowserNews();
  void openInExternalBrowserNews();
  void openNewsNewTab();

  void updateWebView(QModelIndex index);

  void hideWebContent();

  void setVisibleAction(bool show);

  int type_;
  int feedId_;
  int feedParId_;
  int currentNewsIdOld;
  bool autoLoadImages_;

  FindTextContent *findText_;

  NewsModel *newsModel_;
  NewsView *newsView_;
  NewsHeader *newsHeader_;
  QToolBar *newsToolBar_;
  QSplitter *newsTabWidgetSplitter_;

  WebView *webView_;
  QWidget *webPanel_;
  QWidget *webControlPanel_;

  QLabel *newsIconTitle_;
  QMovie *newsIconMovie_;
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
  void slotNewsPageUpPressed();
  void slotNewsPageDownPressed();
  void slotSort(int column, int order);

protected:
  bool eventFilter(QObject *obj, QEvent *event);
  void resizeEvent(QResizeEvent *);

private slots:
  void showContextMenuNews(const QPoint &p);
  void slotSetItemRead(QModelIndex index, int read);
  void slotSetItemStar(QModelIndex index, int starred);
  void slotReadTimer();

  void slotWebViewSetContent(QString content);
  void slotWebTitleLinkClicked(QString urlStr);
  void webHomePage();
  void openPageInExternalBrowser();
  void slotLinkClicked(QUrl url);
  void slotLinkHovered(const QString &link, const QString &, const QString &);
  void slotSetValue(int value);
  void slotLoadStarted();
  void slotLoadFinished(bool);
  void showContextWebPage(const QPoint &p);
  void openUrlInExternalBrowser();

  void slotTabClose();
  void webTitleChanged(QString title);
  void openLinkInNewTab();

  void slotFindText(const QString& text);
  void slotSelectFind();

  void setWebToolbarVisible(bool show = true, bool checked = true);

};

#endif // NEWSTABWIDGET_H
