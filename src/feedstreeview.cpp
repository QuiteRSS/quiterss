#include "feedstreemodel.h"
#include "feedstreeview.h"
#include "delegatewithoutfocus.h"

FeedsTreeView::FeedsTreeView(QWidget * parent) :
  QyurSqlTreeView(parent)
{
  dragPos_ =      QPoint();
  dragStartPos_ = QPoint();

  setObjectName("feedsTreeView_");
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setEditTriggers(QAbstractItemView::NoEditTriggers);

  setSelectionBehavior(QAbstractItemView::SelectRows);
//  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setSelectionMode(QAbstractItemView::SingleSelection);

  setUniformRowHeights(true);

  header()->setStretchLastSection(false);
  header()->setVisible(false);

  DelegateWithoutFocus *itemDelegate = new DelegateWithoutFocus(this);
  setItemDelegate(itemDelegate);

  setContextMenuPolicy(Qt::CustomContextMenu);

  setDragDropMode(QAbstractItemView::InternalMove);
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
}

/*virtual*/ void FeedsTreeView::mousePressEvent(QMouseEvent *event)
{
  if (!indexAt(event->pos()).isValid()) return;

  selectIndex_ = indexAt(event->pos());
  if ((event->buttons() & Qt::MiddleButton)) {
    if (selectIndex_.isValid())
      emit signalMiddleClicked();
  } else if (event->buttons() & Qt::RightButton) {

  } else {
    dragStartPos_ = event->pos();

    QyurSqlTreeView::mousePressEvent(event);
  }
}

void FeedsTreeView::mouseReleaseEvent(QMouseEvent *event)
{
  dragStartPos_ = QPoint();
  QyurSqlTreeView::mouseReleaseEvent(event);
}

/*virtual*/ void FeedsTreeView::mouseMoveEvent(QMouseEvent *event)
{
  if (dragStartPos_.isNull()) return;

  if ((event->pos() - dragStartPos_).manhattanLength() < qApp->startDragDistance())
    return;

  event->accept();

  dragPos_ = event->pos();

  QMimeData *mimeData = new QMimeData;
//  mimeData->setText("MovingItem");

  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setHotSpot(event->pos() + QPoint(10,10));

  drag->exec();
}

/*virtual*/ void FeedsTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
  QyurSqlTreeView::mouseDoubleClickEvent(event);
//  if (!indexAt(event->pos()).isValid()) return;

//  emit signalDoubleClicked(indexAt(event->pos()));
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
  selectIndex_ = current;
  QyurSqlTreeView::currentChanged(current, previous);
}

void FeedsTreeView::dragEnterEvent(QDragEnterEvent *event)
{
  event->accept();
  dragPos_ = event->pos();
  viewport()->update();
}

void FeedsTreeView::dragLeaveEvent(QDragLeaveEvent *event)
{
  event->accept();
  dragPos_ = QPoint();
  viewport()->update();
}

void FeedsTreeView::dragMoveEvent(QDragMoveEvent *event)
{
  if (dragPos_.isNull()) {
    event->ignore();
    viewport()->update();
    return;
  }

  dragPos_ = event->pos();
  QModelIndex dragIndex = indexAt(dragPos_);

  QString feedUrl =
      ((FeedsTreeModel*)model())->dataField(dragIndex, "xmlUrl").toString();

  // обработка категорий
  if (feedUrl.isEmpty()) {
    if (dragIndex == currentIndex().parent())
      event->ignore();  // категория уже является родителем
    else
      event->accept();
  }
  // обработка лент
  else {
    if (dragIndex.parent() == currentIndex().parent())
      event->ignore();  // не перемещаем ленту внутри категории
    else
      event->accept();
  }

  viewport()->update();

  if (shouldAutoScroll(event->pos()))
    startAutoScroll();
}

bool FeedsTreeView::shouldAutoScroll(const QPoint &pos) const
{
    if (!hasAutoScroll())
        return false;
    QRect area = viewport()->rect();
    return (pos.y() - area.top() < autoScrollMargin())
        || (area.bottom() - pos.y() < autoScrollMargin())
        || (pos.x() - area.left() < autoScrollMargin())
        || (area.right() - pos.x() < autoScrollMargin());
}

void FeedsTreeView::dropEvent(QDropEvent *event)
{
  dragPos_ = QPoint();
  viewport()->update();

  event->setDropAction(Qt::MoveAction);
  event->accept();
  handleDrop(event);
}

void FeedsTreeView::paintEvent(QPaintEvent *event)
{
  QyurSqlTreeView::paintEvent(event);

  if (dragPos_.isNull()) return;

  QModelIndex dragIndex = indexAt(dragPos_);
  QString feedUrl =
      ((FeedsTreeModel*)model())->dataField(dragIndex, "xmlUrl").toString();

  bool drawParent = false;
  // Обработка категорий
  if (feedUrl.isEmpty()) {
    if (dragIndex == currentIndex().parent())
      return;
  }
  // Обработка лент
  else
    if (dragIndex.parent() == currentIndex().parent())
      return;
    else drawParent = true;

  QModelIndex indexText;
  if (drawParent)
    indexText = model()->index(dragIndex.parent().row(),
                               ((QyurSqlTreeModel*)model())->proxyColumnByOriginal("text"),
                               dragIndex.parent().parent());
  else
    indexText = model()->index(dragIndex.row(),
                     ((QyurSqlTreeModel*)model())->proxyColumnByOriginal("text"),
                     dragIndex.parent());

  QRect rectText = visualRect(indexText);
  QBrush brush = qApp->palette().brush(QPalette::Highlight);

  QPainter painter;
  painter.begin(this->viewport());

  painter.setPen(Qt::DashLine);

//  if (qAbs(rectText.top() - dragPos_.y()) < 3) {
//    painter.setPen(QPen(brush, 2));
//    painter.drawLine(rectText.topLeft().x()-2, rectText.top(),
//                     viewport()->width()-2, rectText.top());
//    painter.drawLine(rectText.topLeft().x()-2, rectText.top()-2,
//                     rectText.topLeft().x()-2, rectText.top()+2);
//    painter.drawLine(viewport()->width()-2, rectText.top()-2,
//                     viewport()->width()-2, rectText.top()+2);
//  }
//  else if (qAbs(rectText.bottom() - dragPos_.y()) < 3) {
//    painter.setPen(QPen(brush, 2));
//    painter.drawLine(rectText.bottomLeft().x()-2, rectText.bottom(),
//                     viewport()->width()-2, rectText.bottom());
//    painter.drawLine(rectText.topLeft().x()-2, rectText.bottom()-2,
//                     rectText.topLeft().x()-2, rectText.bottom()+2);
//    painter.drawLine(viewport()->width()-2, rectText.bottom()-2,
//                     viewport()->width()-2, rectText.bottom()+2);
//  }
//  else {
    painter.setPen(QPen(brush, 1, Qt::DashLine));
    painter.setOpacity(0.5);
    painter.drawRect(rectText);

    painter.setPen(QPen());
    painter.setBrush(brush);
    painter.setOpacity(0.1);
    painter.drawRect(rectText);
//  }

  painter.end();
}

void FeedsTreeView::setSelectIndex()
{
  selectIndex_ = currentIndex();
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


void FeedsTreeView::handleDrop(QDropEvent *e)
{
  QModelIndex indexWhat = currentIndex();
  QModelIndex indexWhere;

  QModelIndex dropIndex = indexAt(e->pos());
  QString feedUrl =
      ((FeedsTreeModel*)model())->dataField(dropIndex, "xmlUrl").toString();

  bool drawParent = false;
  // Обработка категорий
  if (feedUrl.isEmpty()) {
    if (dropIndex == currentIndex().parent())
      return;
  }
  // Обработка лент
  else
    if (dropIndex.parent() == currentIndex().parent())
      return;
    else drawParent = true;

  if (drawParent)
    indexWhere = model()->index(dropIndex.parent().row(),
                               ((QyurSqlTreeModel*)model())->proxyColumnByOriginal("text"),
                               dropIndex.parent().parent());
  else
    indexWhere = model()->index(dropIndex.row(),
                     ((QyurSqlTreeModel*)model())->proxyColumnByOriginal("text"),
                     dropIndex.parent());

  emit signalDropped(indexWhat, indexWhere);
}
