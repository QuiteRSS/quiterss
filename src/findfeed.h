#ifndef FINDFEED_H
#define FINDFEED_H

#include <QtGui>

class FindFeed : public QLineEdit
{
  Q_OBJECT

public:
  FindFeed(QWidget *parent = 0);
  void retranslateStrings();

  QActionGroup *findGroup_;

protected:
  void resizeEvent(QResizeEvent *);
  void focusInEvent(QFocusEvent *event);
  void focusOutEvent(QFocusEvent *event);

private slots:
  void updateClearButton(const QString &text);
  void slotClear();
  void slotMenuFind();
  void slotSelectFind(QAction *act);

private:
  QMenu *findMenu_;
  QAction *findNameAct_;
  QAction *findLinkAct_;

  QLabel *findLabel_;
  QToolButton *findButton;
  QToolButton *clearButton;

signals:
  void signalClear();
  void signalSelectFind();
};

#endif // FINDFEED_H
