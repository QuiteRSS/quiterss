#ifndef FEEDSTREEVIEW_H
#define FEEDSTREEVIEW_H

#include <QtGui>

#include <qyursqltreeview.h>

class FeedsTreeView : public QyurSqlTreeView
{
  Q_OBJECT
public:
  FeedsTreeView(QWidget * parent = 0);
  QModelIndex selectIndex_;

  QModelIndex indexNextUnread(const QModelIndex &indexCur, int next = 0);
  QModelIndex indexPrevious(const QModelIndex &indexCur);
  QModelIndex indexNext(const QModelIndex &indexCur);

public slots:
  void setSelectIndex();
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
  void signalDoubleClicked(QModelIndex index);
  void signalMiddleClicked();
  void pressKeyUp();
  void pressKeyDown();
  void pressKeyHome();
  void pressKeyEnd();
  void signalDropped(QModelIndex &what, QModelIndex &where);

private:
  QPoint dragPos_;
  QPoint dragStartPos_;

  void handleDrop(QDropEvent *e);
  bool shouldAutoScroll(const QPoint &pos) const;

private slots:
  void slotExpanded(const QModelIndex&index);
  void slotCollapsed(const QModelIndex&index);
};

#endif // FEEDSTREEVIEW_H
