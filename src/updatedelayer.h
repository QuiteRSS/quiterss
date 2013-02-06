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
private:
  QList<int> feedIdList_;
  QList<bool> feedChangedList_;
  QList<int> newCountList_;

  int delayValue_;
  QTimer *delayTimer_;
  QTimer *updateModelTimer_;

  bool nextUpdateFeed_;

private slots:
  void slotDelayTimerTimeout();

public:
  explicit UpdateDelayer(QObject *parent = 0, int delayValue = 50);
  void delayUpdate(int feedId, const bool &feedChanged, int newCount);

public slots:
  void slotNextUpdateFeed();

signals:
  void signalUpdateNeeded(int feedId, const bool &feedChanged, int newCount);
  void signalUpdateModel();
};

#endif // UPDATEDELAYER_H
