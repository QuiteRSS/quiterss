#include <QDebug>

#include "parseobject.h"

ParseObject::ParseObject(QObject *parent) :
    QObject(parent)
{
}

void ParseObject::slotParse(QSqlDatabase *db,
    const QByteArray &xmlData, const QUrl &url)
{
  QString currentTag;
  QString rssItemString;
  QString titleString;
  QString linkString;
  QString authorString;
  QString rssDescriptionString;
  QString commentsString;
  QString rssPubDateString;
  QString rssGuidString;
  QString atomEntryString;
  QString atomIdString;
  QString atomUpdatedString;
  QString atomSummaryString;
  QString atomContentString;

  qDebug() << "=================== parseXml:start ============================";
  // поиск идентификатора ленты с таблице лент
  int parseFeedId = 0;
  QSqlQuery q(*db);
  q.exec(QString("select id from feeds where xmlurl like '%1'").
      arg(url.toString()));
  while (q.next()) {
    parseFeedId = q.value(q.record().indexOf("id")).toInt();
  }

  // идентификатор не найден (например, во время запроса удалили ленту)
  if (0 == parseFeedId) {
    qDebug() << QString("Feed '%1' not found").arg(url.toString());
    return;
  }

  qDebug() << QString("Feed '%1' found with id = %2").arg(url.toString()).
      arg(parseFeedId);

  // собственно сам разбор
  bool feedChanged = false;
  db->transaction();
  int itemCount = 0;
  QXmlStreamReader xml(xmlData);
  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isStartElement()) {
      if (xml.name() == "rss")  qDebug() << "Feed type: RSS";
      if (xml.name() == "feed") qDebug() << "Feed type: Atom";

      if (xml.name() == "item") {  // clear strings
        rssItemString.clear();
        titleString.clear();
        linkString.clear();
        authorString.clear();
        rssDescriptionString.clear();
        commentsString.clear();
        rssPubDateString.clear();
        rssGuidString.clear();
      }
      if (xml.name() == "entry") {  // clear strings
        atomEntryString.clear();
        titleString.clear();
        linkString.clear();
        authorString.clear();
        atomIdString.clear();
        atomUpdatedString.clear();
        atomSummaryString.clear();
        atomContentString.clear();
      }
      currentTag = xml.name().toString();
//      qDebug() << itemCount << ":" << currentTag;
//      for (int i = 0 ; i < xml.attributes().count(); ++i)
//        qDebug() << "     " << xml.attributes().at(i).name() << "=" << xml.attributes().at(i).value();
      if (currentTag == "link")
        linkString = xml.attributes().value("href").toString();
    } else if (xml.isEndElement()) {
      // rss::item
      if (xml.name() == "item") {
        rssPubDateString = parseDate(rssPubDateString);

        // поиск дубликата статей в базе
        QSqlQuery q(*db);
        QString qStr;
        qDebug() << "guid:" << rssGuidString;
        if (!rssGuidString.isEmpty())         // поиск по guid
          qStr = QString("select * from feed_%1 where guid == '%2'").
              arg(parseFeedId).arg(rssGuidString);
        else if (rssPubDateString.isEmpty())  // поиск по title, т.к. поле pubDate пустое
          qStr = QString("select * from feed_%1 where title like '%2'").
              arg(parseFeedId).arg(titleString.replace('\'', '_'));
        else                               // поиск по title и pubDate
          qStr = QString("select * from feed_%1 "
              "where title like '%2' and published == '%3'").
              arg(parseFeedId).arg(titleString.replace('\'', '_')).arg(rssPubDateString);
        q.exec(qStr);
        // проверка правильности запроса
        if (q.lastError().isValid())
          qDebug() << "ERROR: q.exec(" << qStr << ") -> " << q.lastError().text();
        else {
          // если дубликата нет, добавляем статью в базу
          if (!q.next()) {
            qStr = QString("insert into feed_%1("
                           "description, guid, title, author, published, received, link) "
                           "values(?, ?, ?, ?, ?, ?, ?)").
                arg(parseFeedId);
            q.prepare(qStr);
            q.addBindValue(rssDescriptionString);
            q.addBindValue(rssGuidString);
            q.addBindValue(titleString);
            q.addBindValue(authorString);
            q.addBindValue(rssPubDateString);
            q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
            q.addBindValue(linkString);
            q.exec();
            qDebug() << "q.exec(" << q.lastQuery() << ")";
            qDebug() << "       " << rssDescriptionString;
            qDebug() << "       " << rssGuidString;
            qDebug() << "       " << titleString;
            qDebug() << "       " << authorString;
            qDebug() << "       " << rssPubDateString;
            qDebug() << "       " << QDateTime::currentDateTime().toString();
            qDebug() << "       " << linkString;
            qDebug() << q.lastError().number() << ": " << q.lastError().text();
            feedChanged = true;
          }
        }
        q.finish();

        if (!rssItemString.isEmpty()) qWarning() << "rssItamString is not empty";
        ++itemCount;
      }
      // atom::feed
      else if (xml.name() == "entry") {
        atomUpdatedString = parseDate(atomUpdatedString);

        // поиск дубликата статей в базе
        QSqlQuery q(*db);
        QString qStr;
        qDebug() << "atomId:" << atomIdString;
        if (!atomIdString.isEmpty())         // поиск по guid
          qStr = QString("select * from feed_%1 where guid == '%2'").
              arg(parseFeedId).arg(atomIdString);
        else if (atomUpdatedString.isEmpty())  // поиск по title, т.к. поле pubDate пустое
          qStr = QString("select * from feed_%1 where title like '%2'").
              arg(parseFeedId).arg(titleString.replace('\'', '_'));
        else                               // поиск по title и pubDate
          qStr = QString("select * from feed_%1 "
              "where title like '%2' and published == '%3'").
              arg(parseFeedId).arg(titleString.replace('\'', '_')).arg(atomUpdatedString);
        q.exec(qStr);

        // проверка правильности запроса
        if (q.lastError().isValid())
          qDebug() << "ERROR: q.exec(" << qStr << ") -> " << q.lastError().text();
        else {
          // если дубликата нет, добавляем статью в базу
          if (!q.next()) {
            qStr = QString("insert into feed_%1("
                           "description, content, guid, title, author, published, received, link) "
                           "values(?, ?, ?, ?, ?, ?, ?, ?)").
                arg(parseFeedId);
            q.prepare(qStr);
            q.addBindValue(atomSummaryString);
            q.addBindValue(atomContentString);
            q.addBindValue(atomIdString);
            q.addBindValue(titleString);
            q.addBindValue(authorString);
            q.addBindValue(atomUpdatedString);
            q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
            q.addBindValue(linkString);
            q.exec();
            qDebug() << "q.exec(" << q.lastQuery() << ")";
            qDebug() << "       " << atomSummaryString;
            qDebug() << "       " << atomContentString;
            qDebug() << "       " << atomIdString;
            qDebug() << "       " << titleString;
            qDebug() << "       " << authorString;
            qDebug() << "       " << atomUpdatedString;
            qDebug() << "       " << QDateTime::currentDateTime().toString();
            qDebug() << "       " << linkString;
            qDebug() << q.lastError().number() << ": " << q.lastError().text();
            feedChanged = true;
          }
        }
        q.finish();

        if (!atomEntryString.isEmpty()) qWarning() << "atomEntryString is not empty";
        ++itemCount;
      }
    } else if (xml.isCharacters() && !xml.isWhitespace()) {
      if (currentTag == "item")
        rssItemString += xml.text().toString();
      else if (currentTag == "title")
        titleString += xml.text().toString();
      else if (currentTag == "link")
        linkString += xml.text().toString();
      else if (currentTag == "author")
        authorString += xml.text().toString();
      else if (currentTag == "description")
        rssDescriptionString += xml.text().toString();
      else if (currentTag == "comments")
        commentsString += xml.text().toString();
      else if (currentTag == "pubDate")
        rssPubDateString += xml.text().toString();
      else if (currentTag == "guid")
        rssGuidString += xml.text().toString();

      else if (currentTag == "entry")
        atomEntryString += xml.text().toString();
      else if (currentTag == "id")
        atomIdString += xml.text().toString();
      else if (currentTag == "updated")
        atomUpdatedString += xml.text().toString();
      else if (currentTag == "summary")
        atomSummaryString += xml.text().toString();
      else if (currentTag == "content")
        atomContentString += xml.text().toString();
    }
  }
  if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
    QString str = QString("XML ERROR: Line=%1, ErrorString=%2").
        arg(xml.lineNumber()).arg(xml.errorString());
    qDebug() << str;
  }
  db->commit();
  qDebug() << "=================== parseXml:finish ===========================";

  if (feedChanged) emit feedUpdated(url);
}

QString ParseObject::parseDate(QString dateString)
{
  QDateTime dt;
  QString temp;

  if (dateString.isEmpty()) return QString();

  QLocale locale(QLocale::C);

  temp = dateString.left(19);
  dt = locale.toDateTime(temp, "yyyy-MM-ddTHH:mm:ss");
  if (dt.isValid()) return locale.toString(dt, "yyyy-MM-ddTHH:mm:ss");

  temp = dateString.left(25);
  dt = locale.toDateTime(temp, "ddd, dd MMM yyyy HH:mm:ss");
  if (dt.isValid()) return locale.toString(dt, "yyyy-MM-ddTHH:mm:ss");

  qWarning() << "==== parseDate(): error with" << dateString;
  return QString();
}
