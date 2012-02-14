#ifndef NEWSFILTERSDIALOG_H
#define NEWSFILTERSDIALOG_H

#include <QtGui>

class NewsFiltersDialog : public QDialog
{
  Q_OBJECT
private:
  QDialogButtonBox *buttonBox_;

public:
  explicit NewsFiltersDialog(QWidget *parent = 0);

  QLineEdit *feedTitleEdit_;
  QLineEdit *feedUrlEdit_;

signals:

public slots:

};

#endif // NEWSFILTERSDIALOG_H
