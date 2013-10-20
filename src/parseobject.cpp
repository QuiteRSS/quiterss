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
#include "parseobject.h"
#include "db_func.h"
#include "rsslisting.h"
#include "VersionNo.h"

#include <QDebug>
#include <QDesktopServices>
#include <QTextDocumentFragment>
#include <QRegExp>
#if defined(Q_OS_WIN)
#include <windows.h>
#endif

ParseObject::ParseObject(QObject *parent)
  : QObject(0)
  , currentFeedId_(0)
{
  setObjectName("parseObject_");

  QObject *parent_ = parent;
  while(parent_->parent()) {
    parent_ = parent_->parent();
  }
  rssl_ = qobject_cast<RSSListing*>(parent_);

  if (rssl_->storeDBMemory_) {
    db_ = QSqlDatabase::database();
  }
  else {
    db_ = QSqlDatabase::database("secondConnection", false);
    if (!db_.isValid()) {
      db_ = QSqlDatabase::addDatabase("QSQLITE", "secondConnection");
      db_.setDatabaseName(rssl_->dbFileName_);
      db_.open();
    } else {
      db_ = QSqlDatabase::database();
    }
  }

  parseTimer_ = new QTimer(this);
  parseTimer_->setSingleShot(true);
  parseTimer_->setInterval(50);
  connect(parseTimer_, SIGNAL(timeout()), this, SLOT(getQueuedXml()));

  connect(this, SIGNAL(signalReadyParse(QByteArray,int,QDateTime,QString)),
          SLOT(slotParse(QByteArray,int,QDateTime,QString)));
}

/** @brief Queueing xml-data
 *----------------------------------------------------------------------------*/
void ParseObject::parseXml(QByteArray data, int feedId,
                           QDateTime dtReply, QString codecName)
{
  idsQueue_.enqueue(feedId);
  xmlsQueue_.enqueue(data);
  dtReadyQueue_.enqueue(dtReply);
  codecNameQueue_.enqueue(codecName);
  qDebug() << "xmlsQueue_ <<" << feedId << "count=" << xmlsQueue_.count();

  if (!parseTimer_->isActive())
    parseTimer_->start();
}

/** @brief Process xml-data queue
 *----------------------------------------------------------------------------*/
void ParseObject::getQueuedXml()
{
  if (currentFeedId_) return;

  if (idsQueue_.count()) {
    currentFeedId_ = idsQueue_.dequeue();
    QByteArray currentXml_ = xmlsQueue_.dequeue();
    QDateTime currentDtReady_ = dtReadyQueue_.dequeue();
    QString currentCodecName_ = codecNameQueue_.dequeue();
    qDebug() << "xmlsQueue_ >>" << currentFeedId_ << "count=" << xmlsQueue_.count();

    emit signalReadyParse(currentXml_, currentFeedId_, currentDtReady_, currentCodecName_);

    currentFeedId_ = 0;
    parseTimer_->start();
  }
}

/** @brief Parse xml-data
 *----------------------------------------------------------------------------*/
void ParseObject::slotParse(const QByteArray &xmlData, const int &feedId,
                            const QDateTime &dtReply, const QString &codecName)
{
  if (!rssl_->lastFeedPath_.isEmpty()) {
    QFile file(rssl_->lastFeedPath_ + QDir::separator() + "lastfeed.dat");
    file.open(QIODevice::WriteOnly);
    file.write(xmlData);
    file.close();
  }

  qDebug() << "=================== parseXml:start ============================";

  // extract feed id and duplicate news mode from feed table
  parseFeedId_ = feedId;
  QString feedUrl;
  duplicateNewsMode_ = false;
  QSqlQuery q(db_);
  q.setForwardOnly(true);
  q.exec(QString("SELECT duplicateNewsMode, xmlUrl FROM feeds WHERE id=='%1'").arg(parseFeedId_));
  if (q.first()) {
    duplicateNewsMode_ = q.value(0).toBool();
    feedUrl = q.value(1).toString();
  }

  // id not found (ex. feed deleted while updating)
  if (feedUrl.isEmpty()) {
    qDebug() << QString("Feed with id = '%1' not found").arg(parseFeedId_);
    emit signalFinishUpdate(parseFeedId_, false, 0, "0");
    return;
  }

  qDebug() << QString("Feed '%1' found with id = %2").arg(feedUrl).arg(parseFeedId_);

  // actually parsing
  feedChanged_ = false;

  bool codecOk = false;
  QString convertData(xmlData);
  QString feedType;
  QDomDocument doc;
  QString errorStr;
  int errorLine;
  int errorColumn;

  db_.transaction();

  QRegExp rx("encoding=\"([^\"]+)",
             Qt::CaseInsensitive, QRegExp::RegExp2);
  int pos = rx.indexIn(xmlData);
  if (pos == -1) {
    rx.setPattern("encoding='([^']+)");
    pos = rx.indexIn(xmlData);
  }
  if (pos > -1) {
    QString codecNameT = rx.cap(1);
    qDebug() << "Codec name:" << codecNameT;
    QTextCodec *codec = QTextCodec::codecForName(codecNameT.toUtf8());
    if (codec) {
      qDebug() << "Codec found 1";
      convertData = codec->toUnicode(xmlData);
    } else {
      if (codecNameT.contains("us-ascii", Qt::CaseInsensitive)) {
        QString str(xmlData);
        convertData = str.remove(rx.cap(0)+"\"");
      }
    }
  } else {
    if (!codecName.isEmpty()) {
      qDebug() << "Codec name:" << codecName;
      QTextCodec *codec = QTextCodec::codecForName(codecName.toUtf8());
      if (codec) {
        qDebug() << "Codec found 2";
        convertData = codec->toUnicode(xmlData);
        codecOk = true;
      }
    }
    if (!codecOk) {
      codecOk = false;
      QStringList codecNameList;
      codecNameList << "UTF-8" << "Windows-1251" << "KOI8-R" << "KOI8-U"
                    << "ISO 8859-5" << "IBM 866";
      foreach (QString codecNameT, codecNameList) {
        QTextCodec *codec = QTextCodec::codecForName(codecNameT.toUtf8());
        if (codec && codec->canEncode(xmlData)) {
          qDebug() << "Codec name:" << codecNameT;
          qDebug() << "Codec found 3";
          convertData = codec->toUnicode(xmlData);
          codecOk = true;
          break;
        }
      }
      if (!codecOk) {
        convertData = QString::fromLocal8Bit(xmlData);
      }
    }
  }

  if (!doc.setContent(convertData, false, &errorStr, &errorLine, &errorColumn)) {
    qDebug() << QString("Parse data error: line %1, column %2: %3").
                arg(errorLine).arg(errorColumn).arg(errorStr);
  } else {
    QDomElement rootElem = doc.documentElement();
    feedType = rootElem.tagName();
    qDebug() << "Feed type: " << feedType;

    if (feedType == "feed") {
      parseAtom(feedUrl, doc);
    } else if ((feedType == "rss") || (feedType == "rdf:RDF")) {
      parseRss(feedUrl, doc);
    }
  }

  // Set feed update time and recieve data from server time
  QString updated = QLocale::c().toString(QDateTime::currentDateTimeUtc(),
                                          "yyyy-MM-ddTHH:mm:ss");
  QString lastBuildDate = dtReply.toString(Qt::ISODate);
  q.prepare("UPDATE feeds SET updated=?, lastBuildDate=?, status=0 WHERE id=?");
  q.addBindValue(updated);
  q.addBindValue(lastBuildDate);
  q.addBindValue(parseFeedId_);
  q.exec();

  int newCount = 0;
  if (feedChanged_) {
    setUserFilter(db_, parseFeedId_);
    newCount = recountFeedCounts(parseFeedId_, feedUrl, updated, lastBuildDate);
  }

  q.finish();
  db_.commit();

  emit signalFinishUpdate(parseFeedId_, feedChanged_, newCount, "0");
  qDebug() << "=================== parseXml:finish ===========================";
}

void ParseObject::parseAtom(const QString &feedUrl, const QDomDocument &doc)
{
  QDomElement rootElem = doc.documentElement();
  FeedItemStruct feedItem;

  feedItem.linkBase = rootElem.attribute("xml:base");
  feedItem.title = toPlainText(rootElem.namedItem("title").toElement().text());
  feedItem.description = rootElem.namedItem("subtitle").toElement().text();
  feedItem.updated = rootElem.namedItem("updated").toElement().text();
  feedItem.updated = parseDate(feedItem.updated, feedUrl);
  QDomElement authorElem = rootElem.namedItem("author").toElement();
  if (!authorElem.isNull()) {
    feedItem.author = toPlainText(authorElem.namedItem("name").toElement().text());
    if (feedItem.author.isEmpty()) feedItem.author = toPlainText(authorElem.text());
    feedItem.authorUri = authorElem.namedItem("uri").toElement().text();
    feedItem.authorEmail = authorElem.namedItem("email").toElement().text();
  }
  feedItem.language = rootElem.namedItem("language").toElement().text();
  QDomNodeList linksList = rootElem.elementsByTagName("link");
  for (int j = 0; j < linksList.size(); j++) {
    if (linksList.at(j).toElement().attribute("rel") == "alternate") {
      feedItem.link = linksList.at(j).toElement().attribute("href");
      break;
    }
  }
  if (QUrl(feedItem.link).host().isEmpty())
    feedItem.link = feedItem.linkBase + feedItem.link;

  QSqlQuery q(db_);
  q.setForwardOnly(true);
  QString qStr ("UPDATE feeds "
                "SET title=?, description=?, htmlUrl=?, "
                "author_name=?, author_email=?, "
                "author_uri=?, pubdate=?, language=? "
                "WHERE id==?");
  q.prepare(qStr);
  q.addBindValue(feedItem.title);
  q.addBindValue(feedItem.description);
  q.addBindValue(feedItem.link);
  q.addBindValue(feedItem.author);
  q.addBindValue(feedItem.authorEmail);
  q.addBindValue(feedItem.authorUri);
  q.addBindValue(feedItem.updated);
  q.addBindValue(feedItem.language);
  q.addBindValue(parseFeedId_);
  q.exec();

  QDomNodeList newsList = doc.elementsByTagName("entry");
  for (int i = 0; i < newsList.size(); i++) {
    NewsItemStruct newsItem;
    newsItem.id = newsList.item(i).namedItem("id").toElement().text();
    newsItem.title = toPlainText(newsList.item(i).namedItem("title").toElement().text());
    newsItem.updated = newsList.item(i).namedItem("published").toElement().text();
    if (newsItem.updated.isEmpty())
      newsItem.updated = newsList.item(i).namedItem("updated").toElement().text();
    newsItem.updated = parseDate(newsItem.updated, feedUrl);
    QDomElement authorElem = newsList.item(i).namedItem("author").toElement();
    if (!authorElem.isNull()) {
      newsItem.author = toPlainText(authorElem.namedItem("name").toElement().text());
      if (newsItem.author.isEmpty()) newsItem.author = toPlainText(authorElem.text());
      newsItem.authorUri = authorElem.namedItem("uri").toElement().text();
      newsItem.authorEmail = authorElem.namedItem("email").toElement().text();
    }
    newsItem.description = newsList.item(i).namedItem("summary").toElement().text();
    newsItem.content = newsList.item(i).namedItem("content").toElement().text();
    QDomNodeList categoryElem = newsList.item(i).toElement().elementsByTagName("category");
    for (int j = 0; j < categoryElem.size(); j++) {
      if (!newsItem.category.isEmpty()) newsItem.category.append(", ");
      QString category = categoryElem.at(j).toElement().attribute("label");
      if (category.isEmpty())
        category = categoryElem.at(j).toElement().attribute("term");
      newsItem.category.append(toPlainText(category));
    }
    QDomElement enclosureElem = newsList.item(i).namedItem("enclosure").toElement();
    newsItem.eUrl = enclosureElem.attribute("url");
    newsItem.eType = enclosureElem.attribute("type");
    newsItem.eLength = enclosureElem.attribute("length");
    QDomNodeList linksList = newsList.item(i).toElement().elementsByTagName("link");
    for (int j = 0; j < linksList.size(); j++) {
      if (linksList.at(j).toElement().attribute("type") == "text/html") {
        if (linksList.at(j).toElement().attribute("rel") == "self")
          newsItem.link = linksList.at(j).toElement().attribute("href");
        if (linksList.at(j).toElement().attribute("rel") == "alternate")
          newsItem.linkAlternate = linksList.at(j).toElement().attribute("href");
        if (linksList.at(j).toElement().attribute("rel") == "replies")
          newsItem.comments = linksList.at(j).toElement().attribute("href");
      } else if (newsItem.linkAlternate.isEmpty()) {
        if (!(linksList.at(j).toElement().attribute("rel") == "self"))
          newsItem.linkAlternate = linksList.at(j).toElement().attribute("href");
      }
    }
    if (!newsItem.link.isEmpty() && QUrl(newsItem.link).host().isEmpty())
      newsItem.link = feedItem.linkBase + newsItem.link;
    if (!newsItem.linkAlternate.isEmpty() && QUrl(newsItem.linkAlternate).host().isEmpty())
      newsItem.linkAlternate = feedItem.linkBase + newsItem.linkAlternate;

    addAtomNewsIntoBase(newsItem);
  }
}

void ParseObject::addAtomNewsIntoBase(NewsItemStruct &newsItem)
{
  // search news duplicates in base
  QSqlQuery q(db_);
  q.setForwardOnly(true);
  QString qStr;
  qDebug() << "atomId:" << newsItem.id;
  qDebug() << "title:" << newsItem.title;
  qDebug() << "published:" << newsItem.updated;

  if (!newsItem.updated.isEmpty()) {  // search by pubDate
    if (!duplicateNewsMode_)
      qStr.append("AND published=:published");
  } else {
    qStr.append("AND title LIKE :title");
  }

  if (!newsItem.id.isEmpty()) {       // search by guid
    if (duplicateNewsMode_) {
      q.prepare("SELECT * FROM news WHERE feedId=:id AND guid=:guid");
    } else {
      q.prepare(QString("SELECT * FROM news WHERE feedId=:id AND guid=:guid %1").arg(qStr));
      if (!newsItem.updated.isEmpty()) {    // search by pubDate
        q.bindValue(":published", newsItem.updated);
      } else {
        q.bindValue(":title", newsItem.title);
      }
    }
    q.bindValue(":guid", newsItem.id);
  } else {
    q.prepare(QString("SELECT * FROM news WHERE feedId=:id %1").arg(qStr));
    if (!newsItem.updated.isEmpty()) {    // search by pubDate
      if (!duplicateNewsMode_)
        q.bindValue(":published", newsItem.updated);
    } else {
      q.bindValue(":title", newsItem.title);
    }
  }
  q.bindValue(":id", parseFeedId_);
  q.exec();

  // Check request correctness
  if (q.lastError().isValid())
    qDebug() << "ERROR: q.exec(" << qStr << ") -> " << q.lastError().text();
  else {
    // if duplicates not found, add news into base
    if (!q.first()) {
      bool read = false;
      if (rssl_->markIdenticalNewsRead_) {
        q.prepare("SELECT * FROM news WHERE feedId!=:id AND title LIKE :title");
        q.bindValue(":id", parseFeedId_);
        q.bindValue(":title", newsItem.title);
        q.exec();
        if (q.first()) read = true;
      }

      qStr = QString("INSERT INTO news("
                     "feedId, description, content, guid, title, author_name, "
                     "author_uri, author_email, published, received, "
                     "link_href, link_alternate, category, comments, "
                     "enclosure_url, enclosure_type, enclosure_length, new, read) "
                     "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
      q.prepare(qStr);
      q.addBindValue(parseFeedId_);
      q.addBindValue(newsItem.description);
      q.addBindValue(newsItem.content);
      q.addBindValue(newsItem.id);
      q.addBindValue(newsItem.title);
      q.addBindValue(newsItem.author);
      q.addBindValue(newsItem.authorUri);
      q.addBindValue(newsItem.authorEmail);
      if (newsItem.updated.isEmpty())
        newsItem.updated = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
      q.addBindValue(newsItem.updated);
      q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
      q.addBindValue(newsItem.link);
      q.addBindValue(newsItem.linkAlternate);
      q.addBindValue(newsItem.category);
      q.addBindValue(newsItem.comments);
      q.addBindValue(newsItem.eUrl);
      q.addBindValue(newsItem.eType);
      q.addBindValue(newsItem.eLength);
      q.addBindValue(read ? 0 : 1);
      q.addBindValue(read ? 2 : 0);
      q.exec();
//      qDebug() << "q.exec(" << q.lastQuery() << ")";
      qDebug() << "       " << parseFeedId_;
      qDebug() << "       " << newsItem.description;
      qDebug() << "       " << newsItem.content;
      qDebug() << "       " << newsItem.id;
      qDebug() << "       " << newsItem.title;
      qDebug() << "       " << newsItem.author;
      qDebug() << "       " << newsItem.authorUri;
      qDebug() << "       " << newsItem.authorEmail;
      qDebug() << "       " << newsItem.updated;
      qDebug() << "       " << QDateTime::currentDateTime().toString();
      qDebug() << "       " << newsItem.link;
      qDebug() << "       " << newsItem.linkAlternate;
      qDebug() << "       " << newsItem.category;
      qDebug() << "       " << newsItem.comments;
      qDebug() << "       " << newsItem.eUrl;
      qDebug() << "       " << newsItem.eType;
      qDebug() << "       " << newsItem.eLength;
      feedChanged_ = true;
    }
  }
}

void ParseObject::parseRss(const QString &feedUrl, const QDomDocument &doc)
{
  QDomNode channel = doc.documentElement().namedItem("channel");
  FeedItemStruct feedItem;

  feedItem.title = toPlainText(channel.namedItem("title").toElement().text());
  feedItem.description = channel.namedItem("description").toElement().text();
  feedItem.link = channel.namedItem("link").toElement().text();
  feedItem.updated = channel.namedItem("pubDate").toElement().text();
  if (feedItem.updated.isEmpty())
    feedItem.updated = channel.namedItem("pubdate").toElement().text();
  feedItem.updated = parseDate(feedItem.updated, feedUrl);
  feedItem.author = toPlainText(channel.namedItem("author").toElement().text());
  feedItem.language = channel.namedItem("language").toElement().text();

  QSqlQuery q(db_);
  q.setForwardOnly(true);
  QString qStr("UPDATE feeds "
               "SET title=?, description=?, htmlUrl=?, "
               "author_name=?, pubdate=?, language=? "
               "WHERE id==?");
  q.prepare(qStr);
  q.addBindValue(feedItem.title);
  q.addBindValue(feedItem.description);
  q.addBindValue(feedItem.link);
  q.addBindValue(feedItem.author);
  q.addBindValue(feedItem.updated);
  q.addBindValue(feedItem.language);
  q.addBindValue(parseFeedId_);
  q.exec();

  QDomNodeList newsList = doc.elementsByTagName("item");
  for (int i = 0; i < newsList.size(); i++) {
    NewsItemStruct newsItem;
    newsItem.id = newsList.item(i).namedItem("guid").toElement().text();
    newsItem.title = toPlainText(newsList.item(i).namedItem("title").toElement().text());
    newsItem.updated = newsList.item(i).namedItem("pubDate").toElement().text();
    if (newsItem.updated.isEmpty())
      newsItem.updated = newsList.item(i).namedItem("pubdate").toElement().text();
    newsItem.updated = parseDate(newsItem.updated, feedUrl);
    newsItem.author = toPlainText(newsList.item(i).namedItem("author").toElement().text());
    if (newsItem.author.isEmpty())
      newsItem.author = toPlainText(newsList.item(i).namedItem("dc:creator").toElement().text());
    newsItem.link = newsList.item(i).namedItem("link").toElement().text().simplified();
    newsItem.description = newsList.item(i).namedItem("description").toElement().text();
    newsItem.content = newsList.item(i).namedItem("content:encoded").toElement().text();
    QDomNodeList categoryElem = newsList.item(i).toElement().elementsByTagName("category");
    for (int j = 0; j < categoryElem.size(); j++) {
      if (!newsItem.category.isEmpty()) newsItem.category.append(", ");
      newsItem.category.append(toPlainText(categoryElem.at(j).toElement().text()));
    }
    newsItem.comments = newsList.item(i).namedItem("comments").toElement().text();
    QDomElement enclosureElem = newsList.item(i).namedItem("enclosure").toElement();
    newsItem.eUrl = enclosureElem.attribute("url");
    newsItem.eType = enclosureElem.attribute("type");
    newsItem.eLength = enclosureElem.attribute("length");

    addRssNewsIntoBase(newsItem);
  }
}

void ParseObject::addRssNewsIntoBase(NewsItemStruct &newsItem)
{
  // search news duplicates in base
  QSqlQuery q(db_);
  q.setForwardOnly(true);
  QString qStr;
  QString qStr1;
  qDebug() << "guid:     " << newsItem.id;
  qDebug() << "link_href:" << newsItem.link;
  qDebug() << "title:"     << newsItem.title;
  qDebug() << "published:" << newsItem.updated;

  if (!newsItem.updated.isEmpty()) {  // search by pubDate
    if (!duplicateNewsMode_)
      qStr.append(" AND published=:published");
    qStr1.append(" OR (title LIKE :title AND published=:published)");
  } else {
    qStr.append(" AND title LIKE :title");
  }

  if (!newsItem.id.isEmpty()) {       // search by guid
    q.prepare(QString("SELECT * FROM news WHERE feedId=:id AND ((guid=:guid%1)%2)").
              arg(qStr).arg(qStr1));
    q.bindValue(":guid", newsItem.id);
  } else if (!newsItem.link.isEmpty()) {   // search by link_href
    q.prepare(QString("SELECT * FROM news WHERE feedId=:id AND ((link_href=:link_href%1)%2)").
              arg(qStr).arg(qStr1));
    q.bindValue(":link_href", newsItem.link);
  } else {
    qStr.remove(" AND ");
    q.prepare(QString("SELECT * FROM news WHERE feedId=:id AND (%1%2)").arg(qStr).arg(qStr1));
  }
  q.bindValue(":id", parseFeedId_);
  if (!newsItem.updated.isEmpty()) {    // search by pubDate
    q.bindValue(":published", newsItem.updated);
  }
  q.bindValue(":title", newsItem.title);
  q.exec();

  // Check request correctness
  if (q.lastError().isValid())
    qDebug() << "ERROR: q.exec(" << qStr << ") -> " << q.lastError().text();
  else {
    // if duplicates not found, add news into base
    if (!q.first()) {
      bool read = false;
      if (rssl_->markIdenticalNewsRead_) {
        q.prepare("SELECT * FROM news WHERE feedId!=:id AND title LIKE :title");
        q.bindValue(":id", parseFeedId_);
        q.bindValue(":title", newsItem.title);
        q.exec();
        if (q.first()) read = true;
      }

      qStr = QString("INSERT INTO news("
                     "feedId, description, content, guid, title, author_name, "
                     "published, received, link_href, category, comments, "
                     "enclosure_url, enclosure_type, enclosure_length, new, read) "
                     "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
      q.prepare(qStr);
      q.addBindValue(parseFeedId_);
      q.addBindValue(newsItem.description);
      q.addBindValue(newsItem.content);
      q.addBindValue(newsItem.id);
      q.addBindValue(newsItem.title);
      q.addBindValue(newsItem.author);
      if (newsItem.updated.isEmpty())
        newsItem.updated = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
      q.addBindValue(newsItem.updated);
      q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
      q.addBindValue(newsItem.link);
      q.addBindValue(newsItem.category);
      q.addBindValue(newsItem.comments);
      q.addBindValue(newsItem.eUrl);
      q.addBindValue(newsItem.eType);
      q.addBindValue(newsItem.eLength);
      q.addBindValue(read ? 0 : 1);
      q.addBindValue(read ? 2 : 0);
      q.exec();
//      qDebug() << "q.exec(" << q.lastQuery() << ")";
      qDebug() << "       " << parseFeedId_;
      qDebug() << "       " << newsItem.description;
      qDebug() << "       " << newsItem.content;
      qDebug() << "       " << newsItem.id;
      qDebug() << "       " << newsItem.title;
      qDebug() << "       " << newsItem.author;
      qDebug() << "       " << newsItem.updated;
      qDebug() << "       " << QDateTime::currentDateTime().toString();
      qDebug() << "       " << newsItem.link;
      qDebug() << "       " << newsItem.category;
      qDebug() << "       " << newsItem.comments;
      qDebug() << "       " << newsItem.eUrl;
      qDebug() << "       " << newsItem.eType;
      qDebug() << "       " << newsItem.eLength;
      feedChanged_ = true;
    }
  }
}

QString ParseObject::toPlainText(const QString &text)
{
  return QTextDocumentFragment::fromHtml(text.simplified()).toPlainText();
}

/** @brief Date/time string parsing
 *----------------------------------------------------------------------------*/
QString ParseObject::parseDate(const QString &dateString, const QString &urlString)
{
  QDateTime dt;
  QString temp;
  QString timeZone;

  if (dateString.isEmpty()) return QString();

  QString ds = dateString.simplified();
  QLocale locale(QLocale::C);

  if (ds.indexOf(',') != -1) {
    ds = ds.remove(0, ds.indexOf(',')+1).simplified();
  }

  for (int i = 0; i < 2; i++, locale = QLocale::system()) {
    temp     = ds.left(23);
    timeZone = ds.mid(temp.length(), 3);
    dt = locale.toDateTime(temp, "yyyy-MM-ddTHH:mm:ss.z");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp     = ds.left(19);
    timeZone = ds.mid(temp.length(), 3);
    dt = locale.toDateTime(temp, "yyyy-MM-ddTHH:mm:ss");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(23);
    timeZone = ds.mid(temp.length()+1, 3);
    dt = locale.toDateTime(temp, "yyyy-MM-dd HH:mm:ss.z");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(19);
    timeZone = ds.mid(temp.length()+1, 3);
    dt = locale.toDateTime(temp, "yyyy-MM-dd HH:mm:ss");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(20);
    timeZone = ds.mid(temp.length()+1, 3);
    dt = locale.toDateTime(temp, "dd MMM yyyy HH:mm:ss");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(19);
    timeZone = ds.mid(temp.length()+1, 3);
    dt = locale.toDateTime(temp, "d MMM yyyy HH:mm:ss");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(11);
    timeZone = ds.mid(temp.length()+1, 3);
    dt = locale.toDateTime(temp, "dd MMM yyyy");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(10);
    timeZone = ds.mid(temp.length()+1, 3);
    dt = locale.toDateTime(temp, "d MMM yyyy");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(10);
    timeZone = ds.mid(temp.length(), 3);
    dt = locale.toDateTime(temp, "yyyy-MM-dd");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    // @HACK(arhohryakov:2012.01.01):
    // "dd MMM yy HH:mm:ss" format doesn/t parse automatically
    // Reformat it to "dd MMM yyyy HH:mm:ss"
    QString temp2;
    temp2 = ds;  // save ds for output in case of error
    if (70 < ds.mid(7, 2).toInt()) temp2.insert(7, "19");
    else temp2.insert(7, "20");
    temp = temp2.left(20);
    timeZone = ds.mid(temp.length()+1-2, 3);  // "-2", cause 2 symbols inserted
    dt = locale.toDateTime(temp, "dd MMM yyyy HH:mm:ss");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");
  }

  qDebug() << __LINE__ << "parseDate: error with" << dateString << urlString;
  return QString();
}

/** @brief Update feed counts and all its parent categories
 *
 *  Update fields: unread news number,
 *  new news number, categories last update date/time
 * @param feedId - Feed Id
 * @param feedUrl - Feed URL
 * @param updated - Time feed updated
 *----------------------------------------------------------------------------*/
int ParseObject::recountFeedCounts(int feedId, const QString &feedUrl,
                                   const QString &updated, const QString &lastBuildDate)
{
  QSqlQuery q(db_);
  q.setForwardOnly(true);
  QString qStr;
  QString htmlUrl;
  QString title;

  int feedParId = 0;
  q.exec(QString("SELECT parentId, htmlUrl, title FROM feeds WHERE id=='%1'").arg(feedId));
  if (q.first()) {
    feedParId = q.value(0).toInt();
    htmlUrl = q.value(1).toString();
    title = q.value(2).toString();
  }

  int undeleteCount = 0;
  int unreadCount = 0;
  int newNewsCount = 0;

  // Count all news (not marked Deleted)
  qStr = QString("SELECT count(id) FROM news WHERE feedId=='%1' AND deleted==0").
      arg(feedId);
  q.exec(qStr);
  if (q.first()) undeleteCount = q.value(0).toInt();

  // Count unread news
  qStr = QString("SELECT count(read) FROM news WHERE feedId=='%1' AND read==0 AND deleted==0").
      arg(feedId);
  q.exec(qStr);
  if (q.first()) unreadCount = q.value(0).toInt();

  // Count new news
  qStr = QString("SELECT count(new) FROM news WHERE feedId=='%1' AND new==1 AND deleted==0").
      arg(feedId);
  q.exec(qStr);
  if (q.first()) newNewsCount = q.value(0).toInt();

  int unreadCountOld = 0;
  int newCountOld = 0;
  int undeleteCountOld = 0;
  qStr = QString("SELECT unread, newCount, undeleteCount FROM feeds WHERE id=='%1'").
      arg(feedId);
  q.exec(qStr);
  if (q.first()) {
    unreadCountOld = q.value(0).toInt();
    newCountOld = q.value(1).toInt();
    undeleteCountOld = q.value(2).toInt();
  }

  if ((unreadCount == unreadCountOld) && (newNewsCount == newCountOld) &&
      (undeleteCount == undeleteCountOld)) {
    return 0;
  }

  // Set number unread, new and all(undelete) news for feed
  qStr = QString("UPDATE feeds SET unread='%1', newCount='%2', undeleteCount='%3' "
                 "WHERE id=='%4'").
      arg(unreadCount).arg(newNewsCount).arg(undeleteCount).arg(feedId);
  q.exec(qStr);

  FeedCountStruct counts;
  counts.feedId = feedId;
  counts.unreadCount = unreadCount;
  counts.newCount = newNewsCount;
  counts.undeleteCount = undeleteCount;
  counts.updated = updated;
  counts.lastBuildDate = lastBuildDate;
  counts.htmlUrl = htmlUrl;
  counts.xmlUrl = feedUrl;
  counts.title = title;

  emit feedCountsUpdate(counts);

  // Recount counters for all feed parents
  int l_feedParId = feedParId;
  while (l_feedParId) {
    QString updatedParent;
    int newCount = 0;

    qStr = QString("SELECT sum(unread), sum(newCount), sum(undeleteCount), "
                   "max(updated) FROM feeds WHERE parentId=='%1'").
        arg(l_feedParId);
    q.exec(qStr);
    if (q.first()) {
      unreadCount   = q.value(0).toInt();
      newCount      = q.value(1).toInt();
      undeleteCount = q.value(2).toInt();
      updatedParent = q.value(3).toString();
    }
    qStr = QString("UPDATE feeds SET unread='%1', newCount='%2', undeleteCount='%3', "
                   "updated='%4' WHERE id=='%5'").
        arg(unreadCount).arg(newCount).arg(undeleteCount).arg(updatedParent).
        arg(l_feedParId);
    q.exec(qStr);

    FeedCountStruct counts;
    counts.feedId = l_feedParId;

    q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(l_feedParId));
    if (q.first()) l_feedParId = q.value(0).toInt();

    counts.unreadCount = unreadCount;
    counts.newCount = newCount;
    counts.undeleteCount = undeleteCount;
    counts.updated = updatedParent;

    emit feedCountsUpdate(counts);
  }

  return (newNewsCount - newCountOld);
}
