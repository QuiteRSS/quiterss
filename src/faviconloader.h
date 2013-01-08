#ifndef FAVICONLOADER_H
#define FAVICONLOADER_H

#include <QtSql>
#include <QThread>
#include <QNetworkReply>
#include <QQueue>
#include <QTimer>
#include <QUrl>

#include "updateobject.h"

class FaviconLoader : public QThread
{
  Q_OBJECT
public:
  explicit FaviconLoader( QObject *pParent, QSqlDatabase *db);
  ~FaviconLoader();

public slots:
  void slotRequestUrl(const QString &urlString, const QUrl &feedUrl);

protected:
  virtual void run();

private slots:
  void getQueuedUrl();
  void slotFinished(QNetworkReply *reply);

private:  
  QSqlDatabase *db_;
  QList<QUrl> currentUrls_;
  QList<QUrl> currentFeeds_;
  QList<int> currentCntRequests_;
  QQueue<QUrl> urlsQueue_;
  QQueue<QUrl> feedsQueue_;
  QTimer *getUrlTimer_;

  UpdateObject *updateObject_;

  void get(const QUrl &getUrl, const QUrl &feedUrl, const int &cnt);

signals:
  void startGetUrlTimer();
  void signalGet(const QNetworkRequest &request);
  void signalIconRecived(int feedId, const QByteArray &byteArray, const int &cntQueue);
};

#endif // FAVICONLOADER_H
