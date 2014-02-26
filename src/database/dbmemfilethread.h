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
#ifndef DBMEMFILETHREAD_H
#define DBMEMFILETHREAD_H

#include <QThread>
#include <QtSql>

class DBMemFileThread : public QThread
{
  Q_OBJECT
public:
  explicit DBMemFileThread(QObject *parent);
  ~DBMemFileThread();

public slots:
  void startSaveMemoryDB(QThread::Priority priority = QThread::LowestPriority);
  void startSaveTimer();

private:
  void run();

  int saveInterval_;
  QTimer *saveTimer_;

};

#endif // DBMEMFILETHREAD_H
