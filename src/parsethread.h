#ifndef PARSETHREAD_H
#define PARSETHREAD_H

#include "parseobject.h"

class ParseThread : public QThread
{
  Q_OBJECT

private:
  QSqlDatabase *db_;
  QUrl currentUrl_;
  QByteArray currentXml_;
  QQueue<QUrl> urlsQueue_;
  QQueue<QByteArray> xmlsQueue_;

  QTimer *parseTimer_;
  ParseObject *parseObject_;

  void run();

public:
  explicit ParseThread(QObject *parent, QSqlDatabase *db);
  ~ParseThread();

signals:
  void startTimer();
  void signalReadyParse(QSqlDatabase *db,
                        const QByteArray &xml, const QUrl &url);

private slots:
  void getQueuedXml();

public slots:
  void parseXml(const QByteArray &data, const QUrl &url);

};

#endif // PARSETHREAD_H
