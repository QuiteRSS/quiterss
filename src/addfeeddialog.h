#ifndef ADDFEEDDIALOG_H
#define ADDFEEDDIALOG_H

#include <QtGui>

class AddFeedDialog : public QDialog
{
  Q_OBJECT
private:
  QDialogButtonBox *buttonBox_;

public:
  explicit AddFeedDialog(QWidget *parent = 0);

  QLineEdit *feedTitleEdit_;
  QLineEdit *feedUrlEdit_;

signals:

public slots:

};

#endif // ADDFEEDDIALOG_H
