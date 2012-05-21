#ifndef NEWSHEADER_H
#define NEWSHEADER_H

#include <QtGui>
#include "newsmodel.h"

class NewsHeader : public QHeaderView
{
  Q_OBJECT
private:
  void createMenu();

  QTreeView *view_;
  NewsModel *model_;

  QMenu *viewMenu_;
  QActionGroup *pActGroup_;
  QPushButton *buttonColumnView;
  int idxCol;
  int posX1;

public:
  NewsHeader(NewsModel *model, QWidget *parent);

  void init(QSettings *settings);
  void retranslateStrings();

protected:
  bool eventFilter(QObject *, QEvent *);
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);

private slots:
  void slotButtonColumnView();
  void columnVisible(QAction*);
  void slotSectionMoved(int, int, int);

};

#endif // NEWSHEADER_H
