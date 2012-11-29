#ifndef ADDFOLDERDIALOG_H
#define ADDFOLDERDIALOG_H

#include <QtGui>
#include <QtSql>
#include "lineedit.h"

class AddFolderDialog : public QDialog
{
  Q_OBJECT
private:
  QSqlDatabase *db_;
  QDialogButtonBox *buttonBox_;

private slots:
  void nameFeedEditChanged(const QString&);

public:
  explicit AddFolderDialog(QWidget *parent, QSqlDatabase *db);

  LineEdit *nameFeedEdit_;
  QTreeWidget *foldersTree_;

};

#endif // ADDFOLDERDIALOG_H
