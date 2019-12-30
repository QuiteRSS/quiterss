/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2019 QuiteRSS Team <quiterssteam@gmail.com>
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
#ifndef GLOBALS_H
#define GLOBALS_H

#include <QString>

class Globals
{
public:
  Globals();

  void init();

  QString userAgent() const { return userAgent_; }

  // public on purpose
  const bool logFileOutput_;
  bool noDebugOutput_;
  bool isInit_;
  bool isPortable_;
  QString resourcesDir_;
  QString dataDir_;
  QString cacheDir_;
  QString soundNotifyDir_;

private:
  QString userAgent_;

};

extern Globals globals;

#endif // GLOBALS_H
