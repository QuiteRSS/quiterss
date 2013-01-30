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

/** @brief Считывание из файла Cookies
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

/** @brief Сохранение в файл Cookies
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

/** @brief Очистка всех Cookies
 *----------------------------------------------------------------------------*/
void CookieJar::clearCookies()
{
  setAllCookies(QList<QNetworkCookie>());
}

/** @brief Получения всех Cookies
 *----------------------------------------------------------------------------*/
QList<QNetworkCookie> CookieJar::getAllCookies()
{
  return QNetworkCookieJar::allCookies();
}

/** @brief Установка Cookies
 *----------------------------------------------------------------------------*/
void CookieJar::setAllCookies(const QList<QNetworkCookie> &cookieList)
{
  QNetworkCookieJar::setAllCookies(cookieList);
}
