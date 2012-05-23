#include "feedsview.h"
#include "delegatewithoutfocus.h"

FeedsView::FeedsView(QWidget * parent) :
  QTreeView(parent)
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

/*virtual*/ void FeedsView::mousePressEvent(QMouseEvent *event)
{
  if (!(event->buttons() & Qt::MiddleButton)){
    selectFeedIndex = indexAt(event->pos());
    if (event->buttons() & Qt::RightButton) {
    } else QTreeView::mousePressEvent(event);
  }
}

/*virtual*/ void FeedsView::mouseMoveEvent(QMouseEvent *event)
{
  Q_UNUSED(event)
}

/*virtual*/ void FeedsView::keyPressEvent(QKeyEvent *event)
{
  if (!event->modifiers()) {
    if (event->key() == Qt::Key_Up) emit pressKeyUp();
    else if (event->key() == Qt::Key_Down) emit pressKeyDown();
  }
}
