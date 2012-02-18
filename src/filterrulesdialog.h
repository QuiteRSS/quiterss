#ifndef FILTERRULES_H
#define FILTERRULES_H

#include <QtGui>

class FilterRulesDialog : public QDialog
{
  Q_OBJECT
private:
  QSettings *settings_;
  QDialogButtonBox *buttonBox;
  QScrollArea *infoScrollArea;
  QVBoxLayout *infoLayout;
  QWidget *infoWidget;

public:
  explicit FilterRulesDialog(QWidget *parent = 0, QSettings *settings = 0);

signals:

public slots:

private slots:
  void closeDialog();
  void addFilterRules();
  void deleteFilterRules();

};

#endif // FILTERRULES_H
