#ifndef UPDATETHREAD_H
#define UPDATETHREAD_H

#include <QNetworkProxy>
#include <QNetworkReply>
#include <QQueue>
#include <QThread>
#include <QXmlStreamReader>

class UpdateThread : public QThread
{
  Q_OBJECT

private:
  QNetworkAccessManager manager_;
  QNetworkReply *currentReply_;
  QNetworkProxy networkProxy_;
  QUrl currentUrl_;

  QQueue<QUrl> urlsQueue_;

  QString itemString;
  QString titleString;
  QString linkString;
  QString descriptionString;
  QString commentsString;
  QString pubDateString;
  QString guidString;

  void get(const QUrl &url);
  void getQueuedUrl();

public:
  explicit UpdateThread(QObject *parent = 0);
  ~UpdateThread();
  void run();
  void getUrl(const QUrl &url);
  void setProxy(const QNetworkProxy proxy);

signals:
  void readedXml(const QByteArray &xml, const QUrl &url);
  void getUrlDone(const int &result);

private slots:
  void readyRead();
  void metaDataChanged();
  void finished(QNetworkReply *reply);
  void error(QNetworkReply::NetworkError);

public slots:

};

#endif // UPDATETHREAD_H
