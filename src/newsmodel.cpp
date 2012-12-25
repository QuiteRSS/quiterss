#include "newsmodel.h"

NewsModel::NewsModel(QObject *parent, QTreeView *view, QSqlDatabase *db)
  : QSqlTableModel(parent),
    view_(view),
    db_(db)
{
  setEditStrategy(QSqlTableModel::OnManualSubmit);
}

QVariant NewsModel::data(const QModelIndex &index, int role) const
{
  if (index.row() > (view_->verticalScrollBar()->value() + view_->verticalScrollBar()->pageStep()))
    return QSqlTableModel::data(index, role);

  if (role == Qt::DecorationRole) {
    if (QSqlTableModel::fieldIndex("read") == index.column()) {
      QPixmap icon;
      if (1 == QSqlTableModel::index(index.row(), fieldIndex("new")).data(Qt::EditRole).toInt())
        icon.load(":/images/bulletNew");
      else if (0 == QSqlTableModel::index(index.row(), fieldIndex("read")).data(Qt::EditRole).toInt())
        icon.load(":/images/bulletUnread");
      else icon.load(":/images/bulletRead");
      return icon;
    } else if (QSqlTableModel::fieldIndex("starred") == index.column()) {
      QPixmap icon;
      if (0 == QSqlTableModel::index(index.row(), fieldIndex("starred")).data(Qt::EditRole).toInt())
        icon.load(":/images/starOff");
      else icon.load(":/images/starOn");
      return icon;
    } else if (QSqlTableModel::fieldIndex("feedId") == index.column()) {
      QPixmap icon;
      QByteArray byteArray;
      bool isFeed = true;

      QSqlQuery q(*db_);
      q.exec(QString("SELECT image, xmlUrl FROM feeds WHERE id=='%1'").
             arg(QSqlTableModel::index(index.row(), fieldIndex("feedId")).data(Qt::EditRole).toInt()));
      if (q.next()) {
        byteArray = q.value(0).toByteArray();
        if (q.value(1).toString().isEmpty())
          isFeed = false;
      }
      if (!byteArray.isNull()) {
        icon.loadFromData(QByteArray::fromBase64(byteArray));
      } else if (isFeed) {
        icon.load(":/images/feed");
      } else {
        icon.load(":/images/folder");
      }

      return icon;
    } else if (QSqlTableModel::fieldIndex("label") == index.column()) {
      QPixmap icon;
      QByteArray byteArray;
      int num = -1;
      QStringList strLabelIdList = index.data(Qt::EditRole).toString().
          split(",", QString::SkipEmptyParts);
      foreach (QString strLabelId, strLabelIdList) {
        QSqlQuery q(*db_);
        q.exec(QString("SELECT num, image FROM labels WHERE id=='%1'").
               arg(strLabelId));
        if (q.next()) {
          if ((q.value(0).toInt() < num) || (num == -1)) {
            num = q.value(0).toInt();
            byteArray = q.value(1).toByteArray();
          }
        }
      }

      if (!byteArray.isNull())
        icon.loadFromData(byteArray);
      return icon;
    }
  } else if (role == Qt::ToolTipRole) {
    if (QSqlTableModel::fieldIndex("feedId") == index.column()) {
      QSqlQuery q(*db_);
      q.exec(QString("SELECT text FROM feeds WHERE id=='%1'").
             arg(QSqlTableModel::index(index.row(), fieldIndex("feedId")).data(Qt::EditRole).toInt()));
      if (q.next())
        return q.value(0).toString();
    }
    return QString("");
  } else if (role == Qt::DisplayRole) {
    if (QSqlTableModel::fieldIndex("read") == index.column()) {
      return QVariant();
    }
    else if (QSqlTableModel::fieldIndex("starred") == index.column()) {
      return QVariant();
    } else if (QSqlTableModel::fieldIndex("feedId") == index.column()) {
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
        return dtLocal.toString(formatDateTime_.left(formatDateTime_.length()-6));
    }
    else if (QSqlTableModel::fieldIndex("received") == index.column()) {
      QDateTime dateTime = QDateTime::fromString(
            QSqlTableModel::index(index.row(), fieldIndex("received")).data(Qt::EditRole).toString(),
            Qt::ISODate);
      if (QDateTime::currentDateTime().date() == dateTime.date()) {
        return dateTime.toString("hh:mm");
      } else return dateTime.toString(formatDateTime_.left(formatDateTime_.length()-6));
    } else if (QSqlTableModel::fieldIndex("label") == index.column()) {
      QStringList nameLabelList;
      QString strIdLabels = index.data(Qt::EditRole).toString();
      QSqlQuery q(*db_);
      q.exec("SELECT id, name FROM labels ORDER BY num");
      while (q.next()) {
        QString strLabelId = q.value(0).toString();
        if (strIdLabels.contains(QString(",%1,").arg(strLabelId))) {
          nameLabelList << q.value(1).toString();
        }
      }
      return nameLabelList.join(", ");
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
    if (QSqlTableModel::index(index.row(), fieldIndex("label")).data(Qt::EditRole).isValid()) {
      QString strColor;
      int num = -1;
      QStringList strLabelIdList =
          QSqlTableModel::index(index.row(), fieldIndex("label")).data(Qt::EditRole).toString().
          split(",", QString::SkipEmptyParts);
      foreach (QString strLabelId, strLabelIdList) {
        QSqlQuery q(*db_);
        q.exec(QString("SELECT num, color_text FROM labels WHERE id=='%1'").
               arg(strLabelId));
        if (q.next()) {
          if ((q.value(0).toInt() < num) || (num == -1)) {
            num = q.value(0).toInt();
            strColor = q.value(1).toString();
          }
        }
      }

      if (!strColor.isEmpty())
        return QColor(QString("#%1").arg(strColor));
    }

    return qApp->palette().brush(QPalette::WindowText);
  }
  return QSqlTableModel::data(index, role);
}

/*virtual*/ bool	NewsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  return QSqlTableModel::setData(index, value, role);
}

/*virtual*/ void NewsModel::sort(int column, Qt::SortOrder order)
{
  if ((column == fieldIndex("read")) || (column == fieldIndex("starred")))
    emit signalSort(column, order);
  else
    QSqlTableModel::sort(column, order);
}
