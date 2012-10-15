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
