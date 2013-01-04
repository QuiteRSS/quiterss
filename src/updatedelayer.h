#ifndef UPDATEDELAYER_H
#define UPDATEDELAYER_H

/** @brief Задержка обновления ленты
 *
 *  Задержка производится при импорте лент и обновлении всех лент одновременно.
 *  В этом случае обновление ленты вызывается слишком часто, что тормозит
 *  пользовательский интерфейс
 * @file updatedelayer.h
 *---------------------------------------------------------------------------*/

#include <QObject>
#include <QUrl>

class UpdateDelayer : public QObject
{
  Q_OBJECT
public:
  explicit UpdateDelayer(QObject *parent = 0);
  void delayUpdate(const QUrl &feedUrl, const bool &feedChanged);

public slots:

signals:
  void signalUpdateNeeded(const QUrl &feedUrl, const bool &feedChanged);
};

#endif // UPDATEDELAYER_H
