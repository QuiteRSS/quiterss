#include "newsview.h"
#include "delegatewithoutfocus.h"

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
  DelegateWithoutFocus *itemDelegate = new DelegateWithoutFocus(this);
  setItemDelegate(itemDelegate);
//    setAlternatingRowColors(true);
  setContextMenuPolicy(Qt::CustomContextMenu);
}

/*virtual*/ void NewsView::mousePressEvent(QMouseEvent *event)
{
  QModelIndex index = indexAt(event->pos());
  if (event->buttons() & Qt::LeftButton) {
    if (index.column() == model_->fieldIndex("sticky")) {
      QModelIndex curIndex = currentIndex();
      if (model_->index(index.row(), model_->fieldIndex("sticky")).data(Qt::EditRole).toInt() == 0) {
        model_->setData(model_->index(index.row(), model_->fieldIndex("sticky")), 1);
      } else {
        model_->setData(model_->index(index.row(), model_->fieldIndex("sticky")), 0);
      }
      setCurrentIndex(curIndex);
      event->ignore();
      return;
    } else if (index.column() == model_->fieldIndex("read")) {
      if (model_->index(index.row(), model_->fieldIndex("read")).data(Qt::EditRole).toInt() == 0) {
        emit signalSetItemRead(index, 1);
      } else {
        emit signalSetItemRead(index, 0);
      }
      event->ignore();
      return;
    }
    QTreeView::mousePressEvent(event);
  } else if (event->buttons() & Qt::RightButton) {
    setCurrentIndex(index);
  }
}

/*virtual*/ void NewsView::mouseMoveEvent(QMouseEvent *event)
{
}

/*virtual*/ void NewsView::mouseDoubleClickEvent(QMouseEvent *event)
{
  emit signalDoubleClicked(indexAt(event->pos()));
}
