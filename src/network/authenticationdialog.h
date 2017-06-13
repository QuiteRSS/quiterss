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
  explicit AuthenticationDialog(const QUrl &url,
                                QAuthenticator *auth,
                                QWidget *parent = 0);
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
