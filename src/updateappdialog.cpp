#include "updateappdialog.h"
#include "VersionNo.h"

UpdateAppDialog::UpdateAppDialog(QWidget *parent) :
    QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
{
  setWindowTitle(tr("Check for updates"));
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setObjectName("UpdateAppDialog");
  resize(250, 150);

  QVBoxLayout *updateApplayout = new QVBoxLayout(this);
  updateApplayout->setAlignment(Qt::AlignCenter);
  updateApplayout->setMargin(10);
  updateApplayout->setSpacing(10);

  infoLabel = new QLabel(tr("Check for updates..."), this);
  infoLabel->setOpenExternalLinks(true);
  updateApplayout->addWidget(infoLabel, 0);
  updateApplayout->addStretch(1);

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->setAlignment(Qt::AlignRight);
  QPushButton *closeButton = new QPushButton(tr("&Close"), this);
  closeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  closeButton->setDefault(true);
  closeButton->setFocus(Qt::OtherFocusReason);
  connect(closeButton, SIGNAL(clicked()), SLOT(close()));
  buttonLayout->addStretch(1);
  buttonLayout->addWidget(closeButton);
  updateApplayout->addLayout(buttonLayout);

  reply_ = manager_.get(QNetworkRequest(QUrl("http://quite-rss.googlecode.com/hg/appinfo")));
  connect(reply_, SIGNAL(finished()), this, SLOT(finishUpdateApp()));
}

void UpdateAppDialog::finishUpdateApp()
{
  if (reply_->error() == QNetworkReply::NoError) {
    QString str;
    QFile appInfoFile(":/app/appinfo");
    appInfoFile.open(QFile::ReadOnly);
    str = QLatin1String(appInfoFile.readAll());
    QString version = str.left(str.lastIndexOf(" "));
    QString date = str.right(str.length() - str.lastIndexOf(" ") - 1);

    str = QLatin1String(reply_->readAll());
    QString curVersion = str.left(str.lastIndexOf(" "));
    QString curDate = str.right(str.length() - str.lastIndexOf(" ") - 1);

    if (version.contains(curVersion)) {
      str =
          "<a><font color=#4b4b4b>" +
          tr("You already have the latest version") +
          "</a>";
    } else {
      str =
          "<a><font color=#FF8040>" + tr("A new version") + "</a>"
          "<p>" + QString("<a href=\"%1\">%2</a>").
          arg("http://code.google.com/p/quite-rss/downloads/list").
          arg("Click here to go to the download page");
    }
    QString info =
        "<html><style>a { color: blue; text-decoration: none; }</style><body>"
        "<p>" + tr("Your version is: ") +
        "<B>" + version + "</B>" + QString(" (%1)").arg(date) +
        "<BR>" + tr("Current version is: ") +
        "<B>" + curVersion + "</B>" + QString(" (%1)").arg(curDate) +
        "<p>" + str +
        "</body></html>";
    infoLabel->setText(info);
  } else {
//    qDebug() << reply_->error() << reply_->errorString();
    infoLabel->setText(tr("Error checking updates"));
  }
}
