/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2013  David Rosca <nowrep@gmail.com>
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */

#include "downloaditem.h"
#include "webpage.h"
#include "downloadmanager.h"
#include "networkmanager.h"

#ifdef Q_OS_WIN
#include "Shlwapi.h"
#endif

DownloadItem::DownloadItem(QListWidgetItem *item,
                           QNetworkReply *reply,
                           const QString &fileName,
                           bool openAfterDownload,
                           DownloadManager *manager)
  : QWidget()
  , item_(item)
  , reply_(reply)
  , fileName_(fileName)
  , downloadUrl_(reply->url())
  , manager_(manager)
  , downloading_(false)
  , openAfterFinish_(openAfterDownload)
  , downloadStopped_(false)
  , received_(0)
  , total_(0)
{
  downloadTimer_.start();

  if (QFile::exists(fileName)) {
    QFile::remove(fileName);
  }
  qApp->processEvents();

  outputFile_.setFileName(fileName);

  qint64 total = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
  if (total > 0) {
    total_ = total;
  }

  fileNameLabel_ = new QLabel();
  QFileInfo info(fileName);
  fileNameLabel_->setText(info.fileName());
  QFont font = fileNameLabel_->font();
  font.setBold(true);
  fileNameLabel_->setFont(font);

  progressBar_ = new QProgressBar();
  progressBar_->setObjectName("progressBar_");
  progressBar_->setTextVisible(false);
  progressBar_->setFixedHeight(10);
  progressBar_->setFixedWidth(300);
  progressBar_->setMinimum(0);
  progressBar_->setMaximum(0);
  progressBar_->setValue(0);

  QHBoxLayout *progressLayout = new QHBoxLayout();
  progressLayout->setMargin(0);
  progressLayout->addWidget(progressBar_);
  progressLayout->addStretch();

  progressFrame_ = new QFrame();
  progressFrame_->setLayout(progressLayout);

  downloadInfo_ = new QLabel();
  downloadInfo_->setText(tr("Remaining time unavailable"));

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setMargin(5);
  mainLayout->setSpacing(5);
  mainLayout->addWidget(fileNameLabel_);
  mainLayout->addWidget(downloadInfo_);
  mainLayout->addWidget(progressFrame_);
  setLayout(mainLayout);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(QPoint)),
          this, SLOT(customContextMenuRequested(QPoint)));
  connect(&updateInfoTimer_, SIGNAL(timeout()), this, SLOT(updateInfo()));
}

DownloadItem::~DownloadItem()
{
  delete item_;
}

void DownloadItem::startDownloading()
{
  QUrl locationHeader = reply_->header(QNetworkRequest::LocationHeader).toUrl();

  if (locationHeader.isValid()) {
    reply_->abort();
    reply_->deleteLater();

    reply_ = manager_->networkManager_->get(QNetworkRequest(locationHeader));
  }

  reply_->setParent(this);
  connect(reply_, SIGNAL(readyRead()), this, SLOT(readyRead()));
  connect(reply_, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
  connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error()));
  connect(reply_, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
  connect(reply_, SIGNAL(finished()), this, SLOT(finished()));

  downloading_ = true;
  updateInfoTimer_.start(1000);
  readyRead();
  QTimer::singleShot(200, this, SLOT(updateDownload()));

  if (reply_->error() != QNetworkReply::NoError) {
    stop(false);
    error();
  }
}

void DownloadItem::readyRead()
{
  if (!outputFile_.isOpen() && !outputFile_.open(QIODevice::WriteOnly)) {
    stop(false);
    downloadInfo_->setText(tr("Error: Cannot write to file!"));
    return;
  }
  outputFile_.write(reply_->readAll());
}

void DownloadItem::downloadProgress(qint64 received, qint64 total)
{
  qint64 currentValue = 0;
  qint64 totalValue = 0;
  if (total > 0) {
    currentValue = received * 100 / total;
    totalValue = 100;
  }
  progressBar_->setValue(currentValue);
  progressBar_->setMaximum(totalValue);
  curSpeed_ = received * 1000.0 / downloadTimer_.elapsed();
  received_ = received;
  total_ = total;
}

void DownloadItem::metaDataChanged()
{
  QUrl locationHeader = reply_->header(QNetworkRequest::LocationHeader).toUrl();
  if (locationHeader.isValid()) {
    reply_->close();
    reply_->deleteLater();

    reply_ = manager_->networkManager_->get(QNetworkRequest(locationHeader));
    startDownloading();
  }
}

void DownloadItem::error()
{
  if (reply_ && reply_->error() != QNetworkReply::NoError) {
    stop(false);
    downloadInfo_->setText(tr("Error: ") + reply_->errorString());
  }
}

void DownloadItem::finished()
{
  updateInfoTimer_.stop();

  QString host = downloadUrl_.host();
  downloadInfo_->setText(tr("Done - %1").arg(host));
  progressFrame_->hide();
  item_->setSizeHint(sizeHint());
  outputFile_.close();

  reply_->deleteLater();

  downloading_ = false;

  if (openAfterFinish_) {
    openFile();
  }

  emit downloadFinished(true);
}

QString DownloadItem::remaingTimeToString(QTime time)
{
  if (time < QTime(0, 0, 10)) {
    return tr("few seconds");
  } else if (time < QTime(0, 1)) {
    return time.toString("s") + " " + tr("seconds");
  } else if (time < QTime(1, 0)) {
    return time.toString("m") + " " + tr("minutes");
  } else {
    return time.toString("h") + " " + tr("hours");
  }
}

QString DownloadItem::fileSizeToString(qint64 size)
{
  if (size < 0) {
    return tr("Unknown size");
  }
  double correctSize = size / 1024.0; // KB
  if (correctSize < 1000) {
    return QString::number(correctSize > 1 ? correctSize : 1, 'f', 0) + " KB";
  }
  correctSize /= 1024; // MB
  if (correctSize < 1000) {
    return QString::number(correctSize, 'f', 1) + " MB";
  }
  correctSize /= 1024; // GB
  return QString::number(correctSize, 'f', 2) + " GB";
}

QString DownloadItem::currentSpeedToString(double speed)
{
  if (speed < 0) {
    return tr("Unknown speed");
  }
  speed /= 1024; // kB
  if (speed < 1000) {
    return QString::number(speed, 'f', 0) + " kB/s";
  }
  speed /= 1024; //MB
  if (speed < 1000) {
    return QString::number(speed, 'f', 2) + " MB/s";
  }
  speed /= 1024; //GB
  return QString::number(speed, 'f', 2) + " GB/s";
}

void DownloadItem::updateInfo()
{
  int estimatedTime = ((total_ - received_) / 1024) / (curSpeed_ / 1024);
  QString speed = currentSpeedToString(curSpeed_);

  QTime time;
  time = time.addSecs(estimatedTime);
  QString remTime = remaingTimeToString(time);
  remTime_ = time;

  QString curSize = fileSizeToString(received_);
  QString fileSize = fileSizeToString(total_);

  if (fileSize == tr("Unknown size")) {
    downloadInfo_->setText(tr("%2 - unknown size (%3)").arg(curSize, speed));
  } else {
    downloadInfo_->setText(tr("Remaining %1 - %2 of %3 (%4)").arg(remTime, curSize, fileSize, speed));
  }
}

void DownloadItem::stop(bool askForDeleteFile)
{
  if (downloadStopped_)
    return;

  downloadStopped_ = true;
  QString host = downloadUrl_.host();

  openAfterFinish_ = false;
  updateInfoTimer_.stop();
  reply_->abort();
  reply_->deleteLater();

  outputFile_.close();
  QString outputfile = QFileInfo(outputFile_).absoluteFilePath();
  downloadInfo_->setText(tr("Cancelled - %1").arg(host));
  progressFrame_->hide();
  item_->setSizeHint(sizeHint());

  downloading_ = false;

  emit downloadFinished(false);

  if (askForDeleteFile) {
    QMessageBox::StandardButton button =
        QMessageBox::question(manager_, tr("Delete file"), tr("Do you want to also delete dowloaded file?"), QMessageBox::Yes | QMessageBox::No);
    if (button == QMessageBox::Yes) {
      QFile::remove(outputfile);
    }
  }
}

/*virtual*/ void DownloadItem::mouseDoubleClickEvent(QMouseEvent* event)
{
  openFile();
  event->accept();
}

void DownloadItem::customContextMenuRequested(const QPoint &pos)
{
  QMenu menu;
  menu.addAction( tr("Open File"), this, SLOT(openFile()));

  menu.addAction(tr("Open Folder"), this, SLOT(openFolder()));
  menu.addSeparator();
  menu.addAction(tr("Copy Download Link"), this, SLOT(copyDownloadLink()));
  menu.addSeparator();
  menu.addAction(tr("Cancel downloading"), this, SLOT(stop()))->setEnabled(downloading_);
  menu.addAction(tr("Remove"), this, SLOT(clear()))->setEnabled(!downloading_);

  if (downloading_ || downloadInfo_->text().startsWith(tr("Cancelled")) || downloadInfo_->text().startsWith(tr("Error"))) {
    menu.actions().at(0)->setEnabled(false);
  }
  menu.exec(mapToGlobal(pos));
}

void DownloadItem::copyDownloadLink()
{
  QApplication::clipboard()->setText(downloadUrl_.toString());
}

void DownloadItem::clear()
{
  emit deleteItem(this);
}

void DownloadItem::openFile()
{
  if (downloading_) {
    return;
  }
  QFileInfo info(fileName_);
  if (info.exists()) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.absoluteFilePath()));
  } else {
    QMessageBox::warning(manager_, tr("Not found"), tr("Sorry, the file \n %1 \n was not found!").arg(info.absoluteFilePath()));
  }
}

void DownloadItem::openFolder()
{
#ifdef Q_OS_WIN
  QString winFileName = fileName_;
  winFileName.replace(QLatin1Char('/'), "\\");
  QString shExArg = "/e,/select,\"" + winFileName + "\"";
  ShellExecute(NULL, NULL, TEXT("explorer.exe"), shExArg.toStdWString().c_str(), NULL, SW_SHOW);
#else
  QFileInfo info(fileName_);
  QDesktopServices::openUrl(QUrl::fromLocalFile(info.path()));
#endif
}

void DownloadItem::updateDownload()
{
  if ((progressBar_->maximum() == 0) && outputFile_.isOpen() &&
      (reply_ && reply_->isFinished())) {
    downloadProgress(0, 0);
    finished();
  }
}
