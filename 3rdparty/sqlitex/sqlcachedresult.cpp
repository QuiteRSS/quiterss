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

#include "sqlcachedresult.h"

#include <qvariant.h>
#include <qdatetime.h>
#include <qvector.h>

static const uint initial_cache_size = 128;

class SqlCachedResultPrivate
{
public:
    SqlCachedResultPrivate();
    bool canSeek(int i) const;
    inline int cacheCount() const;
    void init(int count, bool fo);
    void cleanup();
    int nextIndex();
    void revertLast();

    SqlCachedResult::ValueCache cache;
    int rowCacheEnd;
    int colCount;
    bool forwardOnly;
    bool atEnd;
};

SqlCachedResultPrivate::SqlCachedResultPrivate():
    rowCacheEnd(0), colCount(0), forwardOnly(false), atEnd(false)
{
}

void SqlCachedResultPrivate::cleanup()
{
    cache.clear();
    forwardOnly = false;
    atEnd = false;
    colCount = 0;
    rowCacheEnd = 0;
}

void SqlCachedResultPrivate::init(int count, bool fo)
{
    Q_ASSERT(count);
    cleanup();
    forwardOnly = fo;
    colCount = count;
    if (fo) {
        cache.resize(count);
        rowCacheEnd = count;
    } else {
        cache.resize(initial_cache_size * count);
    }
}

int SqlCachedResultPrivate::nextIndex()
{
    if (forwardOnly)
        return 0;
    int newIdx = rowCacheEnd;
    if (newIdx + colCount > cache.size())
        cache.resize(qMin(cache.size() * 2, cache.size() + 10000));
    rowCacheEnd += colCount;

    return newIdx;
}

bool SqlCachedResultPrivate::canSeek(int i) const
{
    if (forwardOnly || i < 0)
        return false;
    return rowCacheEnd >= (i + 1) * colCount;
}

void SqlCachedResultPrivate::revertLast()
{
    if (forwardOnly)
        return;
    rowCacheEnd -= colCount;
}

inline int SqlCachedResultPrivate::cacheCount() const
{
    Q_ASSERT(!forwardOnly);
    Q_ASSERT(colCount);
    return rowCacheEnd / colCount;
}

//////////////

SqlCachedResult::SqlCachedResult(const QSqlDriver * db): QSqlResult (db)
{
    d = new SqlCachedResultPrivate();
}

SqlCachedResult::~SqlCachedResult()
{
    delete d;
}

void SqlCachedResult::init(int colCount)
{
    d->init(colCount, isForwardOnly());
}

bool SqlCachedResult::fetch(int i)
{
    if ((!isActive()) || (i < 0))
        return false;
    if (at() == i)
        return true;
    if (d->forwardOnly) {
        // speed hack - do not copy values if not needed
        if (at() > i || at() == QSql::AfterLastRow)
            return false;
        while(at() < i - 1) {
            if (!gotoNext(d->cache, -1))
                return false;
            setAt(at() + 1);
        }
        if (!gotoNext(d->cache, 0))
            return false;
        setAt(at() + 1);
        return true;
    }
    if (d->canSeek(i)) {
        setAt(i);
        return true;
    }
    if (d->rowCacheEnd > 0)
        setAt(d->cacheCount());
    while (at() < i + 1) {
        if (!cacheNext()) {
            if (d->canSeek(i))
                break;
            return false;
        }
    }
    setAt(i);

    return true;
}

bool SqlCachedResult::fetchNext()
{
    if (d->canSeek(at() + 1)) {
        setAt(at() + 1);
        return true;
    }
    return cacheNext();
}

bool SqlCachedResult::fetchPrevious()
{
    return fetch(at() - 1);
}

bool SqlCachedResult::fetchFirst()
{
    if (d->forwardOnly && at() != QSql::BeforeFirstRow) {
        return false;
    }
    if (d->canSeek(0)) {
        setAt(0);
        return true;
    }
    return cacheNext();
}

bool SqlCachedResult::fetchLast()
{
    if (d->atEnd) {
        if (d->forwardOnly)
            return false;
        else
            return fetch(d->cacheCount() - 1);
    }

    int i = at();
    while (fetchNext())
        ++i; /* brute force */
    if (d->forwardOnly && at() == QSql::AfterLastRow) {
        setAt(i);
        return true;
    } else {
        return fetch(i);
    }
}

QVariant SqlCachedResult::data(int i)
{
    int idx = d->forwardOnly ? i : at() * d->colCount + i;
    if (i >= d->colCount || i < 0 || at() < 0 || idx >= d->rowCacheEnd)
        return QVariant();

    return d->cache.at(idx);
}

bool SqlCachedResult::isNull(int i)
{
    int idx = d->forwardOnly ? i : at() * d->colCount + i;
    if (i > d->colCount || i < 0 || at() < 0 || idx >= d->rowCacheEnd)
        return true;

    return d->cache.at(idx).isNull();
}

void SqlCachedResult::cleanup()
{
    setAt(QSql::BeforeFirstRow);
    setActive(false);
    d->cleanup();
}

void SqlCachedResult::clearValues()
{
    setAt(QSql::BeforeFirstRow);
    d->rowCacheEnd = 0;
    d->atEnd = false;
}

bool SqlCachedResult::cacheNext()
{
    if (d->atEnd)
        return false;

    if(isForwardOnly()) {
        d->cache.clear();
        d->cache.resize(d->colCount);
    }

    if (!gotoNext(d->cache, d->nextIndex())) {
        d->revertLast();
        d->atEnd = true;
        return false;
    }
    setAt(at() + 1);
    return true;
}

int SqlCachedResult::colCount() const
{
    return d->colCount;
}

SqlCachedResult::ValueCache &SqlCachedResult::cache()
{
    return d->cache;
}

void SqlCachedResult::virtual_hook(int id, void *data)
{
    switch (id) {
    case QSqlResult::DetachFromResultSet:
    case QSqlResult::SetNumericalPrecision:
        cleanup();
        break;
    default:
        QSqlResult::virtual_hook(id, data);
    }
}
