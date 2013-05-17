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
#ifndef UPDATEDELAYER_H
#define UPDATEDELAYER_H

/** @brief Delay feed updating
 *
 *  Delay is used while import feeds or while updating all feeds simultaneuosly.
 *  In this case feed refreshes to much often than GUI stucks.
 * @file updatedelayer.h
 *---------------------------------------------------------------------------*/

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QPair>
#include <QTimer>
#include <QUrl>

class UpdateDelayer : public QObject
{
  Q_OBJECT
public:
  explicit UpdateDelayer(QObject *parent = 0, int delayValue = 10);
  void delayUpdate(const QString &feedUrl, const bool &feedChanged, int newCount);

public slots:
  void slotNextUpdateFeed();

signals:
  void signalUpdateNeeded(const QString &feedUrl, const bool &feedChanged, int newCount);
  void signalUpdateModel(bool checkFilter = true);

private slots:
  void slotDelayTimerTimeout();

private:
  QList<QString> feedUrlList_;
  QList<bool> feedChangedList_;
  QList<int> newCountList_;

  int delayValue_;
  QTimer *delayTimer_;
  QTimer *updateModelTimer_;

  bool nextUpdateFeed_;

};

#endif // UPDATEDELAYER_H
