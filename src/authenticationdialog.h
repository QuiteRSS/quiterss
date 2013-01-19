#ifndef AUTHENTICATIONDIALOG_H
#define AUTHENTICATIONDIALOG_H

#include <QAuthenticator>
#include <QNetworkReply>
#include "dialog.h"
#include "lineedit.h"

class AuthenticationDialog : public Dialog
{
  Q_OBJECT

public:
  explicit AuthenticationDialog(QWidget *parent, QNetworkReply *reply,
                                QAuthenticator *auth);
  QCheckBox* save_;

private slots:
  void acceptDialog();

private:
  QAuthenticator *auth_;

  QString server_;
  LineEdit *user_;
  LineEdit *pass_;

};

#endif // AUTHENTICATIONDIALOG_H
