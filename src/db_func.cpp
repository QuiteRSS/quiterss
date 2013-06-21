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
/** @file db_func.cpp
 * DB file handling functions
 * @date created 17.04.2012
 *---------------------------------------------------------------------------*/
#include <QtCore>
#include <QtSql>

#include "VersionNo.h"

QString kDbName    = "feeds.db";  ///< DB filename
QString kDbVersion = "0.12.1";    ///< Current DB version

const QString kCreateFeedsTableQuery_v0_1_0(
    "CREATE TABLE feeds("
    "id integer primary key, "
    "text varchar, "             // Feed text (replaces title at the moment)
    "title varchar, "            // Feed title
    "description varchar, "      // Feed description
    "xmlUrl varchar, "           // URL-link of the feed
    "htmlUrl varchar, "          // URL-link site, that contains the feed
    "language varchar, "         // Feed language
    "copyrights varchar, "       // Feed copyrights
    "author_name varchar, "      // Feed author: name
    "author_email varchar, "     //              e-mail
    "author_uri varchar, "       //              personal web page
    "webMaster varchar, "        // e-mail of feed's technical support
    "pubdate varchar, "          // Feed publication timestamp
    "lastBuildDate varchar, "    // Timestamp of last modification of the feed
    "category varchar, "         // Categories of content of the feed
    "contributor varchar, "      // Feed contributors (tab separated)
    "generator varchar, "        // Application has used to generate the feed
    "docs varchar, "             // URL-link to document describing RSS-standart
    "cloud_domain varchar, "     // Web-service providing rssCloud interface
    "cloud_port varchar, "       //   .
    "cloud_path varchar, "       //   .
    "cloud_procedure varchar, "  //   .
    "cloud_protocal varchar, "   //   .
    "ttl integer, "              // Time in minutes the feed can be cached
    "skipHours varchar, "        // Tip for aggregators, not to update the feed (specify hours of the day that can be skipped)
    "skipDays varchar, "         // Tip for aggregators, not to update the feed (specify day of the week that can be skipped)
    "image blob, "               // gif, jpeg, png picture, that can be associated with the feed
    "unread integer, "           // number of unread news
    "newCount integer, "         // number of new news
    "currentNews integer, "      // current displayed news
    "label varchar"              // user purpose label(s)
    ")");

const QString kCreateFeedsTableQuery_v0_9_0(
    "CREATE TABLE feeds("
    "id integer primary key, "
    "text varchar, "             // Feed text (replaces title at the moment)
    "title varchar, "            // Feed title
    "description varchar, "      // Feed description
    "xmlUrl varchar, "           // URL-link of the feed
    "htmlUrl varchar, "          // URL-link site, that contains the feed
    "language varchar, "         // Feed language
    "copyrights varchar, "       // Feed copyrights
    "author_name varchar, "      // Feed author: name
    "author_email varchar, "     //              e-mail
    "author_uri varchar, "       //              personal web page
    "webMaster varchar, "        // e-mail of feed's technical support
    "pubdate varchar, "          // Feed publication timestamp
    "lastBuildDate varchar, "    // Timestamp of last modification of the feed
    "category varchar, "         // Categories of content of the feed
    "contributor varchar, "      // Feed contributors (tab separated)
    "generator varchar, "        // Application has used to generate the feed
    "docs varchar, "             // URL-link to document describing RSS-standart
    "cloud_domain varchar, "     // Web-service providing rssCloud interface
    "cloud_port varchar, "       //   .
    "cloud_path varchar, "       //   .
    "cloud_procedure varchar, "  //   .
    "cloud_protocal varchar, "   //   .
    "ttl integer, "              // Time in minutes the feed can be cached
    "skipHours varchar, "        // Tip for aggregators, not to update the feed (specify hours of the day that can be skipped)
    "skipDays varchar, "         // Tip for aggregators, not to update the feed (specify day of the week that can be skipped)
    "image blob, "               // gif, jpeg, png picture, that can be associated with the feed
    "unread integer, "           // number of unread news
    "newCount integer, "         // number of new news
    "currentNews integer, "      // current displayed news
    "label varchar, "            // user purpose label(s)
    // -- added in v0.9.0 --
    "undeleteCount integer, "    // number of all news (not marked deleted)
    "tags varchar, "             // user purpose tags
    // --- Categories ---
    "hasChildren int, "  // Children presence
    "parentId int, "     // parent id of the feed
    "rowToParent int, "  // sequence number relative to parent
    // --- RSSowl::General ---
    "updateIntervalEnable int, "    // auto update enable flag
    "updateInterval int, "          // auto update interval
    "updateIntervalType varchar, "  // auto update interval type(minutes, hours,...)
    "updateOnStartup int, "         // update the feed on aplication startup
    "displayOnStartup int, "        // show the feed in separate tab in application startup
    // --- RSSowl::Reading ---
    "markReadAfterSecondsEnable int, "    // Enable "Read" timer
    "markReadAfterSeconds int, "          // Number of seconds that must elapse to mark news "Read"
    "markReadInNewspaper int, "           // mark Read when Newspaper layout
    "markDisplayedOnSwitchingFeed int, "  // mark Read on switching to another feed
    "markDisplayedOnClosingTab int, "     // mark Read on tab closing
    "markDisplayedOnMinimize int, "       // mark Read on minimizing to tray
    // --- RSSowl::Display ---
    "layout text, "      // news display layout
    "filter text, "      // news display filter
    "groupBy int, "      // column number to sort by
    "displayNews int, "  // 0 - display content from news; 1 - download content from link
    "displayEmbeddedImages int, "   // display images embedded in news
    "loadTypes text, "              // type of content to load ("images" or "images sounds")
    "openLinkOnEmptyContent int, "  // load link, if content is empty
    // --- RSSowl::Columns ---
    "columns text, "  // columns list and order of the news displayed in list
    "sort text, "     // column name to sort by
    "sortType int, "  // sort type (ascend, descend)
    // --- RSSowl::Clean Up ---
    "maximumToKeep int, "           // maximum number of news to keep
    "maximumToKeepEnable int, "     // enable limitation
    "maximumAgeOfNews int, "        // maximum store time of the news
    "maximumAgoOfNewEnable int, "   // enable limitation
    "deleteReadNews int, "          // delete read news
    "neverDeleteUnreadNews int, "   // don't delete unread news
    "neverDeleteStarredNews int, "  // don't delete starred news
    "neverDeleteLabeledNews int, "  // don't delete labeled news
    // --- RSSowl::Status ---
    "status text, "                 // last update result
    "created text, "                // feed creation timestamp
    "updated text, "                // last update timestamp
    "lastDisplayed text "           // last display timestamp
    ")");

const QString kCreateFeedsTableQuery(
    "CREATE TABLE feeds("
    "id integer primary key, "
    "text varchar, "             // Feed text (replaces title at the moment)
    "title varchar, "            // Feed title
    "description varchar, "      // Feed description
    "xmlUrl varchar, "           // URL-link of the feed
    "htmlUrl varchar, "          // URL-link site, that contains the feed
    "language varchar, "         // Feed language
    "copyrights varchar, "       // Feed copyrights
    "author_name varchar, "      // Feed author: name
    "author_email varchar, "     //              e-mail
    "author_uri varchar, "       //              personal web page
    "webMaster varchar, "        // e-mail of feed's technical support
    "pubdate varchar, "          // Feed publication timestamp
    "lastBuildDate varchar, "    // Timestamp of last modification of the feed
    "category varchar, "         // Categories of content of the feed
    "contributor varchar, "      // Feed contributors (tab separated)
    "generator varchar, "        // Application has used to generate the feed
    "docs varchar, "             // URL-link to document describing RSS-standart
    "cloud_domain varchar, "     // Web-service providing rssCloud interface
    "cloud_port varchar, "       //   .
    "cloud_path varchar, "       //   .
    "cloud_procedure varchar, "  //   .
    "cloud_protocal varchar, "   //   .
    "ttl integer, "              // Time in minutes the feed can be cached
    "skipHours varchar, "        // Tip for aggregators, not to update the feed (specify hours of the day that can be skipped)
    "skipDays varchar, "         // Tip for aggregators, not to update the feed (specify day of the week that can be skipped)
    "image blob, "               // gif, jpeg, png picture, that can be associated with the feed
    "unread integer, "           // number of unread news
    "newCount integer, "         // number of new news
    "currentNews integer, "      // current displayed news
    "label varchar, "            // user purpose label(s)
    // -- added in v0.9.0 --
    "undeleteCount integer, "    // number of all news (not marked deleted)
    "tags varchar, "             // user purpose tags
    // --- Categories ---
    "hasChildren integer default 0, "  // Children presence. Default - none
    "parentId integer default 0, "     // parent id of the feed. Default - tree root
    "rowToParent integer, "            // sequence number relative to parent
    // --- RSSowl::General ---
    "updateIntervalEnable int, "    // auto update enable flag
    "updateInterval int, "          // auto update interval
    "updateIntervalType varchar, "  // auto update interval type(minutes, hours,...)
    "updateOnStartup int, "         // update the feed on aplication startup
    "displayOnStartup int, "        // show the feed in separate tab in application startup
    // --- RSSowl::Reading ---
    "markReadAfterSecondsEnable int, "    // Enable "Read" timer
    "markReadAfterSeconds int, "          // Number of seconds that must elapse to mark news "Read"
    "markReadInNewspaper int, "           // mark Read when Newspaper layout
    "markDisplayedOnSwitchingFeed int, "  // mark Read on switching to another feed
    "markDisplayedOnClosingTab int, "     // mark Read on tab closing
    "markDisplayedOnMinimize int, "       // mark Read on minimizing to tray
    // --- RSSowl::Display ---
    "layout text, "      // news display layout
    "filter text, "      // news display filter
    "groupBy int, "      // column number to sort by
    "displayNews int, "  // 0 - display content from news; 1 - download content from link
    "displayEmbeddedImages integer default 1, "  // display images embedded in news
    "loadTypes text, "                           // type of content to load ("images" or "images sounds" - images only or images and sound)
    "openLinkOnEmptyContent int, "               // load link, if content is empty
    // --- RSSowl::Columns ---
    "columns text, "  // columns list and order of the news displayed in list
    "sort text, "     // column name to sort by
    "sortType int, "  // sort type (ascend, descend)
    // --- RSSowl::Clean Up ---
    "maximumToKeep int, "           // maximum number of news to keep
    "maximumToKeepEnable int, "     // enable limitation
    "maximumAgeOfNews int, "        // maximum store time of the news
    "maximumAgoOfNewEnable int, "   // enable limitation
    "deleteReadNews int, "          // delete read news
    "neverDeleteUnreadNews int, "   // don't delete unread news
    "neverDeleteStarredNews int, "  // don't delete starred news
    "neverDeleteLabeledNews int, "  // don't delete labeled news
    // --- RSSowl::Status ---
    "status text, "                 // last update result
    "created text, "                // feed creation timestamp
    "updated text, "                // last update timestamp
    "lastDisplayed text, "           // last display timestamp
    // -- added in v0.10.0 --`
    "f_Expanded integer default 1, "  // expand folder flag
    "flags text, "                    // more flags (example "focused", "hidden")
    "authentication integer default 0, "    // enable authentification, sets on feed creation
    "duplicateNewsMode integer default 0, " // news duplicates process mode
    "typeFeed integer default 0 "           // reserved for future purposes

    // -- changed in v0.10.0 -- corrected above
    // "displayEmbeddedImages integer default 1, "  // display images embedded in news
    // "hasChildren integer default 0, "  // Children presence. Default - none
    // "parentId integer default 0, "     // parent id of the feed. Default - tree root
    ")");

const QString kCreateNewsTableQuery_v0_1_0(
    "CREATE TABLE feed_%1("
    "id integer primary key, "
    "feed integer, "                       // feed id from feed table
    "guid varchar, "                       // news unique number
    "guidislink varchar default 'true', "  // flag shows that news unique number is URL-link to news
    "description varchar, "                // brief description
    "content varchar, "                    // full content (atom)
    "title varchar, "                      // title
    "published varchar, "                  // publish timestamp
    "modified varchar, "                   // modification timestamp
    "received varchar, "                   // recieve news timestamp (set on recieve)
    "author_name varchar, "                // author name
    "author_uri varchar, "                 // author web page (atom)
    "author_email varchar, "               // author e-mail (atom)
    "category varchar, "                   // category. May be several item tabs separated
    "label varchar, "                      // label (user purpose label(s))
    "new integer default 1, "              // Flag "new". Set on receive, reset on application close
    "read integer default 0, "             // Flag "read". Set after news has been focused
    "sticky integer default 0, "           // Flag "sticky". Set by user
    "deleted integer default 0, "          // Flag "deleted". News is marked deleted by remains in DB,
                                           //   for purpose not to display after next update.
                                           //   News are deleted by cleanup process only
    "attachment varchar, "                 // Links to attachments (tabs separated)
    "comments varchar, "                   // News comments page URL-link
    "enclosure_length, "                   // Media-object, associated to news:
    "enclosure_type, "                     //   length, type,
    "enclosure_url, "                      //   URL-address
    "source varchar, "                     // source, incese of republication (atom: <link via>)
    "link_href varchar, "                  // URL-link to news (atom: <link self>)
    "link_enclosure varchar, "             // URL-link to huge amoun of data,
                                           //   that can't be received in the news
    "link_related varchar, "               // URL-link for related data of the news (atom)
    "link_alternate varchar, "             // URL-link to alternative news representation
    "contributor varchar, "                // contributors (tabs separated)
    "rights varchar "                      // copyrights
    ")");

const QString kCreateNewsTableQuery(
    "CREATE TABLE news("
    "id integer primary key, "
    "feedId integer, "                     // feed id from feed table
    "guid varchar, "                       // news unique number
    "guidislink varchar default 'true', "  // flag shows that news unique number is URL-link to news
    "description varchar, "                // brief description
    "content varchar, "                    // full content (atom)
    "title varchar, "                      // title
    "published varchar, "                  // publish timestamp
    "modified varchar, "                   // modification timestamp
    "received varchar, "                   // recieve news timestamp (set on recieve)
    "author_name varchar, "                // author name
    "author_uri varchar, "                 // author web page (atom)
    "author_email varchar, "               // author e-mail (atom)
    "category varchar, "                   // category. May be several item tabs separated
    "label varchar, "                      // label (user purpose label(s))
    "new integer default 1, "              // Flag "new". Set on receive, reset on application close
    "read integer default 0, "             // Flag "read". Set after news has been focused
    "starred integer default 0, "          // Flag "sticky". Set by user
    "deleted integer default 0, "          // Flag "deleted". News is marked deleted by remains in DB,
                                           //   for purpose not to display after next update.
                                           //   News are deleted by cleanup process only
    "attachment varchar, "                 // Links to attachments (tabs separated)
    "comments varchar, "                   // News comments page URL-link
    "enclosure_length, "                   // Media-object, associated to news:
    "enclosure_type, "                     //   length, type,
    "enclosure_url, "                      //   URL-address
    "source varchar, "                     // source, incese of republication (atom: <link via>)
    "link_href varchar, "                  // URL-link to news (atom: <link self>)
    "link_enclosure varchar, "             // URL-link to huge amoun of data,
                                           //   that can't be received in the news
    "link_related varchar, "               // URL-link for related data of the news (atom)
    "link_alternate varchar, "             // URL-link to alternative news representation
    "contributor varchar, "                // contributors (tabs separated)
    "rights varchar, "                     // copyrights
    "deleteDate varchar, "                 // news delete timestamp
    "feedParentId integer default 0 "      // parent feed id from feed table
    ")");

const QString kCreateFiltersTable_v0_9_0(
    "CREATE TABLE filters("
    "id integer primary key, "
    "name varchar, "            // filter name
    "type int, "                // filter type (and, or, for all)
    "feeds varchar "            // feed list, that are using the filter
    ")");

const QString kCreateFiltersTable(
    "CREATE TABLE filters("
    "id integer primary key, "
    "name varchar, "              // filter name
    "type integer, "              // filter type (and, or, for all)
    "feeds varchar, "             // feed list, that are using the filter
    "enable integer default 1, "  // 1 - filter used; 0 - filter not used
    "num integer "                // Sequence number. Used to sort filters
    ")");

const QString kCreateFilterConditionsTable(
    "CREATE TABLE filterConditions("
    "id integer primary key, "
    "idFilter int, "            // filter Id
    "field varchar, "           // field to filter by
    "condition varchar, "       // condition has applied to filed
    "content varchar "          // field content that is used by filter
    ")");

const QString kCreateFilterActionsTable(
    "CREATE TABLE filterActions("
    "id integer primary key, "
    "idFilter int, "            // filter Id
    "action varchar, "          // action that has appled for filter
    "params varchar "           // action parameters
    ")");

const QString kCreateLabelsTable(
    "CREATE TABLE labels("
    "id integer primary key, "
    "name varchar, "            // label name
    "image blob, "              // label image
    "color_text varchar, "      // news text color displayed in news list
    "color_bg varchar, "        // news background color displayed in news list
    "num integer, "             // sequence number to sort with
    "currentNews integer "      // current displayed news
    ")");

const QString kCreatePasswordsTable(
    "CREATE TABLE passwords("
    "id integer primary key, "
    "server varchar, "          // server
    "username varchar, "        // username
    "password varchar "         // password
    ")");

// ----------------------------------------------------------------------------
void initLabelsTable(QSqlDatabase *db)
{
  QSqlQuery q(*db);
  q.exec(kCreateLabelsTable);
  QStringList strNameLabels;
  strNameLabels << "Important" << "Work" << "Personal"
                << "To Do" << "Later" << "Amusingly";
  for (int i = 0; i < 6; i++) {
    q.prepare("INSERT INTO labels(name, image) "
              "VALUES (:name, :image)");
    q.bindValue(":name", strNameLabels.at(i));

    QFile file(QString(":/images/label_%1").arg(i+1));
    file.open(QFile::ReadOnly);
    q.bindValue(":image", file.readAll());
    file.close();

    q.exec();

    int labelId = q.lastInsertId().toInt();
    q.exec(QString("UPDATE labels SET num='%1' WHERE id=='%1'").arg(labelId));
  }
}

/** Create backup copy of file
 *
 *  Backup filename format:
 *  <old-filename>_<file-version>_<backup-creation-time>.bak
 * @param oldFilename absolute path of file to backup
 * @param oldVersion version of file to backup
 *----------------------------------------------------------------------------*/
void createFileBackup(const QString &oldFilename, const QString &oldVersion)
{
  QFileInfo fi(oldFilename);

  // Create backup folder inside DB-file folder
  QDir backupDir(fi.absoluteDir());
  if (!backupDir.exists("backup"))
    backupDir.mkpath("backup");
  backupDir.cd("backup");

  // Delete old files
  QStringList fileNameList = backupDir.entryList(QStringList(QString("%1*").arg(fi.fileName())),
                                                 QDir::Files, QDir::Time);
  int count = 0;
  foreach (QString fileName, fileNameList) {
    count++;
    if (count >= 3) {
      QFile::remove(backupDir.absolutePath() + '/' + fileName);
    }
  }

  // Create backup
  QString backupFilename(backupDir.absolutePath() + '/' + fi.fileName());
  backupFilename.append(QString("_%1_%2.bak")
          .arg(oldVersion)
          .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")));
  QFile::copy(oldFilename, backupFilename);

}

//-----------------------------------------------------------------------------
QString initDB(const QString &dbFileName, QSettings *settings)
{
  if (!QFile(dbFileName).exists()) {  // DB-init
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "dbFileName_");
    db.setDatabaseName(dbFileName);
    db.open();
    db.transaction();
    db.exec(kCreateFeedsTableQuery);
    db.exec(kCreateNewsTableQuery);
    // Create index for feedId field
    db.exec("CREATE INDEX feedId ON news(feedId)");

    // Create extra feeds table just in case
    db.exec("CREATE TABLE feeds_ex(id integer primary key, "
        "feedId integer, "  // feed Id
        "name varchar, "    // parameter name
        "value varchar "    // parameter value
        ")");
    // Create extra news table just in case
    db.exec("CREATE TABLE news_ex(id integer primary key, "
        "feedId integer, "  // feed Id
        "newsId integer, "  // news Id
        "name varchar, "    // parameter name
        "value varchar "    // parameter value
        ")");
    // Create filters table
    db.exec(kCreateFiltersTable);
    db.exec(kCreateFilterConditionsTable);
    db.exec(kCreateFilterActionsTable);
    // Create extra filters just in case
    db.exec("CREATE TABLE filters_ex(id integer primary key, "
        "idFilter integer, "  // filter Id
        "name text, "         // parameter name
        "value text"          // parameter value
        ")");
    // Create labels table
    initLabelsTable(&db);
    // Create password table
    db.exec(kCreatePasswordsTable);
    //
    db.exec("CREATE TABLE info(id integer primary key, name varchar, value varchar)");
    QSqlQuery q(db);
    q.prepare("INSERT INTO info(name, value) VALUES ('version', :version)");
    q.bindValue(":version", kDbVersion);
    q.exec();
    q.exec("INSERT OR REPLACE INTO info(name, value) VALUES ('rowToParentCorrected_0.12.3', 'true')");
    q.finish();
    db.commit();
    db.close();
  } else {
    QString dbVersionString;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "dbFileName_");
    db.setDatabaseName(dbFileName);
    db.open();
    db.transaction();
    QSqlQuery q(db);
    q.exec("SELECT value FROM info WHERE name='version'");
    if (q.next())
      dbVersionString = q.value(0).toString();

    QString appVersionString;
    q.exec("SELECT value FROM info WHERE name='appVersion'");
    if (q.next())
      appVersionString = q.value(0).toString();

    // Create backups for DB and Settings
    if (appVersionString != STRPRODUCTVER) {
        createFileBackup(dbFileName, STRPRODUCTVER);
        createFileBackup(settings->fileName(), STRPRODUCTVER);
    }

    if (!dbVersionString.isEmpty()) {
      // DB-version 1.0 (0.1.0 indeed)
      if (dbVersionString == "1.0") {
        //-- Convert feeds table
        // Create temporary table for version 0.10.0
        q.exec(QString(kCreateFeedsTableQuery)
            .replace("TABLE feeds", "TEMP TABLE feeds_temp"));
        // Copy only fields than was used before
        q.exec("INSERT "
            "INTO feeds_temp("
               "id, text, title, description, xmlUrl, htmlUrl, "
               "author_name, author_email, author_uri, pubdate, lastBuildDate, "
               "image, unread, newCount, currentNews"
            ") SELECT "
            "id, text, title, description, xmlUrl, htmlUrl, "
            "author_name, author_email, author_uri, pubdate, lastBuildDate, "
            "image, unread, newCount, currentNews "
            "FROM feeds");
        // Bury old table
        q.exec("DROP TABLE feeds");

        // Copy temp table insted of old one
        q.exec(kCreateFeedsTableQuery);
        q.exec("INSERT INTO feeds SELECT * FROM feeds_temp");
        q.exec("DROP TABLE feeds_temp");

        // Modify rowToParent field of the feeds table,
        //   to set all feeds in tree root
        q.exec("SELECT id FROM feeds");
        int rowToParent = 0;
        while (q.next()) {
          int id = q.value(0).toInt();
          QSqlQuery q2(db);
          q2.prepare("UPDATE feeds SET rowToParent=:rowToParent WHERE id=:id");
          q2.bindValue(":rowToParent", rowToParent);
          q2.bindValue(":id", id);
          q2.exec();
          rowToParent++;
        }

        //-- Convert feed_# tables to general news table
        // Create general news table
        q.exec(kCreateNewsTableQuery);

        // Copy all news to one genaral table. Add feedId to each news
        //! \NOTE: We can't delete table at the same time, because in this case
        //! there are more than one active query. It is not permitted.
        //! So, ids are stored in list
        q.exec("SELECT id FROM feeds");
        QList<int> idList;
        while (q.next()) {
          int feedId = q.value(0).toInt();
          idList << feedId;
          QSqlQuery q2(db);
          q2.exec(QString("SELECT "
              "guid, guidislink, description, content, title, published, received, "  // 0..6
              "author_name, author_uri, author_email, category, "                     // 7..10
              "new, read, sticky, deleted, "                                          // 11..14
              "link_href "                                                            // 15
              "FROM feed_%1").arg(feedId));
          while (q2.next()) {
            QSqlQuery q3(db);
            q3.prepare(QString("INSERT INTO news("
                  "feedId, "
                  "guid, guidislink, description, content, title, published, received, "
                  "author_name, author_uri, author_email, category, "
                  "new, read, starred, deleted, "
                  "link_href "
                  ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
            q3.addBindValue(feedId);
            q3.addBindValue(q2.value(0));
            q3.addBindValue(q2.value(1));
            q3.addBindValue(q2.value(2));
            q3.addBindValue(q2.value(3));
            q3.addBindValue(q2.value(4));
            q3.addBindValue(q2.value(5));
            q3.addBindValue(q2.value(6));
            q3.addBindValue(q2.value(7));
            q3.addBindValue(q2.value(8));
            q3.addBindValue(q2.value(9));
            q3.addBindValue(q2.value(10));
            q3.addBindValue(q2.value(11));
            q3.addBindValue(q2.value(12));
            q3.addBindValue(q2.value(13));
            q3.addBindValue(q2.value(14));
            q3.addBindValue(q2.value(15));
            q3.exec();
          }
        }

        // Delete old news tables
        foreach (int id, idList)
          q.exec(QString("DROP TABLE feed_%1").arg(id));

        // Create index on feedId field (for quick DB-search)
        q.exec("CREATE INDEX feedId ON news(feedId)");

        // Create extra feeds table just in case
        q.exec("CREATE TABLE feeds_ex(id integer primary key, "
            "feedId integer, "  // feed Id
            "name varchar, "    // parameter mane
            "value varchar "    // parameter value
            ")");
        // Create extra news tables just in case
        q.exec("CREATE TABLE news_ex(id integer primary key, "
            "feedId integer, "  // feed Id
            "newsId integer, "  // news Id
            "name varchar, "    // parameter mane
            "value varchar "    // parameter value
            ")");

        // Create filters tables
        q.exec(kCreateFiltersTable);
        q.exec(kCreateFilterConditionsTable);
        q.exec(kCreateFilterActionsTable);
        // Create extra filter tables just in case
        q.exec("CREATE TABLE filters_ex(id integer primary key, "
            "idFilter integer, "  // filter Id
            "name text, "         // parameter mane
            "value text"          // parameter value
            ")");

        // Update information table
        q.prepare("UPDATE info SET value=:version WHERE name='version'");
        q.bindValue(":version", kDbVersion);
        q.exec();

        qDebug() << "DB converted to version =" << kDbVersion;
      }
      // DB-version 0.9.0
      else if (dbVersionString == "0.9.0") {
        //-- Convert table feeds
        // Create temporary table for version 0.10.0
        q.exec(QString(kCreateFeedsTableQuery)
            .replace("TABLE feeds", "TEMP TABLE feeds_temp"));
        // Copy only fields than was used before
        q.exec("INSERT "
            "INTO feeds_temp("
               "id, text, title, description, xmlUrl, htmlUrl, "
               "author_name, author_email, author_uri, pubdate, lastBuildDate, "
               "image, unread, newCount, currentNews, undeleteCount, updated, "
               "displayOnStartup, displayEmbeddedImages "
            ") SELECT "
            "id, text, title, description, xmlUrl, htmlUrl, "
            "author_name, author_email, author_uri, pubdate, lastBuildDate, "
            "image, unread, newCount, currentNews, undeleteCount, updated, "
            "displayOnStartup, displayEmbeddedImages "
            "FROM feeds");
        // Bury old table
        q.exec("DROP TABLE feeds");

        // Copy temp table insted of old one
        q.exec(kCreateFeedsTableQuery);
        q.exec("INSERT INTO feeds SELECT * FROM feeds_temp");
        q.exec("DROP TABLE feeds_temp");

        // Modify rowToParent field of the feeds table,
        //   to set all feeds in tree root
        q.exec("SELECT id FROM feeds");
        int rowToParent = 0;
        while (q.next()) {
          int id = q.value(0).toInt();
          QSqlQuery q2(db);
          q2.prepare("UPDATE feeds SET rowToParent=:rowToParent WHERE id=:id");
          q2.bindValue(":rowToParent", rowToParent);
          q2.bindValue(":id", id);
          q2.exec();
          rowToParent++;
        }

        //-- Ignore news table
        //-- Rebuild the index
        q.exec("DROP INDEX feedId");
        q.exec("CREATE INDEX feedId ON news(feedId)");
        // Extra tables had created already
        // Replace old empty filter table by new filter table contained new fields
        q.exec("DROP TABLE filters");
        q.exec(kCreateFiltersTable);
        // Don't tuoch conditions and actions tables

        // Create extra filter tables just in case
        q.exec("CREATE TABLE filters_ex(id integer primary key, "
            "idFilter integer, "  // filter Id
            "name text, "         // parameter mane
            "value text"          // parameter value
            ")");

        // Update information table
        q.prepare("UPDATE info SET value=:version WHERE name='version'");
        q.bindValue(":version", kDbVersion);
        q.exec();

        qDebug() << "DB converted to version =" << kDbVersion;
      }
      // DB-version 0.10.0
      else if (dbVersionString == "0.10.0") {
        // Add fileds for feed table
        q.exec("ALTER TABLE feeds ADD COLUMN authentication integer default 0");
        q.exec("ALTER TABLE feeds ADD COLUMN duplicateNewsMode integer default 0");
        q.exec("ALTER TABLE feeds ADD COLUMN typeFeed integer default 0");

        // Add fileds for news table
        q.exec("ALTER TABLE news ADD COLUMN deleteDate varchar");
        q.exec("ALTER TABLE news ADD COLUMN feedParentId integer default 0");

        // Create labels table
        bool createTable = false;
        q.exec("SELECT count(name) FROM sqlite_master WHERE name='labels'");
        if (q.next()) {
          if (q.value(0).toInt() > 0) createTable = true;
        }
        if (!createTable) {
          initLabelsTable(&db);
        } else {
          q.exec("SELECT count(currentNews) FROM labels");
          if (!q.next()) {
            q.exec("ALTER TABLE labels ADD COLUMN currentNews integer");
          }
        }
        // Create password table
        createTable = false;
        q.exec("SELECT count(name) FROM sqlite_master WHERE name='passwords'");
        if (q.next()) {
          if (q.value(0).toInt() > 0) createTable = true;
        }
        if (!createTable)
          q.exec(kCreatePasswordsTable);

        // Update information table
        q.prepare("UPDATE info SET value=:version WHERE name='version'");
        q.bindValue(":version", kDbVersion);
        q.exec();

        qDebug() << "DB converted to version =" << kDbVersion;
      }
      // DB-version last
      else {

        bool rowToParentCorrected = false;
        q.exec("SELECT value FROM info WHERE name='rowToParentCorrected_0.12.3'");
        if (q.next() && q.value(0).toString()=="true")
          rowToParentCorrected = true;

        if (rowToParentCorrected) {
          qDebug() << "dbVersion =" << dbVersionString;
        }
        else {
          qDebug() << "dbversion =" << dbVersionString << ": rowToParentCorrected_0.12.3 = true";

          createFileBackup(dbFileName, dbVersionString);

          q.exec("INSERT OR REPLACE INTO info(name, value) VALUES ('rowToParentCorrected_0.12.3', 'true')");

          // Start search from prospective parent number 0 (from root)
          bool sortFeeds = settings->value("Settings/sortFeeds", false).toBool();
          QString sortStr("id");
          if (sortFeeds) sortStr = "text";

          QList<int> parentIdsPotential;
          parentIdsPotential << 0;
          while (!parentIdsPotential.empty()) {
            int parentId = parentIdsPotential.takeFirst();

            // Search for Children of parent parentId
            q.prepare(QString("SELECT id FROM feeds WHERE parentId=? ORDER BY %1").
                      arg(sortStr));
            q.addBindValue(parentId);
            q.exec();

            // Write down rowToParent for every child
            // ... and store the child in prospective parent list
            int rowToParent = 0;
            while (q.next()) {
              int parentIdNew = q.value(0).toInt();

              QSqlQuery q2(db);
              q2.prepare("UPDATE feeds SET rowToParent=? WHERE id=?");
              q2.addBindValue(rowToParent);
              q2.addBindValue(parentIdNew);
              q2.exec();

              parentIdsPotential << parentIdNew;
              ++rowToParent;
            }
          }
        }   // if (rowToParentCorrected) {
      } // DB-version last
    }

    // Update appVersion anyway
    q.prepare("INSERT OR REPLACE INTO info(name, value) "
              "VALUES('appVersion', :appVersion)");
    q.bindValue(":appVersion", STRPRODUCTVER);
    q.exec();

    q.finish();

    db.commit();
    db.close();
  }
  QSqlDatabase::removeDatabase("dbFileName_");
  return kDbVersion;
}

//-----------------------------------------------------------------------------
void setUserFilter(int feedId, int filterId)
{
  QSqlQuery q;
  bool onlyNew = true;

  if (filterId != -1) {
    onlyNew = false;
    q.exec(QString("SELECT enable, type FROM filters WHERE id='%1' AND feeds LIKE '%,%2,%'").
           arg(filterId).arg(feedId));
  } else {
    q.exec(QString("SELECT enable, type, id FROM filters WHERE feeds LIKE '%,%1,%' ORDER BY num").
           arg(feedId));
  }

  while (q.next()) {
    if (q.value(0).toInt() == 0) continue;

    if (onlyNew)
      filterId = q.value(2).toInt();
    int filterType = q.value(1).toInt();

    QString qStr("UPDATE news SET");
    QString qStr1;
    QString qStr2;

    QSqlQuery q1;
    q1.exec(QString("SELECT action, params FROM filterActions "
                    "WHERE idFilter=='%1'").arg(filterId));
    while (q1.next()) {
      switch (q1.value(0).toInt()) {
      case 0: // action -> Mark news as read
        if (!qStr1.isNull()) qStr1.append(",");
        qStr1.append(" new=0, read=2");
        break;
      case 1: // action -> Add star
        if (!qStr1.isNull()) qStr1.append(",");
        qStr1.append(" starred=1");
        break;
      case 2: // action -> Delete
        if (!qStr1.isNull()) qStr1.append(",");
        qStr1.append(" new=0, read=2, deleted=1, ");
        qStr1.append(QString("deleteDate='%1'").
            arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
        break;
      case 3: // action -> Add Label
        qStr2.append(QString("%1,").arg(q1.value(1).toInt()));
        break;
      }
    }

    if (!qStr2.isEmpty()) {
      if (!qStr1.isNull()) qStr1.append(",");
      qStr1.append(QString(" label=',%1'").arg(qStr2));
    }
    qStr.append(qStr1);
    qStr.append(QString(" WHERE feedId='%1' AND deleted=0").arg(feedId));

    if (onlyNew) qStr.append(" AND new=1");

    qStr2.clear();
    switch (filterType) {
    case 1: // Match all conditions
      qStr2.append("AND ");
      break;
    case 2: // Match any condition
      qStr2.append("OR ");
      break;
    }

    if ((filterType == 1) || (filterType == 2)) {
      qStr.append(" AND ( ");
      qStr1.clear();

      q1.exec(QString("SELECT field, condition, content FROM filterConditions "
                      "WHERE idFilter=='%1'").arg(filterId));
      while (q1.next()) {
        if (!qStr1.isNull()) qStr1.append(qStr2);
        switch (q1.value(0).toInt()) {
        case 0: // field -> Title
          switch (q1.value(1).toInt()) {
          case 0: // condition -> contains
            qStr1.append(QString("title LIKE '%%1%' ").arg(q1.value(2).toString()));
            break;
          case 1: // condition -> doesn't contains
            qStr1.append(QString("title NOT LIKE '%%1%' ").arg(q1.value(2).toString()));
            break;
          case 2: // condition -> is
            qStr1.append(QString("title LIKE '%1' ").arg(q1.value(2).toString()));
            break;
          case 3: // condition -> isn't
            qStr1.append(QString("title NOT LIKE '%1' ").arg(q1.value(2).toString()));
            break;
          case 4: // condition -> begins with
            qStr1.append(QString("title LIKE '%1%' ").arg(q1.value(2).toString()));
            break;
          case 5: // condition -> ends with
            qStr1.append(QString("title LIKE '%%1' ").arg(q1.value(2).toString()));
            break;
          }
          break;
        case 1: // field -> Description
          switch (q1.value(1).toInt()) {
          case 0: // condition -> contains
            qStr1.append(QString("description LIKE '%%1%' ").arg(q1.value(2).toString()));
            break;
          case 1: // condition -> doesn't contains
            qStr1.append(QString("description NOT LIKE '%%1%' ").arg(q1.value(2).toString()));
            break;
          }
          break;
        case 2: // field -> Author
          switch (q1.value(1).toInt()) {
          case 0: // condition -> contains
            qStr1.append(QString("author_name LIKE '%%1%' ").arg(q1.value(2).toString()));
            break;
          case 1: // condition -> doesn't contains
            qStr1.append(QString("author_name NOT LIKE '%%1%' ").arg(q1.value(2).toString()));
            break;
          case 2: // condition -> is
            qStr1.append(QString("author_name LIKE '%1' ").arg(q1.value(2).toString()));
            break;
          case 3: // condition -> isn't
            qStr1.append(QString("author_name NOT LIKE '%1' ").arg(q1.value(2).toString()));
            break;
          }
          break;
        case 3: // field -> Category
          switch (q1.value(1).toInt()) {
          case 0: // condition -> is
            qStr1.append(QString("category LIKE '%1' ").arg(q1.value(2).toString()));
            break;
          case 1: // condition -> isn't
            qStr1.append(QString("category NOT LIKE '%1' ").arg(q1.value(2).toString()));
            break;
          case 2: // condition -> begins with
            qStr1.append(QString("category LIKE '%1%' ").arg(q1.value(2).toString()));
            break;
          case 3: // condition -> ends with
            qStr1.append(QString("category LIKE '%%1' ").arg(q1.value(2).toString()));
            break;
          }
          break;
        case 4: // field -> Status
          if (q1.value(1).toInt() == 0) { // Status -> is
            switch (q1.value(2).toInt()) {
            case 0:
              qStr1.append("new==1 ");
              break;
            case 1:
              qStr1.append("read>=1 ");
              break;
            case 2:
              qStr1.append("starred==1 ");
              break;
            }
          } else { // Status -> isn't
            switch (q1.value(2).toInt()) {
            case 0:
              qStr1.append("new==0 ");
              break;
            case 1:
              qStr1.append("read==0 ");
              break;
            case 2:
              qStr1.append("starred==0 ");
              break;
            }
          }
          break;
        }
      }
      qStr.append(qStr1).append(")");
    }
    q1.exec(qStr);
//    qCritical() << qStr;
  }
}
