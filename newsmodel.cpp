#include <QDebug>
#include <QFont>

#include "newsmodel.h"

NewsModel::NewsModel(QObject *parent)
  :QSqlTableModel(parent)
{
}

QVariant NewsModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::FontRole) {
    QFont font;
    if (0 == QSqlTableModel::index(index.row(), fieldIndex("read")).data().toInt())
      font.setBold(true);
    return font;
  }

  return QSqlTableModel::data(index, role);
}
