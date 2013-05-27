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
#ifndef NEWSHEADER_H
#define NEWSHEADER_H

#include <QtGui>

#include "newsmodel.h"

class RSSListing;
class NewsTabWidget;

class NewsHeader : public QHeaderView
{
  Q_OBJECT

public:
  NewsHeader(NewsModel *model, QWidget *parent);

  void init();
  void retranslateStrings();
  void setColumns(RSSListing *rssl, const QModelIndex &indexFeed = QModelIndex());
  void saveStateColumns(RSSListing *rssl, NewsTabWidget *newsTabWidget);

  QMenu *viewMenu_;

protected:
  bool eventFilter(QObject *, QEvent *);
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void mouseDoubleClickEvent(QMouseEvent*);

private slots:
  void slotButtonColumnView();
  void slotColumnVisible(QAction*);
  void slotSectionMoved(int, int, int);

private:
  void createMenu();

  /**
   * @brief Adjust width for all columns
   * @param newWidth Widget's width for adjustment
   */
  void adjustAllColumnsWidths(int newWidth);
  QString columnsList();

  NewsModel *model_;
  QActionGroup *columnVisibleActGroup_;
  QPushButton *buttonColumnView_;
  bool show_;
  bool move_;
  int idxCol_;
  int posX_;

};

#endif // NEWSHEADER_H
