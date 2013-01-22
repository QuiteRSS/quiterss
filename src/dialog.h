#ifndef DIALOG_H
#define DIALOG_H

#include <QtGui>

class Dialog : public QDialog
{
  Q_OBJECT
public:
  explicit Dialog(QWidget *parent, Qt::WindowFlags f = 0);

  QVBoxLayout *mainLayout;
  QVBoxLayout *pageLayout;
  QHBoxLayout *buttonsLayout;

  QDialogButtonBox *buttonBox;

};

#endif // DIALOG_H
