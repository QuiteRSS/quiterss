#include <QDebug>
#include <QFont>
#include <QIcon>
#include <QDateTime>

#include "newsmodel.h"

NewsModel::NewsModel(QObject *parent)
  :QSqlTableModel(parent)
{
}

QVariant NewsModel::data(const QModelIndex &index, int role) const
{ 
  if (role == Qt::DecorationRole) {
    if (QSqlTableModel::fieldIndex("read") == index.column()) {
      QIcon icon;
      if (0 == QSqlTableModel::index(index.row(), fieldIndex("read")).data(Qt::EditRole).toInt())
        icon.addFile(":/images/bulletUnread");
      else icon.addFile(":/images/bulletRead");
      return icon;
    }
  } else if (role == Qt::DisplayRole) {
    if (QSqlTableModel::fieldIndex("read") == index.column()) {
      return QVariant();
    } else if (QSqlTableModel::fieldIndex("received") == index.column()) {
      QDateTime dateTime = QDateTime::fromString(
            QSqlTableModel::index(index.row(), fieldIndex("received")).data(Qt::EditRole).toString(),
            dataFormat);
      qDebug() << QDateTime::currentDateTime().toString("yyyy.MM.dd")
               << dateTime.toString("yyyy.MM.dd")
               << QSqlTableModel::index(index.row(), fieldIndex("received")).data(Qt::EditRole).toString();
      if (QDateTime::currentDateTime().toString("yyyy.MM.dd") == dateTime.toString("yyyy.MM.dd")) {
        return dateTime.toString("hh:mm");
      } else return dateTime.toString("yyyy.MM.dd hh:mm");
    }
  } else if (role == Qt::FontRole) {
    QFont font;
    if (0 == QSqlTableModel::index(index.row(), fieldIndex("read")).data(Qt::EditRole).toInt())
      font.setBold(true);
    return font;
  }


  return QSqlTableModel::data(index, role);
}

/*virtual*/ bool	NewsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  return QSqlTableModel::setData(index, value, role);
}
