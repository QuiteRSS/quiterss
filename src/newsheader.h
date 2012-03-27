#ifndef NEWSHEADER_H
#define NEWSHEADER_H

#include "newsmodel.h"

class NewsHeader : public QHeaderView
{
  Q_OBJECT
private:
  QTreeView *view_;
  NewsModel *model_;

  QMenu *viewMenu_;
  QActionGroup *pActGroup_;
  QPushButton *buttonColumnView;
  int idxCol;
  int posX1;

public:
  NewsHeader(Qt::Orientation orientation, QWidget * parent = 0,
             NewsModel *model = 0);

  void initColumns();
  void createMenu();
  void overload();
  void retranslateStrings();

public slots:

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
