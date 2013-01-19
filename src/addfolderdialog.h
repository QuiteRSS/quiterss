#ifndef ADDFOLDERDIALOG_H
#define ADDFOLDERDIALOG_H

#include <QtSql>
#include "dialog.h"
#include "lineedit.h"

class AddFolderDialog : public Dialog
{
  Q_OBJECT

public:
  explicit AddFolderDialog(QWidget *parent);

  LineEdit *nameFeedEdit_;
  QTreeWidget *foldersTree_;

private slots:
  void nameFeedEditChanged(const QString&);

};

#endif // ADDFOLDERDIALOG_H
