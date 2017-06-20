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
#include "common.h"

#include <QtCore>
#include <QApplication>
#if defined Q_OS_WIN
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

#ifdef Q_OS_MAC
#include <CoreServices/CoreServices.h>
#endif

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

QString Common::readAllFileContents(const QString &filename)
{
  return QString::fromUtf8(readAllFileByteContents(filename));
}

QByteArray Common::readAllFileByteContents(const QString &filename)
{
  QFile file(filename);

  if (!filename.isEmpty() && file.open(QFile::ReadOnly)) {
    const QByteArray a = file.readAll();
    file.close();
    return a;
  }

  return QByteArray();
}

void Common::sleep(int ms)
{
#if defined(Q_OS_WIN)
  Sleep(DWORD(ms));
#else
  struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
  nanosleep(&ts, NULL);
#endif
}

QString Common::operatingSystem()
{
#ifdef Q_OS_MAC
  QString str = "Mac OS X";
  return str;
#endif
#ifdef Q_OS_LINUX
  return "Linux";
#endif
#ifdef Q_OS_BSD4
  return "BSD 4.4";
#endif
#ifdef Q_OS_BSDI
  return "BSD/OS";
#endif
#ifdef Q_OS_FREEBSD
  return "FreeBSD";
#endif
#ifdef Q_OS_HPUX
  return "HP-UX";
#endif
#ifdef Q_OS_HURD
  return "GNU Hurd";
#endif
#ifdef Q_OS_LYNX
  return "LynxOS";
#endif
#ifdef Q_OS_NETBSD
  return "NetBSD";
#endif
#ifdef Q_OS_OS2
  return "OS/2";
#endif
#ifdef Q_OS_OPENBSD
  return "OpenBSD";
#endif
#ifdef Q_OS_OSF
  return "HP Tru64 UNIX";
#endif
#ifdef Q_OS_SOLARIS
  return "Sun Solaris";
#endif
#ifdef Q_OS_UNIXWARE
  return "UnixWare 7 / Open UNIX 8";
#endif
#ifdef Q_OS_UNIX
  return "Unix";
#endif
#ifdef Q_OS_HAIKU
  return "Haiku";
#endif
#ifdef Q_OS_WIN32
  QString str = "Windows NT";

  switch (QSysInfo::windowsVersion()) {
  case QSysInfo::WV_NT:
    str.append(" 4.0");
    break;

  case QSysInfo::WV_2000:
    str.append(" 5.0");
    break;

  case QSysInfo::WV_XP:
    str.append(" 5.1");
    break;
  case QSysInfo::WV_2003:
    str.append(" 5.2");
    break;

  case QSysInfo::WV_VISTA:
    str.append(" 6.0");
    break;

  case QSysInfo::WV_WINDOWS7:
    str.append(" 6.1");
    break;

  case QSysInfo::WV_WINDOWS8:
    str.append(" 6.2");
    break;
#if QT_VERSION >= 0x050400
  case QSysInfo::WV_WINDOWS8_1:
    str.append(" 6.3");
    break;
#endif
#if QT_VERSION >= 0x050600
  case QSysInfo::WV_WINDOWS10:
    str.append(" 10.0");
    break;
#endif
  default:
    break;
  }

  return str;
#endif
}

QString Common::cpuArchitecture()
{
#if QT_VERSION >= 0x050400
  return QSysInfo::currentCpuArchitecture();
#else
  return "";
#endif
}

QString Common::operatingSystemLong()
{
  QString os = Common::operatingSystem();
#if QT_VERSION >= 0x050400
#ifdef Q_OS_UNIX
    if (QGuiApplication::platformName() == QL1S("xcb"))
        os.prepend(QL1S("X11; "));
    else if (QGuiApplication::platformName().startsWith(QL1S("wayland")))
        os.prepend(QL1S("Wayland; "));
#endif
#endif

  const QString arch = cpuArchitecture();
  if (arch.isEmpty())
    return os;
  return os + QSL(" ") + arch;
}
