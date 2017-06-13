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
#include "updateappdialog.h"

#include "mainapplication.h"
#include "VersionNo.h"
#include "settings.h"

#include <QNetworkCookie>
#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

UpdateAppDialog::UpdateAppDialog(const QString &lang, QWidget *parent, bool show)
  : Dialog(parent)
  , lang_(lang)
  , showDialog_(show)
  , page_(NULL)
{
  Settings settings;

  networkManagerProxy_ = new NetworkManagerProxy(0, this);

  if (showDialog_) {
    setWindowTitle(tr("Check for Updates"));
    setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setObjectName("UpdateAppDialog");
    resize(450, 350);

    infoLabel = new QLabel(tr("Checking for updates..."), this);
    infoLabel->setOpenExternalLinks(true);

    history_ = new QTextBrowser(this);
    history_->setObjectName("history_");
    history_->setText(tr("Loading history..."));
    history_->setOpenExternalLinks(true);

    remindAboutVersion_ = new QCheckBox(tr("Don't remind about this version"), this);
    remindAboutVersion_->setChecked(settings.value("remindAboutVersion", false).toBool());
    remindAboutVersion_->hide();

    pageLayout->addWidget(infoLabel, 0);
    pageLayout->addWidget(history_, 1);
    pageLayout->addWidget(remindAboutVersion_, 0);

    installButton_ = new QPushButton(tr("&Install"), this);
    installButton_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    installButton_->hide();
    buttonsLayout->insertWidget(0, installButton_, 1);
    connect(installButton_, SIGNAL(clicked()), SLOT(updaterRun()));

    buttonBox->addButton(QDialogButtonBox::Close);

    renderStatistics();

    QString urlHistory;
    if (lang_.contains("ru", Qt::CaseInsensitive))
      urlHistory = "http://quiterss.org/files/updates_new/HISTORY_RU";
    else urlHistory = "http://quiterss.org/files/updates_new/HISTORY_EN";
    historyReply_ = networkManagerProxy_->get(QNetworkRequest(QUrl(urlHistory)));
    connect(historyReply_, SIGNAL(finished()), this, SLOT(slotFinishHistoryReply()));

    connect(this, SIGNAL(finished(int)), this, SLOT(closeDialog()));

    restoreGeometry(settings.value("updateAppDlg/geometry").toByteArray());
  } else {
    renderStatistics();
  }
}

UpdateAppDialog::~UpdateAppDialog()
{

}

void UpdateAppDialog::disconnectObjects()
{
  disconnect(this);
  networkManagerProxy_->disconnectObjects();

  if (page_ != NULL) {
    QWebSettings::clearMemoryCaches();
    page_->disconnect(this);
    delete page_;
  }

  delete networkManagerProxy_;
}

void UpdateAppDialog::closeDialog()
{
  Settings settings;
  settings.setValue("remindAboutVersion", remindAboutVersion_->isChecked());
  settings.setValue("updateAppDlg/geometry", saveGeometry());
}

void UpdateAppDialog::finishUpdatesChecking()
{
  reply_->deleteLater();

  QString info;
  QString newVersion = "";

  if (reply_->error() == QNetworkReply::NoError) {
    QString version = STRPRODUCTVER;
    QString date = STRDATE;

    QString str = QLatin1String(reply_->readAll());
    QString curVersion = str.section('"', 5, 5).section('\\', 0, 0);
    QString curDate = str.section('"', 3, 3).section('\\', 0, 0);

    if (version.contains(curVersion)) {
      str =
          "<a><font color=#4b4b4b>" +
          tr("You already have the latest version") +
          "</a>";
    } else {
      QString urlDownloads;
      if (lang_.contains("ru", Qt::CaseInsensitive))
        urlDownloads = "http://quiterss.org/ru/download";
      else urlDownloads = "http://quiterss.org/en/download";

      str =
          "<a><font color=#FF8040>" +
          tr("A new version of QuiteRSS is available!") + "</a>"
          "<p>" + QString("<a href=\"%1\">%2</a>").
          arg(urlDownloads).
          arg(tr("Click here to go to the download page"));
      newVersion = curVersion;
    }
    info =
        "<html><style>a { color: blue; text-decoration: none; }</style><body>"
        "<table><tr><td>" + tr("Your version is:") + " </td><td>" +
        "<B> " + version + "</B>" + QString(" (%1)").arg(date) + " </td></tr>" +
        "<tr><td>" + tr("Current version is:") + " </td><td>" +
        "<B>" + curVersion + "</B>" + QString(" (%1)").arg(curDate) +
        "</td></tr></table><p>" + str +
        "</body></html>";
  } else {
    qWarning() << "Error checking updates" << reply_->error() << reply_->errorString();
    info = tr("Error checking updates");
    if (showDialog_)
      history_->setText("");
  }

  Settings settings;
  bool remind = settings.value("remindAboutVersion", false).toBool();
  QString currentVersion = settings.value("currentVersionApp", "").toString();
  if (!showDialog_) {
    if (!newVersion.isEmpty()) {
      if (currentVersion != newVersion) {
        settings.setValue("currentVersionApp", newVersion);
        settings.setValue("remindAboutVersion", false);
      } else if (remind) {
          newVersion = "";
      }
    }

    emit signalNewVersion(newVersion);
  } else {
    infoLabel->setText(info);

#if defined(Q_OS_WIN)
    if (QFile::exists(QCoreApplication::applicationDirPath() + "/Updater.exe") &&
        !newVersion.isEmpty() && !mainApp->isPortableAppsCom())
      installButton_->show();
#endif

    if (!newVersion.isEmpty()) {
      if (currentVersion != newVersion) {
        settings.setValue("currentVersionApp", newVersion);
        settings.setValue("remindAboutVersion", false);
        remindAboutVersion_->setChecked(false);
        remindAboutVersion_->show();
      } else if (!remind) {
          remindAboutVersion_->show();
      }
    }
  }
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

void UpdateAppDialog::updaterRun()
{
  close();
#if defined(Q_OS_WIN)
  QString updaterFile = QCoreApplication::applicationDirPath() + "/Updater.exe";
  ShellExecute(0, 0, (wchar_t *)updaterFile.utf16(), 0, 0, SW_SHOWNORMAL);
#endif
}

void UpdateAppDialog::renderStatistics()
{
  Settings settings;
  bool updateCheckEnabled = settings.value("Settings/updateCheckEnabled", true).toBool();
  if (updateCheckEnabled || showDialog_) {
    QNetworkRequest request(QUrl("http://quiterss.org/files/updates_new/VersionNo.h"));
    reply_ = networkManagerProxy_->get(request);
    connect(reply_, SIGNAL(finished()), this, SLOT(finishUpdatesChecking()));
  } else {
    emit signalNewVersion();
  }
}
