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
#include "parsethread.h"

#include <QDebug>

ParseThread::ParseThread(QObject *parent)
  : QThread(parent)
  , parseObject_(0)
{
  qDebug() << "ParseThread::constructor";

  setObjectName("parseThread_");
  start(LowestPriority);
}

ParseThread::~ParseThread()
{
  qDebug() << "ParseThread::~destructor";
}

/*virtual*/ void ParseThread::run()
{
  parseObject_ = new ParseObject(parent());
  connect(parent(), SIGNAL(xmlReadyParse(QByteArray,QString,QDateTime)),
          parseObject_, SLOT(parseXml(QByteArray,QString,QDateTime)));
  connect(parseObject_, SIGNAL(feedUpdated(QString, bool, int)),
          parent(), SLOT(slotUpdateFeed(QString, bool, int)));

  qRegisterMetaType<FeedCountStruct>("FeedCountStruct");
  connect(parseObject_, SIGNAL(feedCountsUpdate(FeedCountStruct)),
          parent(), SLOT(slotFeedCountsUpdate(FeedCountStruct)));

  exec();
}
