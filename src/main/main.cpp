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
#include "mainapplication.h"
#include "logfile.h"

const bool logFileOutput = 1;

int main(int argc, char **argv)
{
  if (logFileOutput) {
#if defined(HAVE_QT5)
    qInstallMessageHandler(LogFile::msgHandler);
#else
    qInstallMsgHandler(LogFile::msgHandler);
#endif
  }

  MainApplication app(argc, argv);

  if (app.isClosing())
    return 0;

  return app.exec();
}
