#include "updateappdialog.h"
#include "VersionNo.h"

UpdateAppDialog::UpdateAppDialog(const QString &lang, QSettings *settings,
                                 QWidget *parent, bool show)
  : QDialog(parent),
    settings_(settings),
    showDialog_(show)
{
  if (showDialog_) {
    setWindowTitle(tr("Check for updates"));
    setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setObjectName("UpdateAppDialog");
    resize(450, 350);

    QVBoxLayout *updateApplayout = new QVBoxLayout(this);
    updateApplayout->setAlignment(Qt::AlignCenter);
    updateApplayout->setMargin(10);
    updateApplayout->setSpacing(10);

    infoLabel = new QLabel(tr("Checking for updates..."), this);
    infoLabel->setOpenExternalLinks(true);
    updateApplayout->addWidget(infoLabel, 0);

    history_ = new QTextBrowser(this);
    history_->setObjectName("history_");
    history_->setText(tr("Loading history..."));
    history_->setOpenExternalLinks(true);
    updateApplayout->addWidget(history_, 1);

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

    QString urlHistory;
    if (lang.contains("ru", Qt::CaseInsensitive))
      urlHistory = "http://quite-rss.googlecode.com/hg/HISTORY_RU";
    else urlHistory = "http://quite-rss.googlecode.com/hg/HISTORY_EN";
    historyReply_ = manager_.get(QNetworkRequest(QUrl(urlHistory)));
    connect(historyReply_, SIGNAL(finished()), this, SLOT(slotFinishHistoryReply()));

    connect(this, SIGNAL(finished(int)), this, SLOT(closeDialog()));

    restoreGeometry(settings_->value("updateAppDlg/geometry").toByteArray());
  }

  reply_ = manager_.get(QNetworkRequest(QUrl("http://quite-rss.googlecode.com/hg/src/VersionNo.h")));
  connect(reply_, SIGNAL(finished()), this, SLOT(finishUpdateApp()));
}

void UpdateAppDialog::closeDialog()
{
  settings_->setValue("updateAppDlg/geometry", saveGeometry());
}

void UpdateAppDialog::finishUpdateApp()
{
  reply_->deleteLater();

  QString info;
  bool newVersion = false;

  if (reply_->error() == QNetworkReply::NoError) {
    QString version = QString(STRFILEVER).section('.', 0, 2);
    QString date = STRDATE;

    QString str = QLatin1String(reply_->readAll());
    QString curVersion = str.section('"', 1, 1).section('.', 0, 2);
    QString curDate = str.section('"', 3).left(10);

    if (version.contains(curVersion)) {
      str =
          "<a><font color=#4b4b4b>" +
          tr("You already have the latest version") +
          "</a>";
    } else {
      str =
          "<a><font color=#FF8040>" +
          tr("A new version of QuiteRSS is available!") + "</a>"
          "<p>" + QString("<a href=\"%1\">%2</a>").
          arg("http://code.google.com/p/quite-rss/downloads/list").
          arg(tr("Click here to go to the download page"));
      newVersion = true;
    }
    info =
        "<html><style>a { color: blue; text-decoration: none; }</style><body>"
        "<p>" + tr("Your version is: ") +
        "<B>" + version + "</B>" + QString(" (%1)").arg(date) +
        "<BR>" + tr("Current version is: ") +
        "<B>" + curVersion + "</B>" + QString(" (%1)").arg(curDate) +
        "<p>" + str +
        "</body></html>";
  } else {
    //    qDebug() << reply_->error() << reply_->errorString();
    info = tr("Error checking updates");
  }

  if (!showDialog_) emit signalNewVersion(newVersion);
  else infoLabel->setText(info);
}

void UpdateAppDialog::slotFinishHistoryReply()
{
  historyReply_->deleteLater();
  if (historyReply_->error() != QNetworkReply::NoError) {
    qDebug() << "error retrieving " << historyReply_->url();
    return;
  }

  QString str = QString::fromUtf8(historyReply_->readAll());

  history_->setHtml(str);
}
