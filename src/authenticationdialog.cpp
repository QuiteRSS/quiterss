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

  QGridLayout *layout = new QGridLayout();
  layout->addWidget(new QLabel(tr("Server:")), 0, 0);
  layout->addWidget(new QLabel(reply->url().host()), 0, 1);
  layout->addWidget(new QLabel(tr("Massage:")), 1, 0);
  layout->addWidget(new QLabel(auth->realm()), 1, 1);
  layout->addWidget(new QLabel(tr("Username:")), 2, 0);
  layout->addWidget(user_, 2, 1);
  layout->addWidget(new QLabel(tr("Password:")), 3, 0);
  layout->addWidget(pass_, 3, 1);

  pageLayout->addLayout(layout);
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
