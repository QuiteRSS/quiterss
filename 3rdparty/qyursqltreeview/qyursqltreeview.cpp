/*******************************************************
**
** Implemention of QyurSqlTreeView
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
#include "qyursqltreeview.h"
#include <QtCore>
#include <QtGui>
#include <QtSql>

namespace {
/*
Invariant: source model is always sorted asc by parid column. Then real user sort is perfomes in context of parid sort.
class QSqlTableModelEx take care about it;
*/
const int DROP_MULTIORDER=-2;
class QSqlTableModelEx: public QSqlTableModel {
	Q_OBJECT
public:
	QSqlTableModelEx(QObject* parent=0, QSqlDatabase db = QSqlDatabase());
	void setSort(int column=-1, Qt::SortOrder order=Qt::AscendingOrder);
protected:
	struct SortingStruct {
		SortingStruct(int field_=0, Qt::SortOrder sortingOrder_=Qt::AscendingOrder) {
			field=field_;
			sortingOrder=sortingOrder_;
		}
		int field;
		Qt::SortOrder sortingOrder;
	};
	QString orderByClause() const;
	QVector<SortingStruct> sortingFields;
};

QSqlTableModelEx::QSqlTableModelEx(QObject* parent, QSqlDatabase db):QSqlTableModel(parent, db) {
	setEditStrategy(QSqlTableModel::OnManualSubmit);
}

QString QSqlTableModelEx::orderByClause() const {
	QString s;
	if (sortingFields.isEmpty())
		return s;
	s.append("ORDER BY ");
	foreach(SortingStruct sortingField,sortingFields)
		s.append(record().fieldName(sortingField.field)).append(sortingField.sortingOrder==Qt::AscendingOrder?" ASC":" DESC").append(",");
	s.chop(1);
	return s;
}

void QSqlTableModelEx::setSort(int column, Qt::SortOrder order) {
	if (!record().field(column).isValid())
		return;
	if (order==DROP_MULTIORDER) {
		sortingFields.clear();
		sortingFields.append(SortingStruct(column,Qt::AscendingOrder));
		emit headerDataChanged(Qt::Horizontal,0,columnCount()-1);//update icons
	}
	else {
		for(int i=0; i<sortingFields.size();i++)
			if (sortingFields[i].field==column) {
				sortingFields[i].sortingOrder= (Qt::SortOrder)!sortingFields[i].sortingOrder;//order;
				return;
			}
		sortingFields.append(SortingStruct(column,order));
	}
}

struct UserData {
	UserData(int id, int parid):id(id),parid(parid) {
	}
	~UserData() {
	}
	int id,parid;
};
}//namespace

class QyurSqlTreeModelPrivate {
	Q_DECLARE_PUBLIC(QyurSqlTreeModel)
	QyurSqlTreeModel* q_ptr;
	mutable QMap<int,int> id2row,parid2row,id2ParentRow,id2rowCount;
	mutable QMap<QPair<int,int>,int> row2id;
	mutable QMap<int,UserData*> id2UserData;
	mutable QMap<QModelIndex,QModelIndex> proxy2source;
	mutable QMap<int,int> proxyColumn2Original;
public:
	QSqlTableModelEx sourceModel;
	int indexOfId, indexOfParid, rootParentId;
	int originalColumnByProxy(int proxyColumn) const {
		return proxyColumn2Original.value(proxyColumn,proxyColumn);
	}
	int getRowById(int) const;
	int getRowByParid(int) const;
	UserData* getUserDataById(int) const;
	QyurSqlTreeModelPrivate() {

	}
	~QyurSqlTreeModelPrivate() {
		clear();
	}
	void clear() {
		qDeleteAll(id2UserData);
		id2row.clear();
		parid2row.clear();
		id2ParentRow.clear();
		id2rowCount.clear();
		row2id.clear();
		id2UserData.clear();
		proxy2source.clear();
	}
}; 

UserData* QyurSqlTreeModelPrivate::getUserDataById(int id) const {
	if (!id2UserData[id]) {
		int parid= sourceModel.record(getRowById(id)).field(indexOfParid).value().toInt();
		id2UserData[id]= new UserData(id,parid);
	}
	return id2UserData[id];
}

int QyurSqlTreeModelPrivate::getRowById(int id) const {
	if (!id2row.contains(id) || sourceModel.index(id2row[id],indexOfId).data().toInt()!=id) {
		for (int i=0; i<sourceModel.rowCount(); i++)
			if (sourceModel.record(i).field(indexOfId).value().toInt()==id) {
				id2row[id]= i;
				break;
			}
	} 
	return id2row.value(id,-1);
}

int QyurSqlTreeModelPrivate::getRowByParid(int parid) const {
	if (!parid2row.contains(parid) || sourceModel.index(parid2row[parid],indexOfParid).data().toInt()!=parid || (parid2row[parid]&&sourceModel.index(parid2row[parid]-1,indexOfParid).data().toInt()==parid) ) {
		for (int i=0; i<sourceModel.rowCount(); i++)
			if (sourceModel.record(i).field(indexOfParid).value().toInt()==parid) {
				parid2row[parid]= i;
				break;
			}
	}
	return parid2row.value(parid,-1);
}

QyurSqlTreeModel::QyurSqlTreeModel(const QString& tableName, const QStringList& captions, const QStringList& fieldNames, int rootParentId, const QString& decoratedField, QObject* parent):QAbstractProxyModel(parent),d_ptr(new QyurSqlTreeModelPrivate) {
	Q_D(QyurSqlTreeModel);
	d->q_ptr= this;
	d->rootParentId= rootParentId;
	setSourceModel(&d->sourceModel);
	d->sourceModel.setTable(tableName/*.toUpper()*/);
	d->indexOfId= d->sourceModel.record().indexOf(fieldNames[Id].toUpper());
	Q_ASSERT(d->indexOfId>=0);
	d->indexOfParid= d->sourceModel.record().indexOf(fieldNames[ParentId].toUpper());
	Q_ASSERT(d->indexOfParid>=0);
	for (int i=0; i<qMin(fieldNames.size(),captions.size()); i++)
		d->sourceModel.setHeaderData(d->sourceModel.record().indexOf(fieldNames[i]),Qt::Horizontal,captions[i]);
	for (int i=0; i<fieldNames.count();i++) {
		int k= d->proxyColumn2Original.value(i,i);
		d->proxyColumn2Original[i]= d->sourceModel.record().indexOf(fieldNames[i]);
		d->proxyColumn2Original[d->sourceModel.record().indexOf(fieldNames[i])]= k;
		if (fieldNames[i]==decoratedField) {
			k= d->proxyColumn2Original.value(0,0);
			d->proxyColumn2Original[0]= d->proxyColumn2Original[i];
			d->proxyColumn2Original[i]= k;
		}
	}
	d->sourceModel.setSort(d->indexOfParid ,Qt::AscendingOrder);
	d->sourceModel.select();
}

int QyurSqlTreeModel::originalColumnByProxy(int proxyColumn) const {
	Q_D(const QyurSqlTreeModel);
	return d->originalColumnByProxy(proxyColumn);
}

int QyurSqlTreeModel::proxyColumnByOriginal(int originalColumn) const {
	Q_D(const QyurSqlTreeModel);
	foreach(int proxy,d->proxyColumn2Original) {
		if (d->proxyColumn2Original[proxy]==originalColumn)
			return proxy;
	}
	return originalColumn;
}

int QyurSqlTreeModel::proxyColumnByOriginal(const QString& field) const {
	Q_D(const QyurSqlTreeModel);
	return proxyColumnByOriginal(d->sourceModel.fieldIndex(field));
}

QVariant QyurSqlTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
	#if QT_VERSION >= 0x040400
	return QAbstractProxyModel::headerData(section,orientation,role);
	#else
	Q_D(const QyurSqlTreeModel);
	return QAbstractProxyModel::headerData(d->originalColumnByProxy(section),orientation,role);
	#endif
}

void QyurSqlTreeModel::refresh() {
	Q_D(QyurSqlTreeModel);
	d->sourceModel.select();
	reset();
	d->clear();
}

void QyurSqlTreeModel::sort(int column, Qt::SortOrder order) {
	Q_D(QyurSqlTreeModel);
	d->sourceModel.setSort(d->indexOfParid,(Qt::SortOrder)DROP_MULTIORDER);
	d->sourceModel.setSort(originalColumnByProxy(column),order);
	d->sourceModel.select();
	reset();
	d->clear();
}

void QyurSqlTreeModel::setFilter(const QString &filter) {
  Q_D(QyurSqlTreeModel);
  d->sourceModel.setFilter(filter);
  reset();
  d->clear();
}

bool QyurSqlTreeModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	Q_D(QyurSqlTreeModel);
	return d->sourceModel.setData(mapToSource(index),value,role);
}

QyurSqlTreeModel::~QyurSqlTreeModel() {
	delete d_ptr;
}

QSqlTableModel* QyurSqlTreeModel::getSourceModel() {
	Q_D(QyurSqlTreeModel);
	return &d->sourceModel;
}

int QyurSqlTreeModel::getFieldPosition(FieldOrder index) const {
	Q_D(const QyurSqlTreeModel);
	if (index==Id)
		return d->indexOfId;
	else if (index==ParentId)
		return d->indexOfParid;
	else return index;
}

QModelIndex QyurSqlTreeModel::mapFromSource(const QModelIndex& sourceIndex) const {
	Q_D(const QyurSqlTreeModel);
	const int parid= sourceIndex.sibling(sourceIndex.row(), d->indexOfParid).data().toInt();
	const int id= sourceIndex.sibling(sourceIndex.row(), d->indexOfId).data().toInt();
	const int rowOfParid= d->getRowByParid(parid);
	int i= rowOfParid;
	for (; i<d->sourceModel.rowCount(); i++)
		if (d->sourceModel.record(i).field(d->indexOfId).value().toInt()==id)
			break;
	return createIndex(i-rowOfParid, proxyColumnByOriginal(sourceIndex.column()), d->getUserDataById(id));
}

QModelIndex QyurSqlTreeModel::mapToSource(const QModelIndex& proxyIndex) const {
	Q_D(const QyurSqlTreeModel);
	if (!d->proxy2source.contains(proxyIndex)) 
		d->proxy2source[proxyIndex]= d->sourceModel.index(d->getRowById(getIdByIndex(proxyIndex)),d->originalColumnByProxy(proxyIndex.column()));
	return d->proxy2source[proxyIndex];
}

bool QyurSqlTreeModel::hasChildren(const QModelIndex& index) const {
	Q_D(const QyurSqlTreeModel);
	if (!index.isValid())
		return true;
	return -1!=d->getRowByParid(getIdByIndex(index));
}

int QyurSqlTreeModel::columnCount(const QModelIndex&) const {
	Q_D(const QyurSqlTreeModel);
	return d->sourceModel.record().count();
}

QModelIndex QyurSqlTreeModel::index(int row, int column, const QModelIndex& parent) const {
	Q_D(const QyurSqlTreeModel);	
	if (d->sourceModel.rowCount()==0)
		return QModelIndex();
	int id= parent.isValid()?getIdByIndex(parent):getRootParentId();
	if (!d->row2id.contains(QPair<int,int>(id,row))) {
		int i= 0;
		if (parent.isValid())
			i= d->getRowByParid(id);
		if (d->sourceModel.record(i+row).field(d->indexOfParid).value().toInt()==id)
			d->row2id[QPair<int,int>(id,row)]= d->sourceModel.record(i+row).field(d->indexOfId).value().toInt();
		else return QModelIndex();
	}
	return createIndex(row,column,d->getUserDataById(d->row2id[QPair<int,int>(id,row)]));//d->row2id[QPair(id,row)];
}

QModelIndex QyurSqlTreeModel::parent(const QModelIndex& index) const {
	Q_D(const QyurSqlTreeModel);
	if (d->sourceModel.rowCount()==0)
		return QModelIndex();
	if (!index.isValid())
		return QModelIndex();
	int parid= getParidByIndex(index);
	if (parid==getRootParentId())
		return QModelIndex();
	
	if (!d->id2ParentRow.contains(getIdByIndex(index))) {
		int paridOfParent= d->sourceModel.record(d->getRowById(parid)).field(d->indexOfParid).value().toInt();
		int rowOfParent=0;
		if (paridOfParent==getRootParentId()) {
			while (d->sourceModel.record(rowOfParent).field(d->indexOfParid).value().toInt()==getRootParentId()&&d->sourceModel.record(rowOfParent).field(d->indexOfId).value().toInt()!=parid)
				rowOfParent++;
		} else { 
			rowOfParent= d->getRowByParid(paridOfParent);
			if (rowOfParent>=0)
				while (d->sourceModel.record(rowOfParent).field(d->indexOfId).value().toInt()!=parid)
					rowOfParent++;
			rowOfParent-=d->getRowByParid(paridOfParent);
		}
		if (d->sourceModel.record(rowOfParent+((paridOfParent==getRootParentId())?0:d->getRowByParid(paridOfParent))).field(d->indexOfId).value().toInt()==parid)
			d->id2ParentRow[getIdByIndex(index)]= rowOfParent;
		else return QModelIndex();
	}
	return createIndex(d->id2ParentRow[getIdByIndex(index)],0,d->getUserDataById(parid));	
}

int QyurSqlTreeModel::rowCount(const QModelIndex& parent) const {
	Q_D(const QyurSqlTreeModel);
	if (d->sourceModel.rowCount()==0)
		return 0;
	int id= parent.isValid()?getIdByIndex(parent):getRootParentId();
	if (!d->id2rowCount.contains(id)) {
		int i=0;
		if (!parent.isValid()) {
			while (d->sourceModel.record(i).field(d->indexOfParid).value().toInt()==getRootParentId()&&i<d->sourceModel.rowCount())
				i++;
		} else { 
			i= d->getRowByParid(getIdByIndex(parent));
			if (i>=0)
				while (d->sourceModel.record(i).field(d->indexOfParid).value().toInt()==getIdByIndex(parent))
					i++;
			i-=d->getRowByParid(getIdByIndex(parent));
		}
		d->id2rowCount[id]=i;
	}
	return d->id2rowCount[id];
}

bool QyurSqlTreeModel::removeRecords(QModelIndexList& list) {
	Q_D(QyurSqlTreeModel);
	//sort list desc
	for(int i=1; i<list.size(); i++) {
		QModelIndex index= list[i];
		int j=i-1;
		while(j>=0&&mapToSource(list[j]).row()<mapToSource(index).row()) {
			list[j+1]=list[j];
			j--;
		}
		list[j+1]= index;
	}
	for (int i=0; i<list.size(); i++) 
		if (list[i].isValid())
			d->sourceModel.removeRows(mapToSource(list[i]).row(),1);
	d->sourceModel.submitAll();
	refresh();
	return true;
}

QModelIndex QyurSqlTreeModel::getIndexById(int id, int parentId) const {
	Q_D(const QyurSqlTreeModel);
	QModelIndex parentIndex;
	if (parentId>0)
		parentIndex= getIndexById(parentId,d->getUserDataById(parentId)->parid);
	for (int i=0; i<rowCount(parentIndex); i++) {
		if (getIdByIndex(index(i,0,parentIndex))==id)
			return index(i,0,parentIndex);
	}
	return QModelIndex();
}

int QyurSqlTreeModel::getIdByIndex(const QModelIndex& index) const {
	if (index.isValid())
		return static_cast<UserData*>(index.internalPointer())->id;
	return 0;
}

int QyurSqlTreeModel::getParidByIndex(const QModelIndex& index) const {
  if (index.isValid())
    return static_cast<UserData*>(index.internalPointer())->parid;
  return 0;
}

int QyurSqlTreeModel::getRootParentId() const {
	Q_D(const QyurSqlTreeModel);
	return d->rootParentId;
}

class QyurSqlTreeViewPrivate {
public:
	QyurSqlTreeViewPrivate();
//	enum Actions {Edit, AddSibling, AddChild, Remove, LastAction};
//	QAction* actions[LastAction];
//	QMenu menu;
//	QSignalMapper signalMapper;
//	QDialog dialog;
//	QGridLayout* dialogGridLayout;
	QList<QyurIntPair> expandedNodeList;
};


QyurSqlTreeViewPrivate::QyurSqlTreeViewPrivate() {
//	const char* labels[]= {"Modify","Add","Add child","Remove"};
//	for (int i=0; i<LastAction; i++) {
//		actions[i]= menu.addAction(QObject::tr(labels[i]));
//		signalMapper.setMapping(actions[i], i);
//		QObject::connect(actions[i],SIGNAL(triggered()),&signalMapper,SLOT(map()));
//	}
//	QVBoxLayout* topLayout= new QVBoxLayout(&dialog);
//	dialogGridLayout= new QGridLayout;
//  QWidget *dialogGridWidget = new QWidget;
//  dialogGridWidget->setLayout(dialogGridLayout);
//  QScrollArea *scrollArea = new QScrollArea;
//  scrollArea->setWidget(dialogGridWidget);
//  scrollArea->setWidgetResizable(true);
//  topLayout->addWidget(scrollArea);

//	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
//	QObject::connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
//	QObject::connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
//	topLayout->addWidget(buttonBox);
}

QyurSqlTreeView::QyurSqlTreeView(QWidget * parent): QTreeView(parent),d_ptr(new QyurSqlTreeViewPrivate){
//	Q_D(QyurSqlTreeView);
	setSortingEnabled(true);
	disconnect(header(), SIGNAL(sectionClicked(int)), this, SLOT(sortByColumn(int)));
	connect(header(), SIGNAL(sectionClicked(int)), SLOT(slotSortByColumnAndSelect(int)));
	setContextMenuPolicy(Qt::CustomContextMenu);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
//	QObject::connect(this,SIGNAL(customContextMenuRequested(const QPoint&)),SLOT(slotCustomContextMenuRequested(const QPoint&)));
	QObject::connect(this,SIGNAL(expanded(const QModelIndex&)),SLOT(slotAddExpanded(const QModelIndex&)));
	QObject::connect(this,SIGNAL(collapsed(const QModelIndex&)),SLOT(slotRemoveExpanded(const QModelIndex&)));
//	QObject::connect(&d->signalMapper, SIGNAL(mapped(int)), SLOT(slotMenuExec(int)));
}

QyurSqlTreeView::~QyurSqlTreeView() {
	delete d_ptr;
}

void QyurSqlTreeView::slotAddExpanded(const QModelIndex& index) {
	Q_D(QyurSqlTreeView);
	d->expandedNodeList.append(QyurIntPair( ((QyurSqlTreeModel*) model())->getIdByIndex(index), ((QyurSqlTreeModel*) model())->getParidByIndex(index)) );
}

void QyurSqlTreeView::slotRemoveExpanded(const QModelIndex& index) {
	Q_D(QyurSqlTreeView);
	d->expandedNodeList.removeAll(QyurIntPair( ((QyurSqlTreeModel*) model())->getIdByIndex(index), ((QyurSqlTreeModel*) model())->getParidByIndex(index)) );
}

void QyurSqlTreeView::slotSortByColumnAndSelect(int column) {
	int id= -1;
	int parid=-1;
	if (selectionModel()->selectedRows().size()>0) {
		id= selectionModel()->selectedRows()[0].sibling(selectionModel()->selectedRows()[0].row(),((QyurSqlTreeModel*) model())->getFieldPosition(QyurSqlTreeModel::Id)).data().toInt();
		parid= selectionModel()->selectedRows()[0].sibling(selectionModel()->selectedRows()[0].row(),((QyurSqlTreeModel*) model())->getFieldPosition(QyurSqlTreeModel::ParentId)).data().toInt();
	}
	QTreeView::sortByColumn(column);
	restore(parid,id);
}

void QyurSqlTreeView::restore(int parentId, int id) {
	Q_D(QyurSqlTreeView);
	if (id!=-1) {
		QModelIndex selectedIndex= ((QyurSqlTreeModel*) model())->getIndexById(id,parentId);
		scrollTo(selectedIndex);
		selectionModel()->select(selectedIndex,QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
	}
	foreach(QyurIntPair pair, d->expandedNodeList)
		setExpanded(((QyurSqlTreeModel*) model())->getIndexById(pair.first,pair.second),true);
}

//void QyurSqlTreeView::slotCustomContextMenuRequested(const QPoint&) {
//	Q_D(QyurSqlTreeView);
//	d->actions[QyurSqlTreeViewPrivate::Edit]->setEnabled(selectionModel()->selectedRows().size()==1);
//	d->actions[QyurSqlTreeViewPrivate::AddSibling]->setEnabled(selectionModel()->selectedRows().size()<2);
//	d->actions[QyurSqlTreeViewPrivate::AddChild]->setEnabled(selectionModel()->selectedRows().size()==1);
//	d->actions[QyurSqlTreeViewPrivate::Remove]->setEnabled(selectionModel()->selectedRows().size()>0);
//	d->menu.exec(QCursor::pos());
//}

//void QyurSqlTreeView::slotMenuExec(int action) {
////	Q_D(QyurSqlTreeView);
//	Q_CHECK_PTR(model());
//	QModelIndexList selected= selectionModel()->selectedRows();
//	switch (action) {
//		case QyurSqlTreeViewPrivate::Edit:
//			executeDialog(false,0);//parentId not used
//			break;
//		case QyurSqlTreeViewPrivate::AddSibling:
//			executeDialog(true,selected.size()?((QyurSqlTreeModel*) model())->getParidByIndex(selected[0]):((QyurSqlTreeModel*) model())->getRootParentId());
//			break;
//		case QyurSqlTreeViewPrivate::AddChild:
//			executeDialog(true,((QyurSqlTreeModel*) model())->getIdByIndex(selected[0]));
//			break;
//		case QyurSqlTreeViewPrivate::Remove:
//			if (QMessageBox::Yes==QMessageBox::question(0,QObject::tr("Warning!"),tr("Continue removing ")+QString::number(selected.size())+tr(" rows with childs?"),QMessageBox::Yes|QMessageBox::No)) {
//				QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//				QStack<QModelIndex> stack;
//				foreach(QModelIndex index,selected)
//					stack.push(index);
//				while(!stack.isEmpty()) {
//					QModelIndex curr= stack.pop();
//					if (-1==selected.indexOf(curr))
//						selected<<curr;
//					for(int i=0; i<curr.model()->rowCount(curr); i++)
//						stack.push(curr.child(i,0));

//				}
//				((QyurSqlTreeModel*) model())->removeRecords(selected);
//				restore();
//				QApplication::restoreOverrideCursor();
//			}
//			break;
//	}
//}
//namespace {
//struct QSqlTableHelper {
//	QSqlTableHelper(QAbstractItemModel* model):model(model){}
//	virtual QModelIndex getIndex(const QModelIndex& index, int siblingColumn=0) {
//		return index.sibling(index.row(), siblingColumn);
//	}
//	QAbstractItemModel* model;
//};
//struct QSqlTreeHelper: public QSqlTableHelper {
//	QSqlTreeHelper(QyurSqlTreeModel* model):QSqlTableHelper(model){}
//	QModelIndex getIndex(const QModelIndex& index, int) {
//		return static_cast<QyurSqlTreeModel*>(model)->mapToSource(index);
//	}
//};
//}
//bool QyurSqlTreeView::executeDialog(bool insertMode, int parentId) {
//	Q_D(QyurSqlTreeView);
//	QSqlTableModel* sourceModel= ((QyurSqlTreeModel*) model())->getSourceModel();
//	QModelIndex selected;
//	QAbstractItemView* usedViewForDelegate;
//	QSqlTableModel forInsertModel;
//	QTableView forInsertView;
//	QSqlTableHelper* helper=0;
//	if (insertMode) {
//		// Existing sorted table model dont place new record to the end of table. //
//		// So to get new record create independent model for insert.              //
//		forInsertModel.setTable(sourceModel->tableName());
//		forInsertModel.setEditStrategy(QSqlTableModel::OnManualSubmit);
//		sourceModel= &forInsertModel;
//		forInsertModel.select();
//		forInsertView.setSelectionBehavior(QAbstractItemView::SelectRows);
//		forInsertView.setSelectionMode(QAbstractItemView::SingleSelection);
//		forInsertView.setModel(&forInsertModel);
		
//		QSqlRecord record= forInsertModel.record();
//		record.setValue(((QyurSqlTreeModel*) model())->getFieldPosition(QyurSqlTreeModel::ParentId),parentId);
//		onInsertRow(record);
//		forInsertModel.insertRecord(-1,record);
//		selected= forInsertModel.index(forInsertModel.rowCount()-1,0);
//		usedViewForDelegate= &forInsertView;
//		helper= new QSqlTableHelper(&forInsertModel);
//	} else {
//		selected= selectionModel()->selectedRows()[0];
//		usedViewForDelegate= this;
//		helper= new QSqlTreeHelper((QyurSqlTreeModel*) model());
//	}
//	QWidgetList widgetList;
//	for (int i=0; i<header()->count(); i++)
//		if (!header()->isSectionHidden(i)) {
//			int dialogGridLayoutRow= d->dialogGridLayout->rowCount();
//			d->dialogGridLayout->addWidget(new QLabel(model()->headerData(i,Qt::Horizontal, Qt::DisplayRole ).toString()),dialogGridLayoutRow,0);
//			QWidget* editor= usedViewForDelegate->itemDelegate()->createEditor(0,viewOptions(),helper->getIndex(selected.sibling(selected.row(),i), ((QyurSqlTreeModel*) model())->originalColumnByProxy(i)));
//			widgetList.append(editor);
//			QMetaProperty property= editor->metaObject()->userProperty();
//			property.write(editor,helper->getIndex(selected.sibling(selected.row(),i),((QyurSqlTreeModel*) model())->originalColumnByProxy(i)).data());
//			d->dialogGridLayout->addWidget(editor,dialogGridLayoutRow,1);
//			if (!i){
//				if (qobject_cast<QLineEdit*>(editor))
//					qobject_cast<QLineEdit*>(editor)->selectAll();
//				editor->setFocus();
			
//			}
//		}
//	d->dialog.move(QCursor::pos());
//	int dialogResut= d->dialog.exec();
//	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//	if (dialogResut==QDialog::Accepted) {
//		int cnt=0;
//		for (int i=0; i<header()->count(); i++)
//			if (!header()->isSectionHidden(i)) {
//				usedViewForDelegate->itemDelegate()->setModelData(widgetList[cnt],sourceModel,helper->getIndex(selected.sibling(selected.row(),i),((QyurSqlTreeModel*) model())->originalColumnByProxy(i)));
//				cnt++;
//			}
//		sourceModel->submitAll();
//    if (sourceModel->lastError().type() != QSqlError::NoError)
//      qDebug() << sourceModel->lastError().number() << sourceModel->lastError().text();
//		if (insertMode) {
//			int newId= selected.sibling(selected.row(),((QyurSqlTreeModel*) model())->getFieldPosition(QyurSqlTreeModel::Id)).data().toInt();
//			((QyurSqlTreeModel*) model())->refresh();
//			restore(parentId,newId);
//		}
//	} else if (insertMode)
//		sourceModel->removeRows(forInsertModel.rowCount()-1,1);
//	if (!((QyurSqlTreeModel*) model())->rowCount())
//		((QyurSqlTreeModel*) model())->getSourceModel()->select();
//	QLayoutItem *child;
//	while ((child = d->dialogGridLayout->takeAt(0))!=0) {//clear layout
//		delete child->widget();
//		delete child;
//	}
//	QApplication::restoreOverrideCursor();
//	delete helper;
//	return dialogResut==QDialog::Accepted;
//}

void QyurSqlTreeView::onInsertRow(QSqlRecord&) {

}

void QyurSqlTreeView::setColumnHidden(const QString& column, bool hide) {
	//QTreeView::setColumnHidden(((QyurSqlTreeModel*) model())->proxyColumnByOriginal(column),hide);
	QTreeView::setColumnHidden(columnIndex(column),hide);
}

int QyurSqlTreeView::columnIndex(const QString& fieldName) const {
	return ((QyurSqlTreeModel*) model())->proxyColumnByOriginal(fieldName);
}
#include "qyursqltreeview.moc"
