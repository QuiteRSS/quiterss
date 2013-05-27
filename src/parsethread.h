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
#ifndef PARSETHREAD_H
#define PARSETHREAD_H

#include <QThread>

#include "parseobject.h"

class ParseThread : public QThread
{
  Q_OBJECT
public:
  explicit ParseThread(QObject *parent, const QString &dataDirPath);
  ~ParseThread();

protected:
  virtual void run();

private:
  QString dataDirPath_;
  ParseObject *parseObject_;

};

#endif // PARSETHREAD_H
