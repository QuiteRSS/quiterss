#ifndef PARSETHREAD_H
#define PARSETHREAD_H

#include <QThread>
#include "parseobject.h"

class ParseThread : public QThread
{
  Q_OBJECT
public:
  explicit ParseThread(QObject *parent, QString dataDirPath);
  ~ParseThread();

protected:
  virtual void run();

private:
  QString dataDirPath_;
  ParseObject *parseObject_;

};

#endif // PARSETHREAD_H
