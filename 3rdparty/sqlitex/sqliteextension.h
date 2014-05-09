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

#ifndef SQLITEEXTENSION_H
#define SQLITEEXTENSION_H

struct sqlite3;

/**
* Install UNICODE support collation and functions for SQLite.
* @param db The SQLite database handle.
*/
void installSQLiteExtension( sqlite3* db );

#endif
