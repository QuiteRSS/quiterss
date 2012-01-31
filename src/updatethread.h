#ifndef UPDATETHREAD_H
#define UPDATETHREAD_H

#include <QDateTime>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QQueue>
#include <QThread>
#include <QXmlStreamReader>

#define REPLY_MAX_COUNT 8

class UpdateThread : public QThread
{
  Q_OBJECT

private:
  QNetworkAccessManager manager_;
  QList<QNetworkReply *>currentReplies_;
  QNetworkProxy networkProxy_;
  QList<QUrl> currentUrls_;
  QList<QDateTime> currentDates_;
//  QByteArray currentData_;

  QQueue<QUrl> urlsQueue_;
  QQueue<QDateTime> dateQueue_;

  QString itemString;
  QString titleString;
  QString linkString;
  QString descriptionString;
  QString commentsString;
  QString pubDateString;
  QString guidString;

  void get(const QUrl &url, const QDateTime &date);
  void head(const QUrl &url, const QDateTime &date);
  void getQueuedUrl();

public:
  explicit UpdateThread(QObject *parent = 0);
  ~UpdateThread();
  void run();
  void getUrl(const QUrl &url, const QDateTime &date);
  void setProxy(const QNetworkProxy proxy);

signals:
  void readedXml(const QByteArray &xml, const QUrl &url);
  void getUrlDone(const int &result, const QDateTime &dtReply = QDateTime());

private slots:
//  void readyRead();
//  void metaDataChanged();
  void finished(QNetworkReply *reply);
//  void error(QNetworkReply::NetworkError);

public slots:

};

#endif // UPDATETHREAD_H
