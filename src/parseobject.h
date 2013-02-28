#ifndef PARSEOBJECT_H
#define PARSEOBJECT_H

#include <QtSql>
#include <QDateTime>
#include <QQueue>
#include <QObject>
#include <QUrl>

struct FeedCountStruct{
  int feedId;
  int parentId;
  int unreadCount;
  int newCount;
  int undeleteCount;
  QString updated;
};

Q_DECLARE_METATYPE(FeedCountStruct)

class ParseObject : public QObject
{
  Q_OBJECT
public:
  explicit ParseObject(QString dataDirPath, QObject *parent = 0);

public slots:
  void parseXml(const QByteArray &data, const QString &feedUrl,
                const QDateTime &dtReply);

signals:
  void startTimer();
  void signalReadyParse(const QByteArray &xml, const QString &feedUrl,
                        const QDateTime &dtReply);
  void feedUpdated(int feedId, const bool &changed, int newCount);
  void feedCountsUpdate(FeedCountStruct counts);

private slots:
  void getQueuedXml();
  void slotParse(const QByteArray &xmlData, const QString &feedUrl,
                 const QDateTime &dtReply);

private:
  QString parseDate(QString dateString, QString urlString);
  int recountFeedCounts(int feedId);

  QString dataDirPath_;
  QString currentFeedUrl_;
  QByteArray currentXml_;
  QDateTime currentDtReady_;
  QQueue<QString> feedsQueue_;
  QQueue<QByteArray> xmlsQueue_;
  QQueue<QDateTime> dtReadyQueue_;

};

#endif // PARSEOBJECT_H
