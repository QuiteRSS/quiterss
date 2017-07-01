/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "newsmodel.h"

#include "mainapplication.h"

NewsModel::NewsModel(QObject *parent, QTreeView *view)
  : QSqlTableModel(parent)
  , simplifiedDateTime_(true)
  , view_(view)
{
  setEditStrategy(QSqlTableModel::OnManualSubmit);
}

QVariant NewsModel::data(const QModelIndex &index, int role) const
{
  if (index.row() > (view_->verticalScrollBar()->value() + view_->verticalScrollBar()->pageStep()))
    return QSqlTableModel::data(index, role);

  MainWindow *mainWindow = mainApp->mainWindow();

  if (role == Qt::DecorationRole) {
    if (QSqlTableModel::fieldIndex("read") == index.column()) {
      QPixmap icon;
      if (1 == QSqlTableModel::index(index.row(), fieldIndex("new")).data(Qt::EditRole).toInt())
        icon.load(":/images/bulletNew");
      else if (0 == index.data(Qt::EditRole).toInt())
        icon.load(":/images/bulletUnread");
      else icon.load(":/images/bulletRead");
      return icon;
    } else if (QSqlTableModel::fieldIndex("starred") == index.column()) {
      QPixmap icon;
      if (0 == index.data(Qt::EditRole).toInt())
        icon.load(":/images/starOff");
      else icon.load(":/images/starOn");
      return icon;
    } else if (QSqlTableModel::fieldIndex("feedId") == index.column()) {
      QPixmap icon;
      int feedId = QSqlTableModel::index(index.row(), fieldIndex("feedId")).data(Qt::EditRole).toInt();
      QModelIndex feedIndex = mainWindow->feedsModel_->indexById(feedId);
      bool isFeed = (feedIndex.isValid() && mainWindow->feedsModel_->isFolder(feedIndex)) ? false : true;

      if (feedIndex.isValid()) {
        QByteArray byteArray = mainWindow->feedsModel_->dataField(feedIndex, "image").toByteArray();
        if (!byteArray.isNull()) {
          icon.loadFromData(QByteArray::fromBase64(byteArray));
        } else if (isFeed) {
          icon.load(":/images/feed");
        } else {
          icon.load(":/images/folder");
        }
      }

      return icon;
    } else if (QSqlTableModel::fieldIndex("label") == index.column()) {
      QIcon icon;
      QString strIdLabels = index.data(Qt::EditRole).toString();
      QList<QTreeWidgetItem *> labelListItems = mainApp->mainWindow()->
          categoriesTree_->getLabelListItems();
      foreach (QTreeWidgetItem *item, labelListItems) {
        if (strIdLabels.contains(QString(",%1,").arg(item->text(2)))) {
          icon = item->icon(0);
          break;
        }
      }
      return icon;
    }
  } else if (role == Qt::ToolTipRole) {
    if (QSqlTableModel::fieldIndex("feedId") == index.column()) {
      int feedId = QSqlTableModel::index(index.row(), fieldIndex("feedId")).data(Qt::EditRole).toInt();
      QModelIndex feedIndex = mainWindow->feedsModel_->indexById(feedId);
      return mainWindow->feedsModel_->dataField(feedIndex, "text").toString();
    } else if (QSqlTableModel::fieldIndex("title") == index.column()) {
      QString title = index.data(Qt::EditRole).toString();
      if ((view_->header()->sectionSize(index.column()) - 14) < view_->header()->fontMetrics().width(title))
        return title;
    }
    return QString("");
  } else if (role == Qt::DisplayRole) {
    if (QSqlTableModel::fieldIndex("read") == index.column()) {
      return QVariant();
    } else if (QSqlTableModel::fieldIndex("starred") == index.column()) {
      return QVariant();
    } else if (QSqlTableModel::fieldIndex("feedId") == index.column()) {
      return QVariant();
    } else if (QSqlTableModel::fieldIndex("rights") == index.column()) {
      int feedId = QSqlTableModel::index(index.row(), fieldIndex("feedId")).data(Qt::EditRole).toInt();
      QModelIndex feedIndex = mainWindow->feedsModel_->indexById(feedId);
      return mainWindow->feedsModel_->dataField(feedIndex, "text").toString();
    } else if (QSqlTableModel::fieldIndex("published") == index.column()) {
      QDateTime dtLocal;
      QString strDate = index.data(Qt::EditRole).toString();

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
      if (simplifiedDateTime_) {
        if (QDateTime::currentDateTime().date() <= dtLocal.date())
          return dtLocal.toString(formatTime_);
        else
          return dtLocal.toString(formatDate_);
      } else {
        return dtLocal.toString(formatDate_ + " " + formatTime_);
      }
    } else if (QSqlTableModel::fieldIndex("received") == index.column()) {
      QDateTime dateTime = QDateTime::fromString(
            index.data(Qt::EditRole).toString(),
            Qt::ISODate);
      if (simplifiedDateTime_) {
        if (QDateTime::currentDateTime().date() == dateTime.date()) {
          return dateTime.toString(formatTime_);
        } else return dateTime.toString(formatDate_);
      } else {
        return dateTime.toString(formatDate_ + " " + formatTime_);
      }
    } else if (QSqlTableModel::fieldIndex("label") == index.column()) {
      QStringList nameLabelList;
      QString strIdLabels = index.data(Qt::EditRole).toString();
      QList<QTreeWidgetItem *> labelListItems = mainApp->mainWindow()->
          categoriesTree_->getLabelListItems();
      foreach (QTreeWidgetItem *item, labelListItems) {
        if (strIdLabels.contains(QString(",%1,").arg(item->text(2)))) {
          nameLabelList << item->text(0);
        }
      }
      return nameLabelList.join(", ");
    } else if (QSqlTableModel::fieldIndex("link_href") == index.column()) {
      QString linkStr = index.data(Qt::EditRole).toString();
      if (linkStr.isEmpty()) {
        linkStr = QSqlTableModel::index(index.row(), fieldIndex("link_alternate")).
            data(Qt::EditRole).toString();
      }
      linkStr = linkStr.simplified();
      linkStr = linkStr.remove("http://");
      linkStr = linkStr.remove("https://");
      return linkStr;
    } else if (QSqlTableModel::fieldIndex("title") == index.column()) {
      if (index.data(Qt::EditRole).toString().isEmpty())
        return tr("(no title)");
    }
  } else if (role == Qt::FontRole) {
    QFont font = view_->font();
    if (0 == QSqlTableModel::index(index.row(), fieldIndex("read")).data(Qt::EditRole).toInt())
      font.setBold(true);
    return font;
  } else if (role == Qt::BackgroundRole) {
    if (index.row() == view_->currentIndex().row()) {
      if (!focusedNewsBGColor_.isEmpty())
        return QColor(focusedNewsBGColor_);
    }

    if (QSqlTableModel::index(index.row(), fieldIndex("label")).data(Qt::EditRole).isValid()) {
      QString strIdLabels = QSqlTableModel::index(index.row(), fieldIndex("label")).data(Qt::EditRole).toString();
      QList<QTreeWidgetItem *> labelListItems = mainApp->mainWindow()->
          categoriesTree_->getLabelListItems();
      foreach (QTreeWidgetItem *item, labelListItems) {
        if (strIdLabels.contains(QString(",%1,").arg(item->text(2)))) {
          QString strColor = item->data(0, CategoriesTreeWidget::colorBgRole).toString();
          if (!strColor.isEmpty())
            return QColor(strColor);
          break;
        }
      }
    }
  } else if (role == Qt::TextColorRole) {
    if (index.row() == view_->currentIndex().row()) {
      return QColor(focusedNewsTextColor_);
    }

    if (QSqlTableModel::index(index.row(), fieldIndex("label")).data(Qt::EditRole).isValid()) {
      QString strIdLabels = QSqlTableModel::index(index.row(), fieldIndex("label")).data(Qt::EditRole).toString();
      QList<QTreeWidgetItem *> labelListItems = mainApp->mainWindow()->
          categoriesTree_->getLabelListItems();
      foreach (QTreeWidgetItem *item, labelListItems) {
        if (strIdLabels.contains(QString(",%1,").arg(item->text(2)))) {
          QString strColor = item->data(0, CategoriesTreeWidget::colorTextRole).toString();
          if (!strColor.isEmpty())
            return QColor(strColor);
          break;
        }
      }
    }

    if (1 == QSqlTableModel::index(index.row(), fieldIndex("new")).data(Qt::EditRole).toInt())
      return QColor(newNewsTextColor_);

    if (0 == QSqlTableModel::index(index.row(), fieldIndex("read")).data(Qt::EditRole).toInt())
      return QColor(unreadNewsTextColor_);

    return QColor(textColor_);
  }
  return QSqlTableModel::data(index, role);
}

/*virtual*/ QVariant NewsModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           int role) const
{
  if (role == Qt::DisplayRole) {
    QString text = QSqlTableModel::headerData(section, orientation, role).toString();
    if (text.isEmpty()) return QVariant();

    int stopColFix = 0;
    for (int i = view_->header()->count()-1; i >= 0; i--) {
      int lIdx = view_->header()->logicalIndex(i);
      if (!view_->header()->isSectionHidden(lIdx)) {
        stopColFix = lIdx;
        break;
      }
    }
    int padding = 8;
    if (stopColFix == section) padding = padding + 20;
    text = view_->header()->fontMetrics().elidedText(
          text, Qt::ElideRight, view_->header()->sectionSize(section)-padding);
    return text;
  }
  return QSqlTableModel::headerData(section, orientation, role);
}

/*virtual*/ bool NewsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  return QSqlTableModel::setData(index, value, role);
}

/*virtual*/ void NewsModel::sort(int column, Qt::SortOrder order)
{
  int newsId = index(view_->currentIndex().row(), fieldIndex("id")).data().toInt();

  if ((column == fieldIndex("read")) || (column == fieldIndex("starred")) ||
      (column == fieldIndex("rights"))) {
    emit signalSort(column, order);
    column = fieldIndex("rights");
  }
  QSqlTableModel::sort(column, order);

  while (canFetchMore())
    fetchMore();

  if (newsId > 0) {
    QModelIndex startIndex = index(0, fieldIndex("id"));
    QModelIndexList indexList = match(startIndex, Qt::EditRole, newsId);
    if (indexList.count()) {
      int newsRow = indexList.first().row();
      view_->setCurrentIndex(index(newsRow, fieldIndex("title")));
    }
  }
}

/*virtual*/ QModelIndexList NewsModel::match(
    const QModelIndex &start, int role, const QVariant &value, int hits,
    Qt::MatchFlags flags) const
{
  return QSqlTableModel::match(start, role, value, hits, flags);
}

// ----------------------------------------------------------------------------
QVariant NewsModel::dataField(int row, const QString &fieldName) const
{
  return index(row, fieldIndex(fieldName)).data(Qt::EditRole);
}

void NewsModel::setFilter(const QString &filter)
{
  QPalette palette = view_->palette();
  palette.setColor(QPalette::AlternateBase, mainApp->mainWindow()->alternatingRowColors_);
  view_->setPalette(palette);

  QSqlTableModel::setFilter(filter);
}

bool NewsModel::select()
{
  QPalette palette = view_->palette();
  palette.setColor(QPalette::AlternateBase, mainApp->mainWindow()->alternatingRowColors_);
  view_->setPalette(palette);

  return QSqlTableModel::select();
}
