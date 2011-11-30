#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QtGui>

class OptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OptionsDialog(QWidget *parent = 0);
public slots:
  void slotCategoriesItemCLicked(QTreeWidgetItem* item, int column);

signals:

private:
  QDialogButtonBox *buttonBox_;

};

#endif // OPTIONSDIALOG_H
