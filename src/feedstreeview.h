#ifndef FEEDSTREEVIEW_H
#define FEEDSTREEVIEW_H

#include <QtGui>

#include <qyursqltreeview.h>

class FeedsTreeView : public QyurSqlTreeView
{
  Q_OBJECT
public:
  FeedsTreeView(QWidget * parent = 0);
  QModelIndex selectIndex;

public slots:
  void setSelectIndex();
  void updateCurrentIndex(const QModelIndex &index);

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
  void pressKeyHome();
  void pressKeyEnd();

};

#endif // FEEDSTREEVIEW_H
