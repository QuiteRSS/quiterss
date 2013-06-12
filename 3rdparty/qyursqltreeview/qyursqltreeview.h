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
/*******************************************************
**
** Definition of QyurSqlTreeView
**
** Copyright (C) 2009 Yuri Yurachkovsky.
**
** QyurSqlTreeView is free software; you can redistribute
** it and/or modify it under the terms of the GNU
** Library General Public License as published by the
** Free Software Foundation; either version 2, or (at
** your option) any later version.
**
********************************************************/
#ifndef QyurSqlTreeView_H
#define QyurSqlTreeView_H
#include <QAbstractProxyModel>
#include <QSqlRecord>
#include <QTreeView>

typedef  QPair<int, int> QyurIntPair;
class QSqlTableModel;
class QyurSqlTreeModelPrivate;
class QyurSqlTreeModel: public QAbstractProxyModel {
  Q_OBJECT
  Q_DECLARE_PRIVATE(QyurSqlTreeModel)
  Q_DISABLE_COPY(QyurSqlTreeModel)
  Q_CLASSINFO("Author", "Yuri Yurachkovsky")
  Q_CLASSINFO("e-mail", "yura_79@mail.ru")
  Q_CLASSINFO("Version", "0.90")
  Q_CLASSINFO("Qt version", "4.3.3")
  QyurSqlTreeModelPrivate* d_ptr;
public:
  enum FieldOrder {Id, ParentId};
  /*
  tableName-		table name itself.
  captions-		friendly captions, must match to fieldNames.
  fieldNames-		real field names in table. id and parentId must lead list!
  rootParentId-	parentId value for top level nodes.
  decoratedField-	makes this field expandedable, i.e. move it to first column.
  */
  QyurSqlTreeModel(const QString& tableName, const QStringList& captions, const QStringList& fieldNames, int rootParentId= 0, QObject* parent=0);
  ~QyurSqlTreeModel();
  QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;
  QModelIndex mapToSource(const QModelIndex& proxyIndex) const;
  bool hasChildren(const QModelIndex& index=QModelIndex()) const;
  int columnCount(const QModelIndex& parent=QModelIndex()) const;
  QModelIndex index(int row, int column, const QModelIndex& parent=QModelIndex()) const;
  QModelIndex parent(const QModelIndex& index) const;
  int rowCount(const QModelIndex& parent=QModelIndex()) const;
  bool setData(const QModelIndex& index, const QVariant& value, int role =Qt::EditRole);
  void sort(int column, Qt::SortOrder order=Qt::AscendingOrder);
  void setFilter(const QString &filter);
  QString filter();
  QVariant headerData(int section, Qt::Orientation orientation, int role=Qt::DisplayRole) const;
  QSqlTableModel* getSourceModel();

  int getIdByIndex(const QModelIndex&) const;
  QModelIndex getIndexById(int id, int parentId = 0) const;
  int getParidByIndex(const QModelIndex&) const;
  int getFieldPosition(FieldOrder) const;
  bool removeRecords(QModelIndexList&);
  int getRootParentId() const;
  int originalColumnByProxy(int proxyColumn) const;
  int proxyColumnByOriginal(int originalColumn) const;
  int proxyColumnByOriginal(const QString& field) const;
public slots:
  void refresh();
};

class QyurSqlTreeViewPrivate;
class QyurSqlTreeView: public QTreeView {
  Q_OBJECT
  Q_DECLARE_PRIVATE(QyurSqlTreeView)
  Q_DISABLE_COPY(QyurSqlTreeView)
  Q_CLASSINFO("Author", "Yuri Yurachkovsky")
  Q_CLASSINFO("e-mail", "yura_79@mail.ru")
  Q_CLASSINFO("Version", "0.92")
  Q_CLASSINFO("Qt version", "4.3.3")
  QyurSqlTreeViewPrivate* d_ptr;
public:
  QyurSqlTreeView(QWidget * parent = 0);
  ~QyurSqlTreeView();
  void setColumnHidden(const QString& column, bool hide);
  int columnIndex(const QString& fieldName) const;
  void restoreExpanded();
private slots:
  void slotSortByColumnAndSelect(int);
  void slotAddExpanded(const QModelIndex&);
  void slotRemoveExpanded(const QModelIndex&);
protected slots:
  //Use this handle when row inserted.
  virtual void onInsertRow(QSqlRecord&);
  //And take care about signals, emitted by QyurSqlTreeModel::getSourceModel() :
  //beforeDelete(int) and beforeUpdate(int, QSqlRecord&), when you need to handle delete and update.
private:
  void restore(int parentId=0, int id=-1);
};
#endif
