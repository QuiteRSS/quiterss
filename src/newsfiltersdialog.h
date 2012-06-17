#ifndef NEWSFILTERSDIALOG_H
#define NEWSFILTERSDIALOG_H

#include <QtGui>

class NewsFiltersDialog : public QDialog
{
  Q_OBJECT
private:
  QSettings *settings_;

  QPushButton *editButton;
  QPushButton *deleteButton;
  QPushButton *moveUpButton;
  QPushButton *moveDownButton;
  QPushButton *applyFilterButton;

public:
  explicit NewsFiltersDialog(QWidget *parent = 0, QSettings *settings = 0);
  QTreeWidget *filtersTree;

private slots:
  void closeDialog();
  void newFilter();
  void editFilter();
  void deleteFilter();

  void moveUpFilter();
  void moveDownFilter();

  void slotCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *);

  void applyFilter();

};

#endif // NEWSFILTERSDIALOG_H
