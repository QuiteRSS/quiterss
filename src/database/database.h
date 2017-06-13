/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#ifndef DATABASE_H
#define DATABASE_H

#include <QtCore>
#include <QtSql>

class Database : public QObject
{
  Q_OBJECT
public:
  static int version();
  static void initialization();
  static QSqlDatabase connection(const QString &connectionName = QString());
  static void sqliteDBMemFile(bool save = true);
  static void setVacuum();

private:
  static void setPragma(QSqlDatabase &db);
  static void createTables(QSqlDatabase &db);
  static void prepareDatabase();
  static void createLabels(QSqlDatabase &db);

  static QStringList tablesList() {
    QStringList tables;
    tables << "feeds" << "news" << "feeds_ex"
           << "news_ex" << "filters" << "filterConditions"
           << "filterActions" << "filters_ex" << "labels"
           << "passwords" << "info";
    return tables;
  }

};

#endif // DATABASE_H
