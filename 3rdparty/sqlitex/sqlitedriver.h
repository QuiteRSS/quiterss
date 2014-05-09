/**************************************************************************
* Extensible SQLite driver for Qt4
* Copyright (C) 2011-2012 Michał Męciński
*
* This library is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License version 2.1
* as published by the Free Software Foundation.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* This library is based on the QtSql module of the Qt Toolkit
* Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
**************************************************************************/

#ifndef SQLITEDRIVER_H
#define SQLITEDRIVER_H

#include <QtSql/qsqldriver.h>
#include <QtSql/qsqlresult.h>

#include "sqlcachedresult.h"

struct sqlite3;

class SQLiteDriverPrivate;
class SQLiteResultPrivate;
class SQLiteDriver;

class SQLiteResult : public SqlCachedResult
{
    friend class SQLiteDriver;
    friend class SQLiteResultPrivate;
public:
    explicit SQLiteResult(const SQLiteDriver* db);
    ~SQLiteResult();
    QVariant handle() const;

protected:
    bool gotoNext(SqlCachedResult::ValueCache& row, int idx);
    bool reset(const QString &query);
    bool prepare(const QString &query);
    bool exec();
    int size();
    int numRowsAffected();
    QVariant lastInsertId() const;
    QSqlRecord record() const;
    void virtual_hook(int id, void *data);
    void setLastError(const QSqlError& e);

private:
    SQLiteResultPrivate* d;
};

class SQLiteDriver : public QSqlDriver
{
    Q_OBJECT
    friend class SQLiteResult;
public:
    explicit SQLiteDriver(QObject *parent = 0);
    explicit SQLiteDriver(sqlite3 *connection, QObject *parent = 0);
    ~SQLiteDriver();
    bool hasFeature(DriverFeature f) const;
    bool open(const QString & db,
                   const QString & user,
                   const QString & password,
                   const QString & host,
                   int port,
                   const QString & connOpts);
    void close();
    QSqlResult *createResult() const;
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    QStringList tables(QSql::TableType) const;

    QSqlRecord record(const QString& tablename) const;
    QSqlIndex primaryIndex(const QString &table) const;
    QVariant handle() const;
    QString escapeIdentifier(const QString &identifier, IdentifierType) const;

protected:
    void setLastError(const QSqlError& e);

private:
    SQLiteDriverPrivate* d;
};

#endif
