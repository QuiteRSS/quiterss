#ifndef FAVICONOBJECT_H
#define FAVICONOBJECT_H

#include "networkmanager.h"
#include <QDateTime>
#include <QObject>
#include <QQueue>
#include <QNetworkReply>
#include <QTimer>

class FaviconObject : public QObject
{
  Q_OBJECT

public:
  explicit FaviconObject(QObject *parent = 0);
  NetworkManager *networkManager_;

public slots:
  void requestUrl(const QString &urlString, const QString &feedUrl);
  void slotGet(const QUrl &getUrl, const QString &feedUrl, const int &cnt);
  void slotIconSave(const QString &feedUrl, const QByteArray &faviconData);

signals:
  void startTimer();
  void signalGet(const QUrl &getUrl, const QString &feedUrl, const int &cnt);
  void signalIconRecived(const QString &feedUrl, const QByteArray &byteArray);
  void signalIconUpdate(int feedId, int feedParId, const QByteArray &faviconData);

private slots:
  void getQueuedUrl();
  void finished(QNetworkReply *reply);
  void slotRequestTimeout();

private:
  QQueue<QString> urlsQueue_;
  QQueue<QString> feedsQueue_;

  QList<QUrl> currentUrls_;
  QList<QString> currentFeeds_;
  QList<int> currentCntRequests_;
  QList<int> currentTime_;
  QList<QUrl> requestUrl_;
  QList<QNetworkReply*> networkReply_;

};

#endif // FAVICONOBJECT_H
