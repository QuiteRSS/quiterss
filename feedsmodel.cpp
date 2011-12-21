#include <QDebug>
#include <QFont>
#include <QBrush>

#include "feedsmodel.h"

FeedsModel::FeedsModel(QObject *parent)
  :QSqlTableModel(parent)
{
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::FontRole) {
    QFont font;
    if ((0 < QSqlTableModel::index(index.row(), fieldIndex("unread")).data(Qt::EditRole).toInt()) &&
        (QSqlTableModel::fieldIndex("unread") != index.column()))
      font.setBold(true);
    return font;
  } else if (role == Qt::DisplayRole){
    if (QSqlTableModel::fieldIndex("unread") == index.column()) {
      if (0 == QSqlTableModel::index(index.row(), fieldIndex("unread")).data(Qt::EditRole).toInt()) {
        return QVariant();
      } else {
        QString qStr = QString("(%1)").
            arg(QSqlTableModel::index(index.row(), fieldIndex("unread")).data(Qt::EditRole).toInt());
        return qStr;
      }
    }
  } else if (role == Qt::TextColorRole) {
    QBrush brush;
    if (QSqlTableModel::fieldIndex("unread") == index.column()) {
      brush.setColor("#0000CA");
    }
    return brush;
  }

  return QSqlTableModel::data(index, role);
}

/*virtual*/ bool	FeedsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  return QSqlTableModel::setData(index, value, role);
}
