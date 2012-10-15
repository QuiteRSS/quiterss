#ifndef GOOGLEREADER_H
#define GOOGLEREADER_H

#include <QtGui>
#include <QNetworkReply>

#define ADDFEED    "subscribe"
#define REMOVEFEED "unsubscribe"
#define RENAMEFEED "edit"

class GoogleReader : public QObject
{
  Q_OBJECT
public:
  explicit GoogleReader(QString email, QString passwd, QObject *parent = 0);

  void editFeed(QString urlFeed, QString action, QString name = "");
  void requestFeedsList();
  void requestUnreadCount();

  QString email_;
  QString passwd_;

private:
  void requestSidAuth();
  void sendHttpPost(QUrl url, QUrl params);
  void sendHttpGet(QUrl url, QNetworkAccessManager *manager);

  QTimer *sessionTimer_;

  QNetworkAccessManager managerAuth_;
  QNetworkAccessManager managerToken_;
  QNetworkAccessManager managerHttpPost_;
  QNetworkAccessManager managerFeedsList_;
  QNetworkAccessManager managerUnreadCount_;

  QString sid_;
  QString auth_;
  QString token_;
  QUrl requestUrl_;
  QUrl postArgs_;

signals:
  void signalReplySidAuth(bool ok);
  void signalReplyToken(bool ok);
  void signalReplyHttpPost(bool ok);

private slots:
  void replySidAuth(QNetworkReply *reply);
  void requestToken();
  void replyToken(QNetworkReply *reply);
  void replyHttpPost(QNetworkReply *reply);
  void replyFeedsList(QNetworkReply *reply);
  void replyUnreadCount(QNetworkReply *reply);

};

#endif // GOOGLEREADER_H
