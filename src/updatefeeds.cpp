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
#include "updatefeeds.h"
#include "rsslisting.h"

#include <QDebug>

#define UPDATE_INTERVAL 5000

UpdateFeeds::UpdateFeeds(QObject *parent, bool add)
  : QObject(parent)
  , updateObject_(NULL)
  , requestFeed_(NULL)
  , parseObject_(NULL)
{
  QObject *parent_ = parent;
  while(parent_->parent()) {
    parent_ = parent_->parent();
  }
  RSSListing *rssl = qobject_cast<RSSListing*>(parent_);

  getFeedThread_ = new QThread();
  getFeedThread_->setObjectName("getFeedThread_");
  updateFeedThread_ = new QThread();
  updateFeedThread_->setObjectName("updateFeedThread_");

  int timeoutRequest = rssl->settings_->value("Settings/timeoutRequest", 15).toInt();
  int numberRequest = rssl->settings_->value("Settings/numberRequest", 10).toInt();
  int numberRepeats = rssl->settings_->value("Settings/numberRepeats", 2).toInt();

  requestFeed_ = new RequestFeed(timeoutRequest, numberRequest, numberRepeats);
  requestFeed_->networkManager_->setCookieJar(rssl->cookieJar_);

  parseObject_ = new ParseObject(parent);
  updateObject_ = new UpdateObject(parent);

  if (add) {
    connect(parent, SIGNAL(signalRequestUrl(int,QString,QDateTime,QString)),
            requestFeed_, SLOT(requestUrl(int,QString,QDateTime,QString)));
    connect(requestFeed_, SIGNAL(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)),
            parent, SLOT(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)),
            Qt::QueuedConnection);

    connect(parent, SIGNAL(xmlReadyParse(QByteArray,int,QDateTime,QString)),
            parseObject_, SLOT(parseXml(QByteArray,int,QDateTime,QString)));
    connect(parseObject_, SIGNAL(signalFinishUpdate(int,bool,int,QString)),
            parent, SLOT(slotUpdateFeed(int,bool,int,QString)),
            Qt::QueuedConnection);
  } else {
    connect(updateObject_, SIGNAL(signalRequestUrl(int,QString,QDateTime,QString)),
            requestFeed_, SLOT(requestUrl(int,QString,QDateTime,QString)));
    connect(requestFeed_, SIGNAL(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)),
            updateObject_, SLOT(getUrlDone(int,int,QString,QString,QByteArray,QDateTime,QString)));
    connect(requestFeed_, SIGNAL(setStatusFeed(int,QString)),
            parent, SLOT(setStatusFeed(int,QString)));

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
    connect(parent, SIGNAL(signalGetAllFeeds()),
            updateObject_, SLOT(slotGetAllFeeds()));
    connect(parent, SIGNAL(signalImportFeeds(QByteArray)),
            updateObject_, SLOT(slotImportFeeds(QByteArray)));
    connect(updateObject_, SIGNAL(showProgressBar(int)),
            parent, SLOT(showProgressBar(int)),
            Qt::QueuedConnection);
    connect(updateObject_, SIGNAL(loadProgress(int)),
            parent, SLOT(slotSetValue(int)),
            Qt::QueuedConnection);
    connect(updateObject_, SIGNAL(signalMessageStatusBar(QString,int)),
            parent, SLOT(showMessageStatusBar(QString,int)));
    connect(parent, SIGNAL(signalRecountCategoryCounts()),
            updateObject_, SLOT(slotRecountCategoryCounts()),
            Qt::QueuedConnection);
    connect(updateObject_, SIGNAL(signalUpdateFeedsModel()),
            parent, SLOT(feedsModelReload()),
            Qt::BlockingQueuedConnection);

    connect(updateObject_, SIGNAL(xmlReadyParse(QByteArray,int,QDateTime,QString)),
            parseObject_, SLOT(parseXml(QByteArray,int,QDateTime,QString)));
    connect(parseObject_, SIGNAL(signalFinishUpdate(int,bool,int,QString)),
            updateObject_, SLOT(finishUpdate(int,bool,int,QString)));
    connect(updateObject_, SIGNAL(feedUpdated(int,bool,int,bool)),
            parent, SLOT(slotUpdateFeed(int,bool,int,bool)));
    connect(updateObject_, SIGNAL(setStatusFeed(int,QString)),
            parent, SLOT(setStatusFeed(int,QString)));

    qRegisterMetaType<FeedCountStruct>("FeedCountStruct");
    connect(parseObject_, SIGNAL(feedCountsUpdate(FeedCountStruct)),
            parent, SLOT(slotFeedCountsUpdate(FeedCountStruct)),
            Qt::QueuedConnection);

    connect(parent, SIGNAL(signalNextUpdate()),
            updateObject_, SLOT(slotNextUpdateFeed()));
    connect(updateObject_, SIGNAL(signalUpdateModel(bool)),
            parent, SLOT(feedsModelReload(bool)),
            Qt::QueuedConnection);
    connect(updateObject_, SIGNAL(signalUpdateNews()),
            parent, SLOT(slotUpdateNews()),
            Qt::QueuedConnection);
    connect(updateObject_, SIGNAL(signalCountsStatusBar(int,int)),
            parent, SLOT(slotCountsStatusBar(int,int)),
            Qt::QueuedConnection);

    connect(parent, SIGNAL(signalRecountCategoryCounts()),
            updateObject_, SLOT(slotRecountCategoryCounts()));
    qRegisterMetaType<QList<int> >("QList<int>");
    qRegisterMetaType<QList<int> >("QList<int>");
    connect(updateObject_, SIGNAL(signalRecountCategoryCounts(QList<int>,QList<int>,QList<int>,QStringList)),
            parent, SLOT(slotRecountCategoryCounts(QList<int>,QList<int>,QList<int>,QStringList)),
            Qt::QueuedConnection);
  }

  connect(requestFeed_->networkManager_,
          SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          parent, SLOT(slotAuthentication(QNetworkReply*,QAuthenticator*)),
          Qt::BlockingQueuedConnection);

  requestFeed_->moveToThread(getFeedThread_);
  parseObject_->moveToThread(updateFeedThread_);
  updateObject_->moveToThread(updateFeedThread_);

  getFeedThread_->start(QThread::LowPriority);
  updateFeedThread_->start();
}

UpdateFeeds::~UpdateFeeds()
{
  getFeedThread_->exit();
  getFeedThread_->wait();
  getFeedThread_->deleteLater();

  updateFeedThread_->exit();
  updateFeedThread_->wait();
  updateFeedThread_->deleteLater();

  requestFeed_->deleteLater();
  parseObject_->deleteLater();
  updateObject_->deleteLater();
}

//------------------------------------------------------------------------------
UpdateObject::UpdateObject(QObject *parent)
  : QObject(0)
  , updateFeedsCount_(0)
{
  setObjectName("updateObject_");

  QObject *parent_ = parent;
  while(parent_->parent()) {
    parent_ = parent_->parent();
  }
  rssl_ = qobject_cast<RSSListing*>(parent_);

  updateModelTimer_ = new QTimer(this);
  updateModelTimer_->setSingleShot(true);
  connect(updateModelTimer_, SIGNAL(timeout()), this, SIGNAL(signalUpdateModel()));

  timerUpdateNews_ = new QTimer(this);
  timerUpdateNews_->setSingleShot(true);
  connect(timerUpdateNews_, SIGNAL(timeout()), this, SIGNAL(signalUpdateNews()));
}

void UpdateObject::slotGetFeedTimer(int feedId)
{
  QSqlQuery q;
  q.exec(QString("SELECT xmlUrl, lastBuildDate, authentication FROM feeds WHERE id=='%1'")
         .arg(feedId));
  if (q.next()) {
    addFeedInQueue(feedId, q.value(0).toString(),
                   q.value(1).toDateTime(), q.value(2).toInt());
  }
  emit showProgressBar(updateFeedsCount_);
}

void UpdateObject::slotGetAllFeedsTimer()
{
  QSqlQuery q;
  q.exec("SELECT id, xmlUrl, lastBuildDate, authentication FROM feeds "
         "WHERE xmlUrl!='' AND (updateIntervalEnable==-1 OR updateIntervalEnable IS NULL)");
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
  QSqlQuery q;
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
  QSqlQuery q;
  q.exec("SELECT id, xmlUrl, lastBuildDate, authentication FROM feeds WHERE xmlUrl!=''");
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
  QSqlQuery q;
  QList<int> idsList;
  QList<QString> urlsList;

  xmlData.replace("&", "&#38;");
  QXmlStreamReader xml(xmlData);

  QSqlDatabase db = QSqlDatabase::database();
  db.transaction();

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
    emit signalMessageStatusBar(QString("Import error: Line=%1, ErrorString=%2").
                                arg(xml.lineNumber()).arg(xml.errorString()), 3000);
  } else {
    emit signalMessageStatusBar(QString("Import: file read done"), 3000);
  }
  db.commit();

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
      QSqlQuery q;
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
    if (result < 0) status = QString("%1 %2").arg(result).arg(error);
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

  QSqlQuery q;
  QString qStr = QString("UPDATE feeds SET status='%1' WHERE id=='%2'").
      arg(status).arg(feedId);
  q.exec(qStr);

  if (changed) {
    if (rssl_->currentNewsTab->type_ == TAB_FEED) {
      bool folderUpdate = false;
      int feedParentId = 0;

      QSqlQuery q;
      q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(feedId));
      if (q.first()) {
        feedParentId = q.value(0).toInt();
        if (feedParentId == rssl_->currentNewsTab->feedId_) folderUpdate = true;
      }

      while (feedParentId && !folderUpdate) {
        q.exec(QString("SELECT parentId FROM feeds WHERE id==%1").arg(feedParentId));
        if (q.first()) {
          feedParentId = q.value(0).toInt();
          if (feedParentId == rssl_->currentNewsTab->feedId_) folderUpdate = true;
        }
      }

      // Click on feed if it is displayed to update view
      if ((feedId == rssl_->currentNewsTab->feedId_) || folderUpdate) {
        if (!timerUpdateNews_->isActive())
          timerUpdateNews_->start(1000);

        int unreadCount = 0;
        int allCount = 0;
        q.exec(QString("SELECT unread, undeleteCount FROM feeds WHERE id=='%1'").
               arg(rssl_->currentNewsTab->feedId_));
        if (q.first()) {
          unreadCount = q.value(0).toInt();
          allCount    = q.value(1).toInt();
        }
        emit signalCountsStatusBar(unreadCount, allCount);
      }
    } else if (rssl_->currentNewsTab->type_ < TAB_WEB) {
      if (!timerUpdateNews_->isActive())
        timerUpdateNews_->start(1000);
    }
  }

  emit feedUpdated(feedId, changed, newCount, finish);
  emit setStatusFeed(feedId, status);
}

/** @brief Start timer if feed presents in queue
 *---------------------------------------------------------------------------*/
void UpdateObject::slotNextUpdateFeed()
{
  if (!updateModelTimer_->isActive())
    updateModelTimer_->start(UPDATE_INTERVAL);
}

void UpdateObject::slotRecountCategoryCounts()
{
  QList<int> deletedList;
  QList<int> starredList;
  QList<int> readList;
  QStringList labelList;
  QSqlQuery q;
  q.exec("SELECT deleted, starred, read, label FROM news WHERE deleted < 2");
  while (q.next()) {
    deletedList.append(q.value(0).toInt());
    starredList.append(q.value(1).toInt());
    readList.append(q.value(2).toInt());
    labelList.append(q.value(3).toString());
  }

  emit signalRecountCategoryCounts(deletedList, starredList, readList, labelList);
}
