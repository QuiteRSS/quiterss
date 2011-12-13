#ifndef NEWSHEADER_H
#define NEWSHEADER_H

#include <QtGui>

class NewsHeader : public QHeaderView
{
  Q_OBJECT
public:
  NewsHeader(Qt::Orientation orientation, QWidget * parent = 0);
  void init();
protected:
  bool eventFilter(QObject *, QEvent *);
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseMoveEvent (QMouseEvent*);

private slots:
  void slotButtonColumnView();

signals:


private:
  QMenu *viewMenu_;
  QPushButton *buttonColumnView;
  int startColFix;
  int stopColFix;
  int idxCol;
  int posX;

};

#endif // NEWSHEADER_H
