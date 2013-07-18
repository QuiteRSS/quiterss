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
#ifndef UPDATETHREAD_H
#define UPDATETHREAD_H

#include <QThread>

#include "updateobject.h"

class UpdateThread : public QThread
{
  Q_OBJECT
public:
  explicit UpdateThread(QObject *parent, int timeoutRequest = 45, int numberRequest = 1, int numberRepeats = 2);
  ~UpdateThread();

  UpdateObject *updateObject_;

protected:
  virtual void run();

private:
  int timeoutRequest_;
  int numberRequest_;
  int numberRepeats_;

};

#endif // UPDATETHREAD_H
