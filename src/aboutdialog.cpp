#include "aboutdialog.h"
#include "VersionNo.h"
#include "VersionRev.h"
#if QT_VERSION >= 0x040800
#include <sqlite_qt48x/sqlite3.h>
#else
#include <sqlite_qt47x/sqlite3.h>
#endif
#include <qyursqltreeview.h>

AboutDialog::AboutDialog(const QString &lang, QWidget *parent) :
  QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
{
  setWindowTitle(tr("About"));
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setObjectName("AboutDialog");
  setMinimumWidth(440);

  QTabWidget *tabWidget = new QTabWidget();

  QyurSqlTreeView treeView;

  QString appInfo =
      "<html><style>a { color: blue; text-decoration: none; }</style><body>"
      "<CENTER>"
      "<IMG SRC=\":/images/images/logo.png\">"
      "<BR><IMG SRC=\":/images/images/logo_text.png\">"
      "<P>"
      + tr("Version ") + "<B>" + QString(STRPRODUCTVER) + "</B>" + QString(" (%1)").arg(STRDATE) + "<BR>"
      + tr("Revision ") + QString(HG_REVISION)
      + "</P>"
      + "<BR>"
      + tr("QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader")
      + "<P>" + tr("Includes: ")
      + QString("Qt-%1, SQLite-%2, QyurSqlTreeView-%3").
      arg(QT_VERSION_STR).arg(SQLITE_VERSION).
      arg(treeView.metaObject()->classInfo(treeView.metaObject()->indexOfClassInfo("Version")).value())
      + "</P>"
      + QString("<a href=\"%1/\">%1</a>").arg("www.code.google.com/p/quite-rss")
      + QString("<br><a href=\"%1/\">%1</a>").arg("www.quiterss.ucoz.ru") +
      "<P>&copy; 2011-2012 QuiteRSS Team "
      + QString("<a href=\"%1/\">E-mail</a>").arg("quiterssteam@gmail.com") + "</P>"
      "</CENTER></body></html>";
  QLabel *infoLabel = new QLabel(appInfo);
  infoLabel->setOpenExternalLinks(true);

  QHBoxLayout *mainLayout = new QHBoxLayout();
  mainLayout->addWidget(infoLabel);
  QWidget *mainWidget = new QWidget();
  mainWidget->setLayout(mainLayout);

  QTextEdit *authorsTextEdit = new QTextEdit(this);
  authorsTextEdit->setFocusPolicy(Qt::NoFocus);
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
  licenseTextEdit->setFocusPolicy(Qt::NoFocus);
  file.setFileName(":/file/COPYING");
  file.open(QFile::ReadOnly);
  QString str = QString(QString::fromUtf8(file.readAll())).section("-----", 1, 1);
  licenseTextEdit->setText(str);
  file.close();

  QHBoxLayout *licenseLayout = new QHBoxLayout();
  licenseLayout->addWidget(licenseTextEdit);

  QWidget *licenseWidget = new QWidget();
  licenseWidget->setLayout(licenseLayout);

  tabWidget->addTab(mainWidget, tr("Version"));
  tabWidget->addTab(authorsWidget, tr("Authors"));
  tabWidget->addTab(historyWidget, tr("History"));
  tabWidget->addTab(licenseWidget, tr("License"));

  QLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->setAlignment(Qt::AlignRight);
  QPushButton *closeButton = new QPushButton(tr("&Close"));
  closeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  closeButton->setDefault(true);
  closeButton->setFocus(Qt::OtherFocusReason);
  connect(closeButton, SIGNAL(clicked()), SLOT(close()));
  buttonLayout->addWidget(closeButton);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(tabWidget);
  layout->addLayout(buttonLayout);
  setLayout(layout);
}
