#ifndef DBMEMFILETHREAD_H
#define DBMEMFILETHREAD_H

#include <QThread>
#include <QtSql>

class DBMemFileThread : public QThread
{
  Q_OBJECT
public:
  explicit DBMemFileThread(QString filename, QObject *pParent);
  ~DBMemFileThread();
  void sqliteDBMemFile(bool save, QThread::Priority priority = QThread::NormalPriority);

protected:
  virtual void run();

private:
  QSqlDatabase memdb_;
  QString filename_;
  bool save_;

};

#endif // DBMEMFILETHREAD_H
