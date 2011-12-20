#ifndef NEWSHEADER_H
#define NEWSHEADER_H

#include <QtGui>
#include "newsmodel.h"

class NewsHeader : public QHeaderView
{
  Q_OBJECT
public:
  NewsHeader(Qt::Orientation orientation, QWidget * parent = 0);
  NewsModel *model_;
  QTreeView *view_;
  void initColumns();
  void createMenu();
  void overload();

public slots:

protected:
  bool eventFilter(QObject *, QEvent *);
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);

private slots:
  void slotButtonColumnView();
  void columnVisible(QAction*);
  void slotSectionMoved(int, int, int);

signals:

private:
  QMenu *viewMenu_;
  QPushButton *buttonColumnView;
  int idxCol;
  int posX1;

};

#endif // NEWSHEADER_H
