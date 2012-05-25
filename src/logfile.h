#ifndef LOGFILE_H
#define LOGFILE_H

#include <QtGlobal>

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QMap>
#include <QSharedPointer>
#include <QTextStream>

const QString appName = "QuiteRSS";

const size_t maxLogFileSize = 4 * 1024 * 1024; //4 MB

class LogFile
{
public:
  explicit LogFile(QtMsgType type) : type_(type) {

    messages_[QtDebugMsg] = "Debug";
    messages_[QtWarningMsg] = "Warning";
    messages_[QtCriticalMsg] = "Critical";
    messages_[QtFatalMsg] = "Fatal";

    fileNames_[QtDebugMsg] = appName + "_debug.log";
    fileNames_[QtWarningMsg] = appName + "_warning.log";
    fileNames_[QtCriticalMsg] = appName + "_critical.log";
    fileNames_[QtFatalMsg] = appName + "_fatal.log";

    file_.setFileName(QCoreApplication::applicationDirPath ()
               + "/"
               + fileNames_[type_]);
    QIODevice::OpenMode openMode = QIODevice::WriteOnly;

    if (file_.exists() && file_.size() < maxLogFileSize) {
      openMode |= QIODevice::Append;
    }

    file_.open(openMode);

    stream_.setDevice(&file_);
    stream_.setCodec("UTF-8");
  }

  ~LogFile() {
    stream_.flush();
    file_.flush();
    file_.close();
  }

  template <class T>
  void setMessage(const T &msg) {
    if (file_.isOpen()) {
      stream_ << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
              << " " << messages_[type_] << ": " << msg << endl;
    }
  }

private:
  LogFile (const LogFile &);
  LogFile &operator= (const LogFile &);

private:
  QtMsgType type_;
  QMap <QtMsgType, QString> messages_;
  QMap <QtMsgType, QString> fileNames_;

  QFile file_;
  QTextStream stream_;
};

typedef QSharedPointer <LogFile> LogFilePtr;

void logMessageOutput(QtMsgType type, const char *msg);

#endif // LOGFILE_H
