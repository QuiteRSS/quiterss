#include <QApplication>
#include <QDateTime>
#include <QDebug>

#include "feedstreemodel.h"

//FeedsTreeModel::FeedsTreeModel(QObject *parent) : QyurSqlTreeModel(parent)
FeedsTreeModel::FeedsTreeModel(const QString& tableName,
    const QStringList& captions, const QStringList& fieldNames,
    int rootParentId, const QString& decoratedField, QObject* parent) :
    QyurSqlTreeModel(
        tableName, captions, fieldNames, rootParentId, decoratedField, parent)
{
}

QVariant FeedsTreeModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::FontRole) {
    QFont font = font_;
    if (QyurSqlTreeModel::proxyColumnByOriginal("text") == index.column()) {
      if ((0 < QyurSqlTreeModel::index(index.row(), proxyColumnByOriginal("unread")).data(Qt::EditRole).toInt()) &&
          (QyurSqlTreeModel::proxyColumnByOriginal("unread") != index.column()))
        font.setBold(true);
    }
    return font;
  } else if (role == Qt::DisplayRole){
    if (QyurSqlTreeModel::proxyColumnByOriginal("unread") == index.column()) {
      if (0 == QyurSqlTreeModel::index(index.row(), proxyColumnByOriginal("unread")).data(Qt::EditRole).toInt()) {
        return QVariant();
      } else {
        QString qStr = QString("(%1)").
            arg(QyurSqlTreeModel::index(index.row(), proxyColumnByOriginal("unread")).data(Qt::EditRole).toInt());
        return qStr;
      }
    } else if (QyurSqlTreeModel::proxyColumnByOriginal("undeleteCount") == index.column()) {
      QString qStr = QString("(%1)").
          arg(QyurSqlTreeModel::index(index.row(), proxyColumnByOriginal("undeleteCount")).data(Qt::EditRole).toInt());
      return qStr;
    } else if (QyurSqlTreeModel::proxyColumnByOriginal("updated") == index.column()) {
      QDateTime dtLocal;
      QString strDate = QyurSqlTreeModel::index(
            index.row(), proxyColumnByOriginal("updated")).data(Qt::EditRole).toString();

      if (!strDate.isNull()) {
        QDateTime dtLocalTime = QDateTime::currentDateTime();
        QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
        int nTimeShift = dtLocalTime.secsTo(dtUTC);

        QDateTime dt = QDateTime::fromString(strDate, Qt::ISODate);
        dtLocal = dt.addSecs(nTimeShift);

        QString strResult;
        if (QDateTime::currentDateTime().date() == dtLocal.date())
          strResult = "1" + dtLocal.toString("hh:mm");
        else
          strResult = "2" + dtLocal.toString(formatDateTime_.left(formatDateTime_.length()-6));
        qDebug() << index << strDate << strResult;
        return strResult;
      } else {
        return QVariant();
      }
    }
  } else if (role == Qt::TextColorRole) {
    QBrush brush;
    brush = qApp->palette().brush(QPalette::WindowText);
    if (QyurSqlTreeModel::proxyColumnByOriginal("unread") == index.column()) {
      brush = qApp->palette().brush(QPalette::Link);
    } else if (QyurSqlTreeModel::proxyColumnByOriginal("text") == index.column()) {
      if (QyurSqlTreeModel::index(index.row(), proxyColumnByOriginal("newCount")).data(Qt::EditRole).toInt() > 0) {
        brush = qApp->palette().brush(QPalette::Link);
      }
    }
    return brush;
  } else if (role == Qt::DecorationRole) {
    if (QyurSqlTreeModel::proxyColumnByOriginal("text") == index.column()) {
      QByteArray byteArray = QyurSqlTreeModel::index(index.row(), proxyColumnByOriginal("image")).
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
    if (QyurSqlTreeModel::proxyColumnByOriginal("id") == index.column()) {
      int flag = Qt::AlignRight|Qt::AlignVCenter;
      return flag;
    }
  }

  return QyurSqlTreeModel::data(index, role);
}

/*virtual*/ bool	FeedsTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  return QyurSqlTreeModel::setData(index, value, role);
}
