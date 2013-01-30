#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include <QFile>
#include <QStringList>
#include <QNetworkCookieJar>

class CookieJar : public QNetworkCookieJar
{
  Q_OBJECT
public:
  explicit CookieJar(QString dataDirPath, QObject *parent);

  QList<QNetworkCookie> getAllCookies();
  void setAllCookies(const QList<QNetworkCookie> &cookieList);

  void saveCookies();
  void loadCookies();
  void clearCookies();

private:
  QString dataDirPath_;

};

#endif // COOKIEJAR_H
