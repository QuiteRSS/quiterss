/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#ifndef FEEDSTREEMODEL_H
#define FEEDSTREEMODEL_H

#include <QSortFilterProxyModel>
#include <QSqlTableModel>
#include <qyursqltreeview.h>

class FeedsProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  FeedsProxyModel(QObject *parent = 0);
  ~FeedsProxyModel();
  void setFilter(const QString &filter);
  QString filter_;

private:
  bool filterAcceptsRow(int source_row, const QModelIndex &sourceParent) const;

};


class FeedsTreeModel : public QyurSqlTreeModel
{
  Q_OBJECT
public:
  FeedsTreeModel(const QString &tableName,
      const QStringList &captions,
      const QStringList &fieldNames,
      int rootParentId = 0,
      QObject* parent = 0);
  void setView(QyurSqlTreeView *view);
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant dataField(const QModelIndex &index, const QString &fieldName) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  Qt::DropActions supportedDropActions() const;
  bool isFolder(const QModelIndex &index) const;
  QModelIndex indexSibling(const QModelIndex &index, const QString &fieldName) const;
  QFont font_;
  QString formatDate_;
  QString formatTime_;
  bool defaultIconFeeds_;
  QString textColor_;
  QString backgroundColor_;
  QString feedWithNewNewsColor_;
  QString countNewsUnreadColor_;

private:
  QyurSqlTreeView *view_;

};

#endif  // FEEDSTREEMODEL_H
