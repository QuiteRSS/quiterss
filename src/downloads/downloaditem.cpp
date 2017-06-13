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
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "downloaditem.h"

#include "mainapplication.h"
#include "networkmanager.h"
#include "webpage.h"

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

DownloadItem::DownloadItem(QListWidgetItem *item,
                           QNetworkReply *reply,
                           const QString &fileName,
                           bool openAfterDownload)
  : QWidget()
  , item_(item)
  , reply_(reply)
  , ftpDownloader_(0)
  , fileName_(fileName)
  , downloadUrl_(reply->url())
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
  if (total > 0) total_ = total;

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

  bool hasFtpUrlInHeader = locationHeader.isValid() && (locationHeader.scheme() == "ftp");
  if (reply_->url().scheme() == "ftp" || hasFtpUrlInHeader) {
    QUrl url = hasFtpUrlInHeader ? locationHeader : reply_->url();
    reply_->abort();
    reply_->deleteLater();
    reply_ = 0;

    startDownloadingFromFtp(url);
    return;
  } else if (locationHeader.isValid()) {
    reply_->abort();
    reply_->deleteLater();

    reply_ = mainApp->networkManager()->get(QNetworkRequest(locationHeader));
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

void DownloadItem::startDownloadingFromFtp(const QUrl &url)
{
  if (!outputFile_.isOpen() && !outputFile_.open(QIODevice::WriteOnly)) {
    stop(false);
    downloadInfo_->setText(tr("Error: Cannot write to file!"));
    return;
  }

  ftpDownloader_ = new FtpDownloader(this);
  connect(ftpDownloader_, SIGNAL(finished()), this, SLOT(finished()));
  connect(ftpDownloader_, SIGNAL(dataTransferProgress(qint64, qint64)),
          this, SLOT(downloadProgress(qint64, qint64)));
  connect(ftpDownloader_, SIGNAL(errorOccured(QFtp::Error)), this, SLOT(error()));
  connect(ftpDownloader_, SIGNAL(ftpAuthenticationRequierd(const QUrl &, QAuthenticator*)),
          mainApp->networkManager(), SLOT(ftpAuthentication(const QUrl &, QAuthenticator*)));

  ftpDownloader_->download(url, &outputFile_);
  downloading_ = true;
  updateInfoTimer_.start(1000);

  QTimer::singleShot(200, this, SLOT(updateDownload()));

  if (ftpDownloader_->error() != QFtp::NoError) {
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
    total_ = total;
  }
  progressBar_->setValue(currentValue);
  progressBar_->setMaximum(totalValue);
  curSpeed_ = received * 1000.0 / downloadTimer_.elapsed();
  received_ = received;

  if (reply_->isFinished())
    finished();
}

void DownloadItem::metaDataChanged()
{
  QUrl locationHeader = reply_->header(QNetworkRequest::LocationHeader).toUrl();
  if (locationHeader.isValid()) {
    reply_->close();
    reply_->deleteLater();

    reply_ = mainApp->networkManager()->get(QNetworkRequest(locationHeader));
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
  QString fileSize = fileSizeToString(total_);

  if (fileSize == tr("Unknown size")) {
    fileSize = fileSizeToString(received_);
  }
  downloadInfo_->setText(QString("%1 - %2 - %3").arg(fileSize, host, QDateTime::currentDateTime().time().toString()));

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
        QMessageBox::question(item_->listWidget()->parentWidget(),
                              tr("Delete file"),
                              tr("Do you want to also delete downloaded file?"),
                              QMessageBox::Yes | QMessageBox::No);
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
  menu.addAction(tr("Cancel Downloading"), this, SLOT(stop()))->setEnabled(downloading_);
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
    QMessageBox::warning(item_->listWidget()->parentWidget(),
                         tr("Not found"),
                         tr("Sorry, the file \n %1 \n was not found!").arg(info.absoluteFilePath()));
  }
}

void DownloadItem::openFolder()
{
#ifdef Q_OS_WIN
  QString winFileName = fileName_;
  winFileName.replace(QLatin1Char('/'), "\\");
  QString shExArg = "/e,/select,\"" + winFileName + "\"";
  ShellExecute(NULL, NULL, TEXT("explorer.exe"), (wchar_t*)shExArg.utf16(), NULL, SW_SHOW);
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

QHash<QString, QAuthenticator*> FtpDownloader::ftpAuthenticatorsCache_ = QHash<QString, QAuthenticator*>();

FtpDownloader::FtpDownloader(QObject* parent)
  : QFtp(parent)
  , ftpLoginId_(-1)
  , anonymousLoginChecked_(false)
  , isFinished_(false)
  , url_(QUrl())
  , dev_(0)
  , lastError_(QFtp::NoError)
{
  connect(this, SIGNAL(commandFinished(int, bool)), this, SLOT(processCommand(int, bool)));
  connect(this, SIGNAL(done(bool)), this, SLOT(onDone(bool)));
}

void FtpDownloader::download(const QUrl &url, QIODevice* dev)
{
  url_ = url;
  dev_ = dev;
  QString server = url_.host();
  if (server.isEmpty()) {
    server = url_.toString();
  }
  int port = 21;
  if (url_.port() != -1) {
    port = url_.port();
  }

  connectToHost(server, port);
}

void FtpDownloader::setError(QFtp::Error err, const QString &errStr)
{
  lastError_ = err;
  lastErrorString_ = errStr;
}

void FtpDownloader::abort()
{
  setError(QFtp::UnknownError, tr("Canceled!"));
  QFtp::abort();
}

QFtp::Error FtpDownloader::error()
{
  if (lastError_ != QFtp::NoError && QFtp::error() == QFtp::NoError) {
    return lastError_;
  } else {
    return QFtp::error();
  }
}

QString FtpDownloader::errorString() const
{
  if (!lastErrorString_.isEmpty()
      && lastError_ != QFtp::NoError
      && QFtp::error() == QFtp::NoError) {
    return lastErrorString_;
  } else {
    return QFtp::errorString();
  }
}

void FtpDownloader::processCommand(int id, bool err)
{
  if (!url_.isValid() || url_.isEmpty() || !dev_) {
    abort();
    return;
  }

  if (err) {
    if (ftpLoginId_ == id) {
      if (!anonymousLoginChecked_) {
        anonymousLoginChecked_ = true;
        ftpAuthenticator(url_)->setUser(QString());
        ftpAuthenticator(url_)->setPassword(QString());
        ftpLoginId_ = login();
        return;
      }
      emit ftpAuthenticationRequierd(url_, ftpAuthenticator(url_));
      ftpLoginId_ = login(ftpAuthenticator(url_)->user(), ftpAuthenticator(url_)->password());
      return;
    }
    abort();
    return;
  }

  switch (currentCommand()) {
  case QFtp::ConnectToHost:
    if (!anonymousLoginChecked_) {
      anonymousLoginChecked_ = ftpAuthenticator(url_)->user().isEmpty()
          && ftpAuthenticator(url_)->password().isEmpty();
    }
    ftpLoginId_ = login(ftpAuthenticator(url_)->user(), ftpAuthenticator(url_)->password());
    break;

  case QFtp::Login:
    get(url_.path(), dev_);
    break;
  default:
    ;
  }
}

void FtpDownloader::onDone(bool err)
{
  disconnect(this, SIGNAL(done(bool)), this, SLOT(onDone(bool)));
  close();
  ftpLoginId_ = -1;
  if (err || lastError_ != QFtp::NoError) {
    emit errorOccured(error());
  }
  else {
    isFinished_ = true;
    emit finished();
  }
}

QAuthenticator *FtpDownloader::ftpAuthenticator(const QUrl &url)
{
  QString key = url.host();
  if (key.isEmpty()) {
    key = url.toString();
  }
  if (!ftpAuthenticatorsCache_.contains(key) || !ftpAuthenticatorsCache_.value(key, 0)) {
    QAuthenticator* auth = new QAuthenticator();
    auth->setUser(url.userName());
    auth->setPassword(url.password());
    ftpAuthenticatorsCache_.insert(key, auth);
  }

  return ftpAuthenticatorsCache_.value(key);
}
