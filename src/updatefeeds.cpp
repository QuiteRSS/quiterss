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
#include "updatefeeds.h"

#include "mainapplication.h"
#include "database.h"
#include "settings.h"

#include <QDebug>
#include <qzregexp.h>

#define UPDATE_INTERVAL 3000
#define UPDATE_INTERVAL_MIN 500

UpdateFeeds::UpdateFeeds(QObject *parent, bool addFeed)
  : QObject(parent)
  , updateObject_(NULL)
  , requestFeed_(NULL)
  , parseObject_(NULL)
  , faviconObject_(NULL)
  , updateFeedThread_(NULL)
  , getFaviconThread_(NULL)
  , addFeed_(addFeed)
  , saveMemoryDBTimer_(NULL)
{
  getFeedThread_ = new QThread();
  getFeedThread_->setObjectName("getFeedThread_");
  updateFeedThread_ = new QThread();
  updateFeedThread_->setObjectName("updateFeedThread_");

  Settings settings;
  int timeoutRequest = settings.value("Settings/timeoutRequest", 15).toInt();
  int numberRequests = settings.value("Settings/numberRequest", 10).toInt();
  int numberRepeats = settings.value("Settings/numberRepeats", 2).toInt();

  requestFeed_ = new RequestFeed(timeoutRequest, numberRequests, numberRepeats);

  parseObject_ = new ParseObject();

  if (addFeed_) {
    connect(parent, SIGNAL(signalRequestUrl(int,QString,QDateTime,QString)),
            requestFeed_, SLOT(requestUrl(int,QString,QDateTime,QString)));
    connect(requestFeed_, SIGNAL(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)),
            parent, SLOT(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)));

    connect(parent, SIGNAL(xmlReadyParse(QByteArray,int,QDateTime,QString)),
            parseObject_, SLOT(parseXml(QByteArray,int,QDateTime,QString)));
    connect(parseObject_, SIGNAL(signalFinishUpdate(int,bool,int,QString)),
            parent, SLOT(slotUpdateFeed(int,bool,int,QString)));
  } else {
    getFaviconThread_ = new QThread();
    getFaviconThread_->setObjectName("getFaviconThread_");

    updateObject_ = new UpdateObject();
    faviconObject_ = new FaviconObject();

    connect(updateObject_, SIGNAL(signalRequestUrl(int,QString,QDateTime,QString)),
            requestFeed_, SLOT(requestUrl(int,QString,QDateTime,QString)));
    connect(requestFeed_, SIGNAL(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)),
            updateObject_, SLOT(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)));
    connect(requestFeed_, SIGNAL(setStatusFeed(int,QString)),
            parent, SLOT(setStatusFeed(int,QString)));
    connect(parent, SIGNAL(signalStopUpdate()),
            requestFeed_, SLOT(stopRequest()));

    connect(parent, SIGNAL(signalGetFeedTimer(int)),
            updateObject_, SLOT(slotGetFeedTimer(int)));
    connect(parent, SIGNAL(signalGetAllFeedsTimer()),
            updateObject_, SLOT(slotGetAllFeedsTimer()));
    connect(parent, SIGNAL(signalGetAllFeeds()),
            updateObject_, SLOT(slotGetAllFeeds()));
    connect(parent, SIGNAL(signalGetFeed(int,QString,QDateTime,int)),
            updateObject_, SLOT(slotGetFeed(int,QString,QDateTime,int)));
    connect(parent, SIGNAL(signalGetFeedsFolder(QString)),
            updateObject_, SLOT(slotGetFeedsFolder(QString)));
    connect(parent, SIGNAL(signalImportFeeds(QByteArray)),
            updateObject_, SLOT(slotImportFeeds(QByteArray)));
    connect(updateObject_, SIGNAL(showProgressBar(int)),
            parent, SLOT(showProgressBar(int)));
    connect(updateObject_, SIGNAL(loadProgress(int)),
            parent, SLOT(slotSetValue(int)));
    connect(updateObject_, SIGNAL(signalMessageStatusBar(QString,int)),
            parent, SLOT(showMessageStatusBar(QString,int)));
    connect(updateObject_, SIGNAL(signalUpdateFeedsModel()),
            parent, SLOT(feedsModelReload()),
            Qt::BlockingQueuedConnection);

    connect(updateObject_, SIGNAL(xmlReadyParse(QByteArray,int,QDateTime,QString)),
            parseObject_, SLOT(parseXml(QByteArray,int,QDateTime,QString)),
            Qt::QueuedConnection);
    connect(parseObject_, SIGNAL(signalFinishUpdate(int,bool,int,QString)),
            updateObject_, SLOT(finishUpdate(int,bool,int,QString)),
            Qt::QueuedConnection);
    connect(updateObject_, SIGNAL(feedUpdated(int,bool,int,bool)),
            parent, SLOT(slotUpdateFeed(int,bool,int,bool)));
    connect(updateObject_, SIGNAL(setStatusFeed(int,QString)),
            parent, SLOT(setStatusFeed(int,QString)));

    qRegisterMetaType<FeedCountStruct>("FeedCountStruct");
    connect(parseObject_, SIGNAL(feedCountsUpdate(FeedCountStruct)),
            parent, SLOT(slotFeedCountsUpdate(FeedCountStruct)));

    connect(parseObject_, SIGNAL(signalPlaySound(QString)),
            parent, SLOT(slotPlaySound(QString)));
    connect(parseObject_, SIGNAL(signalAddColorList(int,QString)),
            parent, SLOT(slotAddColorList(int,QString)));

    connect(parent, SIGNAL(signalNextUpdate(bool)),
            updateObject_, SLOT(slotNextUpdateFeed(bool)));
    connect(updateObject_, SIGNAL(signalUpdateModel(bool)),
            parent, SLOT(feedsModelReload(bool)));
    connect(updateObject_, SIGNAL(signalUpdateNews(int)),
            parent, SLOT(slotUpdateNews(int)));
    connect(updateObject_, SIGNAL(signalCountsStatusBar(int,int)),
            parent, SLOT(slotCountsStatusBar(int,int)));

    connect(parent, SIGNAL(signalRecountCategoryCounts()),
            updateObject_, SLOT(slotRecountCategoryCounts()));
    qRegisterMetaType<QList<int> >("QList<int>");
    connect(updateObject_, SIGNAL(signalRecountCategoryCounts(QList<int>,QList<int>,QList<int>,QStringList)),
            parent, SLOT(slotRecountCategoryCounts(QList<int>,QList<int>,QList<int>,QStringList)),
            Qt::QueuedConnection);
    connect(parent, SIGNAL(signalRecountFeedCounts(int,bool)),
            updateObject_, SLOT(slotRecountFeedCounts(int,bool)));
    connect(updateObject_, SIGNAL(feedCountsUpdate(FeedCountStruct)),
            parent, SLOT(slotFeedCountsUpdate(FeedCountStruct)));
    connect(updateObject_, SIGNAL(signalFeedsViewportUpdate()),
            parent, SLOT(slotFeedsViewportUpdate()));
    connect(parent, SIGNAL(signalSetFeedRead(int,int,int,QList<int>)),
            updateObject_, SLOT(slotSetFeedRead(int,int,int,QList<int>)),
            Qt::DirectConnection);
    connect(parent, SIGNAL(signalMarkFeedRead(int,bool,bool)),
            updateObject_, SLOT(slotMarkFeedRead(int,bool,bool)));
    connect(parent, SIGNAL(signalRefreshInfoTray()),
            updateObject_, SLOT(slotRefreshInfoTray()));
    connect(updateObject_, SIGNAL(signalRefreshInfoTray(int,int)),
            parent, SLOT(slotRefreshInfoTray(int,int)));
    connect(parent, SIGNAL(signalUpdateStatus(int,bool)),
            updateObject_, SLOT(slotUpdateStatus(int,bool)));
    connect(parent, SIGNAL(signalMarkAllFeedsRead()),
            updateObject_, SLOT(slotMarkAllFeedsRead()));
    connect(parent, SIGNAL(signalMarkReadCategory(int,int)),
            updateObject_, SLOT(slotMarkReadCategory(int,int)));
    connect(parent, SIGNAL(signalRefreshNewsView(int)),
            updateObject_, SIGNAL(signalMarkAllFeedsRead(int)));
    connect(updateObject_, SIGNAL(signalMarkAllFeedsRead(int)),
            parent, SLOT(slotRefreshNewsView(int)));
    connect(parent, SIGNAL(signalMarkAllFeedsOld()),
            updateObject_, SLOT(slotMarkAllFeedsOld()));

    connect(parent, SIGNAL(signalSetFeedsFilter(bool)),
            updateObject_, SIGNAL(signalSetFeedsFilter(bool)));
    connect(updateObject_, SIGNAL(signalSetFeedsFilter(bool)),
            parent, SLOT(setFeedsFilter(bool)), Qt::QueuedConnection);

    connect(mainApp, SIGNAL(signalSqlQueryExec(QString)),
            updateObject_, SLOT(slotSqlQueryExec(QString)));
    connect(mainApp, SIGNAL(signalRunUserFilter(int, int)),
            parseObject_, SLOT(runUserFilter(int, int)));

    // faviconObject_
    connect(parent, SIGNAL(faviconRequestUrl(QString,QString)),
            faviconObject_, SLOT(requestUrl(QString,QString)));
    connect(faviconObject_, SIGNAL(signalIconRecived(QString,QByteArray,QString)),
            parent, SLOT(slotIconFeedPreparing(QString,QByteArray,QString)));
    connect(parent, SIGNAL(signalIconFeedReady(QString,QByteArray)),
            updateObject_, SLOT(slotIconSave(QString,QByteArray)));
    connect(updateObject_, SIGNAL(signalIconUpdate(int,QByteArray)),
            parent, SLOT(slotIconFeedUpdate(int,QByteArray)));

    updateObject_->moveToThread(updateFeedThread_);
    faviconObject_->moveToThread(getFaviconThread_);

    getFaviconThread_->start(QThread::LowPriority);

    startSaveTimer();
  }

  requestFeed_->moveToThread(getFeedThread_);
  parseObject_->moveToThread(updateFeedThread_);

  getFeedThread_->start(QThread::LowPriority);
  updateFeedThread_->start(QThread::LowPriority);
}

UpdateFeeds::~UpdateFeeds()
{
  requestFeed_->deleteLater();
  parseObject_->deleteLater();

  if (!addFeed_) {
    updateObject_->deleteLater();
    faviconObject_->deleteLater();

    getFaviconThread_->exit();
    getFaviconThread_->wait();
    delete getFaviconThread_;
  }

  getFeedThread_->exit();
  getFeedThread_->wait();
  delete getFeedThread_;

  updateFeedThread_->exit();
  updateFeedThread_->wait();
  delete updateFeedThread_;
}

void UpdateFeeds::disconnectObjects()
{
  if (!addFeed_) {
    updateObject_->disconnect(updateObject_);
    updateObject_->disconnect(parseObject_);
    updateObject_->disconnect(requestFeed_);
    updateObject_->disconnect(parent());
    faviconObject_->disconnectObjects();
  }

  requestFeed_->disconnectObjects();
  requestFeed_->disconnect(parent());
  parseObject_->disconnectObjects();
}

void UpdateFeeds::startSaveTimer()
{
  if (!mainApp->storeDBMemory()) return;

  if (!saveMemoryDBTimer_) {
    saveMemoryDBTimer_ = new QTimer(this);
    connect(saveMemoryDBTimer_, SIGNAL(timeout()), this, SLOT(saveMemoryDatabase()));
  }

  Settings settings;
  int saveInterval = settings.value("Settings/saveDBMemFileInterval", 30).toInt();
  saveMemoryDBTimer_->start(saveInterval*60*1000);
}

void UpdateFeeds::saveMemoryDatabase()
{
  if (!mainApp->storeDBMemory()) return;
  if (updateObject_->isSaveMemoryDatabase) return;

  QTimer::singleShot(100, updateObject_, SLOT(saveMemoryDatabase()));
}

void UpdateFeeds::quitApp()
{
  QTimer::singleShot(0, updateObject_, SLOT(quitApp()));
}

//------------------------------------------------------------------------------
UpdateObject::UpdateObject(QObject *parent)
  : QObject(parent)
  , isSaveMemoryDatabase(false)
  , updateFeedsCount_(0)
{
  setObjectName("updateObject_");

  mainWindow_ = mainApp->mainWindow();

  db_ = Database::connection("secondConnection");

  updateModelTimer_ = new QTimer(this);
  updateModelTimer_->setSingleShot(true);
  connect(updateModelTimer_, SIGNAL(timeout()), this, SIGNAL(signalUpdateModel()));

  timerUpdateNews_ = new QTimer(this);
  timerUpdateNews_->setSingleShot(true);
  connect(timerUpdateNews_, SIGNAL(timeout()), this, SIGNAL(signalUpdateNews()));

}

UpdateObject::~UpdateObject()
{

}

void UpdateObject::slotGetFeedTimer(int feedId)
{
  QSqlQuery q(db_);
  q.exec(QString("SELECT xmlUrl, lastBuildDate, authentication FROM feeds WHERE id=='%1' AND disableUpdate=0")
         .arg(feedId));
  if (q.next()) {
    addFeedInQueue(feedId, q.value(0).toString(),
                   q.value(1).toDateTime(), q.value(2).toInt());
  }
  emit showProgressBar(updateFeedsCount_);
}

void UpdateObject::slotGetAllFeedsTimer()
{
  QSqlQuery q(db_);
  q.exec("SELECT id, xmlUrl, lastBuildDate, authentication FROM feeds "
         "WHERE xmlUrl!='' AND disableUpdate=0 "
         "AND (updateIntervalEnable==-1 OR updateIntervalEnable IS NULL)");
  while (q.next()) {
    addFeedInQueue(q.value(0).toInt(), q.value(1).toString(),
                   q.value(2).toDateTime(), q.value(3).toInt());
  }
  emit showProgressBar(updateFeedsCount_);
}

/** @brief Process update feed action
 *---------------------------------------------------------------------------*/
void UpdateObject::slotGetFeed(int feedId, QString feedUrl, QDateTime date, int auth)
{
  addFeedInQueue(feedId, feedUrl, date, auth);

  emit showProgressBar(updateFeedsCount_);
}

/** @brief Process update feed in folder action
 *---------------------------------------------------------------------------*/
void UpdateObject::slotGetFeedsFolder(QString query)
{
  QSqlQuery q(db_);
  q.exec(query);
  while (q.next()) {
    addFeedInQueue(q.value(0).toInt(), q.value(1).toString(),
                   q.value(2).toDateTime(), q.value(3).toInt());
  }

  emit showProgressBar(updateFeedsCount_);
}

/** @brief Process update all feeds action
 *---------------------------------------------------------------------------*/
void UpdateObject::slotGetAllFeeds()
{
  QSqlQuery q(db_);
  q.exec("SELECT id, xmlUrl, lastBuildDate, authentication FROM feeds WHERE xmlUrl!='' AND disableUpdate=0");
  while (q.next()) {
    addFeedInQueue(q.value(0).toInt(), q.value(1).toString(),
                   q.value(2).toDateTime(), q.value(3).toInt());
  }
  emit showProgressBar(updateFeedsCount_);
}

/** @brief Import feeds from OPML-file
 *
 * Calls open file system dialog with filter *.opml.
 * Adds all feeds to DB include hierarchy, ignore duplicate feeds
 *---------------------------------------------------------------------------*/
void UpdateObject::slotImportFeeds(QByteArray xmlData)
{
  int elementCount = 0;
  int outlineCount = 0;
  QSqlQuery q(db_);
  QList<int> idsList;
  QList<QString> urlsList;
  QXmlStreamReader xml;
  QString convertData;
  bool codecOk = false;

  QzRegExp rx("&(?!([a-z0-9#]+;))", Qt::CaseInsensitive);
  int pos = 0;
  while ((pos = rx.indexIn(QString::fromLatin1(xmlData), pos)) != -1) {
    xmlData.replace(pos, 1, "&amp;");
    pos += 1;
  }

  rx.setPattern("encoding=\"([^\"]+)");
  pos = rx.indexIn(xmlData);
  if (pos == -1) {
    rx.setPattern("encoding='([^']+)");
    pos = rx.indexIn(xmlData);
  }
  if (pos == -1) {
    QStringList codecNameList;
    codecNameList << "UTF-8" << "Windows-1251" << "KOI8-R" << "KOI8-U"
                  << "ISO 8859-5" << "IBM 866";
    foreach (QString codecNameT, codecNameList) {
      QTextCodec *codec = QTextCodec::codecForName(codecNameT.toUtf8());
      if (codec && codec->canEncode(xmlData)) {
        convertData = codec->toUnicode(xmlData);
        codecOk = true;
        break;
      }
    }
  }
  if (codecOk) {
    xml.addData(convertData);
  } else {
    xml.addData(xmlData);
  }

  db_.transaction();

  // Store hierarchy of "outline" tags. Next nested outline is pushed to stack.
  // When it closes, pop it out from stack. Top of stack is the root outline.
  QStack<int> parentIdsStack;
  parentIdsStack.push(0);
  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isStartElement()) {
      // Search for "outline" only
      if (xml.name() == "outline") {
        qDebug() << outlineCount << "+:" << xml.prefix().toString()
                 << ":" << xml.name().toString();

        QString textString(xml.attributes().value("text").toString());
        QString titleString(xml.attributes().value("title").toString());
        QString xmlUrlString(xml.attributes().value("xmlUrl").toString());
        if (textString.isEmpty()) textString = titleString;

        //Folder finded
        if (xmlUrlString.isEmpty()) {
          int rowToParent = 0;
          q.exec(QString("SELECT count(id) FROM feeds WHERE parentId='%1'").
                 arg(parentIdsStack.top()));
          if (q.next()) rowToParent = q.value(0).toInt();

          q.prepare("INSERT INTO feeds(text, title, xmlUrl, created, f_Expanded, parentId, rowToParent) "
                    "VALUES (:text, :title, :xmlUrl, :feedCreateTime, 0, :parentId, :rowToParent)");
          q.bindValue(":text", textString);
          q.bindValue(":title", textString);
          q.bindValue(":xmlUrl", "");
          q.bindValue(":feedCreateTime",
                      QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
          q.bindValue(":parentId", parentIdsStack.top());
          q.bindValue(":rowToParent", rowToParent);
          q.exec();
          parentIdsStack.push(q.lastInsertId().toInt());
        }
        // Feed finded
        else {
          if (xmlUrlString.contains("feed:", Qt::CaseInsensitive)) {
            if (xmlUrlString.contains("https://", Qt::CaseInsensitive)) {
              xmlUrlString.remove(0, 5);
            } else {
              xmlUrlString.remove(0, 7);
              xmlUrlString = "http://" + xmlUrlString;
            }
          }

          bool isFeedDuplicated = false;
          q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
          q.bindValue(":xmlUrl", xmlUrlString);
          q.exec();
          if (q.next())
            isFeedDuplicated = true;

          if (isFeedDuplicated) {
            qDebug() << "duplicate feed:" << xmlUrlString << textString;
          } else {
            int rowToParent = 0;
            q.exec(QString("SELECT count(id) FROM feeds WHERE parentId='%1'").
                   arg(parentIdsStack.top()));
            if (q.next()) rowToParent = q.value(0).toInt();

            q.prepare("INSERT INTO feeds(text, title, description, xmlUrl, htmlUrl, created, parentId, rowToParent) "
                      "VALUES(?, ?, ?, ?, ?, ?, ?, ?)");
            q.addBindValue(textString);
            q.addBindValue(xml.attributes().value("title").toString());
            q.addBindValue(xml.attributes().value("description").toString());
            q.addBindValue(xmlUrlString);
            q.addBindValue(xml.attributes().value("htmlUrl").toString());
            q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
            q.addBindValue(parentIdsStack.top());
            q.addBindValue(rowToParent);
            q.exec();

            idsList.append(q.lastInsertId().toInt());
            urlsList.append(xmlUrlString);
          }
          parentIdsStack.push(q.lastInsertId().toInt());
        }
      }
    } else if (xml.isEndElement()) {
      if (xml.name() == "outline") {
        parentIdsStack.pop();
        ++outlineCount;
      }
      ++elementCount;
    }
    qDebug() << parentIdsStack;
  }
  if (xml.error()) {
    QString error = QString("Import error: Line = %1, Column = %2; Error = %3").
        arg(xml.lineNumber()).arg(xml.columnNumber()).arg(xml.errorString());
    qCritical() << error;
    emit signalMessageStatusBar(error, 3000);
  } else {
    emit signalMessageStatusBar(QString("Import: file read done"), 3000);
  }

  db_.commit();

  emit signalUpdateFeedsModel();

  for (int i = 0; i < idsList.count(); i++) {
    updateFeedsCount_ = updateFeedsCount_ + 2;
    emit signalRequestUrl(idsList.at(i), urlsList.at(i), QDateTime(), "");
  }
  emit showProgressBar(updateFeedsCount_);
}

// ----------------------------------------------------------------------------
bool UpdateObject::addFeedInQueue(int feedId, const QString &feedUrl,
                                  const QDateTime &date, int auth)
{
  int feedIdIndex = feedIdList_.indexOf(feedId);
  if (feedIdIndex > -1) {
    return false;
  } else {
    feedIdList_.append(feedId);
    updateFeedsCount_ = updateFeedsCount_ + 2;
    QString userInfo;
    if (auth == 1) {
      QSqlQuery q(db_);
      QUrl url(feedUrl);
      q.prepare("SELECT username, password FROM passwords WHERE server=?");
      q.addBindValue(url.host());
      q.exec();
      if (q.next()) {
        userInfo = QString("%1:%2").arg(q.value(0).toString()).
            arg(QString::fromUtf8(QByteArray::fromBase64(q.value(1).toByteArray())));
      }
    }
    emit signalRequestUrl(feedId, feedUrl, date, userInfo);
    return true;
  }
}

/** @brief Process network request completion
 *---------------------------------------------------------------------------*/
void UpdateObject::getUrlDone(int result, int feedId, QString feedUrlStr,
                              QString error, QByteArray data, QDateTime dtReply,
                              QString codecName)
{
  qDebug() << "getUrl result = " << result << "error: " << error << "url: " << feedUrlStr;

  if (updateFeedsCount_ > 0) {
    updateFeedsCount_--;
    emit loadProgress(updateFeedsCount_);
  }

  if (!data.isEmpty()) {
    emit xmlReadyParse(data, feedId, dtReply, codecName);
  } else {
    QString status = "0";
    if (result < 0) {
      status = QString("%1 %2").arg(result).arg(error);
      qWarning() << QString("Request failed: result = %1, error - %2, url - %3").
                    arg(result).arg(error).arg(feedUrlStr);
    }
    finishUpdate(feedId, false, 0, status);
  }
}

void UpdateObject::finishUpdate(int feedId, bool changed, int newCount, QString status)
{
  if (updateFeedsCount_ > 0) {
    updateFeedsCount_--;
    emit loadProgress(updateFeedsCount_);
  }
  bool finish = false;
  if (updateFeedsCount_ <= 0) {
    finish = true;
  }

  int feedIdIndex = feedIdList_.indexOf(feedId);
  if (feedIdIndex > -1) {
    feedIdList_.takeAt(feedIdIndex);
  }

  QSqlQuery q(db_);
  QString qStr = QString("UPDATE feeds SET status='%1' WHERE id=='%2'").
      arg(status).arg(feedId);
  q.exec(qStr);

  if (changed) {
    if (mainWindow_->currentNewsTab->type_ == NewsTabWidget::TabTypeFeed) {
      bool folderUpdate = false;
      int feedParentId = 0;

      QSqlQuery q(db_);
      q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(feedId));
      if (q.first()) {
        feedParentId = q.value(0).toInt();
        if (feedParentId == mainWindow_->currentNewsTab->feedId_)
          folderUpdate = true;
      }

      while (feedParentId && !folderUpdate) {
        q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(feedParentId));
        if (q.first()) {
          feedParentId = q.value(0).toInt();
          if (feedParentId == mainWindow_->currentNewsTab->feedId_)
            folderUpdate = true;
        }
      }

      // Click on feed if it is displayed to update view
      if ((feedId == mainWindow_->currentNewsTab->feedId_) || folderUpdate) {
        if (!timerUpdateNews_->isActive())
          timerUpdateNews_->start(1000);

        int unreadCount = 0;
        int allCount = 0;
        q.exec(QString("SELECT unread, undeleteCount FROM feeds WHERE id=='%1'").
               arg(mainWindow_->currentNewsTab->feedId_));
        if (q.first()) {
          unreadCount = q.value(0).toInt();
          allCount    = q.value(1).toInt();
        }
        emit signalCountsStatusBar(unreadCount, allCount);
      }
    } else if (mainWindow_->currentNewsTab->type_ < NewsTabWidget::TabTypeWeb) {
      if (!timerUpdateNews_->isActive())
        timerUpdateNews_->start(1000);
    }
  }

  emit feedUpdated(feedId, changed, newCount, finish);
  emit setStatusFeed(feedId, status);
}

/** @brief Start timer if feed presents in queue
 *---------------------------------------------------------------------------*/
void UpdateObject::slotNextUpdateFeed(bool finish)
{
  if (!updateModelTimer_->isActive()) {
    if (finish)
      updateModelTimer_->start(UPDATE_INTERVAL_MIN);
    else
      updateModelTimer_->start(UPDATE_INTERVAL);
  }
}

void UpdateObject::slotRecountCategoryCounts()
{
  QList<int> deletedList;
  QList<int> starredList;
  QList<int> readList;
  QStringList labelList;
  QSqlQuery q(db_);
  q.exec("SELECT deleted, starred, read, label FROM news WHERE deleted < 2");
  while (q.next()) {
    deletedList.append(q.value(0).toInt());
    starredList.append(q.value(1).toInt());
    readList.append(q.value(2).toInt());
    labelList.append(q.value(3).toString());
  }

  emit signalRecountCategoryCounts(deletedList, starredList, readList, labelList);
}

/** @brief Update feed counters and all its parents
 *
 * Update fields: unread news number, new news number,
 *   last update feed timestamp
 * Update only feeds, categories are ignored
 * Update right into DB, update view if feed is visible in feed tree
 * @param feedId Feed identifier
 * @param updateViewport Need viewport update flag
 *----------------------------------------------------------------------------*/
void UpdateObject::slotRecountFeedCounts(int feedId, bool updateViewport)
{
  QSqlQuery q(db_);
  QString qStr;

  db_.transaction();

  int feedParId = 0;
  bool isFolder = false;
  qStr = QString("SELECT parentId, xmlUrl FROM feeds WHERE id=='%1'").
      arg(feedId);
  q.exec(qStr);
  if (q.next()) {
    feedParId = q.value(0).toInt();
    if (q.value(1).toString().isEmpty())
      isFolder = true;
  }

  int undeleteCount = 0;
  int unreadCount = 0;
  int newCount = 0;

  if (!isFolder) {
    // Calculate all news (not mark deleted)
    qStr = QString("SELECT count(id) FROM news WHERE feedId=='%1' AND deleted==0").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) undeleteCount = q.value(0).toInt();

    // Calculate unread news
    qStr = QString("SELECT count(read) FROM news WHERE feedId=='%1' AND read==0 AND deleted==0").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) unreadCount = q.value(0).toInt();

    // Calculate new news
    qStr = QString("SELECT count(new) FROM news WHERE feedId=='%1' AND new==1 AND deleted==0").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) newCount = q.value(0).toInt();

    int unreadCountOld = 0;
    int newCountOld = 0;
    int undeleteCountOld = 0;
    qStr = QString("SELECT unread, newCount, undeleteCount FROM feeds WHERE id=='%1'").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) {
      unreadCountOld = q.value(0).toInt();
      newCountOld = q.value(1).toInt();
      undeleteCountOld = q.value(2).toInt();
    }

    if ((unreadCount == unreadCountOld) && (newCount == newCountOld) &&
        (undeleteCount == undeleteCountOld)) {
      db_.commit();
      return;
    }

    // Save unread and new news number for feed
    qStr = QString("UPDATE feeds SET unread='%1', newCount='%2', undeleteCount='%3' "
                   "WHERE id=='%4'").
        arg(unreadCount).arg(newCount).arg(undeleteCount).arg(feedId);
    q.exec(qStr);

    // Update view of the feed
    FeedCountStruct counts;
    counts.feedId = feedId;
    counts.unreadCount = unreadCount;
    counts.newCount = newCount;
    counts.undeleteCount = undeleteCount;
    emit feedCountsUpdate(counts);
  } else {
    bool changed = false;
    QList<int> idParList;
    QList<int> idList = getIdFeedsInList(feedId);
    if (idList.count()) {
      foreach (int id, idList) {
        int parId = 0;
        q.exec(QString("SELECT parentId FROM feeds WHERE id=='%1'").arg(id));
        if (q.next())
          parId = q.value(0).toInt();

        if (parId) {
          if (idParList.indexOf(parId) == -1) {
            idParList.append(parId);
          }
        }

        // Calculate all news (not mark deleted)
        qStr = QString("SELECT count(id) FROM news WHERE feedId=='%1' AND deleted==0").
            arg(id);
        q.exec(qStr);
        if (q.next()) undeleteCount = q.value(0).toInt();

        // Calculate unread news
        qStr = QString("SELECT count(read) FROM news WHERE feedId=='%1' AND read==0 AND deleted==0").
            arg(id);
        q.exec(qStr);
        if (q.next()) unreadCount = q.value(0).toInt();

        // Calculate new news
        qStr = QString("SELECT count(new) FROM news WHERE feedId=='%1' AND new==1 AND deleted==0").
            arg(id);
        q.exec(qStr);
        if (q.next()) newCount = q.value(0).toInt();

        int unreadCountOld = 0;
        int newCountOld = 0;
        int undeleteCountOld = 0;
        qStr = QString("SELECT unread, newCount, undeleteCount FROM feeds WHERE id=='%1'").
            arg(id);
        q.exec(qStr);
        if (q.next()) {
          unreadCountOld = q.value(0).toInt();
          newCountOld = q.value(1).toInt();
          undeleteCountOld = q.value(2).toInt();
        }

        if ((unreadCount == unreadCountOld) && (newCount == newCountOld) &&
            (undeleteCount == undeleteCountOld)) {
          continue;
        }
        changed = true;

        // Save unread and new news number for parent
        qStr = QString("UPDATE feeds SET unread='%1', newCount='%2', undeleteCount='%3' "
                       "WHERE id=='%4'").
            arg(unreadCount).arg(newCount).arg(undeleteCount).arg(id);
        q.exec(qStr);

        // Update view of the parent
        FeedCountStruct counts;
        counts.feedId = id;
        counts.unreadCount = unreadCount;
        counts.newCount = newCount;
        counts.undeleteCount = undeleteCount;
        emit feedCountsUpdate(counts);
      }

      if (!changed) {
        db_.commit();
        return;
      }

      foreach (int l_feedParId, idParList) {
        while (l_feedParId) {
          QString updated;

          qStr = QString("SELECT sum(unread), sum(newCount), sum(undeleteCount), "
                         "max(updated) FROM feeds WHERE parentId=='%1'").
              arg(l_feedParId);
          q.exec(qStr);
          if (q.next()) {
            unreadCount   = q.value(0).toInt();
            newCount      = q.value(1).toInt();
            undeleteCount = q.value(2).toInt();
            updated       = q.value(3).toString();
          }
          qStr = QString("UPDATE feeds SET unread='%1', newCount='%2', undeleteCount='%3', "
                         "updated='%4' WHERE id=='%5'").
              arg(unreadCount).arg(newCount).arg(undeleteCount).arg(updated).
              arg(l_feedParId);
          q.exec(qStr);

          // Update view
          FeedCountStruct counts;
          counts.feedId = l_feedParId;
          counts.unreadCount = unreadCount;
          counts.newCount = newCount;
          counts.undeleteCount = undeleteCount;
          counts.updated = updated;
          emit feedCountsUpdate(counts);

          if (feedId == l_feedParId) break;
          q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(l_feedParId));
          if (q.next()) l_feedParId = q.value(0).toInt();
        }
      }
    }
  }

  // Recalculate counters for all parents
  int l_feedParId = feedParId;
  while (l_feedParId) {
    QString updated;

    qStr = QString("SELECT sum(unread), sum(newCount), sum(undeleteCount), "
                   "max(updated) FROM feeds WHERE parentId=='%1'").
        arg(l_feedParId);
    q.exec(qStr);
    if (q.next()) {
      unreadCount   = q.value(0).toInt();
      newCount      = q.value(1).toInt();
      undeleteCount = q.value(2).toInt();
      updated       = q.value(3).toString();
    }
    qStr = QString("UPDATE feeds SET unread='%1', newCount='%2', undeleteCount='%3', "
                   "updated='%4' WHERE id=='%5'").
        arg(unreadCount).arg(newCount).arg(undeleteCount).arg(updated).
        arg(l_feedParId);
    q.exec(qStr);

    // Update view
    FeedCountStruct counts;
    counts.feedId = l_feedParId;
    counts.unreadCount = unreadCount;
    counts.newCount = newCount;
    counts.undeleteCount = undeleteCount;
    counts.updated = updated;
    emit feedCountsUpdate(counts);

    q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(l_feedParId));
    if (q.next()) l_feedParId = q.value(0).toInt();
  }
  db_.commit();

  if (updateViewport) emit signalFeedsViewportUpdate();
}

/** @brief Get feeds ids list string of folder \a idFolder
 *---------------------------------------------------------------------------*/
QString UpdateObject::getIdFeedsString(int idFolder, int idException)
{
  QList<int> idList = getIdFeedsInList(idFolder);
  if (idList.count()) {
    QString str;
    foreach (int id, idList) {
      if (id == idException) continue;
      if (!str.isEmpty()) str.append(" OR ");
      str.append(QString("feedId=%1").arg(id));
    }
    return str;
  } else {
    return QString("feedId=-1");
  }
}

/** @brief Get feeds ids list of folder \a idFolder
 *---------------------------------------------------------------------------*/
QList<int> UpdateObject::getIdFeedsInList(int idFolder)
{
  QList<int> idList;
  if (idFolder <= 0) return idList;

  QSqlQuery q;
  QQueue<int> parentIds;
  parentIds.enqueue(idFolder);
  while (!parentIds.empty()) {
    int parentId = parentIds.dequeue();
    QString qStr = QString("SELECT id, xmlUrl FROM feeds WHERE parentId='%1'").
        arg(parentId);
    q.exec(qStr);
    while (q.next()) {
      int feedId = q.value(0).toInt();
      if (!q.value(1).toString().isEmpty())
        idList << feedId;
      if (q.value(1).toString().isEmpty())
        parentIds.enqueue(feedId);
    }
  }
  return idList;
}

/** @brief Mark feed Read while clicking on unfocused one
 *---------------------------------------------------------------------------*/
void UpdateObject::slotSetFeedRead(int readType, int feedId, int idException, QList<int> idNewsList)
{
  QSqlDatabase db = QSqlDatabase::database();
  QSqlQuery q(db);

  if (readType != FeedReadSwitchingTab) {
    db.transaction();
    QString idFeedsStr = getIdFeedsString(feedId, idException);
    if (((readType == FeedReadSwitchingFeed) && mainWindow_->markReadSwitchingFeed_) ||
        ((readType == FeedReadClosingTab) && mainWindow_->markReadClosingTab_) ||
        ((readType == FeedReadPlaceToTray) && mainWindow_->markReadMinimize_)) {
      if (idFeedsStr == "feedId=-1") {
        q.exec(QString("UPDATE news SET read=2 WHERE feedId='%1' AND read!=2").arg(feedId));
      } else {
        q.exec(QString("UPDATE news SET read=2 WHERE (%1) AND read!=2").arg(idFeedsStr));
      }
    } else {
      if (idFeedsStr == "feedId=-1") {
        q.exec(QString("UPDATE news SET read=2 WHERE feedId='%1' AND read=1").arg(feedId));
      } else {
        q.exec(QString("UPDATE news SET read=2 WHERE (%1) AND read=1").arg(idFeedsStr));
      }
    }
    if (idFeedsStr == "feedId=-1") {
      q.exec(QString("UPDATE news SET new=0 WHERE feedId='%1' AND new=1").arg(feedId));
    } else {
      q.exec(QString("UPDATE news SET new=0 WHERE (%1) AND new=1").arg(idFeedsStr));
    }
    if (mainWindow_->markNewsReadOn_ && mainWindow_->markPrevNewsRead_)
      q.exec(QString("UPDATE news SET read=2 WHERE id IN (SELECT currentNews FROM feeds WHERE id='%1')").arg(feedId));
    db.commit();

    slotRecountFeedCounts(feedId);
    slotRecountCategoryCounts();

    if (readType != FeedReadPlaceToTray)
      slotRefreshInfoTray();
  } else {
    QString idStr;
    foreach (int newsId, idNewsList) {
      if (!idStr.isEmpty()) idStr.append(" OR ");
      idStr.append(QString("id='%1'").arg(newsId));
    }

    db.transaction();
    q.exec(QString("UPDATE news SET read=2 WHERE (%1) AND read==1").arg(idStr));
    q.exec(QString("UPDATE news SET new=0 WHERE (%1) AND new==1").arg(idStr));
    db.commit();

    if (feedId > -1)
      slotRecountFeedCounts(feedId, false);
  }

  emit signalSetFeedsFilter();
}

void UpdateObject::slotMarkFeedRead(int id, bool isFolder, bool openFeed)
{
  db_.transaction();
  QSqlQuery q(db_);
  QString qStr;
  if (isFolder) {
    qStr = QString("UPDATE news SET read=2 WHERE read!=2 AND deleted==0 AND (%1)").
        arg(getIdFeedsString(id));
    q.exec(qStr);
    qStr = QString("UPDATE news SET new=0 WHERE new==1 AND (%1)").
        arg(getIdFeedsString(id));
    q.exec(qStr);
  } else {
    if (openFeed) {
      qStr = QString("UPDATE news SET read=2 WHERE feedId=='%1' AND read!=2 AND deleted==0").
          arg(id);
      q.exec(qStr);
    } else {
      QString qStr = QString("UPDATE news SET read=1 WHERE feedId=='%1' AND read==0").
          arg(id);
      q.exec(qStr);
    }
    qStr = QString("UPDATE news SET new=0 WHERE feedId=='%1' AND new==1").
        arg(id);
    q.exec(qStr);
  }
  db_.commit();

  if (!openFeed || isFolder)
    slotUpdateStatus(id, true);
}

/** @brief Update status of current feed or feed of current tab
 *---------------------------------------------------------------------------*/
void UpdateObject::slotUpdateStatus(int feedId, bool changed)
{
  if (changed) {
    slotRecountFeedCounts(feedId);
  }
  slotRefreshInfoTray();

  if (feedId > 0) {
    bool folderUpdate = false;
    int feedParentId = 0;
    QSqlQuery q(db_);
    q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(feedId));
    if (q.next()) {
      feedParentId = q.value(0).toInt();
      if (feedParentId == mainWindow_->currentNewsTab->feedId_) folderUpdate = true;
    }

    while (feedParentId && !folderUpdate) {
      q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(feedParentId));
      if (q.next()) {
        feedParentId = q.value(0).toInt();
        if (feedParentId == mainWindow_->currentNewsTab->feedId_) folderUpdate = true;
      }
    }

    // Click on feed if it is displayed to update view
    if ((feedId == mainWindow_->currentNewsTab->feedId_) || folderUpdate) {
      int unreadCount = 0;
      int allCount = 0;
      q.exec(QString("SELECT unread, undeleteCount FROM feeds WHERE id=='%1'").
             arg(mainWindow_->currentNewsTab->feedId_));
      if (q.next()) {
        unreadCount = q.value(0).toInt();
        allCount    = q.value(1).toInt();
      }
      emit signalCountsStatusBar(unreadCount, allCount);
    }
  }
}

void UpdateObject::slotMarkAllFeedsRead()
{
  QSqlQuery q(db_);

  q.exec("UPDATE news SET read=2 WHERE read!=2 AND deleted==0");
  q.exec("UPDATE news SET new=0 WHERE new==1 AND deleted==0");

  q.exec("SELECT id FROM feeds WHERE unread!=0");
  while (q.next()) {
    slotRecountFeedCounts(q.value(0).toInt());
  }
  slotRecountCategoryCounts();

  slotRefreshInfoTray();

  emit signalMarkAllFeedsRead();
}

void UpdateObject::slotMarkReadCategory(int type, int idLabel)
{
  QString qStr;
  switch (type) {
  case NewsTabWidget::TabTypeUnread:
    qStr = "feedId > 0 AND deleted = 0 AND read < 2";
    break;
  case NewsTabWidget::TabTypeStar:
    qStr = "feedId > 0 AND deleted = 0 AND starred = 1";
    break;
  case NewsTabWidget::TabTypeLabel:
    if (idLabel != 0) {
      qStr = QString("feedId > 0 AND deleted = 0 AND label LIKE '%,%1,%'").
          arg(idLabel);
    } else {
      qStr = QString("feedId > 0 AND deleted = 0 AND label!='' AND label!=','");
    }
    break;
  }

  QSqlQuery q;
  q.exec(QString("UPDATE news SET read=1 WHERE %1").arg(qStr));
  q.exec(QString("UPDATE news SET new=0 WHERE %1").arg(qStr));

  QList<int> idList;
  q.exec("SELECT id FROM feeds WHERE unread!=0");
  while (q.next()) {
    idList.append(q.value(0).toInt());
  }
  emit signalMarkAllFeedsRead(0);
  foreach (int id, idList) {
    slotUpdateStatus(id, true);
  }
}

/** @brief Save icon in DB and emit signal to update it
 *----------------------------------------------------------------------------*/
void UpdateObject::slotIconSave(QString feedUrl, QByteArray faviconData)
{
  int feedId = 0;

  QSqlQuery q(db_);
  q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
  q.bindValue(":xmlUrl", feedUrl);
  q.exec();
  if (q.next()) {
    feedId = q.value(0).toInt();
  }

  q.prepare("UPDATE feeds SET image = ? WHERE id == ?");
  q.addBindValue(faviconData.toBase64());
  q.addBindValue(feedId);
  q.exec();

  emit signalIconUpdate(feedId, faviconData);
}

void UpdateObject::slotSqlQueryExec(QString query)
{
  QSqlQuery q(db_);
  if (!q.exec(query)) {
    qCritical() << __PRETTY_FUNCTION__ << __LINE__
                << "q.lastError(): " << q.lastError().text();
  }
}

/** @brief Mark all feeds Not New
 *---------------------------------------------------------------------------*/
void UpdateObject::slotMarkAllFeedsOld()
{
  QSqlQuery q(db_);
  q.exec("UPDATE news SET new=0 WHERE new==1 AND deleted==0");

  q.exec("SELECT id FROM feeds WHERE newCount!=0");
  while (q.next()) {
    slotRecountFeedCounts(q.value(0).toInt());
  }
  slotRecountCategoryCounts();

  if ((mainWindow_->currentNewsTab != NULL) && (mainWindow_->currentNewsTab->type_ < NewsTabWidget::TabTypeWeb)) {
    emit signalUpdateNews(NewsTabWidget::RefreshWithPos);
  }

  slotRefreshInfoTray();
}

void UpdateObject::slotRefreshInfoTray()
{
  // Calculate new and unread news number
  int newCount = 0;
  int unreadCount = 0;
  QSqlQuery q(db_);
  q.exec("SELECT sum(newCount), sum(unread) FROM feeds WHERE xmlUrl!=''");
  if (q.first()) {
    newCount    = q.value(0).toInt();
    unreadCount = q.value(1).toInt();
  }

  emit signalRefreshInfoTray(newCount, unreadCount);
}

void UpdateObject::saveMemoryDatabase()
{
  isSaveMemoryDatabase = true;
  Database::sqliteDBMemFile();
  isSaveMemoryDatabase = false;
}

/** @brief Delete news from the feed by criteria
 *---------------------------------------------------------------------------*/
void UpdateObject::cleanUpShutdown()
{
  Settings settings;
  settings.beginGroup("Settings");
  bool cleanupOnShutdown = settings.value("cleanupOnShutdown", true).toBool();
  int maxDayCleanUp = settings.value("maxDayClearUp", 30).toInt();
  int maxNewsCleanUp = settings.value("maxNewsClearUp", 200).toInt();
  bool dayCleanUpOn = settings.value("dayClearUpOn", true).toBool();
  bool newsCleanUpOn = settings.value("newsClearUpOn", true).toBool();
  bool readCleanUp = settings.value("readClearUp", false).toBool();
  bool neverUnreadCleanUp = settings.value("neverUnreadClearUp", true).toBool();
  bool neverStarCleanUp = settings.value("neverStarClearUp", true).toBool();
  bool neverLabelCleanUp = settings.value("neverLabelClearUp", true).toBool();
  bool cleanUpDeleted = settings.value("cleanUpDeleted", false).toBool();
  bool optimizeDB = settings.value("optimizeDB", false).toBool();
  settings.endGroup();

  QSqlQuery q;
  QString qStr;

  if (!mainApp->storeDBMemory())
    db_ = QSqlDatabase::database();
  db_.transaction();

  q.exec("UPDATE news SET new=0 WHERE new==1");
  q.exec("UPDATE news SET read=2 WHERE read==1");
  q.exec("UPDATE feeds SET newCount=0 WHERE newCount!=0");

  if (cleanupOnShutdown) {
    QList<int> feedsIdList;
    QList<int> foldersIdList;
    q.exec("SELECT id, xmlUrl FROM feeds");
    while (q.next()) {
      if (q.value(1).toString().isEmpty()) {
        foldersIdList << q.value(0).toInt();
      }
      else {
        feedsIdList << q.value(0).toInt();
      }
    }

    // Run Cleanup for all feeds, except categories
    foreach (int feedId, feedsIdList) {
      int countDelNews = 0;
      int countAllNews = 0;

      qStr = QString("SELECT undeleteCount FROM feeds WHERE id=='%1'").arg(feedId);
      q.exec(qStr);
      if (q.next()) countAllNews = q.value(0).toInt();

      QString qStr1 = QString("UPDATE news SET description='', content='', received='', "
                              "author_name='', author_uri='', author_email='', "
                              "category='', new='', read='', starred='', label='', "
                              "deleteDate='', feedParentId='', deleted=2");

      qStr = QString("SELECT id, received FROM news WHERE feedId=='%1' AND deleted == 0").
          arg(feedId);
      if (neverUnreadCleanUp) qStr.append(" AND read!=0");
      if (neverStarCleanUp) qStr.append(" AND starred==0");
      if (neverLabelCleanUp) qStr.append(" AND (label=='' OR label==',' OR label IS NULL)");
      qStr.append(" ORDER BY published");
      q.exec(qStr);
      while (q.next()) {
        int newsId = q.value(0).toInt();

        if (newsCleanUpOn && (countDelNews < (countAllNews - maxNewsCleanUp))) {
          qStr = QString("%1 WHERE id=='%2'").arg(qStr1).arg(newsId);
          QSqlQuery qt;
          qt.exec(qStr);
          countDelNews++;
          continue;
        }

        QDateTime dateTime = QDateTime::fromString(
              q.value(1).toString(),
              Qt::ISODate);
        if (dayCleanUpOn &&
            (dateTime.daysTo(QDateTime::currentDateTime()) > maxDayCleanUp)) {
          qStr = QString("%1 WHERE id=='%2'").arg(qStr1).arg(newsId);
          QSqlQuery qt;
          qt.exec(qStr);
          countDelNews++;
          continue;
        }

        if (readCleanUp) {
          qStr = QString("%1 WHERE read!=0 AND id=='%2'").arg(qStr1).arg(newsId);
          QSqlQuery qt;
          qt.exec(qStr);
          countDelNews++;
        }
      }

      int undeleteCount = 0;
      qStr = QString("SELECT count(id) FROM news WHERE feedId=='%1' AND deleted==0").
          arg(feedId);
      q.exec(qStr);
      if (q.next()) undeleteCount = q.value(0).toInt();

      int unreadCount = 0;
      qStr = QString("SELECT count(read) FROM news WHERE feedId=='%1' AND read==0 AND deleted==0").
          arg(feedId);
      q.exec(qStr);
      if (q.next()) unreadCount = q.value(0).toInt();

      qStr = QString("UPDATE feeds SET unread='%1', undeleteCount='%2' WHERE id=='%3'").
          arg(unreadCount).arg(undeleteCount).arg(feedId);
      q.exec(qStr);
    }

    // Run categories recount, because cleanup may change counts
    foreach (int folderIdStart, foldersIdList) {
      if (folderIdStart < 1) continue;

      int folderId = folderIdStart;
      // Process all parents
      while (0 < folderId) {
        int unreadCount = -1;
        int undeleteCount = -1;

        // Calculate sum of all feeds with same parent
        qStr = QString("SELECT sum(unread), sum(undeleteCount) FROM feeds "
                       "WHERE parentId=='%1'").arg(folderId);
        q.exec(qStr);
        if (q.next()) {
          unreadCount   = q.value(0).toInt();
          undeleteCount = q.value(1).toInt();
        }

        if (unreadCount != -1) {
          qStr = QString("UPDATE feeds SET unread='%1', undeleteCount='%2' WHERE id=='%3'").
              arg(unreadCount).arg(undeleteCount).arg(folderId);
          q.exec(qStr);
        }

        // go to next parent's parent
        qStr = QString("SELECT parentId FROM feeds WHERE id=='%1'").arg(folderId);
        folderId = 0;
        q.exec(qStr);
        if (q.next()) folderId = q.value(0).toInt();
      }
    }

    if (cleanUpDeleted) {
      q.exec("UPDATE news SET description='', content='', received='', "
             "author_name='', author_uri='', author_email='', "
             "category='', new='', read='', starred='', label='', "
             "deleteDate='', feedParentId='', deleted=2 WHERE deleted==1");
    }
  }

  q.finish();
  db_.commit();

  if (cleanupOnShutdown && optimizeDB && !mainApp->storeDBMemory()) {
    db_.exec("VACUUM");
  }
}

void UpdateObject::quitApp()
{
  cleanUpShutdown();

  if (mainApp->storeDBMemory()) {
    saveMemoryDatabase();

    Settings settings;
    settings.beginGroup("Settings");
    bool cleanupOnShutdown = settings.value("cleanupOnShutdown", true).toBool();
    bool optimizeDB = settings.value("optimizeDB", false).toBool();
    settings.endGroup();
    if (cleanupOnShutdown && optimizeDB) {
      Database::setVacuum();
    }
  }

  QTimer::singleShot(0, mainApp, SLOT(quitApplication()));
}
