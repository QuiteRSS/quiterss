/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2014 QuiteRSS Team <quiterssteam@gmail.com>
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
#include "dbmemfilethread.h"

#include "mainapplication.h"
#include "database.h"
#include "settings.h"

#include <QDebug>
#if defined(Q_OS_WIN)
#include <windows.h>
#endif

DBMemFileThread::DBMemFileThread(QObject *parent)
  : QThread(parent)
  , saveTimer_(0)
{
  qDebug() << "DBMemFileThread::constructor";
}

DBMemFileThread::~DBMemFileThread()
{
  qDebug() << "DBMemFileThread::~destructor";

  exit();
  wait();
}

void DBMemFileThread::run()
{
  Database::saveMemoryDatabase();
}

void DBMemFileThread::startSaveMemoryDB(QThread::Priority priority)
{
  if (isRunning()) return;

  start(priority);

  if (priority == QThread::NormalPriority) {
    while(isRunning()) {
      int ms = 100;
#if defined(Q_OS_WIN)
      Sleep(DWORD(ms));
#else
      struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
      nanosleep(&ts, NULL);
#endif
    }
  }
}

void DBMemFileThread::startSaveTimer()
{
  if (!saveTimer_) {
    saveTimer_ = new QTimer(this);
    connect(saveTimer_, SIGNAL(timeout()), this, SLOT(startSaveMemoryDB()));
  }

  Settings settings;
  saveInterval_ = settings.value("Settings/saveDBMemFileInterval", 30).toInt();
  saveTimer_->start(saveInterval_*60*1000);
}
