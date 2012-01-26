#ifndef FEEDSVIEW_H
#define FEEDSVIEW_H

#include <QtGui>

class FeedsView : public QTreeView
{
  Q_OBJECT
public:
  FeedsView(QWidget * parent = 0);

protected:
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);

};

#endif // FEEDSVIEW_H
