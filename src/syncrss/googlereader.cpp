/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "googlereader.h"

#include <QSslConfiguration>

GoogleReader::GoogleReader(const QString &email, const QString &passwd, QObject *parent)
  : QObject(parent)
  , email_(email)
  , passwd_(passwd)
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

//! Authorization request
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

//! Session identificator request (session timeout - 30 min, session request - every 20 min)
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

//! HTTP POST request
void GoogleReader::sendHttpPost(QUrl url, QUrl params)
{
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
  request.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(auth_).toUtf8());

  managerHttpPost_.post(request, params.encodedQuery());
}

//! Process HTTP POST reply
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

//! HTTP GET request
void GoogleReader::sendHttpGet(QUrl url, QNetworkAccessManager *manager)
{
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
  request.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(auth_).toUtf8());

  manager->get(request);
}

//! Adding, deleting, renaming feed
void GoogleReader::editFeed(const QString &urlFeed, const QString &action, const QString &name)
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

//! Feed list request
void GoogleReader::requestFeedsList()
{
  sendHttpGet(QUrl("https://www.google.com/reader/api/0/subscription/list?output=xml"),
              &managerFeedsList_);
}

//! Process feed list reply
void GoogleReader::replyFeedsList(QNetworkReply *reply)
{
  QString dataStr;
  if (reply->error() == QNetworkReply::NoError) {
    dataStr = QString::fromUtf8(reply->readAll());
  } else {
    qCritical() << "Error requestFeedsList: " << reply->errorString();
  }
}

//! News unread number request
void GoogleReader::requestUnreadCount()
{
  sendHttpGet(QUrl("https://www.google.com/reader/api/0/unread-count?output=xml"),
              &managerUnreadCount_);
}

//! Process news unread number reply
void GoogleReader::replyUnreadCount(QNetworkReply *reply)
{
  QString dataStr;
  if (reply->error() == QNetworkReply::NoError) {
    dataStr = QString::fromUtf8(reply->readAll());
  } else {
    qCritical() << "Error requestUnreadCount: " << reply->errorString();
  }
}

//! Feed News request
void GoogleReader::requestFeed(const QString &urlFeed, int ot)
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

//! Process feed news reply
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

//! Mark news read or starred
void GoogleReader::editItem(const QString &urlFeed, const QString &itemId, const QString &action)
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
