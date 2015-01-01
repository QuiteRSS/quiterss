/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2015 QuiteRSS Team <quiterssteam@gmail.com>
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
#include "common.h"

#include <QtCore>

bool Common::removePath(const QString &path)
{
  bool result = true;
  QFileInfo info(path);
  if (info.isDir()) {
    QDir dir(path);
    foreach (const QString &entry, dir.entryList(QDir::AllDirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot)) {
      result &= removePath(dir.absoluteFilePath(entry));
    }
    if (!info.dir().rmdir(info.fileName()))
      return false;
  } else {
    result = QFile::remove(path);
  }
  return result;
}

/** @brief Matches domain (assumes both pattern and domain not starting with dot)
 * @param pattern = domain to be matched
 * @param domain = site domain
 *----------------------------------------------------------------------------*/
bool Common::matchDomain(const QString &pattern, const QString &domain)
{
  if (pattern == domain) {
    return true;
  }

  if (!domain.endsWith(pattern)) {
    return false;
  }

  int index = domain.indexOf(pattern);

  return index > 0 && domain[index - 1] == QLatin1Char('.');
}

QString Common::filterCharsFromFilename(const QString &name)
{
  QString value = name;

  value.replace(QLatin1Char('/'), QLatin1Char('-'));
  value.remove(QLatin1Char('\\'));
  value.remove(QLatin1Char(':'));
  value.remove(QLatin1Char('*'));
  value.remove(QLatin1Char('?'));
  value.remove(QLatin1Char('"'));
  value.remove(QLatin1Char('<'));
  value.remove(QLatin1Char('>'));
  value.remove(QLatin1Char('|'));

  return value;
}

QString Common::ensureUniqueFilename(const QString &name, const QString &appendFormat)
{
  if (!QFile::exists(name)) {
    return name;
  }

  QString tmpFileName = name;
  int i = 1;
  while (QFile::exists(tmpFileName)) {
    tmpFileName = name;
    int index = tmpFileName.lastIndexOf(QLatin1Char('.'));

    QString appendString = appendFormat.arg(i);
    if (index == -1) {
      tmpFileName.append(appendString);
    }
    else {
      tmpFileName = tmpFileName.left(index) + appendString + tmpFileName.mid(index);
    }
    i++;
  }
  return tmpFileName;
}

/** Create backup copy of file
 *
 *  Backup filename format:
 *  <old-filename>_<file-version>_<backup-creation-time>.bak
 * @param oldFilename absolute path of file to backup
 * @param oldVersion version of file to backup
 *----------------------------------------------------------------------------*/
void Common::createFileBackup(const QString &oldFilename, const QString &oldVersion)
{
  QFileInfo fileInfo(oldFilename);

  // Create backup folder inside DB-file folder
  QDir backupDir(fileInfo.absoluteDir());
  if (!backupDir.exists("backup"))
    backupDir.mkpath("backup");
  backupDir.cd("backup");

  // Delete old files
  QStringList fileNameList = backupDir.entryList(QStringList(QString("%1*").arg(fileInfo.fileName())),
                                                 QDir::Files, QDir::Time);
  int count = 0;
  foreach (QString fileName, fileNameList) {
    count++;
    if (count >= 4) {
      QFile::remove(backupDir.absolutePath() + '/' + fileName);
    }
  }

  // Create backup
  QString backupFilename(backupDir.absolutePath() + '/' + fileInfo.fileName());
  backupFilename.append(QString("_%1_%2.bak")
                        .arg(oldVersion)
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")));
  QFile::copy(oldFilename, backupFilename);
}
