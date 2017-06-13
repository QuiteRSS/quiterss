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
#include "aboutdialog.h"
#include "mainapplication.h"
#include "settings.h"
#include "VersionNo.h"

#include <sqlite3.h>
#ifdef HAVE_QT5
#include <QWebPage>
#else
#include <qwebkitversion.h>
#endif

AboutDialog::AboutDialog(const QString &lang, QWidget *parent) :
  Dialog(parent, Qt::MSWindowsFixedSizeDialogHint)
{
  setWindowTitle(tr("About"));
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setObjectName("AboutDialog");
  setMinimumWidth(480);

  QTabWidget *tabWidget = new QTabWidget();

  QString revisionStr;
  if (QString("%1").arg(VCS_REVISION) != "0") {
      revisionStr = "<BR>" + tr("Revision") + " " + QString("%1").arg(VCS_REVISION);
  }
  QString appInfo =
      "<html><style>a { color: blue; text-decoration: none; }</style><body>"
      "<CENTER>"
      "<IMG SRC=\":/images/images/logo.png\">"
      "<BR><IMG SRC=\":/images/images/logo_text.png\">"
      "<P>"
      + tr("Version") + " " + "<B>" + QString(STRPRODUCTVER) + "</B>" + QString(" (%1)").arg(STRDATE)
      + revisionStr
      + "</P>"
      + "<BR>"
      + tr("QuiteRSS is a open-source cross-platform RSS/Atom news reader")
      + "<P>" + tr("Includes:")
      + QString(" Qt-%1, SQLite-%2, WebKit-%4").
      arg(QT_VERSION_STR).arg(SQLITE_VERSION).arg(qWebKitVersion())
      + "</P>"
      + QString("<a href=\"%1\">%1</a>").arg("http://quiterss.org") +
      "<P>Copyright &copy; 2011-2017 QuiteRSS Team "
      + QString("<a href=\"%1\">E-mail</a>").arg("mailto:quiterssteam@gmail.com") + "</P>"
      "</CENTER></body></html>";
  QLabel *infoLabel = new QLabel(appInfo);
  infoLabel->setOpenExternalLinks(true);
  infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

  QHBoxLayout *mainLayout = new QHBoxLayout();
  mainLayout->addWidget(infoLabel);
  QWidget *mainWidget = new QWidget();
  mainWidget->setLayout(mainLayout);

  QTextEdit *authorsTextEdit = new QTextEdit(this);
  authorsTextEdit->setReadOnly(true);
  QFile file;
  file.setFileName(":/file/AUTHORS");
  file.open(QFile::ReadOnly);
  authorsTextEdit->setText(QString::fromUtf8(file.readAll()));
  file.close();

  QHBoxLayout *authorsLayout = new QHBoxLayout();
  authorsLayout->addWidget(authorsTextEdit);
  QWidget *authorsWidget = new QWidget();
  authorsWidget->setLayout(authorsLayout);

  QTextBrowser *historyTextBrowser = new QTextBrowser();
  historyTextBrowser->setOpenExternalLinks(true);
  if (lang.contains("ru", Qt::CaseInsensitive))
    file.setFileName(":/file/HISTORY_RU");
  else
    file.setFileName(":/file/HISTORY_EN");
  file.open(QFile::ReadOnly);
  historyTextBrowser->setHtml(QString::fromUtf8(file.readAll()));
  file.close();

  QHBoxLayout *historyLayout = new QHBoxLayout();
  historyLayout->addWidget(historyTextBrowser);
  QWidget *historyWidget = new QWidget();
  historyWidget->setLayout(historyLayout);

  QTextEdit *licenseTextEdit = new QTextEdit();
  licenseTextEdit->setReadOnly(true);
  file.setFileName(":/file/COPYING");
  file.open(QFile::ReadOnly);
  QString str = QString(QString::fromUtf8(file.readAll())).section("-----", 1, 1);
  licenseTextEdit->setText(str);
  file.close();

  QHBoxLayout *licenseLayout = new QHBoxLayout();
  licenseLayout->addWidget(licenseTextEdit);
  QWidget *licenseWidget = new QWidget();
  licenseWidget->setLayout(licenseLayout);

  QString portable;
  if (mainApp->isPortable()) {
    if (!mainApp->isPortableAppsCom())
      portable = "(Portable)";
    else
      portable = "(PortableApps)";
  }
  Settings settings;
  QString information =
      "<table border=\"0\"><tr>"
      "<td>" + tr("Version") + " </td>"
      "<td>" + QString("%1.%2 %3 %4").arg(STRPRODUCTVER).arg(VCS_REVISION).arg(portable).arg(STRDATE) + "</td>"
      "</tr><tr></tr><tr>"
      "<td>" + tr("Application directory:") + " </td>"
      "<td>" + QCoreApplication::applicationDirPath() + "</td>"
      "</tr><tr>"
      "<td>" + tr("Resource directory:") + " </td>"
      "<td>" + mainApp->resourcesDir() + "</td>"
      "</tr><tr>"
      "<td>" + tr("Data directory:") + " </td>"
      "<td>" + mainApp->dataDir() + "</td>"
      "</tr><tr>"
      "<td>" + tr("Backup directory:") + " </td>"
      "<td>" + mainApp->dataDir() + "/backup" + "</td>"
      "</tr><tr></tr><tr>"
      "<td>" + tr("Database file:") + " </td>"
      "<td>" + mainApp->dbFileName() + "</td>"
      "</tr><tr>"
      "<td>" + tr("Settings file:") + " </td>"
      "<td>" + settings.fileName() + "</td>"
      "</tr><tr>"
      "<td>" + tr("Log file:") + " </td>"
      "<td>" + mainApp->dataDir() + "/debug.log" + "</td>"
      "</tr></table>";

  QTextEdit *informationTextEdit = new QTextEdit();
  informationTextEdit->setReadOnly(true);
  informationTextEdit->setText(information);

  QHBoxLayout *informationLayout = new QHBoxLayout();
  informationLayout->addWidget(informationTextEdit);

  QWidget *informationWidget = new QWidget();
  informationWidget->setLayout(informationLayout);

  tabWidget->addTab(mainWidget, tr("Version"));
  tabWidget->addTab(authorsWidget, tr("Authors"));
  tabWidget->addTab(historyWidget, tr("History"));
  tabWidget->addTab(licenseWidget, tr("License"));
  tabWidget->addTab(informationWidget, tr("Information"));

  pageLayout->addWidget(tabWidget);

  buttonBox->addButton(QDialogButtonBox::Close);
}
