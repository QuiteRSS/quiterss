#ifndef UPDATETHREAD_H
#define UPDATETHREAD_H

#include <QDateTime>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QQueue>
#include <QTimer>
#include <QThread>
#include <QXmlStreamReader>

#include "updateobject.h"

#define REPLY_MAX_COUNT 8

class UpdateThread : public QThread
{
  Q_OBJECT

private:
  QNetworkProxy networkProxy_;
  QList<QUrl> currentUrls_;
  QList<QUrl> currentFeeds_;
  QList<QDateTime> currentDates_;
  QList<bool> currentHead_;

  QQueue<QUrl> urlsQueue_;
  QQueue<QDateTime> dateQueue_;
  QQueue<QString> userInfo_;

  QString itemString;
  QString titleString;
  QString linkString;
  QString descriptionString;
  QString commentsString;
  QString pubDateString;
  QString guidString;

  QTimer *getUrlTimer_;

  UpdateObject *updateObject_;

  void run();
  void get(const QUrl &getUrl, const QUrl &feedUrl, const QDateTime &date);
  void head(const QUrl &getUrl, const QUrl &feedUrl, const QDateTime &date);

public:
  explicit UpdateThread(QObject *parent = 0);
  ~UpdateThread();
  void requestUrl(const QString &urlString, const QDateTime &date, const QString &userInfo = "");
  void setProxy(const QNetworkProxy proxy);

signals:
  void startGetUrlTimer();
  void readedXml(const QByteArray &xml, const QUrl &url);
  void getUrlDone(const int &result, const QDateTime &dtReply = QDateTime());
  void signalHead(const QNetworkRequest &request);
  void signalGet(const QNetworkRequest &request);
  void signalAuthentication(QNetworkReply* reply, QAuthenticator* auth);

private slots:
  void getQueuedUrl();
  void finished(QNetworkReply *reply);

public slots:

};

#endif // UPDATETHREAD_H
