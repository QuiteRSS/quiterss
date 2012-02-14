#ifndef NEWSMODEL_H
#define NEWSMODEL_H

#include <QSqlTableModel>
#include <QtGui>

class NewsModel : public QSqlTableModel
{
  Q_OBJECT
private:
  QTreeView *view_;

public:
  NewsModel(QObject *parent, QTreeView *view = 0);
  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

};

#endif // NEWSMODEL_H
