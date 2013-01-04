#include "updatedelayer.h"

UpdateDelayer::UpdateDelayer(QObject *parent) :
  QObject(parent)
{
}

/** @brief Обработка постановки URL-ленты в очередь на обновление
 *
 *  Пока никакой задержки не производится
 * @param feedUrl URL-ленты
 * @param feedChanged Флаг того, что ленты действительно изменялась
 *---------------------------------------------------------------------------*/
void UpdateDelayer::delayUpdate(const QUrl &feedUrl, const bool &feedChanged)
{
  emit signalUpdateNeeded(feedUrl, feedChanged);
}

