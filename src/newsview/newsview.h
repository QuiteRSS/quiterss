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
#ifndef NEWSVIEW_H
#define NEWSVIEW_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include "newsmodel.h"

class NewsView : public QTreeView
{
  Q_OBJECT
public:
  NewsView(QWidget * parent = 0);

signals:
  void signalSetItemRead(QModelIndex index, int read);
  void signalSetItemStar(QModelIndex index, int starred);
  void signalDoubleClicked(QModelIndex index);
  void signalMiddleClicked(QModelIndex index);
  void signaNewslLabelClicked(QModelIndex index);
  void pressKeyUp(const QModelIndex &index);
  void pressKeyDown(const QModelIndex &index);
  void pressKeyHome(const QModelIndex &index);
  void pressKeyEnd(const QModelIndex &index);
  void pressKeyPageUp(const QModelIndex &index);
  void pressKeyPageDown(const QModelIndex &index);

protected:
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void mouseDoubleClickEvent(QMouseEvent*);
  virtual void keyPressEvent(QKeyEvent*);

private:
  QModelIndex indexClicked_;

};

#endif // NEWSVIEW_H
