#include <QApplication>
#include <QDebug>

#include "updatedelayer.h"

#define UPDATE_INTERVAL 3000

UpdateDelayer::UpdateDelayer(QObject *parent, int delayValue)
    : QObject(parent), delayValue_(delayValue)
{
  next_ = true;
  delayTimer_ = new QTimer(this);
  delayTimer_->setSingleShot(true);
  connect(delayTimer_, SIGNAL(timeout()), this, SLOT(slotDelayTimerTimeout()));

  updateModelTimer_ = new QTimer(this);
  updateModelTimer_->setSingleShot(true);
  connect(updateModelTimer_, SIGNAL(timeout()), this, SIGNAL(signalUpdateModel()));
}

/** @brief Обработка постановки Id-ленты в очередь на обновление
 *
 *  Лента добавляется в очередь и запускается таймер, если он ещё не запущен
 * @param feedId Id-ленты
 * @param feedChanged Флаг того, что ленты действительно изменялась
 *---------------------------------------------------------------------------*/
void UpdateDelayer::delayUpdate(int feedId, const bool &feedChanged, int newCount)
{
  int feedIdIndex = feedIdList_.indexOf(feedId);
  // Если лента уже есть в списке
  if ((-1 < feedIdIndex) && feedId) {
    // Если лента изменялясь, то устанавливаем этот флаг принудительно
    if (feedChanged) {
      feedChangedList_[feedIdIndex] = feedChanged;  // i.e. true
      newCountList_[feedIdIndex] = newCount;
    }
  }
  // иначе добавляем ленту
  else {
    feedIdList_.append(feedId);
    feedChangedList_.append(feedChanged);
    newCountList_.append(newCount);
  }

  // Запуск таймера, если добавили первую ленту в список
  // Своеобразная защита от запуска во время обработки таймаута
  if ((feedIdList_.size() == 1) && next_) {
    next_ = false;
    delayTimer_->start(10);
    timer_.start();

    if (!updateModelTimer_->isActive())
      updateModelTimer_->start(UPDATE_INTERVAL);
  }

}

/** @brief Обработка срабатывания таймера
 *
 *  Производится обновление следующей в очереди ленты
 *---------------------------------------------------------------------------*/
void UpdateDelayer::slotDelayTimerTimeout()
{
  int feedId = feedIdList_.takeFirst();
  bool feedChanged = feedChangedList_.takeFirst();
  int newCount = newCountList_.takeFirst();
  emit signalUpdateNeeded(feedId, feedChanged, newCount);
}

/** @brief Запуск таймера при наличии в очереди лент
 *---------------------------------------------------------------------------*/
void UpdateDelayer::slotNext()
{
  qApp->processEvents();  // при перемещении окна оно не перерисовывается о_О
  if (feedIdList_.size()) {
    delayTimer_->start(delayValue_);
    timer_.start();

    if (!updateModelTimer_->isActive())
      updateModelTimer_->start(UPDATE_INTERVAL);
  } else {
    next_ = true;
    if (!updateModelTimer_->isActive())
      updateModelTimer_->start(UPDATE_INTERVAL);
  }
}
