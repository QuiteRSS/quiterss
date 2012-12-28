#ifndef NEWSLABELDIALOG_H
#define NEWSLABELDIALOG_H

#include <QtGui>
#include "lineedit.h"

class LabelDialog : public QDialog
{
  Q_OBJECT
private:
  QToolButton *iconButton_;
  QToolButton *colorTextButton_;
  QToolButton *colorBgButton_;
  QDialogButtonBox *buttonBox_;

private slots:
  void nameEditChanged(const QString&);
  void selectIcon(QAction *action);

public:
  explicit LabelDialog(QWidget *parent);

  LineEdit *nameEdit_;

};

#endif // NEWSLABELDIALOG_H
