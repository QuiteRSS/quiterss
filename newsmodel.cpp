#include <QDebug>
#include <QFont>
#include <QIcon>

#include "newsmodel.h"

NewsModel::NewsModel(QObject *parent)
  :QSqlTableModel(parent)
{
}

QVariant NewsModel::data(const QModelIndex &index, int role) const
{
  if (QSqlTableModel::fieldIndex("read") == index.column()) {
    if (role == Qt::DecorationRole) {
      QIcon icon;
      if (0 == QSqlTableModel::index(index.row(), fieldIndex("read")).data(Qt::EditRole).toInt())
        icon.addFile(":/images/bulletUnread");
      else icon.addFile(":/images/bulletRead");
      return icon;
    } else if (role == Qt::DisplayRole) {
      return QVariant();
    }
  } else {
    if (role == Qt::FontRole) {
      QFont font;
      if (0 == QSqlTableModel::index(index.row(), fieldIndex("read")).data(Qt::EditRole).toInt())
        font.setBold(true);
      return font;
    }
  }

  return QSqlTableModel::data(index, role);
}

/*virtual*/ bool	NewsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  return QSqlTableModel::setData(index, value, role);
}
