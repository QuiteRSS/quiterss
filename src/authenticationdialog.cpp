#include "authenticationdialog.h"
#include <QSqlQuery>

AuthenticationDialog::AuthenticationDialog(QWidget *parent, QNetworkReply *reply,
                                           QAuthenticator *auth)
  : Dialog(parent, Qt::MSWindowsFixedSizeDialogHint),
    auth_(auth)
{
  save_ = new QCheckBox(tr("Save password"), this);
  save_->setChecked(false);

  server_ = reply->url().host();
  if (server_.isEmpty()) {
      server_ = reply->url().toString();
  }

  QSqlQuery q;
  q.prepare("SELECT username, password FROM passwords WHERE server=?");
  q.addBindValue(server_);
  q.exec();
  if (q.next()) {
    auth_->setUser(q.value(0).toString());
    auth_->setPassword(QString::fromUtf8(QByteArray::fromBase64(q.value(1).toByteArray())));
    save_->setChecked(true);
  } else {
    setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Authorization required"));
    setMinimumWidth(400);

    user_ = new LineEdit(this);
    pass_ = new LineEdit(this);
    pass_->setEchoMode(QLineEdit::Password);

    QGridLayout *layout = new QGridLayout();
    layout->addWidget(new QLabel(tr("Server:")), 0, 0);
    layout->addWidget(new QLabel(reply->url().host()), 0, 1);
    layout->addWidget(new QLabel(tr("Message:")), 1, 0);
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
}

void AuthenticationDialog::acceptDialog()
{
  auth_->setUser(user_->text());
  auth_->setPassword(pass_->text());

  if (save_->isChecked()) {
    QSqlQuery q;
    q.prepare("INSERT INTO passwords (server, username, password) "
              "VALUES (:server, :username, :password)");
    q.bindValue(":server", server_);
    q.bindValue(":username", user_->text());
    q.bindValue(":password", pass_->text().toUtf8().toBase64());
    q.exec();
  }

  accept();
}
