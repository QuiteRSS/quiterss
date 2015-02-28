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

#include "sqlitedriver.h"

#include "sqliteextension.h"

#include <qcoreapplication.h>
#include <qvariant.h>
#include <qsqlerror.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qsqlquery.h>
#include <qstringlist.h>
#include <qvector.h>
#include <qdebug.h>
#include <qsqldriverplugin.h>

#if defined Q_OS_WIN
# include <qt_windows.h>
#else
# include <unistd.h>
#endif

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

#ifdef HAVE_QT5
Q_DECLARE_OPAQUE_POINTER(sqlite3*)
Q_DECLARE_OPAQUE_POINTER(sqlite3_stmt*)
#endif
Q_DECLARE_METATYPE(sqlite3*)
Q_DECLARE_METATYPE(sqlite3_stmt*)

static QString _q_escapeIdentifier(const QString &identifier) 
{
    QString res = identifier;
    if(!identifier.isEmpty() && identifier.left(1) != QString(QLatin1Char('"')) && identifier.right(1) != QString(QLatin1Char('"')) ) {
        res.replace(QLatin1Char('"'), QLatin1String("\"\""));
        res.prepend(QLatin1Char('"')).append(QLatin1Char('"'));
        res.replace(QLatin1Char('.'), QLatin1String("\".\""));
    }
    return res;
}

static QVariant::Type qGetColumnType(const QString &tpName)
{
    const QString typeName = tpName.toLower();

    if (typeName == QLatin1String("integer")
        || typeName == QLatin1String("int"))
        return QVariant::Int;
    if (typeName == QLatin1String("double")
        || typeName == QLatin1String("float")
        || typeName.startsWith(QLatin1String("numeric")))
        return QVariant::Double;
    if (typeName == QLatin1String("blob"))
        return QVariant::ByteArray;
    return QVariant::String;
}

static QString stringFromUnicode(const QChar* unicode)
{
#if (QT_VERSION >= 0x040700)
    return QString(unicode);
#else
    int size = 0;
    while (unicode[size] != 0)
        size++;
    return QString(unicode, size);
#endif
}

static QSqlError qMakeError(sqlite3 *access, const QString &descr, QSqlError::ErrorType type,
                            int errorCode = -1)
{
    return QSqlError(descr,
                     stringFromUnicode(reinterpret_cast<const QChar *>(sqlite3_errmsg16(access))),
                     type, errorCode);
}

#if defined(SQLITEDRIVER_DEBUG)

static void trace( void* /*arg*/, const char* query )
{
    qDebug() << "SQLite:" << QString::fromUtf8( query );
}

#endif

class SQLiteDriverPrivate
{
public:
    inline SQLiteDriverPrivate() : access(0) {}
    sqlite3 *access;
};


class SQLiteResultPrivate
{
public:
    SQLiteResultPrivate(SQLiteResult *res);
    void cleanup();
    bool fetchNext(SqlCachedResult::ValueCache &values, int idx, bool initialFetch);
    // initializes the recordInfo and the cache
    void initColumns(bool emptyResultset);
    void finalize();

    SQLiteResult* q;
    sqlite3 *access;

    sqlite3_stmt *stmt;

    bool skippedStatus; // the status of the fetchNext() that's skipped
    bool skipRow; // skip the next fetchNext()?
    QSqlRecord rInf;
    QVector<QVariant> firstRow;
};

SQLiteResultPrivate::SQLiteResultPrivate(SQLiteResult* res) : q(res), access(0),
    stmt(0), skippedStatus(false), skipRow(false)
{
}

void SQLiteResultPrivate::cleanup()
{
    finalize();
    rInf.clear();
    skippedStatus = false;
    skipRow = false;
    q->setAt(QSql::BeforeFirstRow);
    q->setActive(false);
    q->cleanup();
}

void SQLiteResultPrivate::finalize()
{
    if (!stmt)
        return;

    sqlite3_finalize(stmt);
    stmt = 0;
}

void SQLiteResultPrivate::initColumns(bool emptyResultset)
{
    int nCols = sqlite3_column_count(stmt);
    if (nCols <= 0)
        return;

    q->init(nCols);

    for (int i = 0; i < nCols; ++i) {
        QString colName = stringFromUnicode(reinterpret_cast<const QChar *>(
                    sqlite3_column_name16(stmt, i))
                    ).remove(QLatin1Char('"'));

        // must use typeName for resolving the type to match SQLiteDriver::record
        QString typeName = stringFromUnicode(reinterpret_cast<const QChar *>(
                    sqlite3_column_decltype16(stmt, i)));

        int dotIdx = colName.lastIndexOf(QLatin1Char('.'));
        QSqlField fld(colName.mid(dotIdx == -1 ? 0 : dotIdx + 1), qGetColumnType(typeName));

        // sqlite3_column_type is documented to have undefined behavior if the result set is empty
        int stp = emptyResultset ? -1 : sqlite3_column_type(stmt, i);
        fld.setSqlType(stp);
        rInf.append(fld);
    }
}

bool SQLiteResultPrivate::fetchNext(SqlCachedResult::ValueCache &values, int idx, bool initialFetch)
{
    int res;
    int i;

    if (skipRow) {
        // already fetched
        Q_ASSERT(!initialFetch);
        skipRow = false;
        for(int i=0;i<firstRow.count();i++)
            values[i]=firstRow[i];
        return skippedStatus;
    }
    skipRow = initialFetch;

    if(initialFetch) {
        firstRow.clear();
        firstRow.resize(sqlite3_column_count(stmt));
    }

    if (!stmt) {
        q->setLastError(QSqlError(QCoreApplication::translate("SQLiteResult", "Unable to fetch row"),
                                  QCoreApplication::translate("SQLiteResult", "No query"), QSqlError::ConnectionError));
        q->setAt(QSql::AfterLastRow);
        return false;
    }
    res = sqlite3_step(stmt);

    switch(res) {
    case SQLITE_ROW:
        // check to see if should fill out columns
        if (rInf.isEmpty())
            // must be first call.
            initColumns(false);
        if (idx < 0 && !initialFetch)
            return true;
        for (i = 0; i < rInf.count(); ++i) {
            switch (sqlite3_column_type(stmt, i)) {
            case SQLITE_BLOB:
                values[i + idx] = QByteArray(static_cast<const char *>(
                            sqlite3_column_blob(stmt, i)),
                            sqlite3_column_bytes(stmt, i));
                break;
            case SQLITE_INTEGER:
                values[i + idx] = sqlite3_column_int64(stmt, i);
                break;
            case SQLITE_FLOAT:
                switch(q->numericalPrecisionPolicy()) {
                    case QSql::LowPrecisionInt32:
                        values[i + idx] = sqlite3_column_int(stmt, i);
                        break;
                    case QSql::LowPrecisionInt64:
                        values[i + idx] = sqlite3_column_int64(stmt, i);
                        break;
                    case QSql::LowPrecisionDouble:
                    case QSql::HighPrecision:
                    default:
                        values[i + idx] = sqlite3_column_double(stmt, i);
                        break;
                };
                break;
            case SQLITE_NULL:
                values[i + idx] = QVariant(QVariant::String);
                break;
            default:
                values[i + idx] = QString(reinterpret_cast<const QChar *>(
                            sqlite3_column_text16(stmt, i)),
                            sqlite3_column_bytes16(stmt, i) / sizeof(QChar));
                break;
            }
        }
        return true;
    case SQLITE_DONE:
        if (rInf.isEmpty())
            // must be first call.
            initColumns(true);
        q->setAt(QSql::AfterLastRow);
        sqlite3_reset(stmt);
        return false;
    case SQLITE_CONSTRAINT:
    case SQLITE_ERROR:
        // SQLITE_ERROR is a generic error code and we must call sqlite3_reset()
        // to get the specific error message.
        res = sqlite3_reset(stmt);
        q->setLastError(qMakeError(access, QCoreApplication::translate("SQLiteResult",
                        "Unable to fetch row"), QSqlError::ConnectionError, res));
        q->setAt(QSql::AfterLastRow);
        return false;
    case SQLITE_MISUSE:
    case SQLITE_BUSY:
    default:
        // something wrong, don't get col info, but still return false
        q->setLastError(qMakeError(access, QCoreApplication::translate("SQLiteResult",
                        "Unable to fetch row"), QSqlError::ConnectionError, res));
        sqlite3_reset(stmt);
        q->setAt(QSql::AfterLastRow);
        return false;
    }
    return false;
}

SQLiteResult::SQLiteResult(const SQLiteDriver* db)
    : SqlCachedResult(db)
{
    d = new SQLiteResultPrivate(this);
    d->access = db->d->access;
}

SQLiteResult::~SQLiteResult()
{
    d->cleanup();
    delete d;
}

#ifdef HAVE_QT5
void SQLiteResult::detachFromResultSet()
{
  if (d->stmt)
    sqlite3_reset(d->stmt);
}
#else
void SQLiteResult::virtual_hook(int id, void *data)
{
    switch (id) {
    case QSqlResult::DetachFromResultSet:
        if (d->stmt)
            sqlite3_reset(d->stmt);
        break;
    default:
        SqlCachedResult::virtual_hook(id, data);
    }
}
#endif


bool SQLiteResult::reset(const QString &query)
{
    if (!prepare(query))
        return false;
    return exec();
}

bool SQLiteResult::prepare(const QString &query)
{
    if (!driver() || !driver()->isOpen() || driver()->isOpenError())
        return false;

    d->cleanup();

    setSelect(false);

    const void *pzTail = NULL;

#if (SQLITE_VERSION_NUMBER >= 3003011)
    int res = sqlite3_prepare16_v2(d->access, query.constData(), (query.size() + 1) * sizeof(QChar),
                                   &d->stmt, &pzTail);
#else
    int res = sqlite3_prepare16(d->access, query.constData(), (query.size() + 1) * sizeof(QChar),
                                &d->stmt, &pzTail);
#endif

    if (res != SQLITE_OK) {
#if defined(SQLITEDRIVER_DEBUG)
        trace(NULL, query.toUtf8().constData());
#endif
        setLastError(qMakeError(d->access, QCoreApplication::translate("SQLiteResult",
                     "Unable to execute statement"), QSqlError::StatementError, res));
        d->finalize();
        return false;
    } else if (pzTail && !stringFromUnicode(reinterpret_cast<const QChar *>(pzTail)).trimmed().isEmpty()) {
        setLastError(qMakeError(d->access, QCoreApplication::translate("SQLiteResult",
            "Unable to execute multiple statements at a time"), QSqlError::StatementError, SQLITE_MISUSE));
        d->finalize();
        return false;
    }
    return true;
}

bool SQLiteResult::exec()
{
    const QVector<QVariant> values = boundValues();

    d->skippedStatus = false;
    d->skipRow = false;
    d->rInf.clear();
    clearValues();
    setLastError(QSqlError());

    int res = sqlite3_reset(d->stmt);
    if (res != SQLITE_OK) {
        setLastError(qMakeError(d->access, QCoreApplication::translate("SQLiteResult",
                     "Unable to reset statement"), QSqlError::StatementError, res));
        d->finalize();
        return false;
    }
    int paramCount = sqlite3_bind_parameter_count(d->stmt);
    if (paramCount == values.count()) {
        for (int i = 0; i < paramCount; ++i) {
            res = SQLITE_OK;
            const QVariant value = values.at(i);

            if (value.isNull()) {
                res = sqlite3_bind_null(d->stmt, i + 1);
            } else {
                switch (value.type()) {
                case QVariant::ByteArray: {
                    const QByteArray *ba = static_cast<const QByteArray*>(value.constData());
                    res = sqlite3_bind_blob(d->stmt, i + 1, ba->constData(),
                                            ba->size(), SQLITE_STATIC);
                    break; }
                case QVariant::Int:
                    res = sqlite3_bind_int(d->stmt, i + 1, value.toInt());
                    break;
                case QVariant::Double:
                    res = sqlite3_bind_double(d->stmt, i + 1, value.toDouble());
                    break;
                case QVariant::UInt:
                case QVariant::LongLong:
                    res = sqlite3_bind_int64(d->stmt, i + 1, value.toLongLong());
                    break;
                case QVariant::String: {
                    // lifetime of string == lifetime of its qvariant
                    const QString *str = static_cast<const QString*>(value.constData());
                    res = sqlite3_bind_text16(d->stmt, i + 1, str->utf16(),
                                              (str->size()) * sizeof(QChar), SQLITE_STATIC);
                    break; }
                default: {
                    QString str = value.toString();
                    // SQLITE_TRANSIENT makes sure that sqlite buffers the data
                    res = sqlite3_bind_text16(d->stmt, i + 1, str.utf16(),
                                              (str.size()) * sizeof(QChar), SQLITE_TRANSIENT);
                    break; }
                }
            }
            if (res != SQLITE_OK) {
                setLastError(qMakeError(d->access, QCoreApplication::translate("SQLiteResult",
                             "Unable to bind parameters"), QSqlError::StatementError, res));
                d->finalize();
                return false;
            }
        }
    } else {
        setLastError(QSqlError(QCoreApplication::translate("SQLiteResult",
                        "Parameter count mismatch"), QString(), QSqlError::StatementError));
        return false;
    }
    d->skippedStatus = d->fetchNext(d->firstRow, 0, true);
    if (lastError().isValid()) {
        setSelect(false);
        setActive(false);
        return false;
    }
    setSelect(!d->rInf.isEmpty());
    setActive(true);
    return true;
}

bool SQLiteResult::gotoNext(SqlCachedResult::ValueCache& row, int idx)
{
    return d->fetchNext(row, idx, false);
}

int SQLiteResult::size()
{
    return -1;
}

int SQLiteResult::numRowsAffected()
{
    return sqlite3_changes(d->access);
}

QVariant SQLiteResult::lastInsertId() const
{
    if (isActive()) {
        qint64 id = sqlite3_last_insert_rowid(d->access);
        if (id)
            return id;
    }
    return QVariant();
}

QSqlRecord SQLiteResult::record() const
{
    if (!isActive() || !isSelect())
        return QSqlRecord();
    return d->rInf;
}

QVariant SQLiteResult::handle() const
{
    return qVariantFromValue(d->stmt);
}

void SQLiteResult::setLastError(const QSqlError& e)
{
#if defined(SQLITEDRIVER_DEBUG)
    if (e.isValid())
        qDebug() << "SQLite error:" << e.driverText() << e.databaseText();
#endif
    SqlCachedResult::setLastError(e);
}

/////////////////////////////////////////////////////////

SQLiteDriver::SQLiteDriver(QObject * parent)
    : QSqlDriver(parent)
{
    d = new SQLiteDriverPrivate();
}

SQLiteDriver::SQLiteDriver(sqlite3 *connection, QObject *parent)
    : QSqlDriver(parent)
{
    d = new SQLiteDriverPrivate();
    d->access = connection;
    setOpen(true);
    setOpenError(false);
}


SQLiteDriver::~SQLiteDriver()
{
    delete d;
}

bool SQLiteDriver::hasFeature(DriverFeature f) const
{
    switch (f) {
    case BLOB:
    case Transactions:
    case Unicode:
    case LastInsertId:
    case PreparedQueries:
    case PositionalPlaceholders:
    case SimpleLocking:
    case FinishQuery:
    case LowPrecisionNumbers:
        return true;
    case QuerySize:
    case NamedPlaceholders:
    case BatchOperations:
    case EventNotifications:
    case MultipleResultSets:
        return false;
    default:
      break;
    }
    return false;
}

/*
   SQLite dbs have no user name, passwords, hosts or ports.
   just file names.
*/
bool SQLiteDriver::open(const QString & db, const QString &, const QString &, const QString &, int, const QString &)
{
    if (isOpen())
        close();

    if (db.isEmpty())
        return false;

    if (sqlite3_open16(db.utf16(), &d->access) == SQLITE_OK) {
        sqlite3_busy_timeout(d->access, 500);
#if defined(SQLITEDRIVER_DEBUG)
        sqlite3_trace(d->access, trace, NULL);
#endif
        setOpen(true);
        setOpenError(false);
        installSQLiteExtension(d->access);
        return true;
    } else {
        setLastError(qMakeError(d->access, tr("Error opening database"),
                     QSqlError::ConnectionError));
        setOpenError(true);
        return false;
    }
}

void SQLiteDriver::close()
{
    if (isOpen()) {
        if (sqlite3_close(d->access) != SQLITE_OK)
            setLastError(qMakeError(d->access, tr("Error closing database"),
                                    QSqlError::ConnectionError));
        d->access = 0;
        setOpen(false);
        setOpenError(false);
    }
}

QSqlResult *SQLiteDriver::createResult() const
{
    return new SQLiteResult(this);
}

bool SQLiteDriver::beginTransaction()
{
    if (!isOpen() || isOpenError())
        return false;

    QSqlQuery q(createResult());
    if (!q.exec(QLatin1String("BEGIN"))) {
        setLastError(QSqlError(tr("Unable to begin transaction"),
                               q.lastError().databaseText(), QSqlError::TransactionError));
        return false;
    }

    return true;
}

bool SQLiteDriver::commitTransaction()
{
    if (!isOpen() || isOpenError())
        return false;

    QSqlQuery q(createResult());
    if (!q.exec(QLatin1String("COMMIT"))) {
        setLastError(QSqlError(tr("Unable to commit transaction"),
                               q.lastError().databaseText(), QSqlError::TransactionError));
        return false;
    }

    return true;
}

bool SQLiteDriver::rollbackTransaction()
{
    if (!isOpen() || isOpenError())
        return false;

    QSqlQuery q(createResult());
    if (!q.exec(QLatin1String("ROLLBACK"))) {
        setLastError(QSqlError(tr("Unable to rollback transaction"),
                               q.lastError().databaseText(), QSqlError::TransactionError));
        return false;
    }

    return true;
}

QStringList SQLiteDriver::tables(QSql::TableType type) const
{
    QStringList res;
    if (!isOpen())
        return res;

    QSqlQuery q(createResult());
    q.setForwardOnly(true);

    QString sql = QLatin1String("SELECT name FROM sqlite_master WHERE %1 "
                                "UNION ALL SELECT name FROM sqlite_temp_master WHERE %1");
    if ((type & QSql::Tables) && (type & QSql::Views))
        sql = sql.arg(QLatin1String("type='table' OR type='view'"));
    else if (type & QSql::Tables)
        sql = sql.arg(QLatin1String("type='table'"));
    else if (type & QSql::Views)
        sql = sql.arg(QLatin1String("type='view'"));
    else
        sql.clear();

    if (!sql.isEmpty() && q.exec(sql)) {
        while(q.next())
            res.append(q.value(0).toString());
    }

    if (type & QSql::SystemTables) {
        // there are no internal tables beside this one:
        res.append(QLatin1String("sqlite_master"));
    }

    return res;
}

static QSqlIndex qGetTableInfo(QSqlQuery &q, const QString &tableName, bool onlyPIndex = false)
{
    QString schema;
    QString table(tableName);
    int indexOfSeparator = tableName.indexOf(QLatin1Char('.'));
    if (indexOfSeparator > -1) {
        schema = tableName.left(indexOfSeparator).append(QLatin1Char('.'));
        table = tableName.mid(indexOfSeparator + 1);
    }
    q.exec(QLatin1String("PRAGMA ") + schema + QLatin1String("table_info (") + _q_escapeIdentifier(table) + QLatin1String(")"));

    QSqlIndex ind;
    while (q.next()) {
        bool isPk = q.value(5).toInt();
        if (onlyPIndex && !isPk)
            continue;
        QString typeName = q.value(2).toString().toLower();
        QSqlField fld(q.value(1).toString(), qGetColumnType(typeName));
        if (isPk && (typeName == QLatin1String("integer")))
            // INTEGER PRIMARY KEY fields are auto-generated in sqlite
            // INT PRIMARY KEY is not the same as INTEGER PRIMARY KEY!
            fld.setAutoValue(true);
        fld.setRequired(q.value(3).toInt() != 0);
        fld.setDefaultValue(q.value(4));
        ind.append(fld);
    }
    return ind;
}

QSqlIndex SQLiteDriver::primaryIndex(const QString &tblname) const
{
    if (!isOpen())
        return QSqlIndex();

    QString table = tblname;
#if (QT_VERSION >= 0x040500)
    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);
#endif

    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    return qGetTableInfo(q, table, true);
}

QSqlRecord SQLiteDriver::record(const QString &tbl) const
{
    if (!isOpen())
        return QSqlRecord();

    QString table = tbl;
#if (QT_VERSION >= 0x040500)
    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);
#endif

    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    return qGetTableInfo(q, table);
}

QVariant SQLiteDriver::handle() const
{
    return QVariant::fromValue(d->access);
}

QString SQLiteDriver::escapeIdentifier(const QString &identifier, IdentifierType type) const
{
    Q_UNUSED(type);
    return _q_escapeIdentifier(identifier);
}

void SQLiteDriver::setLastError(const QSqlError& e)
{
#if defined(SQLITEDRIVER_DEBUG)
    if (e.isValid())
        qDebug() << "SQLite error:" << e.driverText() << e.databaseText();
#endif
    QSqlDriver::setLastError(e);
}

class SQLiteDriverPlugin : public QSqlDriverPlugin
{
public:
    SQLiteDriverPlugin();

    QSqlDriver* create(const QString &);
    QStringList keys() const;
};

SQLiteDriverPlugin::SQLiteDriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver* SQLiteDriverPlugin::create(const QString &name)
{
    if (name == QLatin1String("SQLITEX")) {
        SQLiteDriver* driver = new SQLiteDriver();
        return driver;
    }
    return 0;
}

QStringList SQLiteDriverPlugin::keys() const
{
    QStringList l;
    l << QLatin1String("SQLITEX");
    return l;
}

#ifndef HAVE_QT5
#if defined(SQLITEDRIVER_EXPORT)

Q_EXPORT_STATIC_PLUGIN(SQLiteDriverPlugin)
Q_EXPORT_PLUGIN2(sqlitex, SQLiteDriverPlugin)

#else

QObject* qt_plugin_instance_sqlitex()
{
    static QPointer<QObject> instance;
    if (!instance)
        instance = new SQLiteDriverPlugin();
    return instance;
}

Q_IMPORT_PLUGIN(sqlitex)

#endif
#endif
