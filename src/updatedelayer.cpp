#include <QApplication>
#include <QDebug>

#include "updatedelayer.h"

#define UPDATE_INTERVAL 3000

UpdateDelayer::UpdateDelayer(QObject *parent, int delayValue)
    : QObject(parent), delayValue_(delayValue)
{
  delayTimer_ = new QTimer(this);
  delayTimer_->setSingleShot(true);
  connect(delayTimer_, SIGNAL(timeout()), this, SLOT(slotDelayTimerTimeout()));
}

/** @brief Обработка постановки URL-ленты в очередь на обновление
 *
 *  Лента добавляется в очередь и запускается таймер, если он ещё не запущен
 * @param feedUrl URL-ленты
 * @param feedChanged Флаг того, что ленты действительно изменялась
 *---------------------------------------------------------------------------*/
void UpdateDelayer::delayUpdate(const QUrl &feedUrl, const bool &feedChanged)
{
  int feedUrlIndex = feedUrlList_.indexOf(feedUrl);
  // Если лента уже есть в списке
  if (-1 < feedUrlIndex) {
    // Если лента изменялясь, то устанавливаем этот флаг принудительно
    if (feedChanged) {
      feedChangedList_[feedUrlIndex] = feedChanged;  // i.e. true
    }
  }
  // иначе добавляем ленту
  else {
    feedUrlList_.append(feedUrl);
    feedChangedList_.append(feedChanged);
    qCritical() << feedUrl.toString() << "добавлен в очередь на обновление";
  }

  // Запуск таймера, если добавили первую ленту в список
  // Своеобразная защита от запуска во время обработки таймаута
  if (feedUrlList_.size() == 1) {
    delayTimer_->start(delayValue_);
    timer_.start();
  }

}

/** @brief Обработка срабатывания таймера
 *
 *  Производится обновление всех накопившихся лент.
 *  Используется счетчик, так как во время обновления список может пополняться.
 *  Обновляем новые через паузу.
 *---------------------------------------------------------------------------*/
void UpdateDelayer::slotDelayTimerTimeout()
{
  qCritical() << "Delayed update: " << "______________________________________";

  // Обрабатываем все ленты на момент срабатывания таймера
  for (int i = feedUrlList_.size(); 0 < i; --i) {
    qCritical() << "feedUrlList_.size() =" << feedUrlList_.size();
    QUrl feedUrl = feedUrlList_.takeFirst();
    bool feedChanged = feedChangedList_.takeFirst();

    qCritical() << "Delayed update: " << timer_.elapsed() << feedUrl.toString() << feedChanged << "start";

    emit signalUpdateNeeded(feedUrl, feedChanged);

    qCritical() << "Delayed update: " << timer_.elapsed() << feedUrl.toString() << feedChanged << "finish";

    qApp->processEvents();  // при перемещении окна оно не перерисовывается о_О

    // Прерываем обновление, чтобы "отморозить" интерфейс
    if (delayValue_ + UPDATE_INTERVAL < timer_.elapsed()) break;
  }

  qCritical() << "Delayed update: " << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^";

  // Если список пополнился во время обработки, запускаем таймер обновления вновь
  if (feedUrlList_.size()) {
    delayTimer_->start(delayValue_);
    timer_.start();
  }
}

