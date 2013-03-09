#ifndef FEEDSTREEMODEL_H
#define FEEDSTREEMODEL_H

#include <qyursqltreeview.h>

class FeedsTreeModel : public QyurSqlTreeModel
{
  Q_OBJECT
public:
  FeedsTreeModel(const QString &tableName,
      const QStringList &captions,
      const QStringList &fieldNames,
      int rootParentId = 0,
      const QString &decoratedField = QString(),
      QObject* parent = 0);
  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant dataField(const QModelIndex &index, const QString &fieldName) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  Qt::DropActions supportedDropActions() const;
  bool isFolder(const QModelIndex &index) const;
  QFont font_;
  QString formatDate_;
  QString formatTime_;
  bool defaultIconFeeds_;
};

#endif  // FEEDSTREEMODEL_H
