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
#include "sslerrordialog.h"

SslErrorDialog::SslErrorDialog(QWidget* parent)
  : Dialog(parent, Qt::MSWindowsFixedSizeDialogHint)
{
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("SSL Certificate Error!"));
  setMinimumWidth(400);
  setMinimumHeight(200);

  errorLabel_ = new QLabel();

  pageLayout->addWidget(errorLabel_);

  buttonBox->addButton(QDialogButtonBox::Yes);
  buttonBox->addButton(QDialogButtonBox::No);
  buttonBox->addButton(tr("Only for this session"), QDialogButtonBox::ApplyRole);
  buttonBox->button(QDialogButtonBox::No)->setFocus();
  connect(buttonBox, SIGNAL(clicked(QAbstractButton*)),
          this, SLOT(buttonClicked(QAbstractButton*)));
}

SslErrorDialog::~SslErrorDialog()
{

}

void SslErrorDialog::setText(const QString &text)
{
  errorLabel_->setText(text);
}

SslErrorDialog::Result SslErrorDialog::result()
{
  return result_;
}

void SslErrorDialog::buttonClicked(QAbstractButton* button)
{
  switch (buttonBox->buttonRole(button)) {
  case QDialogButtonBox::YesRole:
    result_ = Yes;
    accept();
    break;

  case QDialogButtonBox::ApplyRole:
    result_ = OnlyForThisSession;
    accept();
    break;

  default:
    result_ = No;
    reject();
    break;
  }
}

QString SslErrorDialog::certificateItemText(const QSslCertificate &cert)
{
#if QT_VERSION >= 0x050000
    QString commonName = cert.subjectInfo(QSslCertificate::CommonName).isEmpty() ? QString() : cert.subjectInfo(QSslCertificate::CommonName).at(0);
    QString organization = cert.subjectInfo(QSslCertificate::Organization).isEmpty() ? QString() : cert.subjectInfo(QSslCertificate::Organization).at(0);
#else
    QString commonName = cert.subjectInfo(QSslCertificate::CommonName);
    QString organization = cert.subjectInfo(QSslCertificate::Organization);
#endif

    if (commonName.isEmpty()) {
        return clearCertSpecialSymbols(organization);
    }

    return clearCertSpecialSymbols(commonName);
}

QString SslErrorDialog::clearCertSpecialSymbols(const QString &string)
{
#if QT_VERSION >= 0x050000
  QString n = string.toHtmlEscaped();
#else
  QString n = Qt::escape(string);
#endif

  if (!n.contains(QLatin1String("\\"))) {
    return n;
  }

  // Credits to http://blade.nagaokaut.ac.jp/cgi-bin/scat.rb/ruby/ruby-talk/176679?help-en
  n.replace(QLatin1String("\\xC3\\x80"), QLatin1String("A"));
  n.replace(QLatin1String("\\xC3\\x81"), QLatin1String("A"));
  n.replace(QLatin1String("\\xC3\\x82"), QLatin1String("A"));
  n.replace(QLatin1String("\\xC3\\x83"), QLatin1String("A"));
  n.replace(QLatin1String("\\xC3\\x84"), QLatin1String("A"));
  n.replace(QLatin1String("\\xC3\\x85"), QLatin1String("A"));
  n.replace(QLatin1String("\\xC3\\x86"), QLatin1String("AE"));
  n.replace(QLatin1String("\\xC3\\x87"), QLatin1String("C"));
  n.replace(QLatin1String("\\xC3\\x88"), QLatin1String("E"));
  n.replace(QLatin1String("\\xC3\\x89"), QLatin1String("E"));
  n.replace(QLatin1String("\\xC3\\x8A"), QLatin1String("E"));
  n.replace(QLatin1String("\\xC3\\x8B"), QLatin1String("E"));
  n.replace(QLatin1String("\\xC3\\x8C"), QLatin1String("I"));
  n.replace(QLatin1String("\\xC3\\x8D"), QLatin1String("I"));
  n.replace(QLatin1String("\\xC3\\x8E"), QLatin1String("I"));
  n.replace(QLatin1String("\\xC3\\x8F"), QLatin1String("I"));
  n.replace(QLatin1String("\\xC3\\x90"), QLatin1String("D"));
  n.replace(QLatin1String("\\xC3\\x91"), QLatin1String("N"));
  n.replace(QLatin1String("\\xC3\\x92"), QLatin1String("O"));
  n.replace(QLatin1String("\\xC3\\x93"), QLatin1String("O"));
  n.replace(QLatin1String("\\xC3\\x94"), QLatin1String("O"));
  n.replace(QLatin1String("\\xC3\\x95"), QLatin1String("O"));
  n.replace(QLatin1String("\\xC3\\x96"), QLatin1String("O"));
  n.replace(QLatin1String("\\xC3\\x98"), QLatin1String("O"));
  n.replace(QLatin1String("\\xC3\\x99"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC3\\x9A"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC3\\x9B"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC3\\x9C"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC3\\x9D"), QLatin1String("Y"));
  n.replace(QLatin1String("\\xC3\\x9E"), QLatin1String("P"));
  n.replace(QLatin1String("\\xC3\\x9F"), QLatin1String("ss"));
  n.replace(QLatin1String("\\xC9\\x99"), QLatin1String("e"));
  n.replace(QLatin1String("\\xC3\\xA0"), QLatin1String("a"));
  n.replace(QLatin1String("\\xC3\\xA1"), QLatin1String("a"));
  n.replace(QLatin1String("\\xC3\\xA2"), QLatin1String("a"));
  n.replace(QLatin1String("\\xC3\\xA3"), QLatin1String("a"));
  n.replace(QLatin1String("\\xC3\\xA4"), QLatin1String("a"));
  n.replace(QLatin1String("\\xC3\\xA5"), QLatin1String("a"));
  n.replace(QLatin1String("\\xC3\\xA6"), QLatin1String("ae"));
  n.replace(QLatin1String("\\xC3\\xA7"), QLatin1String("c"));
  n.replace(QLatin1String("\\xC3\\xA8"), QLatin1String("e"));
  n.replace(QLatin1String("\\xC3\\xA9"), QLatin1String("e"));
  n.replace(QLatin1String("\\xC3\\xAA"), QLatin1String("e"));
  n.replace(QLatin1String("\\xC3\\xAB"), QLatin1String("e"));
  n.replace(QLatin1String("\\xC3\\xAC"), QLatin1String("i"));
  n.replace(QLatin1String("\\xC3\\xAD"), QLatin1String("i"));
  n.replace(QLatin1String("\\xC3\\xAE"), QLatin1String("i"));
  n.replace(QLatin1String("\\xC3\\xAF"), QLatin1String("i"));
  n.replace(QLatin1String("\\xC3\\xB0"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC3\\xB1"), QLatin1String("n"));
  n.replace(QLatin1String("\\xC3\\xB2"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC3\\xB3"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC3\\xB4"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC3\\xB5"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC3\\xB6"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC3\\xB8"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC3\\xB9"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC3\\xBA"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC3\\xBB"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC3\\xBC"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC3\\xBD"), QLatin1String("y"));
  n.replace(QLatin1String("\\xC3\\xBE"), QLatin1String("p"));
  n.replace(QLatin1String("\\xC3\\xBF"), QLatin1String("y"));
  n.replace(QLatin1String("\\xC7\\xBF"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC4\\x80"), QLatin1String("A"));
  n.replace(QLatin1String("\\xC4\\x81"), QLatin1String("a"));
  n.replace(QLatin1String("\\xC4\\x82"), QLatin1String("A"));
  n.replace(QLatin1String("\\xC4\\x83"), QLatin1String("a"));
  n.replace(QLatin1String("\\xC4\\x84"), QLatin1String("A"));
  n.replace(QLatin1String("\\xC4\\x85"), QLatin1String("a"));
  n.replace(QLatin1String("\\xC4\\x86"), QLatin1String("C"));
  n.replace(QLatin1String("\\xC4\\x87"), QLatin1String("c"));
  n.replace(QLatin1String("\\xC4\\x88"), QLatin1String("C"));
  n.replace(QLatin1String("\\xC4\\x89"), QLatin1String("c"));
  n.replace(QLatin1String("\\xC4\\x8A"), QLatin1String("C"));
  n.replace(QLatin1String("\\xC4\\x8B"), QLatin1String("c"));
  n.replace(QLatin1String("\\xC4\\x8C"), QLatin1String("C"));
  n.replace(QLatin1String("\\xC4\\x8D"), QLatin1String("c"));
  n.replace(QLatin1String("\\xC4\\x8E"), QLatin1String("D"));
  n.replace(QLatin1String("\\xC4\\x8F"), QLatin1String("d"));
  n.replace(QLatin1String("\\xC4\\x90"), QLatin1String("D"));
  n.replace(QLatin1String("\\xC4\\x91"), QLatin1String("d"));
  n.replace(QLatin1String("\\xC4\\x92"), QLatin1String("E"));
  n.replace(QLatin1String("\\xC4\\x93"), QLatin1String("e"));
  n.replace(QLatin1String("\\xC4\\x94"), QLatin1String("E"));
  n.replace(QLatin1String("\\xC4\\x95"), QLatin1String("e"));
  n.replace(QLatin1String("\\xC4\\x96"), QLatin1String("E"));
  n.replace(QLatin1String("\\xC4\\x97"), QLatin1String("e"));
  n.replace(QLatin1String("\\xC4\\x98"), QLatin1String("E"));
  n.replace(QLatin1String("\\xC4\\x99"), QLatin1String("e"));
  n.replace(QLatin1String("\\xC4\\x9A"), QLatin1String("E"));
  n.replace(QLatin1String("\\xC4\\x9B"), QLatin1String("e"));
  n.replace(QLatin1String("\\xC4\\x9C"), QLatin1String("G"));
  n.replace(QLatin1String("\\xC4\\x9D"), QLatin1String("g"));
  n.replace(QLatin1String("\\xC4\\x9E"), QLatin1String("G"));
  n.replace(QLatin1String("\\xC4\\x9F"), QLatin1String("g"));
  n.replace(QLatin1String("\\xC4\\xA0"), QLatin1String("G"));
  n.replace(QLatin1String("\\xC4\\xA1"), QLatin1String("g"));
  n.replace(QLatin1String("\\xC4\\xA2"), QLatin1String("G"));
  n.replace(QLatin1String("\\xC4\\xA3"), QLatin1String("g"));
  n.replace(QLatin1String("\\xC4\\xA4"), QLatin1String("H"));
  n.replace(QLatin1String("\\xC4\\xA5"), QLatin1String("h"));
  n.replace(QLatin1String("\\xC4\\xA6"), QLatin1String("H"));
  n.replace(QLatin1String("\\xC4\\xA7"), QLatin1String("h"));
  n.replace(QLatin1String("\\xC4\\xA8"), QLatin1String("I"));
  n.replace(QLatin1String("\\xC4\\xA9"), QLatin1String("i"));
  n.replace(QLatin1String("\\xC4\\xAA"), QLatin1String("I"));
  n.replace(QLatin1String("\\xC4\\xAB"), QLatin1String("i"));
  n.replace(QLatin1String("\\xC4\\xAC"), QLatin1String("I"));
  n.replace(QLatin1String("\\xC4\\xAD"), QLatin1String("i"));
  n.replace(QLatin1String("\\xC4\\xAE"), QLatin1String("I"));
  n.replace(QLatin1String("\\xC4\\xAF"), QLatin1String("i"));
  n.replace(QLatin1String("\\xC4\\xB0"), QLatin1String("I"));
  n.replace(QLatin1String("\\xC4\\xB1"), QLatin1String("i"));
  n.replace(QLatin1String("\\xC4\\xB2"), QLatin1String("IJ"));
  n.replace(QLatin1String("\\xC4\\xB3"), QLatin1String("ij"));
  n.replace(QLatin1String("\\xC4\\xB4"), QLatin1String("J"));
  n.replace(QLatin1String("\\xC4\\xB5"), QLatin1String("j"));
  n.replace(QLatin1String("\\xC4\\xB6"), QLatin1String("K"));
  n.replace(QLatin1String("\\xC4\\xB7"), QLatin1String("k"));
  n.replace(QLatin1String("\\xC4\\xB8"), QLatin1String("k"));
  n.replace(QLatin1String("\\xC4\\xB9"), QLatin1String("L"));
  n.replace(QLatin1String("\\xC4\\xBA"), QLatin1String("l"));
  n.replace(QLatin1String("\\xC4\\xBB"), QLatin1String("L"));
  n.replace(QLatin1String("\\xC4\\xBC"), QLatin1String("l"));
  n.replace(QLatin1String("\\xC4\\xBD"), QLatin1String("L"));
  n.replace(QLatin1String("\\xC4\\xBE"), QLatin1String("l"));
  n.replace(QLatin1String("\\xC4\\xBF"), QLatin1String("L"));
  n.replace(QLatin1String("\\xC5\\x80"), QLatin1String("l"));
  n.replace(QLatin1String("\\xC5\\x81"), QLatin1String("L"));
  n.replace(QLatin1String("\\xC5\\x82"), QLatin1String("l"));
  n.replace(QLatin1String("\\xC5\\x83"), QLatin1String("N"));
  n.replace(QLatin1String("\\xC5\\x84"), QLatin1String("n"));
  n.replace(QLatin1String("\\xC5\\x85"), QLatin1String("N"));
  n.replace(QLatin1String("\\xC5\\x86"), QLatin1String("n"));
  n.replace(QLatin1String("\\xC5\\x87"), QLatin1String("N"));
  n.replace(QLatin1String("\\xC5\\x88"), QLatin1String("n"));
  n.replace(QLatin1String("\\xC5\\x89"), QLatin1String("n"));
  n.replace(QLatin1String("\\xC5\\x8A"), QLatin1String("N"));
  n.replace(QLatin1String("\\xC5\\x8B"), QLatin1String("n"));
  n.replace(QLatin1String("\\xC5\\x8C"), QLatin1String("O"));
  n.replace(QLatin1String("\\xC5\\x8D"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC5\\x8E"), QLatin1String("O"));
  n.replace(QLatin1String("\\xC5\\x8F"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC5\\x90"), QLatin1String("O"));
  n.replace(QLatin1String("\\xC5\\x91"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC5\\x92"), QLatin1String("CE"));
  n.replace(QLatin1String("\\xC5\\x93"), QLatin1String("ce"));
  n.replace(QLatin1String("\\xC5\\x94"), QLatin1String("R"));
  n.replace(QLatin1String("\\xC5\\x95"), QLatin1String("r"));
  n.replace(QLatin1String("\\xC5\\x96"), QLatin1String("R"));
  n.replace(QLatin1String("\\xC5\\x97"), QLatin1String("r"));
  n.replace(QLatin1String("\\xC5\\x98"), QLatin1String("R"));
  n.replace(QLatin1String("\\xC5\\x99"), QLatin1String("r"));
  n.replace(QLatin1String("\\xC5\\x9A"), QLatin1String("S"));
  n.replace(QLatin1String("\\xC5\\x9B"), QLatin1String("s"));
  n.replace(QLatin1String("\\xC5\\x9C"), QLatin1String("S"));
  n.replace(QLatin1String("\\xC5\\x9D"), QLatin1String("s"));
  n.replace(QLatin1String("\\xC5\\x9E"), QLatin1String("S"));
  n.replace(QLatin1String("\\xC5\\x9F"), QLatin1String("s"));
  n.replace(QLatin1String("\\xC5\\xA0"), QLatin1String("S"));
  n.replace(QLatin1String("\\xC5\\xA1"), QLatin1String("s"));
  n.replace(QLatin1String("\\xC5\\xA2"), QLatin1String("T"));
  n.replace(QLatin1String("\\xC5\\xA3"), QLatin1String("t"));
  n.replace(QLatin1String("\\xC5\\xA4"), QLatin1String("T"));
  n.replace(QLatin1String("\\xC5\\xA5"), QLatin1String("t"));
  n.replace(QLatin1String("\\xC5\\xA6"), QLatin1String("T"));
  n.replace(QLatin1String("\\xC5\\xA7"), QLatin1String("t"));
  n.replace(QLatin1String("\\xC5\\xA8"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC5\\xA9"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC5\\xAA"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC5\\xAB"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC5\\xAC"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC5\\xAD"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC5\\xAE"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC5\\xAF"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC5\\xB0"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC5\\xB1"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC5\\xB2"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC5\\xB3"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC5\\xB4"), QLatin1String("W"));
  n.replace(QLatin1String("\\xC5\\xB5"), QLatin1String("w"));
  n.replace(QLatin1String("\\xC5\\xB6"), QLatin1String("Y"));
  n.replace(QLatin1String("\\xC5\\xB7"), QLatin1String("y"));
  n.replace(QLatin1String("\\xC5\\xB8"), QLatin1String("Y"));
  n.replace(QLatin1String("\\xC5\\xB9"), QLatin1String("Z"));
  n.replace(QLatin1String("\\xC5\\xBA"), QLatin1String("z"));
  n.replace(QLatin1String("\\xC5\\xBB"), QLatin1String("Z"));
  n.replace(QLatin1String("\\xC5\\xBC"), QLatin1String("z"));
  n.replace(QLatin1String("\\xC5\\xBD"), QLatin1String("Z"));
  n.replace(QLatin1String("\\xC5\\xBE"), QLatin1String("z"));
  n.replace(QLatin1String("\\xC6\\x8F"), QLatin1String("E"));
  n.replace(QLatin1String("\\xC6\\xA0"), QLatin1String("O"));
  n.replace(QLatin1String("\\xC6\\xA1"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC6\\xAF"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC6\\xB0"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC7\\x8D"), QLatin1String("A"));
  n.replace(QLatin1String("\\xC7\\x8E"), QLatin1String("a"));
  n.replace(QLatin1String("\\xC7\\x8F"), QLatin1String("I"));
  n.replace(QLatin1String("\\xC7\\x93"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC7\\x90"), QLatin1String("i"));
  n.replace(QLatin1String("\\xC7\\x91"), QLatin1String("O"));
  n.replace(QLatin1String("\\xC7\\x92"), QLatin1String("o"));
  n.replace(QLatin1String("\\xC7\\x97"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC7\\x94"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC7\\x95"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC7\\x96"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC7\\x9B"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC7\\x98"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC7\\x99"), QLatin1String("U"));
  n.replace(QLatin1String("\\xC7\\x9A"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC7\\xBD"), QLatin1String("ae"));
  n.replace(QLatin1String("\\xC7\\x9C"), QLatin1String("u"));
  n.replace(QLatin1String("\\xC7\\xBB"), QLatin1String("a"));
  n.replace(QLatin1String("\\xC7\\xBC"), QLatin1String("AE"));
  n.replace(QLatin1String("\\xC7\\xBE"), QLatin1String("O"));
  n.replace(QLatin1String("\\xC7\\xBA"), QLatin1String("A"));

  n.replace(QLatin1String("\\xC2\\x82"), QLatin1String(","));        // High code comma
  n.replace(QLatin1String("\\xC2\\x84"), QLatin1String(",,"));       // High code double comma
  n.replace(QLatin1String("\\xC2\\x85"), QLatin1String("..."));      // Tripple dot
  n.replace(QLatin1String("\\xC2\\x88"), QLatin1String("^"));        // High carat
  n.replace(QLatin1String("\\xC2\\x91"), QLatin1String("\\x27"));    // Forward single quote
  n.replace(QLatin1String("\\xC2\\x92"), QLatin1String("\\x27"));    // Reverse single quote
  n.replace(QLatin1String("\\xC2\\x93"), QLatin1String("\\x22"));    // Forward double quote
  n.replace(QLatin1String("\\xC2\\x94"), QLatin1String("\\x22"));    // Reverse double quote
  n.replace(QLatin1String("\\xC2\\x96"), QLatin1String("-"));        // High hyphen
  n.replace(QLatin1String("\\xC2\\x97"), QLatin1String("--"));       // Double hyphen
  n.replace(QLatin1String("\\xC2\\xA6"), QLatin1String("|"));        // Split vertical bar
  n.replace(QLatin1String("\\xC2\\xAB"), QLatin1String("<<"));       // Double less than
  n.replace(QLatin1String("\\xC2\\xBB"), QLatin1String(">>"));       // Double greater than
  n.replace(QLatin1String("\\xC2\\xBC"), QLatin1String("1/4"));      // one quarter
  n.replace(QLatin1String("\\xC2\\xBD"), QLatin1String("1/2"));      // one half
  n.replace(QLatin1String("\\xC2\\xBE"), QLatin1String("3/4"));      // three quarters
  n.replace(QLatin1String("\\xCA\\xBF"), QLatin1String("\\x27"));    // c-single quote
  n.replace(QLatin1String("\\xCC\\xA8"), QString());                 // modifier - under curve
  n.replace(QLatin1String("\\xCC\\xB1"), QString());                 // modifier - under line

  return n;
}

QString SslErrorDialog::clearCertSpecialSymbols(const QStringList &stringList)
{
  if (stringList.isEmpty()) {
    return QString();
  }

  return clearCertSpecialSymbols(stringList.at(0));
}
