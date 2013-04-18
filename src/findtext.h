#ifndef FINDTEXT_H
#define FINDTEXT_H

#include <QtGui>

class FindTextContent : public QLineEdit
{
  Q_OBJECT

public:
  FindTextContent(QWidget *parent = 0);
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
  QAction *findInNewsAct_;
  QAction *findTitleAct_;
  QAction *findAuthorAct_;
  QAction *findCategoryAct_;
  QAction *findContentAct_;
  QAction *findInBrowserAct_;

  QLabel *findLabel_;
  QToolButton *findButton;
  QToolButton *clearButton;

signals:
  void signalClear();
  void signalSelectFind();
};

#endif // FINDTEXT_H
