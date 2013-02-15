#ifndef UPDATETHREAD_H
#define UPDATETHREAD_H

#include <QThread>
#include "updateobject.h"

class UpdateThread : public QThread
{
  Q_OBJECT

public:
  explicit UpdateThread(QObject *parent, int requestTimeout = 90);
  ~UpdateThread();

  UpdateObject *updateObject_;

private:
  void run();

  int requestTimeout_;

};

#endif // UPDATETHREAD_H
