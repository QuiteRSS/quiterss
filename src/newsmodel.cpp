#include "newsmodel.h"

NewsModel::NewsModel(QObject *parent, QTreeView *view)
  : QSqlTableModel(parent),
    view_(view)
{
  setEditStrategy(QSqlTableModel::OnFieldChange);
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
    } else if (QSqlTableModel::fieldIndex("starred") == index.column()) {
      QIcon icon;
      if (0 == QSqlTableModel::index(index.row(), fieldIndex("starred")).data(Qt::EditRole).toInt())
        icon.addFile(":/images/starOff");
      else icon.addFile(":/images/starOn");
      return icon;
    } else if (QSqlTableModel::fieldIndex("title") == index.column()) {
      QIcon icon;
      if (1 == QSqlTableModel::index(index.row(), fieldIndex("new")).data(Qt::EditRole).toInt())
        icon.addFile(":/images/bulletNew");
      else icon.addFile(":/images/bulletNoNew");
      return icon;
    }
  } else if (role == Qt::DisplayRole) {
    if (QSqlTableModel::fieldIndex("read") == index.column()) {
      return QVariant();
    }
    else if (QSqlTableModel::fieldIndex("starred") == index.column()) {
      return QVariant();
    }
    else if (QSqlTableModel::fieldIndex("published") == index.column()) {
      QDateTime dtLocal;
      QString strDate = QSqlTableModel::index(
            index.row(), fieldIndex("published")).data(Qt::EditRole).toString();

      if (!strDate.isNull()) {
        QDateTime dtLocalTime = QDateTime::currentDateTime();
        QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
        int nTimeShift = dtLocalTime.secsTo(dtUTC);

        QDateTime dt = QDateTime::fromString(strDate, Qt::ISODate);
        dtLocal = dt.addSecs(nTimeShift);
      } else {
        dtLocal = QDateTime::fromString(
              QSqlTableModel::index(index.row(), fieldIndex("received")).data(Qt::EditRole).toString(),
              Qt::ISODate);
      }
      if (QDateTime::currentDateTime().date() == dtLocal.date())
        return dtLocal.toString("hh:mm");
      else
        return dtLocal.toString(formatDateTime_);
    }
    else if (QSqlTableModel::fieldIndex("received") == index.column()) {
      QDateTime dateTime = QDateTime::fromString(
            QSqlTableModel::index(index.row(), fieldIndex("received")).data(Qt::EditRole).toString(),
            Qt::ISODate);
      if (QDateTime::currentDateTime().date() == dateTime.date()) {
        return dateTime.toString("hh:mm");
      } else return dateTime.toString(formatDateTime_);
    }
  } else if (role == Qt::FontRole) {
    QFont font = view_->font();
    if (0 == QSqlTableModel::index(index.row(), fieldIndex("read")).data(Qt::EditRole).toInt())
      font.setBold(true);
    return font;
  } else if (role == Qt::BackgroundRole) {
    if ((index.column() == view_->header()->sortIndicatorSection()) &&
        (!view_->selectionModel()->isSelected(index))) {
      return qApp->palette().brush(QPalette::AlternateBase);
    }
  } else if (role == Qt::TextColorRole) {
    return qApp->palette().brush(QPalette::WindowText);
  }
  return QSqlTableModel::data(index, role);
}

/*virtual*/ bool	NewsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  return QSqlTableModel::setData(index, value, role);
}
