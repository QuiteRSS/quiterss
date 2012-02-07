#ifndef FAVICONLOADER_H
#define FAVICONLOADER_H

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
  explicit FaviconLoader( QObject *pParent = 0);
  ~FaviconLoader();

public slots:
  void requestUrl(const QUrl &url, const QUrl &feedUrl);

protected:
  virtual void run();

private slots:
  void getQueuedUrl();
  void slotFinished(QNetworkReply *reply);

private:  
  QList<QUrl> currentUrls_;
  QList<QUrl> currentFeeds_;
  QQueue<QUrl> urlsQueue_;
  QQueue<QUrl> feedsQueue_;
  QTimer *getUrlTimer_;

  UpdateObject *updateObject_;

  void get(const QUrl &getUrl, const QUrl &feedUrl);

signals:
  void startGetUrlTimer();
  void signalGet(const QNetworkRequest &request);
  void signalIconRecived(const QString& strUrl, const QByteArray& byteArray);
};

#endif // FAVICONLOADER_H
