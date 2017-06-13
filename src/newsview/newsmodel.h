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
#ifndef NEWSMODEL_H
#define NEWSMODEL_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QtSql>

class NewsModel : public QSqlTableModel
{
  Q_OBJECT
public:
  NewsModel(QObject *parent, QTreeView *view);
  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  virtual void sort(int column, Qt::SortOrder order);
  virtual QModelIndexList match(
      const QModelIndex &start, int role, const QVariant &value, int hits = 1,
      Qt::MatchFlags flags =
      Qt::MatchFlags(Qt::MatchExactly|Qt::MatchWrap)
      ) const;
  QVariant dataField(int row, const QString &fieldName) const;
  void setFilter(const QString &filter);
  bool select();

  QString formatDate_;
  QString formatTime_;
  bool simplifiedDateTime_;
  QString textColor_;
  QString newNewsTextColor_;
  QString unreadNewsTextColor_;
  QString focusedNewsTextColor_;
  QString focusedNewsBGColor_;

signals:
  void signalSort(int column, int order);

private:
  QTreeView *view_;

};

#endif // NEWSMODEL_H
