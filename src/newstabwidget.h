/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
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
#ifndef NEWSTABWIDGET_H
#define NEWSTABWIDGET_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QtSql>
#include <QtWebKit>

#include "feedsproxymodel.h"
#include "feedsmodel.h"
#include "feedsview.h"
#include "findtext.h"
#include "lineedit.h"
#include "locationbar.h"
#include "newsheader.h"
#include "newsmodel.h"
#include "newsview.h"
#include "webview.h"

class MainWindow;

#define TOP_POSITION    0
#define BOTTOM_POSITION 1
#define RIGHT_POSITION  2
#define LEFT_POSITION   3

#define RESIZESTEP 25   // News list/browser size step

class NewsTabWidget : public QWidget
{
  Q_OBJECT
public:
  enum TabType {
    TabTypeFeed,
    TabTypeUnread,
    TabTypeStar,
    TabTypeDel,
    TabTypeLabel,
    TabTypeWeb,
    TabTypeDownloads
  };

  enum RefreshNewspaper {
    RefreshAll,
    RefreshInsert,
    RefreshWithPos
  };

  explicit NewsTabWidget(QWidget *parent, TabType type, int feedId = -1, int feedParId = -1);
  ~NewsTabWidget();

  void disconnectObjects();

  void retranslateStrings();
  void setSettings(bool init = true, bool newTab = true);
  void setNewsLayout();
  void setBrowserPosition();
  void markNewsRead();
  void markAllNewsRead();
  void markNewsStar();
  void setLabelNews(int labelId);
  void deleteNews();
  void deleteAllNewsList();
  void restoreNews();
  void slotCopyLinkNews();
  void showLabelsMenu();
  void savePageAsDescript();

  bool openUrl(const QUrl &url);
  void openInBrowserNews();
  void openInExternalBrowserNews();
  void openNewsNewTab();

  void updateWebView(QModelIndex index);
  void loadNewspaper(int refresh = RefreshAll);
  void hideWebContent();
  QString getLinkNews(int row);

  void reduceNewsList();
  void increaseNewsList();

  int findUnreadNews(bool next);

  void setTextTab(const QString &text);

  void slotShareNews(QAction *action);

  /*! \brief Convert \a countString to unreadCount depending on \a type_
   * \param countString from categories tree
   * \return unreadCount for displaying in status
   */
  int getUnreadCount(QString countString);

  TabType type_;
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
  QToolBar *webToolBar_;
  LocationBar *locationBar_;
  QWidget *webControlPanel_;

  QLabel *newsIconTitle_;
  QMovie *newsIconMovie_;
  QLabel *newsTextTitle_;
  QWidget *newsTitleLabel_;
  QToolButton *closeButton_;

  QAction *separatorRAct_;

public slots:
  void setAutoLoadImages(bool apply = true);
  void slotNewsViewClicked(QModelIndex index);
  void slotNewsViewSelected(QModelIndex index, bool clicked=false);
  void slotNewsViewDoubleClicked(QModelIndex index);
  void slotNewsMiddleClicked(QModelIndex index);
  void slotNewsUpPressed(QModelIndex index=QModelIndex());
  void slotNewsDownPressed(QModelIndex index=QModelIndex());
  void slotNewsHomePressed(QModelIndex index=QModelIndex());
  void slotNewsEndPressed(QModelIndex index=QModelIndex());
  void slotNewsPageUpPressed(QModelIndex index=QModelIndex());
  void slotNewsPageDownPressed(QModelIndex index=QModelIndex());
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

  void slotSetHtmlWebView(const QString &html, const QUrl &baseUrl);
  void webHomePage();
  void openPageInExternalBrowser();
  void slotLinkClicked(QUrl url);
  void slotLinkHovered(const QString &link, const QString &str1="", const QString &str2="");
  void slotSetValue(int value);
  void slotLoadStarted();
  void slotLoadFinished(bool);
  void slotUrlEnter();
  void slotUrlChanged(const QUrl &url);
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
  QString getHtmlLabels(int row);
  void actionNewspaper(QUrl url);

  MainWindow *mainWindow_;
  QSqlDatabase db_;
  QWidget *newsWidget_;

  FeedsModel *feedsModel_;
  FeedsProxyModel *feedsProxyModel_;
  FeedsView *feedsView_;

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

  QUrl linkUrl_;
  QString linkNewsString_;

  QWidget *newsPanelWidget_;
  bool webToolbarShow_;

  QString newspaperHeadHtml_;
  QString newspaperHtml_;
  QString newspaperHtmlRtl_;
  QString htmlString_;
  QString htmlRtlString_;
  QString cssString_;
  QString audioPlayerHtml_;
  QString videoPlayerHtml_;

};

#endif // NEWSTABWIDGET_H
