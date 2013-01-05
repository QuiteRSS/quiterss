#ifndef UPDATEDELAYER_H
#define UPDATEDELAYER_H

/** @brief Задержка обновления ленты
 *
 *  Задержка производится при импорте лент и обновлении всех лент одновременно.
 *  В этом случае обновление ленты вызывается слишком часто, что тормозит
 *  пользовательский интерфейс
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

  QList<QUrl> feedUrlList_;
  QList<bool> feedChangedList_;

  int delayValue_;
  QTimer *delayTimer_;

  QElapsedTimer timer_;  // таймер для отладочного вывода

private slots:
  void slotDelayTimerTimeout();

public:
  explicit UpdateDelayer(QObject *parent = 0, int delayValue = 3000);
  void delayUpdate(const QUrl &feedUrl, const bool &feedChanged);

public slots:

signals:
  void signalUpdateNeeded(const QUrl &feedUrl, const bool &feedChanged);
};

#endif // UPDATEDELAYER_H
