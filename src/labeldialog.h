#ifndef NEWSLABELDIALOG_H
#define NEWSLABELDIALOG_H

#include "dialog.h"
#include "lineedit.h"

class LabelDialog : public Dialog
{
  Q_OBJECT
private:
  QToolButton *iconButton_;
  QToolButton *colorTextButton_;
  QToolButton *colorBgButton_;

private slots:
  void nameEditChanged(const QString&);
  void selectIcon(QAction *action);
  void loadIcon();
  void defaultColorText();
  void selectColorText();
  void defaultColorBg();
  void selectColorBg();

public:
  explicit LabelDialog(QWidget *parent);

  void loadData();

  LineEdit *nameEdit_;
  QIcon icon_;
  QString colorTextStr_;
  QString colorBgStr_;
};

#endif // NEWSLABELDIALOG_H
