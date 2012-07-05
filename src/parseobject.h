#ifndef PARSEOBJECT_H
#define PARSEOBJECT_H

#include <QtSql>

#include <QObject>

class ParseObject : public QObject
{
  Q_OBJECT
public:
  explicit ParseObject(QObject *parent = 0);

private:
  QString parseDate(QString dateString, QString urlString);

signals:
  void feedUpdated(const QUrl &url, const bool &changed);

public slots:
  void slotParse(QSqlDatabase *db,
                 const QByteArray &xmlData, const QUrl &url);

};

#endif // PARSEOBJECT_H
