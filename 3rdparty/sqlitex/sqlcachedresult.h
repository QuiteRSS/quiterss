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

#ifndef SQLCACHEDRESULT_H
#define SQLCACHEDRESULT_H

#include <QtSql/qsqlresult.h>

class QVariant;
template <typename T> class QVector;

class SqlCachedResultPrivate;

class SqlCachedResult: public QSqlResult
{
public:
    virtual ~SqlCachedResult();

    typedef QVector<QVariant> ValueCache;

protected:
    SqlCachedResult(const QSqlDriver * db);

    void init(int colCount);
    void cleanup();
    void clearValues();

    virtual bool gotoNext(ValueCache &values, int index) = 0;

    QVariant data(int i);
    bool isNull(int i);
    bool fetch(int i);
    bool fetchNext();
    bool fetchPrevious();
    bool fetchFirst();
    bool fetchLast();

    int colCount() const;
    ValueCache &cache();

    void virtual_hook(int id, void *data);
private:
    bool cacheNext();
    SqlCachedResultPrivate *d;
};

#endif
