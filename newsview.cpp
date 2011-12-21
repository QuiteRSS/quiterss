#include "newsview.h"

NewsView::NewsView(QWidget * parent) :
    QTreeView(parent)
{
  setObjectName("newsView_");
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setMinimumWidth(120);
  setSortingEnabled(true);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);
//    setAlternatingRowColors(true);
}

/*virtual*/ void NewsView::mousePressEvent(QMouseEvent *event)
{
  QModelIndex index = indexAt(event->pos());
  QList<QModelIndex> indexList = selectionModel()->selectedRows(model_->fieldIndex("sticky"));
  if (index.column() == model_->fieldIndex("sticky")) {
    int sticky;
    if (model_->index(index.row(), model_->fieldIndex("sticky")).data(Qt::EditRole).toInt() == 0) {
      sticky = 1;
    } else {
      sticky = 0;
    }
    model_->setData(model_->index(index.row(), model_->fieldIndex("sticky")), sticky);

    if (indexList.count())
      selectionModel()->select(indexList[0], QItemSelectionModel::Select|QItemSelectionModel::Rows);

    event->ignore();
    return;
  } else if (index.column() == model_->fieldIndex("read")) {
    int read;
    if (model_->index(index.row(), model_->fieldIndex("read")).data(Qt::EditRole).toInt() == 0) {
      read = 1;
    } else {
      read = 0;
    }
    model_->setData(model_->index(index.row(), model_->fieldIndex("read")), read);
//    updateStatus();
    if (indexList.count())
      selectionModel()->select(indexList[0], QItemSelectionModel::Select|QItemSelectionModel::Rows);

    event->ignore();
    return;
  }
  QTreeView::mousePressEvent(event);
}

/*virtual*/ void NewsView::mouseMoveEvent(QMouseEvent *event)
{
}
