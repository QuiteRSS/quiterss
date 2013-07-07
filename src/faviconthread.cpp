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
#include "faviconthread.h"

#include <QDebug>
#include <QPixmap>
#include <QBuffer>

FaviconThread::FaviconThread(QObject *parent)
  : QThread(parent)
  , faviconObject_(NULL)
{
  qDebug() << "FaviconLoader::constructor";

  setObjectName("faviconThread_");
  start(LowestPriority);
}

FaviconThread::~FaviconThread()
{
  qDebug() << "FaviconLoader::~destructor";
}

/*virtual*/ void FaviconThread::run()
{
  faviconObject_ = new FaviconObject();
  connect(parent(), SIGNAL(faviconRequestUrl(QString,QString)),
          faviconObject_, SLOT(requestUrl(QString,QString)));
  connect(faviconObject_, SIGNAL(signalIconRecived(QString, const QByteArray&)),
          parent(), SLOT(slotIconFeedPreparing(QString, const QByteArray&)));
  connect(parent(), SIGNAL(signalIconFeedReady(QString, const QByteArray&)),
          faviconObject_, SLOT(slotIconSave(QString, const QByteArray&)));
  connect(faviconObject_, SIGNAL(signalIconUpdate(int, const QByteArray&)),
          parent(), SLOT(slotIconFeedUpdate(int, const QByteArray&)));

  exec();
}
