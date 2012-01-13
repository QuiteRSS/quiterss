#include "aboutdialog.h"
#include "VersionNo.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
{
  setWindowTitle(tr("About"));
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setObjectName("AboutDialog");

  QBoxLayout *aboutlayout = new QHBoxLayout(this);
  aboutlayout->setAlignment(Qt::AlignCenter);
  aboutlayout->setMargin(10);
  aboutlayout->setSpacing(10);

  QLabel *logo = new QLabel(this);
  logo->setObjectName("logoLabel");
  logo->setPixmap(QPixmap(":/images/images/QtRSS32.png"));
  aboutlayout->addWidget(logo, 0, Qt::AlignTop);

  QBoxLayout *layout = new QVBoxLayout();
  layout->setAlignment(Qt::AlignCenter);
  layout->setSpacing(10);
  aboutlayout->addLayout(layout);

  QString info =
      "<html><style>a { color: blue; text-decoration: none; }</style><body>"
      "<h1><font color=#5b5b5b>"
      + QString("QuiteRSS") + "</font></h1>"
      "<p>" + tr("Version ") +
      + "<B>" + QString("%1").arg(QString(STRFILEVER).section('.', 0, 2)) + "</B></p>"
      "<HR>"
      + tr("The authors:") +
      "<UL>"
      "<li>Aleksey Khokhryakov   <a href='mailto:aleksey.khokhryakov@gmail.com'>e-mail</a></li>"
      "<li>Shilyaev Egor   <a href='mailto:egor.shilyaev@gmail.com'>e-mail</a></li>"
      "</UL>"
      "<HR>"
      + QString("<a href=\"%1/\">%1</a>").arg("www.code.google.com/p/quite-rss") +
      "<p>&copy; 2011 QuiteTeam</p>"
      "</body></html>";
  QLabel *infoLabel = new QLabel(info, this);
  infoLabel->setMinimumWidth(200);
  infoLabel->setOpenExternalLinks(true);
  layout->addWidget(infoLabel);


  QLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->setAlignment(Qt::AlignRight);
  QPushButton *closeButton = new QPushButton(tr("&Close"), this);
  closeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  closeButton->setDefault(true);
  closeButton->setFocus(Qt::OtherFocusReason);
  connect(closeButton, SIGNAL(clicked()), SLOT(close()));
  buttonLayout->addWidget(closeButton);

  layout->addLayout(buttonLayout);
}
