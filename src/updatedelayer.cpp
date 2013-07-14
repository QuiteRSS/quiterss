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
 * @param feedUrl Feed URL
 * @param feedChanged Flag that indicate changed feed
 * @param newCount (need description)
 *---------------------------------------------------------------------------*/
void UpdateDelayer::delayUpdate(const QString &feedUrl, const bool &feedChanged, int newCount)
{
  if (!feedChanged) {
    emit signalUpdateNeeded(feedUrl, feedChanged, newCount);
    return;
  }

  int feedIdIndex = feedUrlList_.indexOf(feedUrl);
  // If feed is in list already, ...
  if (-1 < feedIdIndex) {
    // If feed has changed, force enabling flag
    if (feedChanged) {
      feedChangedList_[feedIdIndex] = feedChanged;  // i.e. true
      newCountList_[feedIdIndex] = newCount;
    }
  }
  // ..., else queueing feed
  else {
    feedUrlList_.append(feedUrl);
    feedChangedList_.append(feedChanged);
    newCountList_.append(newCount);
  }

  // Start timer, if first feed added into queueing
  // Protect from starting while timeout is being processed
  if ((feedUrlList_.size() == 1) && nextUpdateFeed_) {
    nextUpdateFeed_ = false;
    delayTimer_->start();

    if (!updateModelTimer_->isActive()) {
      updateModelTimer_->start(MIN_UPDATE_INTERVAL);
    } else {
      if (updateModelTimer_->interval() == MIN_UPDATE_INTERVAL) {
        updateModelTimer_->start(UPDATE_INTERVAL - MIN_UPDATE_INTERVAL);
      }
    }
  }

}

/** @brief Process delay timer timeout
 *
 *  Call update for next feed from queue
 *---------------------------------------------------------------------------*/
void UpdateDelayer::slotDelayTimerTimeout()
{
  QString feedUrl = feedUrlList_.takeFirst();
  bool feedChanged = feedChangedList_.takeFirst();
  int newCount = newCountList_.takeFirst();
  emit signalUpdateNeeded(feedUrl, feedChanged, newCount);
}

/** @brief Start timer if feed presents in queue
 *---------------------------------------------------------------------------*/
void UpdateDelayer::slotNextUpdateFeed()
{
  if (feedUrlList_.size()) {
    delayTimer_->start();

    if (!updateModelTimer_->isActive())
      updateModelTimer_->start(UPDATE_INTERVAL);
  } else {
    nextUpdateFeed_ = true;

   if (!updateModelTimer_->isActive())
      updateModelTimer_->start(MIN_UPDATE_INTERVAL);
  }
}
