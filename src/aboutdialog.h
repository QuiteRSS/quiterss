#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include "dialog.h"

class AboutDialog : public Dialog
{
  Q_OBJECT
public:
  explicit AboutDialog(const QString &lang, QWidget *parent = 0);
};

#endif // ABOUTDIALOG_H
