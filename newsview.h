#ifndef NEWSVIEW_H
#define NEWSVIEW_H

#include <QtGui>
#include "newsmodel.h"

class NewsView : public QTreeView
{
  Q_OBJECT
public:
  NewsView(QWidget * parent = 0);
  NewsModel *model_;

protected:
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void mouseDoubleClickEvent(QMouseEvent*);
  virtual void currentChanged(const QModelIndex & current, const QModelIndex & previous);

signals:
  void signalSetItemRead(QModelIndex index, int read);
  void signalCurrentChanged(const QModelIndex & current, const QModelIndex & previous);
  void doubleClicked(QModelIndex index);
};

#endif // NEWSVIEW_H
