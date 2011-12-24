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
    } else if (QSqlTableModel::fieldIndex("sticky") == index.column()) {
      QIcon icon;
      if (0 == QSqlTableModel::index(index.row(), fieldIndex("sticky")).data(Qt::EditRole).toInt())
        icon.addFile(":/images/starOff");
      else icon.addFile(":/images/starOn");
      return icon;
    }
  } else if (role == Qt::DisplayRole) {
    if (QSqlTableModel::fieldIndex("read") == index.column()) {
      return QVariant();
    }
    else if (QSqlTableModel::fieldIndex("sticky") == index.column()) {
      return QVariant();
    }
    else if (QSqlTableModel::fieldIndex("published") == index.column()) {
      QString strPublished;
      strPublished = QSqlTableModel::index(index.row(), fieldIndex("published")).data(Qt::EditRole).toString();
      QLocale locale(QLocale::C);
      QDateTime dt = locale.toDateTime(strPublished, "yyyy-MM-ddTHH:mm:ss");
      int daysToCurrentDT = dt.daysTo(QDateTime::currentDateTimeUtc());
      if (daysToCurrentDT < 0) {
        qWarning() << "News has future published Date";
        return QString();
      }
      else if (daysToCurrentDT < 1) return dt.toString("hh:mm");
      else return dt.toString("yyyy.MM.dd hh:mm");
    }
    else if (QSqlTableModel::fieldIndex("received") == index.column()) {
      QDateTime dateTime = QDateTime::fromString(
            QSqlTableModel::index(index.row(), fieldIndex("received")).data(Qt::EditRole).toString(),
            Qt::ISODate);
      if (QDateTime::currentDateTime().date() == dateTime.date()) {
        return dateTime.toString("hh:mm");
      } else return dateTime.toString("yyyy.MM.dd hh:mm");
    }
  } else if (role == Qt::FontRole) {
    QFont font;
    if (0 == QSqlTableModel::index(index.row(), fieldIndex("read")).data(Qt::EditRole).toInt())
      font.setBold(true);
    return font;
  } else if (role == Qt::BackgroundRole) {
    if (index.column() == view_->header()->sortIndicatorSection()) {
      return QBrush(QColor("#F5F5F5"));
    }
  }
  return QSqlTableModel::data(index, role);
}

/*virtual*/ bool	NewsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  return QSqlTableModel::setData(index, value, role);
}
