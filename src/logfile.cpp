#include <stdio.h>

#include "logfile.h"

void logMessageOutput(QtMsgType type, const char *msg)
{
  static QMap <QtMsgType, LogFilePtr> files;

  if (!files.contains (type)) {
    files[type] = LogFilePtr(new LogFile(type));
  }

  files[type]->setMessage(QString::fromUtf8(msg));
}
