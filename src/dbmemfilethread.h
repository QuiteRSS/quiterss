#ifndef DBMEMFILETHREAD_H
#define DBMEMFILETHREAD_H

#include <QThread>
#include <QtSql>

class DBMemFileThread : public QThread
{
  Q_OBJECT
public:
  explicit DBMemFileThread( QObject *pParent = 0);
  ~DBMemFileThread();
  void sqliteDBMemFile(QSqlDatabase memdb, QString filename, bool save);

protected:
  virtual void run();

private:
  QSqlDatabase memdb_;
  QString filename_;
  bool save_;

};

#endif // DBMEMFILETHREAD_H
