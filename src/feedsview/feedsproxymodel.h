#ifndef FEEDSPROXYMODEL_H
#define FEEDSPROXYMODEL_H

#include <QSortFilterProxyModel>

class FeedsProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  FeedsProxyModel(QObject *parent = 0);
  ~FeedsProxyModel();

  void reset();
  void setFilter(const QString &filterAct, const QList<int> &idList,
                 const QString &findAct, const QString &findText);
  QModelIndex mapFromSource(const QModelIndex & sourceIndex) const;
  QModelIndex mapFromSource(int id) const;
  QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
  QModelIndex index(int row, const QString &fieldName, const QModelIndex & parent = QModelIndex()) const;

private:
  bool filterAcceptsRow(int source_row, const QModelIndex &sourceParent) const;
  QString filterAct_;
  QList<int> idList_;
  QString findAct_;
  QString findText_;

};

#endif // FEEDSPROXYMODEL_H
