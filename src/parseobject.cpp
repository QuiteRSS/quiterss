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
#include "parseobject.h"

#include "mainapplication.h"
#include "database.h"
#include "VersionNo.h"
#include "common.h"

#include <QDebug>
#include <QDesktopServices>
#include <QTextDocumentFragment>
#if defined(Q_OS_WIN)
#include <windows.h>
#endif
#include <qzregexp.h>

ParseObject::ParseObject(QObject *parent)
  : QObject(parent)
  , currentFeedId_(0)
{
  setObjectName("parseObject_");

  db_ = Database::connection("secondConnection");

  parseTimer_ = new QTimer(this);
  parseTimer_->setSingleShot(true);
  parseTimer_->setInterval(10);
  connect(parseTimer_, SIGNAL(timeout()), this, SLOT(getQueuedXml()),
          Qt::QueuedConnection);

  connect(this, SIGNAL(signalReadyParse(QByteArray,int,QDateTime,QString)),
          SLOT(slotParse(QByteArray,int,QDateTime,QString)));
}

ParseObject::~ParseObject()
{

}

void ParseObject::disconnectObjects()
{
  disconnect(this);
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
  if (mainApp->isSaveDataLastFeed()) {
    QFile file(mainApp->dataDir()  + "/lastfeed.dat");
    file.open(QIODevice::WriteOnly);
    file.write(xmlData);
    file.close();
  }

  qDebug() << "=================== parseXml:start ============================";

  db_.transaction();

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
    qWarning() << QString("Feed with id = '%1' not found").arg(parseFeedId_);
    emit signalFinishUpdate(parseFeedId_, false, 0, "0");
    db_.commit();
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

  QzRegExp rx("encoding=\"([^\"]+)", Qt::CaseInsensitive);
  int pos = rx.indexIn(xmlData);
  if (pos == -1) {
    rx.setPattern("encoding='([^']+)");
    pos = rx.indexIn(xmlData);
  }
  if (pos > -1) {
    QString codecNameT = rx.cap(1);
    qDebug() << "Codec name (1):" << codecNameT;
    QTextCodec *codec = QTextCodec::codecForName(codecNameT.toUtf8());
    if (codec) {
      convertData = codec->toUnicode(xmlData);
    } else {
      qWarning() << "Codec not found (1): " << codecNameT << feedUrl;
      if (codecNameT.contains("us-ascii", Qt::CaseInsensitive)) {
        QString str(xmlData);
        convertData = str.remove(rx.cap(0)+"\"");
      }
    }
  } else {
    if (!codecName.isEmpty()) {
      qDebug() << "Codec name (2):" << codecName;
      QTextCodec *codec = QTextCodec::codecForName(codecName.toUtf8());
      if (codec) {
        convertData = codec->toUnicode(xmlData);
        codecOk = true;
      } else {
        qWarning() << "Codec not found (2): " << codecName << feedUrl;
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
          qDebug() << "Codec name (3):" << codecNameT;
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
    qWarning() << QString("Parse data error (2): url %1, id %2, line %3, column %4: %5").
                  arg(feedUrl).arg(parseFeedId_).
                  arg(errorLine).arg(errorColumn).arg(errorStr);
  } else {
    QDomElement rootElem = doc.documentElement();
    feedType = rootElem.tagName();
    qDebug() << "Feed type: " << feedType;

    q.exec(QString("SELECT id, guid, title, published, link_href FROM news WHERE feedId='%1'").
           arg(parseFeedId_));
    if (q.lastError().isValid()) {
      qWarning() << __PRETTY_FUNCTION__ << __LINE__
                 << "q.lastError(): " << q.lastError().text();
    }
    else {
      while (q.next()) {
        QString str = q.value(2).toString();
        titleList_.append(str);

        str = q.value(1).toString();
        guidList_.append(str);

        str = q.value(3).toString();
        publishedList_.append(str);
        str = q.value(4).toString();
        linkList_.append(str);
      }
    }
    q.finish();

    if (feedType == "feed") {
      parseAtom(feedUrl, doc);
    } else if ((feedType == "rss") || (feedType == "rdf:RDF")) {
      parseRss(feedUrl, doc);
    }

    guidList_.clear();
    titleList_.clear();
    publishedList_.clear();
    linkList_.clear();
  }

  // Set feed update time and receive data from server time
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
    runUserFilter(parseFeedId_);
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
  if (feedItem.link.isEmpty()) {
    for (int j = 0; j < linksList.size(); j++) {
        if (!(linksList.at(j).toElement().attribute("rel") == "self")) {
          feedItem.link = linksList.at(j).toElement().attribute("href");
          break;
        }
    }
  }

  if (QUrl(feedItem.link).host().isEmpty() || (QUrl(feedItem.link).host().indexOf('.')) == -1) {
    if (!feedItem.linkBase.isEmpty() && !QUrl(feedItem.linkBase).host().isEmpty())
      feedItem.link = QUrl(feedItem.linkBase).scheme() %  "://" % QUrl(feedItem.linkBase).host();
    else
      feedItem.link = QUrl(feedUrl).scheme() %  "://" % QUrl(feedUrl).host();
  }
  if (feedItem.linkBase.isEmpty() && !QUrl(feedItem.link).host().isEmpty())
    feedItem.linkBase = QUrl(feedItem.link).scheme() %  "://" % QUrl(feedItem.link).host();
  if (QUrl(feedItem.link).host().isEmpty())
    feedItem.link = feedItem.linkBase + feedItem.link;
  feedItem.link = toPlainText(feedItem.link);

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
    QDomNode nodeContent = newsList.item(i).namedItem("content");
    if (nodeContent.toElement().attribute("type") == "xhtml") {
      QTextStream in(&newsItem.content);
      nodeContent.save(in, 0);
    } else {
      newsItem.content = nodeContent.toElement().text();
    }
    if (!(newsItem.content.isEmpty() ||
          (newsItem.description.length() > newsItem.content.length()))) {
      newsItem.description = newsItem.content;
    }
    newsItem.content.clear();

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
        if (linksList.at(j).toElement().attribute("rel") == "alternate")
          newsItem.linkAlternate = linksList.at(j).toElement().attribute("href");
      }
    }
    for (int j = 0; j < linksList.size(); j++) {
      if (newsItem.linkAlternate.isEmpty()) {
        if (!(linksList.at(j).toElement().attribute("rel") == "self")) {
          newsItem.linkAlternate = linksList.at(j).toElement().attribute("href");
          break;
        }
      }
    }

    if (!newsItem.link.isEmpty() && QUrl(newsItem.link).host().isEmpty())
      newsItem.link = feedItem.linkBase + newsItem.link;
    newsItem.link = toPlainText(newsItem.link);
    if (!newsItem.linkAlternate.isEmpty() && QUrl(newsItem.linkAlternate).host().isEmpty())
      newsItem.linkAlternate = feedItem.linkBase + newsItem.linkAlternate;
    newsItem.linkAlternate = toPlainText(newsItem.linkAlternate);
    if (newsItem.link.isEmpty()) {
      newsItem.link = newsItem.linkAlternate;
      newsItem.linkAlternate.clear();
    }

    addAtomNewsIntoBase(&newsItem);
  }
}

void ParseObject::addAtomNewsIntoBase(NewsItemStruct *newsItem)
{
  Common::sleep(5);

  // search news duplicates in base
  QSqlQuery q(db_);
  q.setForwardOnly(true);
  QString qStr;
  qDebug() << "atomId:" << newsItem->id;
  qDebug() << "title:" << newsItem->title;
  qDebug() << "published:" << newsItem->updated;

  bool isDuplicate = false;
  for (int i = 0; i < guidList_.count(); ++i) {
    if (!newsItem->id.isEmpty()) {         // search by guid if present
      if (guidList_.at(i) == newsItem->id) {
        if (duplicateNewsMode_) {       // autodelete duplicate news enabled
          isDuplicate = true;
        } else {                        // autodelete dupl. news disabled
          if (!newsItem->updated.isEmpty()) {  // search by pubDate if present
            if (publishedList_.at(i) == newsItem->updated)
              isDuplicate = true;
          } else {                      // ... or by title
            if (!newsItem->title.isEmpty() && (titleList_.at(i) == newsItem->title))
              isDuplicate = true;
          }
        }
      }
    } else {                                // guid is absent
      if (!newsItem->updated.isEmpty()) {    // search by pubDate if present
        if (publishedList_.at(i) == newsItem->updated)
          isDuplicate = true;
      } else {                              // ... or by title
        if (!newsItem->title.isEmpty() && (titleList_.at(i) == newsItem->title))
          isDuplicate = true;
      }
    }
    if (isDuplicate) break;
  }

  // if duplicates not found, add news into base
  if (!isDuplicate) {
    bool read = false;
    if (mainApp->mainWindow()->markIdenticalNewsRead_) {
      q.prepare("SELECT id FROM news WHERE title LIKE :title AND feedId!=:id");
      q.bindValue(":id", parseFeedId_);
      q.bindValue(":title", newsItem->title);
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
    q.addBindValue(newsItem->description);
    q.addBindValue(newsItem->content);
    q.addBindValue(newsItem->id);
    q.addBindValue(newsItem->title);
    q.addBindValue(newsItem->author);
    q.addBindValue(newsItem->authorUri);
    q.addBindValue(newsItem->authorEmail);
    QString updated = newsItem->updated;
    if (updated.isEmpty())
      updated = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    q.addBindValue(updated);
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    q.addBindValue(newsItem->link);
    q.addBindValue(newsItem->linkAlternate);
    q.addBindValue(newsItem->category);
    q.addBindValue(newsItem->comments);
    q.addBindValue(newsItem->eUrl);
    q.addBindValue(newsItem->eType);
    q.addBindValue(newsItem->eLength);
    q.addBindValue(read ? 0 : 1);
    q.addBindValue(read ? 2 : 0);
    if (!q.exec()) {
      qWarning() << __PRETTY_FUNCTION__ << __LINE__
                 << "q.lastError(): " << q.lastError().text();
    }
    q.finish();
    qDebug() << "q.exec(" << q.lastQuery() << ")";
    qDebug() << "       " << parseFeedId_;
    qDebug() << "       " << newsItem->description;
    qDebug() << "       " << newsItem->content;
    qDebug() << "       " << newsItem->id;
    qDebug() << "       " << newsItem->title;
    qDebug() << "       " << newsItem->author;
    qDebug() << "       " << newsItem->authorUri;
    qDebug() << "       " << newsItem->authorEmail;
    qDebug() << "       " << newsItem->updated;
    qDebug() << "       " << QDateTime::currentDateTime().toString();
    qDebug() << "       " << newsItem->link;
    qDebug() << "       " << newsItem->linkAlternate;
    qDebug() << "       " << newsItem->category;
    qDebug() << "       " << newsItem->comments;
    qDebug() << "       " << newsItem->eUrl;
    qDebug() << "       " << newsItem->eType;
    qDebug() << "       " << newsItem->eLength;
    feedChanged_ = true;
  }
}

void ParseObject::parseRss(const QString &feedUrl, const QDomDocument &doc)
{
  QDomNode channel = doc.documentElement().namedItem("channel");
  if (channel.isNull())
    channel = doc.documentElement().namedItem("rss:channel");
  FeedItemStruct feedItem;

  feedItem.title = toPlainText(channel.namedItem("title").toElement().text());
  if (feedItem.title.isEmpty())
    feedItem.title = toPlainText(channel.namedItem("rss:title").toElement().text());
  feedItem.description = channel.namedItem("description").toElement().text();
  if (feedItem.description.isEmpty())
    feedItem.description = toPlainText(channel.namedItem("rss:description").toElement().text());
  feedItem.link = toPlainText(channel.namedItem("link").toElement().text());
  if (feedItem.link.isEmpty())
    feedItem.link = toPlainText(channel.namedItem("rss:link").toElement().text());
  QUrl url = QUrl(feedItem.link);
  if (url.host().isEmpty())
    url.setHost(QUrl(feedUrl).host());
  if (url.scheme().isEmpty())
    url.setScheme(QUrl(feedUrl).scheme());
  feedItem.link = url.toString();

  feedItem.updated = channel.namedItem("pubDate").toElement().text();
  if (feedItem.updated.isEmpty())
    feedItem.updated = channel.namedItem("pubdate").toElement().text();
  feedItem.updated = parseDate(feedItem.updated, feedUrl);
  feedItem.author = toPlainText(channel.namedItem("author").toElement().text());
  feedItem.language = channel.namedItem("language").toElement().text();
  if (feedItem.language.isEmpty())
    feedItem.language = channel.namedItem("dc:language").toElement().text();

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
  if (newsList.isEmpty())
    newsList = doc.elementsByTagName("rss:item");
  for (int i = 0; i < newsList.size(); i++) {
    NewsItemStruct newsItem;
    newsItem.id = newsList.item(i).namedItem("guid").toElement().text();
    newsItem.title = toPlainText(newsList.item(i).namedItem("title").toElement().text());
    if (newsItem.title.isEmpty())
      newsItem.title = toPlainText(newsList.item(i).namedItem("rss:title").toElement().text());
    newsItem.updated = newsList.item(i).namedItem("pubDate").toElement().text();
    if (newsItem.updated.isEmpty())
      newsItem.updated = newsList.item(i).namedItem("pubdate").toElement().text();
    if (newsItem.updated.isEmpty())
      newsItem.updated = newsList.item(i).namedItem("dc:date").toElement().text();
    newsItem.updated = parseDate(newsItem.updated, feedUrl);
    newsItem.author = toPlainText(newsList.item(i).namedItem("author").toElement().text());
    if (newsItem.author.isEmpty())
      newsItem.author = toPlainText(newsList.item(i).namedItem("dc:creator").toElement().text());
    newsItem.link = toPlainText(newsList.item(i).namedItem("link").toElement().text());
    if (newsItem.link.isEmpty()) {
        newsItem.link = toPlainText(newsList.item(i).namedItem("rss:link").toElement().text());
        if (newsItem.link.isEmpty()) {
            if (newsList.item(i).namedItem("guid").toElement().attribute("isPermaLink") == "true")
                newsItem.link = newsItem.id;
        }
    }
    url = QUrl(newsItem.link);
    if (url.host().isEmpty())
      url.setHost(QUrl(feedUrl).host());
    if (url.scheme().isEmpty())
      url.setScheme(QUrl(feedUrl).scheme());
    newsItem.link = url.toString();

    newsItem.description = newsList.item(i).namedItem("description").toElement().text();
    newsItem.content = newsList.item(i).namedItem("content:encoded").toElement().text();
    if (newsItem.content.isEmpty() || (newsItem.description.length() > newsItem.content.length())) {
      newsItem.content.clear();
    } else {
      newsItem.description = newsItem.content;
    }

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

    if (newsItem.title.isEmpty()) {
      newsItem.title = toPlainText(newsItem.description);
      if (newsItem.title.size() > 50) {
        newsItem.title.resize(50);
        newsItem.title = newsItem.title % "...";
      }
    }

    addRssNewsIntoBase(&newsItem);
  }
}

void ParseObject::addRssNewsIntoBase(NewsItemStruct *newsItem)
{
  Common::sleep(5);

  // search news duplicates in base
  QSqlQuery q(db_);
  q.setForwardOnly(true);
  QString qStr;
  qDebug() << "guid:     " << newsItem->id;
  qDebug() << "link_href:" << newsItem->link;
  qDebug() << "title:"     << newsItem->title;
  qDebug() << "published:" << newsItem->updated;

  bool isDuplicate = false;
  for (int i = 0; i < guidList_.count(); ++i) {
    if (!newsItem->id.isEmpty()) {         // search by guid if present
      if (guidList_.at(i) == newsItem->id) {
        if (!newsItem->updated.isEmpty()) {  // search by pubDate if present
          if (!duplicateNewsMode_) {
            if (publishedList_.at(i) == newsItem->updated)
              isDuplicate = true;
          }
          else {
            isDuplicate = true;
          }
        } else {                            // ... or by title
          if (!newsItem->title.isEmpty() && (titleList_.at(i) == newsItem->title))
            isDuplicate = true;
        }
      }
      if (!isDuplicate) {
        if (!newsItem->updated.isEmpty()) {
          if ((publishedList_.at(i) == newsItem->updated) &&
              (titleList_.at(i) == newsItem->title)) {
            isDuplicate = true;
          }
        }
      }
    }
    else if (!newsItem->link.isEmpty()) {   // search by link_href
      if (linkList_.at(i) == newsItem->link) {
        if (!newsItem->updated.isEmpty()) {  // search by pubDate if present
          if (!duplicateNewsMode_) {
            if (publishedList_.at(i) == newsItem->updated)
              isDuplicate = true;
          }
          else {
            isDuplicate = true;
          }
        } else {                            // ... or by title
          if (!newsItem->title.isEmpty() && (titleList_.at(i) == newsItem->title))
            isDuplicate = true;
        }
      }
      if (!isDuplicate) {
        if (!newsItem->updated.isEmpty()) {
          if ((publishedList_.at(i) == newsItem->updated) &&
              (titleList_.at(i) == newsItem->title)) {
            isDuplicate = true;
          }
        }
      }
    }
    else {                                // guid is absent
      if (!newsItem->updated.isEmpty()) {  // search by pubDate if present
        if (!duplicateNewsMode_) {
          if (publishedList_.at(i) == newsItem->updated)
            isDuplicate = true;
        }
        else {
          isDuplicate = true;
        }
      } else {                            // ... or by title
        if (!newsItem->title.isEmpty() && (titleList_.at(i) == newsItem->title))
          isDuplicate = true;
      }
      if (!isDuplicate) {
        if (!newsItem->updated.isEmpty()) {
          if ((publishedList_.at(i) == newsItem->updated) &&
              (titleList_.at(i) == newsItem->title)) {
            isDuplicate = true;
          }
        }
      }
    }
    if (isDuplicate) break;
  }

  // if duplicates not found, add news into base
  if (!isDuplicate) {
    bool read = false;
    if (mainApp->mainWindow()->markIdenticalNewsRead_) {
      q.prepare("SELECT id FROM news WHERE title LIKE :title AND feedId!=:id");
      q.bindValue(":id", parseFeedId_);
      q.bindValue(":title", newsItem->title);
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
    q.addBindValue(newsItem->description);
    q.addBindValue(newsItem->content);
    q.addBindValue(newsItem->id);
    q.addBindValue(newsItem->title);
    q.addBindValue(newsItem->author);
    QString updated = newsItem->updated;
    if (updated.isEmpty())
      updated = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    q.addBindValue(updated);
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    q.addBindValue(newsItem->link);
    q.addBindValue(newsItem->category);
    q.addBindValue(newsItem->comments);
    q.addBindValue(newsItem->eUrl);
    q.addBindValue(newsItem->eType);
    q.addBindValue(newsItem->eLength);
    q.addBindValue(read ? 0 : 1);
    q.addBindValue(read ? 2 : 0);
    if (!q.exec()) {
      qWarning() << __PRETTY_FUNCTION__ << __LINE__
                 << "q.lastError(): " << q.lastError().text();
    }
    q.finish();
    qDebug() << "q.exec(" << q.lastQuery() << ")";
    qDebug() << "       " << parseFeedId_;
    qDebug() << "       " << newsItem->description;
    qDebug() << "       " << newsItem->content;
    qDebug() << "       " << newsItem->id;
    qDebug() << "       " << newsItem->title;
    qDebug() << "       " << newsItem->author;
    qDebug() << "       " << newsItem->updated;
    qDebug() << "       " << QDateTime::currentDateTime().toString();
    qDebug() << "       " << newsItem->link;
    qDebug() << "       " << newsItem->category;
    qDebug() << "       " << newsItem->comments;
    qDebug() << "       " << newsItem->eUrl;
    qDebug() << "       " << newsItem->eType;
    qDebug() << "       " << newsItem->eLength;
    feedChanged_ = true;
  }
}

QString ParseObject::toPlainText(const QString &text)
{
  return QTextDocumentFragment::fromHtml(text).toPlainText().simplified();
}

/** @brief Date/time string parsing
 *----------------------------------------------------------------------------*/
QString ParseObject::parseDate(const QString &dateString, const QString &urlString)
{
  QDateTime dt;
  QString temp;
  QString timeZone;

  if (dateString.isEmpty()) return QString();

  QDateTime dtLocalTime = QDateTime::currentDateTime();
  QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
  int nTimeShift = dtLocalTime.secsTo(dtUTC)/3600;

  QString ds = dateString.simplified();
  QLocale locale(QLocale::C);

  if (ds.indexOf(',') != -1) {
    ds = ds.remove(0, ds.indexOf(',')+1).simplified();
  }

  for (int i = 0; i < 2; i++, locale = QLocale::system()) {
    temp     = ds.left(23);
    timeZone = ds.mid(temp.length(), 3);
    if (timeZone.isEmpty()) timeZone = QString::number(nTimeShift);
    dt = locale.toDateTime(temp, "yyyy-MM-ddTHH:mm:ss.z");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp     = ds.left(19);
    timeZone = ds.mid(temp.length(), 3);
    if (timeZone.isEmpty()) timeZone = QString::number(nTimeShift);
    dt = locale.toDateTime(temp, "yyyy-MM-ddTHH:mm:ss");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(23);
    timeZone = ds.mid(temp.length()+1, 3);
    if (timeZone.isEmpty()) timeZone = QString::number(nTimeShift);
    dt = locale.toDateTime(temp, "yyyy-MM-dd HH:mm:ss.z");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(19);
    timeZone = ds.mid(temp.length()+1, 3);
    if (timeZone.isEmpty()) timeZone = QString::number(nTimeShift);
    dt = locale.toDateTime(temp, "yyyy-MM-dd HH:mm:ss");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(20);
    timeZone = ds.mid(temp.length()+1, 3);
    if (timeZone.contains("EDT"))
      timeZone="-4";
    if (timeZone.isEmpty()) timeZone = QString::number(nTimeShift);
    dt = locale.toDateTime(temp, "dd MMM yyyy HH:mm:ss");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(19);
    timeZone = ds.mid(temp.length()+1, 3);
    if (timeZone.isEmpty()) timeZone = QString::number(nTimeShift);
    dt = locale.toDateTime(temp, "d MMM yyyy HH:mm:ss");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(11);
    timeZone = ds.mid(temp.length()+1, 3);
    if (timeZone.isEmpty()) timeZone = QString::number(nTimeShift);
    dt = locale.toDateTime(temp, "dd MMM yyyy");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(10);
    timeZone = ds.mid(temp.length()+1, 3);
    if (timeZone.isEmpty()) timeZone = QString::number(nTimeShift);
    dt = locale.toDateTime(temp, "d MMM yyyy");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");

    temp = ds.left(10);
    timeZone = ds.mid(temp.length(), 3);
    if (timeZone.isEmpty()) timeZone = QString::number(nTimeShift);
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
    if (timeZone.isEmpty()) timeZone = QString::number(nTimeShift);
    dt = locale.toDateTime(temp, "dd MMM yyyy HH:mm:ss");
    if (dt.isValid()) return locale.toString(dt.addSecs(timeZone.toInt() * -3600), "yyyy-MM-ddTHH:mm:ss");
  }

  qDebug() << __LINE__ << "parseDate: error with" << dateString << urlString;
  return QString();
}

/** @brief Apply user filters
 * @param feedId - Feed Id
 * @param filterId - Id of particular filter
 *---------------------------------------------------------------------------*/
void ParseObject::runUserFilter(int feedId, int filterId)
{
  QSqlQuery q(db_);
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
    if ((q.value(0).toInt() == 0) && onlyNew) continue;

    if (onlyNew)
      filterId = q.value(2).toInt();
    int filterType = q.value(1).toInt();

    QString qStr("UPDATE news SET");
    QString qStr1;
    QString qStr2;
    QString whereStr;
    QList<int> idLabelsList;
    QStringList soundList;
    QStringList colorList;

    QSqlQuery q1(db_);
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
        idLabelsList.append(q1.value(1).toInt());
        break;
      case 4: // action -> Play Sound
        soundList.append(q1.value(1).toString());
        break;
      case 5: // action -> Show News in Notifier
        colorList.append(q1.value(1).toString());
        break;
      }
    }

    if (qStr1.isEmpty())
      qStr.clear();
    else
      qStr.append(qStr1);

    whereStr = QString(" WHERE feedId='%1' AND deleted=0").arg(feedId);

    if (onlyNew) whereStr.append(" AND new=1");

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
      whereStr.append(" AND ( ");
      qStr1.clear();

      q1.exec(QString("SELECT field, condition, content FROM filterConditions "
                      "WHERE idFilter=='%1'").arg(filterId));
      while (q1.next()) {
        if (!qStr1.isNull()) qStr1.append(qStr2);
        QString content = q1.value(2).toString().replace("'", "''");
        switch (q1.value(0).toInt()) {
        case 0: // field -> Title
          switch (q1.value(1).toInt()) {
          case 0: // condition -> contains
            qStr1.append(QString("UPPER(title) LIKE '%%1%' ").arg(content.toUpper()));
            break;
          case 1: // condition -> doesn't contains
            qStr1.append(QString("UPPER(title) NOT LIKE '%%1%' ").arg(content.toUpper()));
            break;
          case 2: // condition -> is
            qStr1.append(QString("UPPER(title) LIKE '%1' ").arg(content.toUpper()));
            break;
          case 3: // condition -> isn't
            qStr1.append(QString("UPPER(title) NOT LIKE '%1' ").arg(content.toUpper()));
            break;
          case 4: // condition -> begins with
            qStr1.append(QString("UPPER(title) LIKE '%1%' ").arg(content.toUpper()));
            break;
          case 5: // condition -> ends with
            qStr1.append(QString("UPPER(title) LIKE '%%1' ").arg(content.toUpper()));
            break;
          case 6: // condition -> regExp
            qStr1.append(QString("title REGEXP '%1' ").arg(content));
            break;
          }
          break;
        case 1: // field -> Description
          switch (q1.value(1).toInt()) {
          case 0: // condition -> contains
            qStr1.append(QString("UPPER(description) LIKE '%%1%' ").arg(content.toUpper()));
            break;
          case 1: // condition -> doesn't contains
            qStr1.append(QString("UPPER(description) NOT LIKE '%%1%' ").arg(content.toUpper()));
            break;
          case 2: // condition -> regExp
            qStr1.append(QString("description REGEXP '%1' ").arg(content));
            break;
          }
          break;
        case 2: // field -> Author
          switch (q1.value(1).toInt()) {
          case 0: // condition -> contains
            qStr1.append(QString("UPPER(author_name) LIKE '%%1%' ").arg(content.toUpper()));
            break;
          case 1: // condition -> doesn't contains
            qStr1.append(QString("UPPER(author_name) NOT LIKE '%%1%' ").arg(content.toUpper()));
            break;
          case 2: // condition -> is
            qStr1.append(QString("UPPER(author_name) LIKE '%1' ").arg(content.toUpper()));
            break;
          case 3: // condition -> isn't
            qStr1.append(QString("UPPER(author_name) NOT LIKE '%1' ").arg(content.toUpper()));
            break;
          case 4: // condition -> regExp
            qStr1.append(QString("author_name REGEXP '%1' ").arg(content));
            break;
          }
          break;
        case 3: // field -> Category
          switch (q1.value(1).toInt()) {
          case 0: // condition -> contains
            qStr1.append(QString("UPPER(category) LIKE '%%1%' ").arg(content.toUpper()));
            break;
          case 1: // condition -> doesn't contains
            qStr1.append(QString("UPPER(category) NOT LIKE '%%1%' ").arg(content.toUpper()));
            break;
          case 2: // condition -> is
            qStr1.append(QString("UPPER(category) LIKE '%1' ").arg(content.toUpper()));
            break;
          case 3: // condition -> isn't
            qStr1.append(QString("UPPER(category) NOT LIKE '%1' ").arg(content.toUpper()));
            break;
          case 4: // condition -> begins with
            qStr1.append(QString("UPPER(category) LIKE '%1%' ").arg(content.toUpper()));
            break;
          case 5: // condition -> ends with
            qStr1.append(QString("UPPER(category) LIKE '%%1' ").arg(content.toUpper()));
            break;
          case 6: // condition -> regExp
            qStr1.append(QString("category REGEXP '%1' ").arg(content));
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
        case 5: // field -> Link
          switch (q1.value(1).toInt()) {
          case 0: // condition -> contains
            qStr1.append(QString("link_href LIKE '%%1%' ").arg(content));
            break;
          case 1: // condition -> doesn't contains
            qStr1.append(QString("link_href NOT LIKE '%%1%' ").arg(content));
            break;
          case 2: // condition -> is
            qStr1.append(QString("link_href LIKE '%1' ").arg(content));
            break;
          case 3: // condition -> isn't
            qStr1.append(QString("link_href NOT LIKE '%1' ").arg(content));
            break;
          case 4: // condition -> begins with
            qStr1.append(QString("link_href LIKE '%1%' ").arg(content));
            break;
          case 5: // condition -> ends with
            qStr1.append(QString("link_href LIKE '%%1' ").arg(content));
            break;
          case 6: // condition -> regExp
            qStr1.append(QString("link_href REGEXP '%1' ").arg(content));
            break;
          }
          break;
        case 6: // field -> News
          switch (q1.value(1).toInt()) {
          case 0: // condition -> contains
            qStr1.append(QString("(UPPER(title) LIKE '%%1%' OR UPPER(description) LIKE '%%1%') ").arg(content.toUpper()));
            break;
          case 1: // condition -> doesn't contains
            qStr1.append(QString("(UPPER(title) NOT LIKE '%%1%' OR UPPER(description) NOT LIKE '%%1%') ").arg(content.toUpper()));
            break;
          case 2: // condition -> regExp
            qStr1.append(QString("(title REGEXP '%1' OR description REGEXP '%1') ").arg(content));
            break;
          }
          break;
        }
      }
      whereStr.append(qStr1).append(")");
    }

    if (q1.exec(QString("SELECT id, label FROM news").append(whereStr))) {
      QSqlQuery q2(db_);
      bool isPlaySound = false;

      while (q1.next()) {
        if (!qStr.isEmpty()) {
          qStr1 = qStr % QString(" WHERE id='%1'").arg(q1.value(0).toInt());
          if (!q2.exec(qStr1)) {
            qWarning() << __PRETTY_FUNCTION__ << __LINE__
                       << "q.lastError(): " << q2.lastError().text();
          }
        }

        if (!idLabelsList.isEmpty()) {
          QString idLabelsStr = q1.value(1).toString();
          foreach (int idLabel, idLabelsList) {
            if (idLabelsStr.contains(QString(",%1,").arg(idLabel))) continue;
            if (idLabelsStr.isEmpty()) idLabelsStr.append(",");
            idLabelsStr.append(QString("%1,").arg(idLabel));

          }
          qStr1 = QString("UPDATE news SET label='%1' WHERE id='%2'").arg(idLabelsStr).
              arg(q1.value(0).toInt());
          if (!q2.exec(qStr1)) {
            qWarning() << __PRETTY_FUNCTION__ << __LINE__
                       << "q.lastError(): " << q2.lastError().text();
          }
        }

        if (!colorList.isEmpty()) {
          emit signalAddColorList(q1.value(0).toInt(), colorList.at(0));
        }

        isPlaySound = true;
      }

      if (isPlaySound && !soundList.isEmpty())
        emit signalPlaySound(soundList.at(0));
    } else {
      qWarning() << __PRETTY_FUNCTION__ << __LINE__
                 << "q.lastError(): " << q1.lastError().text();
    }

  }
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

  FeedCountStruct counts;
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
    counts.feedId = feedId;
    counts.unreadCount = unreadCount;
    counts.newCount = newNewsCount;
    counts.undeleteCount = undeleteCount;
    counts.updated = updated;
    counts.lastBuildDate = lastBuildDate;

    emit feedCountsUpdate(counts);
    return 0;
  }

  // Set number unread, new and all(undelete) news for feed
  qStr = QString("UPDATE feeds SET unread='%1', newCount='%2', undeleteCount='%3' "
                 "WHERE id=='%4'").
      arg(unreadCount).arg(newNewsCount).arg(undeleteCount).arg(feedId);
  q.exec(qStr);

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
