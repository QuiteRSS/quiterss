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
#include "downloadmanager.h"

#include "mainapplication.h"
#include "networkmanager.h"
#include "authenticationdialog.h"
#include "downloaditem.h"
#include "settings.h"
#include "common.h"

#include <qzregexp.h>

DownloadManager::DownloadManager(QWidget *parent)
  : QWidget(parent)
{
  listWidget_ = new QListWidget();
  listWidget_->setFrameStyle(QFrame::NoFrame);

  listClaerAct_ = new QAction(QIcon(":/images/list_clear"), tr("Clear"), this);

  QToolBar *toolBar = new QToolBar(this);
  toolBar->setObjectName("newsToolBar");
  toolBar->setStyleSheet("QToolBar { border: none; padding: 0px; }");
  toolBar->setIconSize(QSize(18, 18));
  toolBar->addAction(listClaerAct_);

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->setMargin(2);
  buttonLayout->addWidget(toolBar);

  QWidget *buttonPanelWidget = new QWidget(this);
  buttonPanelWidget->setObjectName("buttonPanelWidget");
  buttonPanelWidget->setStyleSheet(
        QString("#buttonPanelWidget {border-bottom: 1px solid %1;}").
        arg(qApp->palette().color(QPalette::Dark).name()));

  buttonPanelWidget->setLayout(buttonLayout);

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setMargin(0);
  mainLayout->setSpacing(0);
  mainLayout->addWidget(buttonPanelWidget);
  mainLayout->addWidget(listWidget_);
  setLayout(mainLayout);

  connect(listClaerAct_, SIGNAL(triggered()), this, SLOT(clearList()));
  connect(this, SIGNAL(signalItemCreated(QListWidgetItem*,DownloadItem*)),
          this, SLOT(itemCreated(QListWidgetItem*,DownloadItem*)));
  connect(&updateInfoTimer_, SIGNAL(timeout()), this, SLOT(updateInfo()));

  updateInfoTimer_.start(2000);

  hide();
}

DownloadManager::~DownloadManager()
{

}

void DownloadManager::download(const QNetworkRequest &request)
{
  handleUnsupportedContent(mainApp->networkManager()->get(request), true);
}

void DownloadManager::handleUnsupportedContent(QNetworkReply* reply, bool askDownloadLocation)
{
  Settings settings;
  QString downloadLocation = mainApp->mainWindow()->downloadLocation_;
  if (askDownloadLocation || !QFile::exists(downloadLocation))
    downloadLocation = settings.value("Settings/curDownloadLocation", downloadLocation).toString();

  QString fileName(getFileName(reply));
  fileName = Common::ensureUniqueFilename(downloadLocation + "/" + fileName);
  QFileInfo fileInfo(fileName);
  if (askDownloadLocation || fileInfo.exists() ||
      !QFile::exists(downloadLocation)) {
    QString filter = QString(tr("File %1 (*.%2)") + ";;" + tr("All Files (*.*)")).
        arg(fileInfo.suffix().toUpper()).
        arg(fileInfo.suffix().toLower());
    fileName = QFileDialog::getSaveFileName(mainApp->mainWindow(),
                                            tr("Save As..."),
                                            fileName, filter);

    fileInfo.setFile(fileName);
    if (!fileName.isEmpty())
      settings.setValue("Settings/curDownloadLocation", fileInfo.absolutePath());

  }
  if (fileName.isNull()) {
    reply->abort();
    reply->deleteLater();
    return;
  }

  reply->setProperty("downloadReply", QVariant(true));
  QListWidgetItem *item = new QListWidgetItem(listWidget_);
  DownloadItem *downItem = new DownloadItem(item, reply, fileName, false);
  emit signalItemCreated(item, downItem);
}

QString DownloadManager::getFileName(QNetworkReply* reply)
{
  QString path;
  if (reply->hasRawHeader("Content-Disposition")) {
    QString value = QString::fromUtf8(reply->rawHeader("Content-Disposition"));

    if (value.contains(QzRegExp("filename\\s*\\*\\s*=\\s*UTF-8", Qt::CaseInsensitive))) {
      QzRegExp reg("filename\\s*\\*\\s*=\\s*UTF-8''([^;]*)", Qt::CaseInsensitive);
      reg.indexIn(value);
      path = QUrl::fromPercentEncoding(reg.cap(1).toUtf8()).trimmed();
    }
    else if (value.contains(QzRegExp("filename\\s*=", Qt::CaseInsensitive))) {
      QzRegExp reg("filename\\s*=([^;]*)", Qt::CaseInsensitive);
      reg.indexIn(value);
      path = QUrl::fromPercentEncoding(reg.cap(1).toUtf8()).trimmed();

      if (path.startsWith(QLatin1Char('"')) && path.endsWith(QLatin1Char('"'))) {
        path = path.mid(1, path.length() - 2);
      } else if (path.startsWith(QLatin1Char('"'))) {
        path = path.right(path.length() - 1);
      }
    }
  }

  if (path.isEmpty()) {
    path = reply->url().path();
  }

  QFileInfo info(path);
  QString baseName = info.completeBaseName();
  QString endName = info.suffix();

  if (baseName.isEmpty()) {
    baseName = "no_name";
  }

  if (!endName.isEmpty()) {
    endName.prepend(QLatin1Char('.'));
  }

  QString name = baseName + endName;

  name.replace(QzRegExp("[;:<>?\"]"), "_");

  return name;
}

void DownloadManager::startExternalApp(const QString &executable, const QUrl &url)
{
  QStringList arguments;
  arguments.append(url.toEncoded());

  bool success = QProcess::startDetached(executable, arguments);

  if (!success) {
    QString info = "<ul><li><b>%1</b>%2</li><li><b>%3</b>%4</li></ul>";
    info = info.arg(tr("Executable: "), executable,
                    tr("Arguments: "), arguments.join(QLatin1String(" ")));

    QMessageBox::critical(0, QObject::tr("Cannot start external program"),
                          QObject::tr("Cannot start external program! %1").arg(info));
  }
}

void DownloadManager::itemCreated(QListWidgetItem* item, DownloadItem* downItem)
{
  connect(downItem, SIGNAL(deleteItem(DownloadItem*)), this, SLOT(deleteItem(DownloadItem*)));

  listWidget_->setItemWidget(item, downItem);
  item->setSizeHint(downItem->sizeHint());
  downItem->show();

  emit signalShowDownloads(false);
  downItem->startDownloading();
  updateInfo();
}

void DownloadManager::deleteItem(DownloadItem* item)
{
  if (item && !item->isDownloading()) {
    delete item;
  }
}

void DownloadManager::clearList()
{
  QList<DownloadItem*> items;
  for (int i = 0; i < listWidget_->count(); i++) {
    DownloadItem* downItem = qobject_cast<DownloadItem*>(listWidget_->itemWidget(listWidget_->item(i)));
    if (!downItem) {
      continue;
    }
    if (downItem->isDownloading()) {
      continue;
    }
    items.append(downItem);
  }
  qDeleteAll(items);
}

void DownloadManager::updateInfo()
{
  QVector<QTime> remTimes;
  for (int i = 0; i < listWidget_->count(); i++) {
    DownloadItem* downItem = qobject_cast<DownloadItem*>(listWidget_->itemWidget(listWidget_->item(i)));
    if (!downItem || !downItem->isDownloading()) {
      continue;
    }
    remTimes.append(downItem->remainingTime());
  }
  QTime remaining;
  foreach (const QTime &time, remTimes) {
    if (time > remaining) {
      remaining = time;
    }
  }

  QString info;
  if (remTimes.count())
    info = QString("%1 (%2)").arg(remaining.toString("mm:ss")).arg(remTimes.count());

  emit signalUpdateInfo(info);
}

void DownloadManager::ftpAuthentication(const QUrl &url, QAuthenticator *auth)
{
  AuthenticationDialog *authenticationDialog =
      new AuthenticationDialog(url, auth);

  if (!authenticationDialog->save_->isChecked())
    authenticationDialog->exec();

  delete authenticationDialog;
}

void DownloadManager::retranslateStrings()
{
  listClaerAct_->setText(tr("Clear"));
}
