#ifndef FEEDSVIEW_H
#define FEEDSVIEW_H

#include <QtGui>

class FeedsView : public QTreeView
{
  Q_OBJECT
public:
  FeedsView(QWidget * parent = 0);
  QModelIndex selectIndex;

public slots:
  void setSelectIndex();

protected:
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void mouseDoubleClickEvent(QMouseEvent*);
  virtual void keyPressEvent(QKeyEvent*);
  virtual void currentChanged(const QModelIndex &current,
                              const QModelIndex &previous);

signals:
  void signalDoubleClicked(QModelIndex index);
  void signalMiddleClicked();
  void pressKeyUp();
  void pressKeyDown();

};

#endif // FEEDSVIEW_H
