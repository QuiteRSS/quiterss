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
#include "updatedelayer.h"

#include <QApplication>

#define UPDATE_INTERVAL 3000
#define MIN_UPDATE_INTERVAL 500

UpdateDelayer::UpdateDelayer(QObject *parent, int delayValue)
  : QObject(parent)
  , delayValue_(delayValue)
  , nextUpdateFeed_(true)
{
  delayTimer_ = new QTimer(this);
  delayTimer_->setSingleShot(true);
  delayTimer_->setInterval(delayValue_);
  connect(delayTimer_, SIGNAL(timeout()), this, SLOT(slotDelayTimerTimeout()));

  updateModelTimer_ = new QTimer(this);
  updateModelTimer_->setSingleShot(true);
  connect(updateModelTimer_, SIGNAL(timeout()), this, SIGNAL(signalUpdateModel()));
}

/** @brief Process queueing feed
 *
 *  Feed is added to queue. Start timer if is not started yet.
 * @param feedId Feed Id
 * @param feedChanged Flag that indicate changed feed
 * @param newCount (need description)
 *---------------------------------------------------------------------------*/
void UpdateDelayer::delayUpdate(const int &feedId, const bool &feedChanged,
                                const int &newCount, const QString &status)
{
  if (!feedChanged) {
    emit signalUpdateNeeded(feedId, feedChanged, newCount, status);
    return;
  }

  int feedIdIndex = feedIdList_.indexOf(feedId);
  // If feed is in list already, ...
  if (-1 < feedIdIndex) {
    // If feed has changed, force enabling flag
    if (feedChanged) {
      feedChangedList_[feedIdIndex] = feedChanged;  // i.e. true
      newCountList_[feedIdIndex] = newCount;
      statusList_[feedIdIndex] = status;
    }
  }
  // ..., else queueing feed
  else {
    feedIdList_.append(feedId);
    feedChangedList_.append(feedChanged);
    newCountList_.append(newCount);
    statusList_.append(status);
  }

  // Start timer, if first feed added into queueing
  // Protect from starting while timeout is being processed
  if (nextUpdateFeed_) {
    nextUpdateFeed_ = false;
    delayTimer_->start();
  }
}

/** @brief Process delay timer timeout
 *
 *  Call update for next feed from queue
 *---------------------------------------------------------------------------*/
void UpdateDelayer::slotDelayTimerTimeout()
{
  int feedId = feedIdList_.takeFirst();
  bool feedChanged = feedChangedList_.takeFirst();
  int newCount = newCountList_.takeFirst();
  QString status = statusList_.takeFirst();
  emit signalUpdateNeeded(feedId, feedChanged, newCount, status);
}

/** @brief Start timer if feed presents in queue
 *---------------------------------------------------------------------------*/
void UpdateDelayer::slotNextUpdateFeed()
{
//  qApp->processEvents();
  if (feedIdList_.size()) {
    delayTimer_->start();

    if (!updateModelTimer_->isActive())
      updateModelTimer_->start(UPDATE_INTERVAL);
  } else {
    nextUpdateFeed_ = true;

    if (!updateModelTimer_->isActive())
      updateModelTimer_->start(UPDATE_INTERVAL);
  }
}
