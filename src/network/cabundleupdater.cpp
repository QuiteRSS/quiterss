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
* Copyright (C) 2010-2014  David Rosca <nowrep@gmail.com>
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
#include "cabundleupdater.h"
#include "mainapplication.h"
#include "networkmanager.h"
#include "common.h"

#include <QTimer>
#include <QDateTime>
#include <QNetworkReply>
#include <QDebug>
#include <QFile>

CaBundleUpdater::CaBundleUpdater(NetworkManager* manager, QObject* parent)
  : QObject(parent)
  , m_manager(manager)
  , m_progress(Start)
  , m_reply(0)
  , m_latestBundleVersion(0)
{
  m_bundleVersionFileName = mainApp->dataDir() + "/certificates/bundle_version";
  m_bundleFileName = mainApp->dataDir() + "/certificates/ca-bundle.crt";
  m_lastUpdateFileName = mainApp->dataDir() + "/certificates/last_update";

  int updateTime = 30 * 1000;

  // Check immediately on first run
  if (!QFile(m_lastUpdateFileName).exists()) {
    updateTime = 0;
  }

  QTimer::singleShot(updateTime, this, SLOT(start()));
}

void CaBundleUpdater::start()
{
  QFile updateFile(m_lastUpdateFileName);
  bool updateNow = false;

  if (updateFile.exists()) {
    if (!updateFile.open(QFile::ReadOnly)) {
      qWarning() << "CaBundleUpdater::start cannot open file for reading" << m_lastUpdateFileName;
    }
    else {
      QDateTime updateTime = QDateTime::fromString(updateFile.readAll());
      updateNow = updateTime.addDays(5) < QDateTime::currentDateTime();
    }
  }
  else {
    updateNow = true;
  }

  if (updateNow) {
    m_progress = CheckLastUpdate;

    QUrl url = QUrl::fromEncoded(QString(Common::Qz_RAWVCS + "/src/lib/data/data/bundle_version").toUtf8());
    m_reply = m_manager->get(QNetworkRequest(url));
    connect(m_reply, SIGNAL(finished()), this, SLOT(replyFinished()));
  }
}

void CaBundleUpdater::replyFinished()
{
  if (m_progress == CheckLastUpdate) {
    QByteArray response = m_reply->readAll().trimmed();
    m_reply->close();
    m_reply->deleteLater();

    if (m_reply->error() != QNetworkReply::NoError || response.isEmpty()) {
      return;
    }

    m_latestBundleVersion = response.toInt();
    int currentBundleVersion = Common::readAllFileContents(m_bundleVersionFileName).trimmed().toInt();

    if (m_latestBundleVersion == 0) {
      return;
    }

    if (m_latestBundleVersion > currentBundleVersion) {
      m_progress = LoadBundle;

      QUrl url = QUrl::fromEncoded(QString(Common::Qz_RAWVCS + "/src/lib/data/data/ca-bundle.crt").toUtf8());
      m_reply = m_manager->get(QNetworkRequest(url));
      connect(m_reply, SIGNAL(finished()), this, SLOT(replyFinished()));
    }

    QFile file(m_lastUpdateFileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
      qWarning() << "CaBundleUpdater::replyFinished cannot open file for writing" << m_lastUpdateFileName;
    }

    file.write(QDateTime::currentDateTime().toString().toUtf8());
  }
  else if (m_progress == LoadBundle) {
    QByteArray response = m_reply->readAll();
    m_reply->close();
    m_reply->deleteLater();

    if (m_reply->error() != QNetworkReply::NoError || response.isEmpty()) {
      return;
    }

    QFile file(m_bundleVersionFileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
      qWarning() << "CaBundleUpdater::replyFinished cannot open file for writing" << m_bundleVersionFileName;
    }

    file.write(QByteArray::number(m_latestBundleVersion));
    file.close();

    file.setFileName(m_bundleFileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
      qWarning() << "CaBundleUpdater::replyFinished cannot open file for writing" << m_bundleFileName;
    }

    file.write(response);

    // Reload newly downloaded certificates
    mainApp->networkManager()->loadSettings();
  }
}
