#ifndef ADDFEEDDIALOG_H
#define ADDFEEDDIALOG_H

#include <QtGui>
#include "lineedit.h"

class AddFeedDialog : public QDialog
{
  Q_OBJECT
private:
  QDialogButtonBox *buttonBox_;

public:
  explicit AddFeedDialog(QWidget *parent = 0);

  LineEdit *feedTitleEdit_;
  LineEdit *feedUrlEdit_;

signals:

public slots:

};

#endif // ADDFEEDDIALOG_H
