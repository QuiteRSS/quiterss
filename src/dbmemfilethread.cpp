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
#include "settings.h"

#include <QDebug>
#if defined(Q_OS_WIN) || defined(Q_OS_OS2) || defined(Q_OS_MAC)
#if QT_VERSION >= 0x050100
#include <sqlite_qt51x/sqlite3.h>
#elif QT_VERSION >= 0x040800
#include <sqlite_qt48x/sqlite3.h>
#else
#include <sqlite_qt47x/sqlite3.h>
#endif
#else
#include <sqlite3.h>
#endif

DBMemFileThread::DBMemFileThread(const QString &fileName, QObject *parent)
  : QThread(parent)
  , fileName_(fileName)
  , save_(false)
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

/*
** This function is used to load the contents of a database file on disk
** into the "main" database of open database connection pInMemory, or
** to save the current contents of the database opened by pInMemory into
** a database file on disk. pInMemory is probably an in-memory database,
** but this function will also work fine if it is not.
**
** Parameter zFilename points to a nul-terminated string containing the
** name of the database file on disk to load from or save to. If parameter
** isSave is non-zero, then the contents of the file zFilename are
** overwritten with the contents of the database opened by pInMemory. If
** parameter isSave is zero, then the contents of the database opened by
** pInMemory are replaced by data loaded from the file zFilename.
**
** If the operation is successful, SQLITE_OK is returned. Otherwise, if
** an error occurs, an SQLite error code is returned.
*/
/*virtual*/ void DBMemFileThread::run()
{
//  qDebug() << "sqliteDBMemFile(): save =" << save_;
  if (save_) qDebug() << "sqliteDBMemFile(): from memory to file";
  else qDebug() << "sqliteDBMemFile(): from file to memory" ;
  int rc = -1;                   /* Function return code */
  QVariant v = QSqlDatabase::database().driver()->handle();
  if (v.isValid() && qstrcmp(v.typeName(),"sqlite3*") == 0) {
    // v.data() returns a pointer to the handle
    sqlite3 * handle = *static_cast<sqlite3 **>(v.data());
    if (handle != 0) {  // check that it is not NULL
      sqlite3 * pInMemory = handle;
      sqlite3 *pFile;           /* Database connection opened on zFilename */
      sqlite3_backup *pBackup;  /* Backup object used to copy data */
      sqlite3 *pTo;             /* Database to copy to (pFile or pInMemory) */
      sqlite3 *pFrom;           /* Database to copy from (pFile or pInMemory) */

      /* Open the database file identified by zFilename. Exit early if this fails
      ** for any reason. */
      rc = sqlite3_open( fileName_.toUtf8().data(), &pFile );
      if (rc == SQLITE_OK) {

        /* If this is a 'load' operation (isSave==0), then data is copied
        ** from the database file just opened to database pInMemory.
        ** Otherwise, if this is a 'save' operation (isSave==1), then data
        ** is copied from pInMemory to pFile.  Set the variables pFrom and
        ** pTo accordingly. */
        pFrom = ( save_ ? pInMemory : pFile);
        pTo   = ( save_ ? pFile     : pInMemory);

        /* Set up the backup procedure to copy from the "main" database of
        ** connection pFile to the main database of connection pInMemory.
        ** If something goes wrong, pBackup will be set to NULL and an error
        ** code and  message left in connection pTo.
        **
        ** If the backup object is successfully created, call backup_step()
        ** to copy data from pFile to pInMemory. Then call backup_finish()
        ** to release resources associated with the pBackup object.  If an
        ** error occurred, then  an error code and message will be left in
        ** connection pTo. If no error occurred, then the error code belonging
        ** to pTo is set to SQLITE_OK.
        */

        pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
//        if (pBackup) {
//          qCritical() << "backuping...";
//          (void)sqlite3_backup_step(pBackup, -1);
//          (void)sqlite3_backup_finish(pBackup);
//        }
//        rc = sqlite3_errcode(pTo);

        /* Each iteration of this loop copies 5 database pages from database
        ** pDb to the backup database. If the return value of backup_step()
        ** indicates that there are still further pages to copy, sleep for
        ** 250 ms before repeating. */
        do {
          rc = sqlite3_backup_step(pBackup, 10000);

#ifndef QT_NO_DEBUG_OUTPUT
          int remaining = sqlite3_backup_remaining(pBackup);
          int pagecount = sqlite3_backup_pagecount(pBackup);
          qDebug() << rc << "backup " << pagecount << "remain" << remaining;
#endif

          if( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED ){
            sqlite3_sleep(100);
          }
        } while( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED );

        /* Release resources allocated by backup_init(). */
        (void)sqlite3_backup_finish(pBackup);
      }

      /* Close the database connection opened on database file zFilename
      ** and return the result of this function. */
      (void)sqlite3_close(pFile);

      qDebug() << "sqlite3_libversion() = " << sqlite3_libversion();
      qDebug() << "sqlite3_backup_step(): return code = " << rc;
    }
  }
  qDebug() << "sqliteDBMemFile(): return code =" << rc;
}

void DBMemFileThread::sqliteDBMemFile(bool save, QThread::Priority priority)
{
  if (isRunning()) return;

  save_ = save;
  start(priority);
}

void DBMemFileThread::startSaveTimer()
{
  if (!saveTimer_) {
    saveTimer_ = new QTimer(this);
    connect(saveTimer_, SIGNAL(timeout()), this, SLOT(sqliteDBMemFile()));
  }

  Settings settings;
  saveInterval_ = settings.value("Settings/saveDBMemFileInterval", 30).toInt();
  saveTimer_->start(saveInterval_*60*1000);
}
