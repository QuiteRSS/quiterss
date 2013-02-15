#ifndef FAVICONLOADER_H
#define FAVICONLOADER_H

#include <QThread>

#include "faviconobject.h"

class FaviconThread : public QThread
{
  Q_OBJECT
public:
  explicit FaviconThread(QObject *parent);
  ~FaviconThread();

  FaviconObject *faviconObject_;

protected:
  virtual void run();

};

#endif // FAVICONLOADER_H
