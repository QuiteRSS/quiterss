#ifndef FEEDSVIEW_H
#define FEEDSVIEW_H

class FeedsView : public QTreeView
{
  Q_OBJECT
public:
  FeedsView(QWidget * parent = 0);

protected:
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void keyPressEvent(QKeyEvent*);

signals:
  void pressKeyUp();
  void pressKeyDown();

};

#endif // FEEDSVIEW_H
