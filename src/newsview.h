#ifndef NEWSVIEW_H
#define NEWSVIEW_H

#include <QtGui>
#include "newsmodel.h"

class NewsView : public QTreeView
{
  Q_OBJECT

public:
  NewsView(QWidget * parent = 0);

protected:
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void mouseDoubleClickEvent(QMouseEvent*);
  virtual void keyPressEvent(QKeyEvent*);

signals:
  void signalSetItemRead(QModelIndex index, int read);
  void signalSetItemStar(QModelIndex index, int starred);
  void signalDoubleClicked(QModelIndex index);
  void pressKeyUp();
  void pressKeyDown();

};

#endif // NEWSVIEW_H
