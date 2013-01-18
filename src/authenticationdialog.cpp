#include "authenticationdialog.h"

AuthenticationDialog::AuthenticationDialog(QWidget *parent, QNetworkReply *reply,
                                           QAuthenticator *auth)
  : Dialog(parent, Qt::MSWindowsFixedSizeDialogHint),
    auth_(auth)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Authorization required"));
  setMinimumWidth(400);

  user_ = new LineEdit(this);
  pass_ = new LineEdit(this);
  pass_->setEchoMode(QLineEdit::Password);
  save_ = new QCheckBox(tr("Save password"), this);

  QGridLayout *serverLayout = new QGridLayout();
  serverLayout->addWidget(new QLabel(tr("Server:")), 0, 0);
  serverLayout->addWidget(new QLabel(reply->url().host()), 0, 1);
  serverLayout->addWidget(new QLabel(tr("Massage:")), 1, 0);
  serverLayout->addWidget(new QLabel(auth->realm()), 1, 1);
  serverLayout->setColumnStretch(1, 1);

  QGridLayout *userLayout = new QGridLayout();
  userLayout->addWidget(new QLabel(tr("Username:")), 0, 0);
  userLayout->addWidget(user_, 0, 1);
  userLayout->addWidget(new QLabel(tr("Password:")), 1, 0);
  userLayout->addWidget(pass_, 1, 1);

  pageLayout->addLayout(serverLayout);
  pageLayout->addSpacing(10);
  pageLayout->addLayout(userLayout);
  pageLayout->addSpacing(10);
  pageLayout->addWidget(save_);

  buttonBox->addButton(QDialogButtonBox::Ok);
  buttonBox->addButton(QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptDialog()));
}

void AuthenticationDialog::acceptDialog()
{
  auth_->setUser(user_->text());
  auth_->setPassword(pass_->text());
  accept();
}
