#include <QDebug>

#include "parseobject.h"

ParseObject::ParseObject(QObject *parent) :
    QObject(parent)
{
}

void ParseObject::slotParse(QSqlDatabase *db,
    const QByteArray &xmlData, const QUrl &url)
{
  QFile file("lastfeed.dat");
  file.open(QIODevice::WriteOnly);
  file.write(xmlData);
  file.close();

  QString currentTag;
  QString currentTagText;
  QStack<QString> tagsStack;
  QString titleString;
  QString linkString;
  QString authorString;
  QString authorUriString;
  QString authorEmailString;
  QString rssDescriptionString;
  QString commentsString;
  QString rssPubDateString;
  QString rssGuidString;
  QString atomIdString;
  QString atomUpdatedString;
  QString atomSummaryString;
  QString contentString;

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
  bool isHeader = true;  //!< флаг заголовка ленты - элементы до первой новости
  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isStartElement()) {
      tagsStack.push(currentTag);
      currentTag = xml.name().toString();
      if (currentTag == "rss")  qDebug() << "Feed type: RSS";
      if (currentTag == "feed") qDebug() << "Feed type: Atom";

      if (currentTag == "item") {  // clear strings
        if (isHeader) {
          rssPubDateString = parseDate(rssPubDateString);

          QSqlQuery q(*db);
          QString qStr;
          qStr = QString("update feeds set "
                         "title='%1', description='%2', "
                         "author_name='%3', pubdate='%4' "
                         "where id=='%5'").
              arg(titleString).
              arg(rssDescriptionString).
              arg(authorString).
              arg(rssPubDateString).
              arg(parseFeedId);
          q.exec(qStr);
          qDebug() << "q.exec(" << q.lastQuery() << ")";
          qDebug() << q.lastError().number() << ": " << q.lastError().text();
        }
        isHeader = false;
        titleString.clear();
        linkString.clear();
        authorString.clear();
        rssDescriptionString.clear();
        commentsString.clear();
        rssPubDateString.clear();
        rssGuidString.clear();
        contentString.clear();
      }
      if (currentTag == "entry") {  // clear strings
        atomUpdatedString = parseDate(atomUpdatedString);

        if (isHeader) {
          QSqlQuery q(*db);
          QString qStr;
          qStr = QString("update feeds set "
                         "title='%1', description='%2', "
                         "author_name='%3', author_email='%4', "
                         "author_uri='%5', pubdate='%6' "
                         "where id=='%7'").
              arg(titleString).
              arg(rssDescriptionString).
              arg(authorString).
              arg(authorEmailString).
              arg(authorUriString).
              arg(atomUpdatedString).
              arg(parseFeedId);
          q.exec(qStr);
          qDebug() << "q.exec(" << q.lastQuery() << ")";
          qDebug() << q.lastError().number() << ": " << q.lastError().text();
        }
        isHeader = false;
        titleString.clear();
        linkString.clear();
        authorString.clear();
        authorUriString.clear();
        authorEmailString.clear();
        atomIdString.clear();
        atomUpdatedString.clear();
        atomSummaryString.clear();
        contentString.clear();
      }
      if (currentTag == "link")
        linkString = xml.attributes().value("href").toString();
      if (isHeader) {
        if (xml.namespaceUri().isEmpty()) qDebug() << itemCount << ":" << currentTag;
        else qDebug() << itemCount << ":" << xml.qualifiedName();
        for (int i = 0 ; i < xml.attributes().count(); ++i)
          qDebug() << "      " << xml.attributes().at(i).name() << "=" << xml.attributes().at(i).value();
      }
      currentTagText.clear();
//      qDebug() << tagsStack << currentTag;
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
                           "description, content, guid, title, author_name, published, received, link_href) "
                           "values(?, ?, ?, ?, ?, ?, ?, ?)").
                arg(parseFeedId);
            q.prepare(qStr);
            q.addBindValue(rssDescriptionString);
            q.addBindValue(contentString);
            q.addBindValue(rssGuidString);
            q.addBindValue(titleString);
            q.addBindValue(authorString);
            q.addBindValue(rssPubDateString);
            q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
            q.addBindValue(linkString);
            q.exec();
            qDebug() << "q.exec(" << q.lastQuery() << ")";
            qDebug() << "       " << rssDescriptionString;
            qDebug() << "       " << contentString;
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
                           "description, content, guid, title, author_name, "
                           "author_uri, author_email, published, received, link_href) "
                           "values(?, ?, ?, ?, ?, ?, ?, ?, ? ,? )").
                arg(parseFeedId);
            q.prepare(qStr);
            q.addBindValue(atomSummaryString);
            q.addBindValue(contentString);
            q.addBindValue(atomIdString);
            q.addBindValue(titleString);
            q.addBindValue(authorString);
            q.addBindValue(authorUriString);
            q.addBindValue(authorEmailString);
            q.addBindValue(atomUpdatedString);
            q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
            q.addBindValue(linkString);
            q.exec();
            qDebug() << "q.exec(" << q.lastQuery() << ")";
            qDebug() << "       " << atomSummaryString;
            qDebug() << "       " << contentString;
            qDebug() << "       " << atomIdString;
            qDebug() << "       " << titleString;
            qDebug() << "       " << authorString;
            qDebug() << "       " << authorUriString;
            qDebug() << "       " << authorEmailString;
            qDebug() << "       " << atomUpdatedString;
            qDebug() << "       " << QDateTime::currentDateTime().toString();
            qDebug() << "       " << linkString;
            qDebug() << q.lastError().number() << ": " << q.lastError().text();
            feedChanged = true;
          }
        }
        q.finish();

        ++itemCount;
      }
      if (isHeader) {
        if (!currentTagText.isEmpty()) qDebug() << itemCount << "::   " << currentTagText;
      }
      currentTag = tagsStack.pop();
//      qDebug() << tagsStack << currentTag;
    } else if (xml.isCharacters() && !xml.isWhitespace()) {
      currentTagText += xml.text().toString();

      if (currentTag == "title") {
        if ((tagsStack.top() == "channel") ||
            (tagsStack.top() == "item") ||
            (tagsStack.top() == "entry"))
          titleString += xml.text().toString();
//        if (tagsStack.top() == "image")
//          imageTitleString += xml.text().toString();
      }
      else if (currentTag == "link")
        linkString += xml.text().toString();
      else if (currentTag == "author")  //rss
        authorString += xml.text().toString();
      else if (currentTag == "creator")  //rss::dc:creator
        authorString += xml.text().toString();
      else if (currentTag == "name")   //atom::author
        authorString += xml.text().toString();
      else if (currentTag == "uri")    //atom::uri
        authorUriString += xml.text().toString();
      else if (currentTag == "email")  //atom::email
        authorEmailString += xml.text().toString();
      else if (currentTag == "description")
        rssDescriptionString += xml.text().toString();
      else if (currentTag == "comments")
        commentsString += xml.text().toString();
      else if (currentTag == "pubDate")
        rssPubDateString += xml.text().toString();
      else if (currentTag == "guid")
        rssGuidString += xml.text().toString();
      else if (currentTag == "encoded")  //rss::content:encoded
        contentString += xml.text().toString();

      else if (currentTag == "id")
        atomIdString += xml.text().toString();
      else if (currentTag == "updated")
        atomUpdatedString += xml.text().toString();
      else if (currentTag == "summary")
        atomSummaryString += xml.text().toString();
      else if (currentTag == "content")
        contentString += xml.text().toString();
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

  temp = dateString.left(dateString.lastIndexOf(' '));
  dt = locale.toDateTime(temp, "d MMM yyyy HH:mm:ss");
  if (dt.isValid()) return locale.toString(dt, "yyyy-MM-ddTHH:mm:ss");

  temp = dateString.left(11);
  dt = locale.toDateTime(temp, "dd MMM yyyy");
  if (dt.isValid()) return locale.toString(dt, "yyyy-MM-ddTHH:mm:ss");

  temp = dateString;
  dt = locale.toDateTime(temp, "yyyy-MM-dd HH:mm:ss.z");
  if (dt.isValid()) return locale.toString(dt, "yyyy-MM-ddTHH:mm:ss");

  dt = locale.toDateTime(temp, "yyyy-MM-dd");
  if (dt.isValid()) return locale.toString(dt, "yyyy-MM-ddTHH:mm:ss");

  // @HACK(arhohryakov:2012.01.01):
  // Формат "ddd, dd MMM yy HH:mm:ss" не распознаётся автоматически. Приводим
  // его к формату "ddd, dd MMM yyyy HH:mm:ss"
  temp = dateString;
  if (70 < dateString.mid(13, 2).toInt()) temp = temp.insert(13, "19");
  else temp = temp.insert(12, "20");
  temp = temp.left(25);
  dt = locale.toDateTime(temp, "ddd, dd MMM yyyy HH:mm:ss");
  if (dt.isValid()) return locale.toString(dt, "yyyy-MM-ddTHH:mm:ss");

  qWarning() << "==== parseDate(): error with" << dateString;
  return QString();
}
