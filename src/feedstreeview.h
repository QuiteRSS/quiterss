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
#ifndef FEEDSTREEVIEW_H
#define FEEDSTREEVIEW_H

#include <QtGui>

#include <qyursqltreeview.h>

class FeedsTreeView : public QyurSqlTreeView
{
  Q_OBJECT
public:
  FeedsTreeView(QWidget * parent = 0);
  int selectId_;
  int selectParentId_;
  bool selectIdEn_;
  bool autocollapseFolder_;

  QModelIndex indexNextUnread(const QModelIndex &indexCur, int next = 0);
  QModelIndex firstFeedInFolder(const QModelIndex &indexFolder);
  QModelIndex lastFeedInFolder(const QModelIndex &indexFolder);
  QModelIndex indexPrevious(const QModelIndex &indexCur, bool isParent = false);
  QModelIndex indexNext(const QModelIndex &indexCur, bool isParent = false);

public slots:
  QPersistentModelIndex selectIndex();
  void updateCurrentIndex(const QModelIndex &index);

protected:
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseReleaseEvent(QMouseEvent*event);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void mouseDoubleClickEvent(QMouseEvent*);
  virtual void keyPressEvent(QKeyEvent*);
  virtual void currentChanged(const QModelIndex &current,
                              const QModelIndex &previous);
  void dragEnterEvent(QDragEnterEvent *event);
  void dragLeaveEvent(QDragLeaveEvent *event);
  void dragMoveEvent(QDragMoveEvent *event);
  void dropEvent(QDropEvent *event);
  void paintEvent(QPaintEvent *event);

signals:
  void signalDoubleClicked();
  void signalMiddleClicked();
  void pressKeyUp();
  void pressKeyDown();
  void pressKeyHome();
  void pressKeyEnd();
  void signalDropped(QModelIndex &what, QModelIndex &where, int how);

private:
  QPoint dragPos_;
  QPoint dragStartPos_;

  void handleDrop(QDropEvent *e);
  bool shouldAutoScroll(const QPoint &pos) const;

  int selectOldId_;
  int selectOldParentId_;

private slots:
  void slotExpanded(const QModelIndex&index);
  void slotCollapsed(const QModelIndex&index);
};

#endif // FEEDSTREEVIEW_H
