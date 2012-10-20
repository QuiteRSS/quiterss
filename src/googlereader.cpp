#include "googlereader.h"
#include <QSslConfiguration>

GoogleReader::GoogleReader(QString email, QString passwd, QObject *parent) :
  QObject(parent),
  email_(email),
  passwd_(passwd)
{
  qCritical() << "GoogleReader init";

  sessionTimer_ = new QTimer(this);
  connect(sessionTimer_, SIGNAL(timeout()),
          this, SLOT(requestToken()));
  connect(&managerToken_, SIGNAL(finished(QNetworkReply *)),
          this, SLOT(replyToken(QNetworkReply *)));
  connect(&managerHttpPost_, SIGNAL(finished(QNetworkReply *)),
          this, SLOT(replyHttpPost(QNetworkReply *)));

  connect(&managerFeedsList_, SIGNAL(finished(QNetworkReply *)),
          this, SLOT(replyFeedsList(QNetworkReply *)));
  connect(&managerUnreadCount_, SIGNAL(finished(QNetworkReply *)),
          this, SLOT(replyUnreadCount(QNetworkReply*)));
  connect(&managerFeed_, SIGNAL(finished(QNetworkReply *)),
          this, SLOT(replyFeed(QNetworkReply *)));

  requestSidAuth();
}

//! Запрос авторизации
void GoogleReader::requestSidAuth()
{
  QNetworkRequest request(QUrl("https://www.google.com/accounts/ClientLogin"));
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  QSslConfiguration sslconf;
  sslconf.setPeerVerifyMode(QSslSocket::VerifyNone);
  request.setSslConfiguration(sslconf);

  QUrl params;
  params.addQueryItem("Email", email_);
  params.addQueryItem("Passwd", passwd_);
  params.addQueryItem("service", "reader");

  connect(&managerAuth_, SIGNAL(finished(QNetworkReply *)),
          this, SLOT(replySidAuth(QNetworkReply *)));

  managerAuth_.post(request, params.encodedQuery());
}

void GoogleReader::replySidAuth(QNetworkReply *reply)
{
  bool ok = false;
  if (reply->error() == QNetworkReply::NoError) {
    QString str = QString::fromUtf8(reply->readAll());
    sid_ = QString(str.split("SID=").at(1)).split('\n').at(0);
    auth_ = QString(str.split("Auth=").at(1)).remove('\n');

    sessionTimer_->start(1);
    ok = true;
  } else {
    qCritical() << "Error replySidAuth: " << reply->errorString();
  }
  emit signalReplySidAuth(ok);
}

//! Запрос идентификатора сессии (жизнь сессии - 30 мин, запрос каждые 20 мин)
void GoogleReader::requestToken()
{
  sessionTimer_->stop();
  sessionTimer_->start(20000);

  QNetworkRequest request(QUrl("http://www.google.com/reader/api/0/token"));
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  request.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(auth_).toUtf8());

  managerToken_.get(request);
}

void GoogleReader::replyToken(QNetworkReply *reply)
{
  bool ok = false;
  if (reply->error() == QNetworkReply::NoError) {
    QString str = QString::fromUtf8(reply->readAll());
    token_ = str;
    ok = true;
  } else {
    qCritical() << "Error requestToken: " << reply->errorString();
  }
  emit signalReplyToken(ok);
}

//! Отправка HTTP POST запроса
void GoogleReader::sendHttpPost(QUrl url, QUrl params)
{
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
  request.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(auth_).toUtf8());

  managerHttpPost_.post(request, params.encodedQuery());
}

//! Получение ответа на HTTP POST запрос
void GoogleReader::replyHttpPost(QNetworkReply *reply)
{
  bool ok = false;
  if (reply->error() == QNetworkReply::NoError) {
    QString str = QString::fromUtf8(reply->readAll());
    if (str == "OK") ok = true;
    else
      qCritical() << "No";
  } else {
    qCritical() << "Error sendHttpPost: " << reply->errorString();
  }
  emit signalReplyHttpPost(ok);
}

//! Отправка HTTP GET запроса
void GoogleReader::sendHttpGet(QUrl url, QNetworkAccessManager *manager)
{
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
  request.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(auth_).toUtf8());

  manager->get(request);
}

//! Добавление, удаление и переименование ленты
void GoogleReader::editFeed(QString urlFeed, QString action, QString name)
{
  QUrl params;
  params.addQueryItem("client", "QuiteRSS");
  params.addQueryItem("s", QString("feed/%1").arg(urlFeed));
  params.addQueryItem("ac", action);
  params.addQueryItem("t", name);
  params.addQueryItem("T", token_);
  qCritical() << token_;

  sendHttpPost(QUrl("https://www.google.com/reader/api/0/subscription/edit"),
               params);
}

//! Запрос списка лент
void GoogleReader::requestFeedsList()
{
  sendHttpGet(QUrl("https://www.google.com/reader/api/0/subscription/list?output=xml"),
              &managerFeedsList_);
}

//! Ответ на запрос списка лент
void GoogleReader::replyFeedsList(QNetworkReply *reply)
{
  QString dataStr;
  if (reply->error() == QNetworkReply::NoError) {
    dataStr = QString::fromUtf8(reply->readAll());
  } else {
    qCritical() << "Error requestFeedsList: " << reply->errorString();
  }
}

//! Запрос количества непрочитанных новостей
void GoogleReader::requestUnreadCount()
{
  sendHttpGet(QUrl("https://www.google.com/reader/api/0/unread-count?output=xml"),
              &managerUnreadCount_);
}

//! Ответ на запрос количества непрочитанных новостей
void GoogleReader::replyUnreadCount(QNetworkReply *reply)
{
  QString dataStr;
  if (reply->error() == QNetworkReply::NoError) {
    dataStr = QString::fromUtf8(reply->readAll());
  } else {
    qCritical() << "Error requestUnreadCount: " << reply->errorString();
  }
}

//! Запрос новостей ленты
void GoogleReader::requestFeed(QString urlFeed, int ot)
{
  QUrl params;
  params.setUrl("http://www.google.com/reader/api/0/stream/contents/" +
                QString("feed/%1").arg(urlFeed));


  QDateTime dtLocalTime = QDateTime::currentDateTime();
  QDateTime dt = QDateTime::fromString("1970-01-01T00:00:00", Qt::ISODate);
  int nTimeShift = dt.secsTo(dtLocalTime);
  params.addQueryItem("ck", QString::number(nTimeShift));
  if (ot)
    params.addQueryItem("ot", QString::number(ot));
  params.addQueryItem("client", "QuiteRSS");

  sendHttpGet(params, &managerFeed_);
}

//! Ответ на запрос новостей ленты
void GoogleReader::replyFeed(QNetworkReply *reply)
{
  QString dataStr;
  if (reply->error() == QNetworkReply::NoError) {
    dataStr = QString::fromUtf8(reply->readAll());
    qCritical() << dataStr;
  } else {
    qCritical() << "Error requestUnreadCount: " << reply->errorString();
  }
}

//! Пометка новости прочитанной или звездой
void GoogleReader::editItem(QString urlFeed, QString itemId, QString action)
{
  QUrl params;
  params.addQueryItem("client", "QuiteRSS");
  if (action == "read")
    params.addQueryItem("a", "user/-/state/com.google/read");
  else if (action == "unread")
    params.addQueryItem("r", "user/-/state/com.google/read");
  else if (action == "starred")
    params.addQueryItem("a", "user/-/state/com.google/starred");
  else if (action == "unstarred")
    params.addQueryItem("r", "user/-/state/com.google/starred");
  params.addQueryItem("async", "true");
  params.addQueryItem("s", QString("feed/%1").arg(urlFeed));
  params.addQueryItem("i", QString("tag:google.com,2005:reader/item/%1").arg(itemId));
  params.addQueryItem("T", token_);

  sendHttpPost(QUrl("http://www.google.com/reader/api/0/edit-tag"),
               params);
}
