#ifndef NEWSFILTERSDIALOG_H
#define NEWSFILTERSDIALOG_H

#include <QtGui>

class NewsFiltersDialog : public QDialog
{
  Q_OBJECT
private:
  QDialogButtonBox *buttonBox_;
  QTreeWidget *filtersTree;

  QPushButton *editButton;
  QPushButton *deleteButton;
  QPushButton *moveUpButton;
  QPushButton *moveDownButton;

public:
  explicit NewsFiltersDialog(QWidget *parent = 0);

  QLineEdit *feedTitleEdit_;
  QLineEdit *feedUrlEdit_;

signals:

public slots:

private slots:
  void newFilter();
  void editeFilter();
  void deleteFilter();

  void moveUpFilter();
  void moveDownFilter();

  void slotCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem * previous);

};

#endif // NEWSFILTERSDIALOG_H
