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
#define TAB_CAT_UNREAD 1
#define TAB_CAT_STAR   2
#define TAB_CAT_DEL    3
#define TAB_CAT_LABEL  4
#define TAB_WEB        5
#define TAB_DOWNLOADS  6

#define RESIZESTEP 25   // News list/browser size step

class NewsTabWidget : public QWidget
{
  Q_OBJECT
public:
  explicit NewsTabWidget(QWidget *parent, int type, int feedId = -1, int feedParId = -1);
  ~NewsTabWidget();

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
  void slotCopyLinkNews();

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

  QString linkColor_;
  QString titleColor_;
  QString dateColor_;
  QString authorColor_;
  QString newsTitleBackgroundColor_;
  QString newsBackgroundColor_;

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

signals:
  void signalSetHtmlWebView(const QString &html = "", const QUrl &baseUrl = QUrl());
  void signalSetTextTab(const QString &text, NewsTabWidget *widget);
  void loadProgress(int);

private slots:
  void showContextMenuNews(const QPoint &pos);
  void slotSetItemRead(QModelIndex index, int read);
  void slotSetItemStar(QModelIndex index, int starred);
  void slotMarkReadTimeout();

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

private:
  void createNewsList();
  void createWebWidget();

  RSSListing *rsslisting_;
  QSqlDatabase db_;
  QWidget *newsWidget_;

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

  QString htmlString_;
  QString cssString_;

};

#endif // NEWSTABWIDGET_H
