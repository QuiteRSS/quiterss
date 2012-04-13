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
  aboutlayout->setMargin(0);
  aboutlayout->setSpacing(10);

  QLabel *logo = new QLabel(this);
  logo->setObjectName("logoLabel");
  logo->setFixedWidth(110);
  logo->setAlignment(Qt::AlignTop|Qt::AlignHCenter);
  logo->setPixmap(QPixmap(":/images/images/logo.png"));
  aboutlayout->addWidget(logo);

  QBoxLayout *layout = new QVBoxLayout();
  layout->setAlignment(Qt::AlignCenter);
  layout->setMargin(10);
  layout->setSpacing(10);
  aboutlayout->addLayout(layout);

  QString info =
      "<html><style>a { color: blue; text-decoration: none; }</style><body>"
      "<h1><font color=#5b5b5b>"
      + QString("QuiteRSS") + "</font></h1>"
      "<p>" + tr("Version ") +
      + "<B>" + QString(STRFILEVER).section('.', 0, 2) + "</B>"
      + QString(" (%1)").arg(STRDATE) + "</p>"
      "<HR>"
      + tr("The authors:") +
      "<UL>"
      "<li>Aleksey Khokhryakov   <a href='mailto:aleksey.khokhryakov@gmail.com'>e-mail</a></li>"
      "<li>Shilyaev Egor   <a href='mailto:egor.shilyaev@gmail.com'>e-mail</a></li>"
      "<li>Kalinin Andrey   <a href='mailto:mikhalych.kalinin@gmail.com'>e-mail</a></li>"
      "</UL>"
      + tr("Acknowledgements:") +
      "<UL>"
      "<li>Elbert Pol</li>"
      "<li>TI_Eugene</li>"
      "<li>Glad Deschrijver</li>"
      "</UL>"
      "<HR>"
      + QString("<a href=\"%1/\">%1</a>").arg("www.code.google.com/p/quite-rss") +
      "<p>&copy; 2011-2012 QuiteRSS Team</p>"
      "</body></html>";
  QLabel *infoLabel = new QLabel(info, this);
  infoLabel->setMinimumWidth(250);
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
