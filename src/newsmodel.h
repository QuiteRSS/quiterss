#ifndef NEWSMODEL_H
#define NEWSMODEL_H

#include <QtSql>
#include <QtGui>

class NewsModel : public QSqlTableModel
{
  Q_OBJECT
private:
  QTreeView *view_;

public:
  NewsModel(QObject *parent, QTreeView *view);
  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  virtual void sort(int column, Qt::SortOrder order);
  virtual QModelIndexList match(
      const QModelIndex &start, int role, const QVariant &value, int hits = 1,
      Qt::MatchFlags flags =
      Qt::MatchFlags(Qt::MatchExactly)
      ) const;
  QString formatDateTime_;

signals:
  void signalSort(int column, int order);

};

#endif // NEWSMODEL_H
