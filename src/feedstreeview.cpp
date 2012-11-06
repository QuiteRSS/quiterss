#include "feedstreeview.h"
#include "delegatewithoutfocus.h"

FeedsTreeView::FeedsTreeView(QWidget * parent) :
  QyurSqlTreeView(parent)
{
  setObjectName("feedsTreeView_");
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setEditTriggers(QAbstractItemView::NoEditTriggers);

  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  setUniformRowHeights(true);

  header()->setStretchLastSection(false);
  header()->setVisible(false);

  DelegateWithoutFocus *itemDelegate = new DelegateWithoutFocus(this);
  setItemDelegate(itemDelegate);

  setContextMenuPolicy(Qt::CustomContextMenu);
}

/*virtual*/ void FeedsTreeView::mousePressEvent(QMouseEvent *event)
{
  if (!indexAt(event->pos()).isValid()) return;

  selectIndex = indexAt(event->pos());
  if ((event->buttons() & Qt::MiddleButton)) {
    if (selectIndex.isValid())
      emit signalMiddleClicked();
  } else if (event->buttons() & Qt::RightButton) {

  } else QTreeView::mousePressEvent(event);
}

/*virtual*/ void FeedsTreeView::mouseMoveEvent(QMouseEvent *event)
{
  Q_UNUSED(event)
}

/*virtual*/ void FeedsTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (!indexAt(event->pos()).isValid()) return;

  emit signalDoubleClicked(indexAt(event->pos()));
}

/*virtual*/ void FeedsTreeView::keyPressEvent(QKeyEvent *event)
{
  if (!event->modifiers()) {
    if (event->key() == Qt::Key_Up)         emit pressKeyUp();
    else if (event->key() == Qt::Key_Down)  emit pressKeyDown();
    else if (event->key() == Qt::Key_Home)  emit pressKeyHome();
    else if (event->key() == Qt::Key_End)   emit pressKeyEnd();
  }
}

/*virtual*/ void FeedsTreeView::currentChanged(const QModelIndex &current,
                                           const QModelIndex &previous)
{
  selectIndex = current;
  QTreeView::currentChanged(current, previous);
}

void FeedsTreeView::setSelectIndex()
{
  selectIndex = currentIndex();
}

/** @brief Обновление курсора без пролистывания списка ************************/
void FeedsTreeView::updateCurrentIndex(const QModelIndex &index)
{
  setUpdatesEnabled(false);
  int topRow = verticalScrollBar()->value();
  setCurrentIndex(index);
  verticalScrollBar()->setValue(topRow);
  setUpdatesEnabled(true);
}
