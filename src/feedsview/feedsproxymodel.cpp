#include "feedsproxymodel.h"
#include "feedsmodel.h"

FeedsProxyModel::FeedsProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent)
  , filterAct_("filterFeedsAll_")
{
  setObjectName("FeedsProxyModel");
}

FeedsProxyModel::~FeedsProxyModel()
{

}

void FeedsProxyModel::reset()
{
#ifdef HAVE_QT5
  QSortFilterProxyModel::beginResetModel();
  QSortFilterProxyModel::endResetModel();
#else
  QSortFilterProxyModel::reset();
#endif
}

void FeedsProxyModel::setFilter(const QString &filterAct, const QList<int> &idList,
                                const QString &findAct, const QString &findText)
{
  if ((filterAct_ != filterAct) || (filterAct != "filterFeedsAll_") ||
      (findAct_ != findAct) || (findText_ != findText) || (idList_ != idList)) {
    filterAct_ = filterAct;
    findAct_ = findAct;
    findText_ = findText;
    idList_ = idList;

    invalidateFilter();
  }
}

bool FeedsProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
  bool accept = false;
  QModelIndex index;

  if (filterAct_ == "filterFeedsAll_") {
    accept = true;
  } else if (filterAct_ == "filterFeedsNew_") {
    index = sourceModel()->index(sourceRow, ((FeedsModel*)sourceModel())->indexColumnOf("newCount"), sourceParent);
    if (sourceModel()->data(index, Qt::EditRole).toInt() > 0)
      accept = true;
  } else if (filterAct_ == "filterFeedsUnread_") {
    index = sourceModel()->index(sourceRow, ((FeedsModel*)sourceModel())->indexColumnOf("unread"), sourceParent);
    if (sourceModel()->data(index, Qt::EditRole).toInt() > 0)
      accept = true;
  }

  if (idList_.count()) {
    index = sourceModel()->index(sourceRow, ((FeedsModel*)sourceModel())->indexColumnOf("id"), sourceParent);
    if (idList_.contains(sourceModel()->data(index, Qt::EditRole).toInt()))
      accept = true;
  }

  if (accept && !findText_.isEmpty()) {
    index = sourceModel()->index(sourceRow, ((FeedsModel*)sourceModel())->indexColumnOf("xmlUrl"), sourceParent);
    if (!sourceModel()->data(index, Qt::EditRole).toString().isEmpty()) {
      if (findAct_ == "findLinkAct") {
        accept = sourceModel()->data(index, Qt::EditRole).toString().contains(findText_, Qt::CaseInsensitive);
      } else {
        index = sourceModel()->index(sourceRow, ((FeedsModel*)sourceModel())->indexColumnOf("text"), sourceParent);
        accept = sourceModel()->data(index, Qt::EditRole).toString().contains(findText_, Qt::CaseInsensitive);
      }
    }
  }

  return accept;
}

QModelIndex FeedsProxyModel::mapFromSource(const QModelIndex & sourceIndex) const
{
  return QSortFilterProxyModel::mapFromSource(sourceIndex);
}

QModelIndex FeedsProxyModel::mapFromSource(int id) const
{
  return QSortFilterProxyModel::mapFromSource(((FeedsModel*)sourceModel())->indexById(id));
}

QModelIndex FeedsProxyModel::index(int row, int column, const QModelIndex & parent) const
{
  return QSortFilterProxyModel::index(row, column, parent);
}

QModelIndex FeedsProxyModel::index(int row, const QString& fieldName, const QModelIndex & parent) const
{
  int column = ((FeedsModel*)sourceModel())->indexColumnOf(fieldName);
  return QSortFilterProxyModel::index(row, column, parent);
}
