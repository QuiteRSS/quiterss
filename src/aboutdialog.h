#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QtGui>

class AboutDialog : public QDialog
{
  Q_OBJECT
public:
  explicit AboutDialog(const QString &lang, QWidget *parent = 0);
};

#endif // ABOUTDIALOG_H
