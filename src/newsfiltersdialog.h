#ifndef NEWSFILTERSDIALOG_H
#define NEWSFILTERSDIALOG_H

class NewsFiltersDialog : public QDialog
{
  Q_OBJECT
private:
  QSettings *settings_;
  QTreeWidget *filtersTree;

  QPushButton *editButton;
  QPushButton *deleteButton;
  QPushButton *moveUpButton;
  QPushButton *moveDownButton;

public:
  explicit NewsFiltersDialog(QWidget *parent = 0, QSettings *settings = 0);
  QStringList feedsList_;

signals:

public slots:

private slots:
  void closeDialog();
  void newFilter();
  void editFilter();
  void deleteFilter();

  void moveUpFilter();
  void moveDownFilter();

  void slotCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem * previous);

};

#endif // NEWSFILTERSDIALOG_H
