#ifndef AUTHENTICATIONDIALOG_H
#define AUTHENTICATIONDIALOG_H

#include <QNetworkReply>
#include <QAuthenticator>
#include "dialog.h"
#include "lineedit.h"

class AuthenticationDialog : public Dialog
{
  Q_OBJECT

public:
  explicit AuthenticationDialog(QWidget *parent, QNetworkReply *reply,
                                QAuthenticator *auth);

private slots:
  void acceptDialog();

private:
  QAuthenticator *auth_;

  LineEdit *user_;
  LineEdit *pass_;
  QCheckBox* save_;

};

#endif // AUTHENTICATIONDIALOG_H
