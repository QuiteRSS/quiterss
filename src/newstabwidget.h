/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2021 QuiteRSS Team <quiterssteam@gmail.com>
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

#include "feedsproxymodel.h"
#include "feedsmodel.h"
#include "feedsview.h"
#include "findtext.h"
#include "lineedit.h"
#include "newsheader.h"
#include "newsmodel.h"
#include "newsview.h"

class MainWindow;

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
    TabTypeDownloads
  };


  explicit NewsTabWidget(QWidget *parent, TabType type, int feedId = -1, int feedParId = -1);
  ~NewsTabWidget();

  void disconnectObjects();

  void retranslateStrings();
  void setSettings(bool init = true, bool newTab = true);
  void markNewsRead();
  void markAllNewsRead();
  void markNewsStar();
  void setLabelNews(int labelId);
  void deleteNews();
  void deleteAllNewsList();
  void restoreNews();
  void slotCopyLinkNews();
  void showLabelsMenu();

  bool openUrl(const QUrl &url);
  void openInBrowserNews();
  void openInExternalBrowserNews();
  void openNewsNewTab();

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
  int labelId_;
  QString categoryFilterStr_;

  FindTextContent *findText_;

  NewsModel *newsModel_;
  NewsView *newsView_;
  NewsHeader *newsHeader_;
  QToolBar *newsToolBar_;
  QSplitter *newsTabWidgetSplitter_;

  QWidget *newsWidget_;

  QLabel *newsIconTitle_;
  QMovie *newsIconMovie_;
  QLabel *newsTextTitle_;
  QWidget *newsTitleLabel_;
  QToolButton *closeButton_;

  QAction *separatorRAct_;

public slots:
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
  void signalSetTextTab(const QString &text, NewsTabWidget *widget);
  void loadProgress(int);

private slots:
  void showContextMenuNews(const QPoint &pos);
  void slotSetItemRead(QModelIndex index, int read);
  void slotSetItemStar(QModelIndex index, int starred);
  void slotMarkReadTimeout();

  void slotTabClose();

  void slotNewslLabelClicked(QModelIndex index);

private:
  void createNewsList();

  MainWindow *mainWindow_;
  QSqlDatabase db_;

  FeedsModel *feedsModel_;
  FeedsProxyModel *feedsProxyModel_;
  FeedsView *feedsView_;

  QTimer *markNewsReadTimer_;

  QWidget *newsPanelWidget_;

};

#endif // NEWSTABWIDGET_H
