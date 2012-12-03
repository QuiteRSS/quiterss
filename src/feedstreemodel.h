#ifndef FEEDSTREEMODEL_H
#define FEEDSTREEMODEL_H

#include <QSortFilterProxyModel>
#include <qyursqltreeview.h>

//class FeedsTreeModel : public QyurSqlTreeModel
class FeedsTreeModel : public QSortFilterProxyModel
{
  Q_OBJECT
  QyurSqlTreeModel *sourceModel_;

  //* Переменные фильтра лент.
  //* Отображаются те ленты, которые или у которых:
  QList<int> filterIdsList_;  //*< ...идентификатор имеется в этом списке
  bool filterUnread_;         //*< ...есть непрочтенные новосте
  bool filterNew_;            //*< ...есть новые новости
  QString filterLabel_;       //*< ...в поле label содержится значение этого поля
  QString filterFind_;        //*< ...удовлетворяют этому поисковому запросу

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

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
  bool setSourceData(int feedId, int feedParId, const QString &fieldName, const QVariant &value, int role = Qt::EditRole);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  Qt::DropActions supportedDropActions() const;
  bool isFolder(const QModelIndex &index) const;
  bool isSourceFolder(int feedId, int feedParId) const;
  QFont font_;
  QString formatDateTime_;

  int getIdByIndex(const QModelIndex &index) const;
  int getIdBySourceIndex(const QModelIndex &indexSource) const;
  int getParidByIndex(const QModelIndex &index) const;
  int getParidBySourceIndex(const QModelIndex &index) const;
  QModelIndex getIndexById(int id, int parentId) const;
  QModelIndex getSourceIndexById(int id, int parentId) const;
  int proxyColumnByOriginal(const QString& field) const;
//  void setFilter(const QString &filter);
  void setFilter(const QList<int> &feedIdsList = QList<int>(),
                 bool isUnread = false,
                 bool isNew = false,
                 const QString &labelString = QString(),
                 const QString &findString = QString());
public slots:
  void refresh();
};

#endif  // FEEDSTREEMODEL_H
