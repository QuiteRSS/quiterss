#include "cookiejar.h"
#include <QNetworkCookie>
#include <QDateTime>
#include <QDir>

CookieJar::CookieJar(QString dataDirPath, QObject *parent)
  : QNetworkCookieJar(parent),
    dataDirPath_(dataDirPath)
{
  loadCookies();
}

/** @brief Load cookies from file
 *----------------------------------------------------------------------------*/
void CookieJar::loadCookies()
{
  if (!QFile::exists(dataDirPath_ + QDir::separator() + "cookies.dat")) {
    return;
  }

  QDateTime now = QDateTime::currentDateTime();

  QList<QNetworkCookie> loadedCookies;
  QFile file(dataDirPath_ + QDir::separator() + "cookies.dat");
  file.open(QIODevice::ReadOnly);
  QDataStream stream(&file);
  int count;

  stream >> count;
  for (int i = 0; i < count; i++) {
    QByteArray rawForm;
    stream >> rawForm;
    const QList<QNetworkCookie> &cookieList = QNetworkCookie::parseCookies(rawForm);
    if (cookieList.isEmpty()) {
      continue;
    }

    const QNetworkCookie &cookie = cookieList.at(0);
    if (cookie.expirationDate() < now) {
      continue;
    }
    loadedCookies.append(cookie);
  }

  file.close();
  setAllCookies(loadedCookies);
}

/** @brief Save cookies to file
 *----------------------------------------------------------------------------*/
void CookieJar::saveCookies()
{
  QList<QNetworkCookie> allCookies = getAllCookies();

  QFile file(dataDirPath_ + QDir::separator() + "cookies.dat");
  file.open(QIODevice::WriteOnly);
  QDataStream stream(&file);
  int count = allCookies.count();

  stream << count;
  for (int i = 0; i < count; i++) {
    const QNetworkCookie &cookie = allCookies.at(i);

    if (cookie.isSessionCookie()) {
      continue;
    }
    stream << cookie.toRawForm();
  }

  file.close();
}

/** @brief Clear all cookies
 *----------------------------------------------------------------------------*/
void CookieJar::clearCookies()
{
  setAllCookies(QList<QNetworkCookie>());
}

/** @brief Retrive all cookies
 *----------------------------------------------------------------------------*/
QList<QNetworkCookie> CookieJar::getAllCookies()
{
  return QNetworkCookieJar::allCookies();
}

/** @brief Setup cookies from list
 *----------------------------------------------------------------------------*/
void CookieJar::setAllCookies(const QList<QNetworkCookie> &cookieList)
{
  QNetworkCookieJar::setAllCookies(cookieList);
}
