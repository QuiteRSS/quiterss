#include <QFont>
#include <QBrush>
#include <QIcon>

#include "feedsmodel.h"

FeedsModel::FeedsModel(QObject *parent)
  :QSqlTableModel(parent)
{
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::FontRole) {
    QFont font = font_;
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
    } else if (QSqlTableModel::fieldIndex("text") == index.column()) {
      if (QSqlTableModel::index(index.row(), fieldIndex("newCount")).data(Qt::EditRole).toInt() > 0) {
        brush.setColor("#0000FF");
      }
    }
    return brush;
  } else if (role == Qt::DecorationRole) {
    if (QSqlTableModel::fieldIndex("text") == index.column()) {
      QByteArray byteArray = QSqlTableModel::index(index.row(), fieldIndex("image")).
          data(Qt::EditRole).toByteArray();
      if (!byteArray.isNull()) {
        QPixmap icon;
        icon.loadFromData(QByteArray::fromBase64(byteArray));
        return icon;
      }
      return QPixmap(":/images/feed");
    }
  } else if (role == Qt::TextAlignmentRole) {
    if (QSqlTableModel::fieldIndex("id") == index.column()) {
      int flag = Qt::AlignRight|Qt::AlignVCenter;
      return flag;
    }
  }

  return QSqlTableModel::data(index, role);
}

/*virtual*/ bool	FeedsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  return QSqlTableModel::setData(index, value, role);
}
