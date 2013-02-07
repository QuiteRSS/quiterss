#ifndef PARSETHREAD_H
#define PARSETHREAD_H

#include <QtSql>

#include <QQueue>
#include <QThread>
#include <QUrl>
#include <QXmlStreamReader>

#include "parseobject.h"

class ParseThread : public QThread
{
  Q_OBJECT

private:
  QString dataDirPath_;
  QString currentFeedUrl_;
  QByteArray currentXml_;
  QQueue<QString> feedsQueue_;
  QQueue<QByteArray> xmlsQueue_;

  QTimer *parseTimer_;
  ParseObject *parseObject_;

  void run();

public:
  explicit ParseThread(QObject *parent, QString dataDirPath);
  ~ParseThread();

signals:
  void startTimer();
  void signalReadyParse(const QByteArray &xml, const QString &feedUrl);

private slots:
  void getQueuedXml();

public slots:
  void parseXml(const QByteArray &data, const QString &feedUrl);

};

#endif // PARSETHREAD_H
