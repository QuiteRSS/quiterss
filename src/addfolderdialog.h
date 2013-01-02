#ifndef ADDFOLDERDIALOG_H
#define ADDFOLDERDIALOG_H

#include <QtSql>
#include "dialog.h"
#include "lineedit.h"

class AddFolderDialog : public Dialog
{
  Q_OBJECT
private:
  QSqlDatabase *db_;

private slots:
  void nameFeedEditChanged(const QString&);

public:
  explicit AddFolderDialog(QWidget *parent, QSqlDatabase *db);

  LineEdit *nameFeedEdit_;
  QTreeWidget *foldersTree_;

};

#endif // ADDFOLDERDIALOG_H
