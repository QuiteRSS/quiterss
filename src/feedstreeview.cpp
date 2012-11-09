#include "feedstreeview.h"
#include "delegatewithoutfocus.h"

FeedsTreeView::FeedsTreeView(QWidget * parent) :
  QyurSqlTreeView(parent)
{
  isDraging_ = false;
  dragPos_ = QPoint();
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
  qDebug() << selectIndex_ << selectIndex_.flags();
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

  qDebug() << "Drag start";
  isDraging_ = true;
  dragPos_ = event->pos();

  QMimeData *mimeData = new QMimeData;
//  mimeData->setText("MovingItem");

  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setHotSpot(event->pos() + QPoint(10,10));

  Qt::DropAction dropAction = drag->exec();
  qDebug() << "dropAction : " << dropAction;
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
  selectIndex_ = current;
  QyurSqlTreeView::currentChanged(current, previous);
}

void FeedsTreeView::dragEnterEvent(QDragEnterEvent *event)
{
  qDebug() << "DragEnter";
//  if (event->mimeData()->hasFormat("image/x-puzzle-piece"))
    event->accept();
//  else
//    event->ignore();
}

void FeedsTreeView::dragLeaveEvent(QDragLeaveEvent *event)
{
  qDebug() << "DragLeave";
//  QRect updateRect = highlightedRect;
  dragPos_ = QPoint();
  viewport()->update();
  event->accept();
}

void FeedsTreeView::dragMoveEvent(QDragMoveEvent *event)
{
//  QyurSqlTreeView::dragMoveEvent(event);

  if (indexAt(event->pos()).isValid() && (dropIndicatorPosition() != QAbstractItemView::QAbstractItemView::OnViewport))
    event->accept();
  else
    event->ignore();

  if (isDraging_) dragPos_ = event->pos();

  viewport()->update();
}

void FeedsTreeView::dropEvent(QDropEvent *event)
{
  isDraging_ = false;
  dragPos_ = QPoint();
  viewport()->update();

  event->setDropAction(Qt::MoveAction);
  event->accept();
  qDebug() << "Drag finished";
  handleDrop(event);
}

void FeedsTreeView::paintEvent(QPaintEvent *event)
{
  QyurSqlTreeView::paintEvent(event);

  if (dragPos_.isNull()) return;

  QPainter painter;
  painter.begin(this->viewport());

  //    painter.setBrush(QColor("#ffcccc"));
  painter.setPen(Qt::DashLine);

  QModelIndex index = indexAt(dragPos_);
  QModelIndex indexText =
      model()->index(index.row(),
                     ((QyurSqlTreeModel*)model())->proxyColumnByOriginal("text"),
                     index.parent());

  QRect rectText = visualRect(indexText);

  if (qAbs(rectText.top() - dragPos_.y()) < 3) {
    qDebug() << "^^^" << index.row();
    painter.drawLine(0, rectText.top(), width(), rectText.top());
  }
  else if (qAbs(rectText.bottom() - dragPos_.y()) < 3) {
    qDebug() << "___" << index.row();
    painter.drawLine(0, rectText.bottom(), width(), rectText.bottom());
  }
  else {
    qDebug() << "===" << index.row();
    painter.drawRect(rectText);
  }

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
  QModelIndex indexWhere = indexAt(e->pos());
//  emit signalDropped(indexWhat, indexWhere);
  qDebug() << indexWhat << indexWhere;
}
