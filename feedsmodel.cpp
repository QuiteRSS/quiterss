#include <QDebug>
#include <QFont>

#include "feedsmodel.h"

FeedsModel::FeedsModel(QObject *parent)
  :QSqlTableModel(parent)
{
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::FontRole) {
    QFont font;
    if (0 < QSqlTableModel::index(index.row(), fieldIndex("unread")).data().toInt())
      font.setBold(true);
    return font;
  }

  return QSqlTableModel::data(index, role);
}
