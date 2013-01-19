#ifndef PARSEOBJECT_H
#define PARSEOBJECT_H

#include <QtSql>

#include <QObject>

class ParseObject : public QObject
{
  Q_OBJECT
public:
  explicit ParseObject(QString dataDirPath, QObject *parent = 0);

private:
  QString dataDirPath_;
  QString parseDate(QString dateString, QString urlString);
  int recountFeedCounts(int feedId);

signals:
  void feedUpdated(int feedId, const bool &changed, int newCount);

public slots:
  void slotParse(const QByteArray &xmlData, const QUrl &url);

};

#endif // PARSEOBJECT_H
