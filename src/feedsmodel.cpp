
#include "feedsmodel.h"

FeedsModel::FeedsModel(QObject *parent)
  :QSqlTableModel(parent)
{
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::FontRole) {
    QFont font = font_;
    if (QSqlTableModel::fieldIndex("text") == index.column()) {
      if ((0 < QSqlTableModel::index(index.row(), fieldIndex("unread")).data(Qt::EditRole).toInt()) &&
          (QSqlTableModel::fieldIndex("unread") != index.column()))
        font.setBold(true);
    }
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
    } else if (QSqlTableModel::fieldIndex("undeleteCount") == index.column()) {
      QString qStr = QString("(%1)").
          arg(QSqlTableModel::index(index.row(), fieldIndex("undeleteCount")).data(Qt::EditRole).toInt());
      return qStr;
    } else if (QSqlTableModel::fieldIndex("updated") == index.column()) {
      QDateTime dtLocal;
      QString strDate = QSqlTableModel::index(
            index.row(), fieldIndex("updated")).data(Qt::EditRole).toString();

      if (!strDate.isNull()) {
        QDateTime dtLocalTime = QDateTime::currentDateTime();
        QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
        int nTimeShift = dtLocalTime.secsTo(dtUTC);

        QDateTime dt = QDateTime::fromString(strDate, Qt::ISODate);
        dtLocal = dt.addSecs(nTimeShift);

        if (QDateTime::currentDateTime().date() == dtLocal.date())
          return dtLocal.toString("hh:mm");
        else
          return dtLocal.toString(formatDateTime_.left(formatDateTime_.length()-6));
      } else {
        return QVariant();
      }
    }
  } else if (role == Qt::TextColorRole) {
    QBrush brush;
    brush = qApp->palette().brush(QPalette::WindowText);
    if (QSqlTableModel::fieldIndex("unread") == index.column()) {
      brush = qApp->palette().brush(QPalette::Link);
    } else if (QSqlTableModel::fieldIndex("text") == index.column()) {
      if (QSqlTableModel::index(index.row(), fieldIndex("newCount")).data(Qt::EditRole).toInt() > 0) {
        brush = qApp->palette().brush(QPalette::Link);
      }
    }
    return brush;
  } else if (role == Qt::DecorationRole) {
    if (QSqlTableModel::fieldIndex("text") == index.column()) {
      QByteArray byteArray = QSqlTableModel::index(index.row(), fieldIndex("image")).
          data(Qt::EditRole).toByteArray();
      if (!byteArray.isNull()) {
        QPixmap icon;
        if (icon.loadFromData(QByteArray::fromBase64(byteArray))) {
          return icon;
        }
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
