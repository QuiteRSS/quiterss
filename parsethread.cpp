#include <QDateTime>
#include <QDebug>
#include <QMessageBox>

#include "parsethread.h"

ParseThread::ParseThread(QObject *parent, QSqlDatabase *db) :
    QThread(parent)
{
  qDebug() << "ParseThread::constructor";
  db_ = db;
}


ParseThread::~ParseThread()
{
  qDebug() << "ParseThread::~destructor";
}

void ParseThread::parseXml(const QByteArray &data, const QUrl &url)
{
  urlsQueue_.enqueue(url);
  xmlsQueue_.enqueue(data);
  qDebug() << "xmlsQueue_ <<" << url << "count=" << xmlsQueue_.count();
  getQueuedXml();
}

void ParseThread::getQueuedXml()
{
  if (!currentUrl_.isEmpty()) return;

  while (!urlsQueue_.isEmpty()) {
    currentUrl_ = urlsQueue_.dequeue();
    currentXml_ = xmlsQueue_.dequeue();
    qDebug() << "xmlsQueue_ >>" << currentUrl_ << "count=" << xmlsQueue_.count();
    parse();
  }

  currentUrl_.clear();
}

void ParseThread::parse()
{
  qDebug() << "=================== parseXml:start ============================";
  // поиск идентификатора ленты с таблице лент
  int parseFeedId = 0;
  QSqlQuery q(*db_);
  q.exec(QString("select id from feeds where xmlurl like '%1'").
      arg(currentUrl_.toString()));
  while (q.next()) {
    parseFeedId = q.value(q.record().indexOf("id")).toInt();
  }

  // идентификатор не найден (например, во время запроса удалили ленту)
  if (0 == parseFeedId) {
    qDebug() << QString("Feed '%1' not found").arg(currentUrl_.toString());
    return;
  }

  qDebug() << QString("Feed '%1' found with id = %2").arg(currentUrl_.toString()).
      arg(parseFeedId);

  // собственно сам разбор
  bool feedChanged = false;
  db_->transaction();
  int itemCount = 0;
  QXmlStreamReader xml(currentXml_);
  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isStartElement()) {
      if (xml.name() == "rss")  qDebug() << "Feed type: RSS";
      if (xml.name() == "feed") qDebug() << "Feed type: Atom";

      if (xml.name() == "item") {  // clear strings
        linkString = xml.attributes().value("rss:about").toString();
        rssItemString.clear();
        titleString.clear();
        linkString.clear();
        rssDescriptionString.clear();
        commentsString.clear();
        rssPubDateString.clear();
        rssGuidString.clear();
      }
      if (xml.name() == "entry") {  // clear strings
        atomEntryString.clear();
        titleString.clear();
        atomIdString.clear();
        atomUpdatedString.clear();
        atomSummaryString.clear();
        atomContentString.clear();
      }
      currentTag = xml.name().toString();
//      qDebug() << itemCount << ": " << currentTag;
    } else if (xml.isEndElement()) {
      // rss::item
      if (xml.name() == "item") {

        // поиск дубликата статей в базе
        QSqlQuery q(*db_);
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
                           "description, guid, title, published, received) "
                           "values(?, ?, ?, ?, ?)").
                arg(parseFeedId);
            q.prepare(qStr);
            q.addBindValue(rssDescriptionString);
            q.addBindValue(rssGuidString);
            q.addBindValue(titleString);
            q.addBindValue(rssPubDateString);
            q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
            q.exec();
            qDebug() << "q.exec(" << q.lastQuery() << ")";
            qDebug() << "       " << rssDescriptionString;
            qDebug() << "       " << rssGuidString;
            qDebug() << "       " << titleString;
            qDebug() << "       " << rssPubDateString;
            qDebug() << "       " << QDateTime::currentDateTime().toString();
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
            qDebug() << "  entry:  " << atomEntryString;
            qDebug() << "  title:  " << titleString;
            qDebug() << "  id:     " << atomIdString;
            qDebug() << "  updated:" << atomUpdatedString;
            qDebug() << "  summary:" << atomSummaryString;
            qDebug() << "  content:" << atomContentString;

        // поиск дубликата статей в базе
        QSqlQuery q(*db_);
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
                           "description, content, guid, title, published, received) "
                           "values(?, ?, ?, ?, ?, ?)").
                arg(parseFeedId);
            q.prepare(qStr);
            q.addBindValue(atomSummaryString);
            q.addBindValue(atomContentString);
            q.addBindValue(atomIdString);
            q.addBindValue(titleString);
            q.addBindValue(atomUpdatedString);
            q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
            q.exec();
            qDebug() << "q.exec(" << q.lastQuery() << ")";
            qDebug() << "       " << atomSummaryString;
            qDebug() << "       " << atomContentString;
            qDebug() << "       " << atomIdString;
            qDebug() << "       " << titleString;
            qDebug() << "       " << atomUpdatedString;
            qDebug() << "       " << QDateTime::currentDateTime().toString();
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
  db_->commit();
  qDebug() << "=================== parseXml:finish ===========================";

  if (feedChanged) emit feedUpdated(currentUrl_);
}
