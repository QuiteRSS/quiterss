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
#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QDir>

#define TRACKING_ID "UA-99877778-1"

#define ADBLOCK_EASYLIST_URL "https://easylist-downloads.adblockplus.org/easylist.txt"

#ifndef Q_UNLIKELY
#define Q_UNLIKELY(x) x
#endif

#ifndef QSL
#if QT_VERSION >= 0x050000
#define QSL(x) QStringLiteral(x)
#else
#define QSL(x) QLatin1String(x)
#endif
#endif

#ifndef QL1S
#define QL1S(x) QLatin1String(x)
#endif

#ifndef QL1C
#define QL1C(x) QLatin1Char(x)
#endif

namespace Common
{
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
  static const QString Qz_RAWVCS = "https://raw.githubusercontent.com/QupZilla/qupzilla/v1.8";
#endif

  bool removePath(const QString &path);
  bool matchDomain(const QString &pattern, const QString &domain);
  QString filterCharsFromFilename(const QString &name);
  QString ensureUniqueFilename(const QString &name, const QString &appendFormat = QString("(%1)"));
  void createFileBackup(const QString &oldFilename, const QString &oldVersion);

  QString readAllFileContents(const QString &filename);
  QByteArray readAllFileByteContents(const QString &filename);

  void sleep(int ms);

  QString operatingSystem();
  QString cpuArchitecture();
  QString operatingSystemLong();
}

#endif // COMMON_H
