/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "authenticationdialog.h"

#include <QSqlQuery>

AuthenticationDialog::AuthenticationDialog(const QUrl &url,
                                           QAuthenticator *auth,
                                           QWidget *parent)
  : Dialog(parent, Qt::MSWindowsFixedSizeDialogHint),
    auth_(auth)
{
  save_ = new QCheckBox(tr("Save password"), this);
  save_->setChecked(false);

  server_ = url.host();
  if (server_.isEmpty()) {
    server_ = url.toString();
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
    user_->setText(auth->user());
    pass_ = new LineEdit(this);
    pass_->setEchoMode(QLineEdit::Password);

    QGridLayout *layout = new QGridLayout();
    layout->addWidget(new QLabel(tr("Server:")), 0, 0);
    layout->addWidget(new QLabel(server_), 0, 1);
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
