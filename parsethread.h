#ifndef PARSETHREAD_H
#define PARSETHREAD_H

#include <QtSql>

#include <QQueue>
#include <QThread>
#include <QUrl>
#include <QXmlStreamReader>

class ParseThread : public QThread
{
  Q_OBJECT

private:
  QSqlDatabase *db_;
  QUrl currentUrl_;
  QByteArray currentXml_;
  QQueue<QUrl> urlsQueue_;
  QQueue<QByteArray> xmlsQueue_;

  QString currentTag;
  QString rssItemString;
  QString titleString;
  QString linkString;
  QString rssDescriptionString;
  QString commentsString;
  QString rssPubDateString;
  QString rssGuidString;
  QString atomEntryString;
  QString atomIdString;
  QString atomUpdatedString;
  QString atomSummaryString;
  QString atomContentString;

  void parse();
  void getQueuedXml();

public:
  explicit ParseThread(QObject *parent, QSqlDatabase *db);
  ~ParseThread();
  void parseXml(const QByteArray &data, const QUrl &url);

signals:
  void feedUpdated(const QUrl &url);

public slots:

};

#endif // PARSETHREAD_H
