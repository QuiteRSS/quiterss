#include <QSqlTableModel>
#include <QSqlQuery>

#include "feedstreemodel.h"
#include "feedstreeview.h"
#include "delegatewithoutfocus.h"

FeedsTreeView::FeedsTreeView(QWidget * parent) :
  QyurSqlTreeView(parent)
{
  dragPos_ =      QPoint();
  dragStartPos_ = QPoint();
  selectIdEn_ = true;

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

  connect(this, SIGNAL(expanded(const QModelIndex&)), SLOT(slotExpanded(const QModelIndex&)));
  connect(this, SIGNAL(collapsed(const QModelIndex&)), SLOT(slotCollapsed(const QModelIndex&)));
}

/**
 * @brief Поиск следующей непрочитанной ленты
 * details Производится поиск следующай непрочитанная лента. Если следующей
 *    ленты нет, то ищется предыдущая непрочитанная лента
 * @param index Индекс, от которого начинаем искать
 * @param next Условие поиска: 1 - ищёт следующую, 2 - ищет предыдущую,
 * 0 - ищет следующую, если не находит, то ищет предыдущую
 * @return найденный индекс либо QModelIndex()
 ******************************************************************************/
QModelIndex FeedsTreeView::indexNextUnread(const QModelIndex &indexCur, int next)
{
  if (next != 2) {
    // ищем следующую непрочитанную, исключая категории
    QModelIndex index = indexBelow(indexCur);
    while (index.isValid()) {
      bool isFeedFolder = ((FeedsTreeModel*)model())->isFolder(index);
      int feedUnreadCount = ((FeedsTreeModel*)model())->dataField(index, "unread").toInt();
      if (!isFeedFolder && (0 < feedUnreadCount))
        return index;  // нашли

      index = indexBelow(index);
    }
  }

  if (next != 1) {
    // ищем предыдущую непрочитанную, исключая категории
    QModelIndex index = indexAbove(indexCur);
    while (index.isValid()) {
      bool isFeedFolder = ((FeedsTreeModel*)model())->isFolder(index);
      int feedUnreadCount = ((FeedsTreeModel*)model())->dataField(index, "unread").toInt();
      if (!isFeedFolder && (0 < feedUnreadCount))
        return index;  // нашли

      index = indexAbove(index);
    }
  }

  // не нашли
  return QModelIndex();
}

/**
 * @brief Поиск следующей ленты
 * @param index Индекс, от которого начинаем искать
 * @return найденный индекс либо QModelIndex()
 ******************************************************************************/
QModelIndex FeedsTreeView::indexPrevious(const QModelIndex &indexCur)
{
  QModelIndex index = indexAbove(indexCur);
  while (index.isValid()) {
    bool isFeedFolder = ((FeedsTreeModel*)model())->isFolder(index);
    if (!isFeedFolder)
      return index;  // нашли

    index = indexAbove(index);
  }

  // не нашли
  return QModelIndex();
}

/**
 * @brief Поиск предыдущей ленты
 * @param index Индекс, от которого начинаем искать
 * @return найденный индекс либо QModelIndex()
 ******************************************************************************/
QModelIndex FeedsTreeView::indexNext(const QModelIndex &indexCur)
{
  QModelIndex index = indexBelow(indexCur);
  while (index.isValid()) {
    bool isFeedFolder = ((FeedsTreeModel*)model())->isFolder(index);
    if (!isFeedFolder)
      return index;  // нашли

    index = indexBelow(index);
  }

  // не нашли
  return QModelIndex();
}

/**
 * @brief Собственная обработка нажатия мыши
 * @details Фиксирует нажатый индекс в selectedIndex_, обрабатывает нажатие
 *    средней клавиши, игнорирует нажатия правой клавиши, для левой клавиши
 *    вызывает стандартный обработчик
 * @param event Структура, содержащая данные события
 * @sa selectedIndex_
 ******************************************************************************/
void FeedsTreeView::mousePressEvent(QMouseEvent *event)
{
  QModelIndex index = indexAt(event->pos());
  QRect rectText = visualRect(index);

  if (event->buttons() & Qt::RightButton) {
    if (event->pos().x() >= rectText.x()) {
      selectId_ = ((FeedsTreeModel*)model())->getIdByIndex(index);
      selectParentId_ = ((FeedsTreeModel*)model())->getParidByIndex(index);
    }
    return;
  }

  if (!index.isValid() || !(event->pos().x() >= rectText.x())) return;

  selectId_ = ((FeedsTreeModel*)model())->getIdByIndex(index);
  selectParentId_ = ((FeedsTreeModel*)model())->getParidByIndex(index);

  if ((event->buttons() & Qt::MiddleButton)) {
    emit signalMiddleClicked();
  } else if (event->buttons() & Qt::LeftButton) {
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
  if (selectIdEn_) {
    QModelIndex index = current;
    selectId_ = ((FeedsTreeModel*)model())->getIdByIndex(index);
    selectParentId_ = ((FeedsTreeModel*)model())->getParidByIndex(index);
  }
  selectIdEn_ = true;

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

  // обработка категорий
  if (((FeedsTreeModel*)model())->isFolder(dragIndex)) {
    if (dragIndex == currentIndex().parent())
      event->ignore();  // категория уже является родителем
    else if (dragIndex == currentIndex())
      event->ignore();  // не перемещаем категорию саму на себя
    else {
      bool ignore = false;
      QModelIndex child = dragIndex.parent();
      while (child.isValid()) {
        if (child == currentIndex()) {
          event->ignore();  // не перемещаем категорию внутри себя
          ignore = true;
          break;
        }
        child = child.parent();
      }
      if (!ignore) event->accept();
    }
  }
  // обработка лент
  else {
    if (dragIndex == currentIndex())
      event->ignore();  // не перемещаем ленту внутри категории
    else if (dragIndex.parent() == currentIndex())
      event->ignore();  // не перемещаем категорию внутри категории
    else {
      bool ignore = false;
      QModelIndex child = dragIndex.parent();
      while (child.isValid()) {
        if (child == currentIndex()) {
          event->ignore();  // не перемещаем категорию внутри себя
          ignore = true;
          break;
        }
        child = child.parent();
      }
      if (!ignore) event->accept();
    }
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

/** @brief ОБработка разворачивания узла
 *----------------------------------------------------------------------------*/
void FeedsTreeView::slotExpanded(const QModelIndex &index)
{
  QModelIndex indexExpanded = index.sibling(index.row(), columnIndex("f_Expanded"));
  model()->setData(indexExpanded, 1);

  int feedId = ((FeedsTreeModel*)model())->getIdByIndex(indexExpanded);
  QSqlQuery q;
  q.exec(QString("UPDATE feeds SET f_Expanded=1 WHERE id=='%2'").arg(feedId));
}

/** @brief Обработка сворачивания узла
 *----------------------------------------------------------------------------*/
void FeedsTreeView::slotCollapsed(const QModelIndex &index)
{
  QModelIndex indexExpanded = index.sibling(index.row(), columnIndex("f_Expanded"));
  model()->setData(indexExpanded, 0);

  int feedId = ((FeedsTreeModel*)model())->getIdByIndex(indexExpanded);
  QSqlQuery q;
  q.exec(QString("UPDATE feeds SET f_Expanded=0 WHERE id=='%2'").arg(feedId));
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

  // Обработка категорий
  if (((FeedsTreeModel*)model())->isFolder(dragIndex)) {
    if ((dragIndex == currentIndex().parent()) ||
        (dragIndex == currentIndex()))
      return;

    QModelIndex child = dragIndex.parent();
    while (child.isValid()) {
      if (child == currentIndex()) return;
      child = child.parent();
    }
  }
  // Обработка лент
  else {
    if ((dragIndex == currentIndex()) ||
        (dragIndex.parent() == currentIndex()))
      return;
    QModelIndex child = dragIndex.parent();
    while (child.isValid()) {
      if (child == currentIndex()) return;
      child = child.parent();
    }
  }

  QModelIndex indexText = model()->index(dragIndex.row(),
                                         ((QyurSqlTreeModel*)model())->proxyColumnByOriginal("text"),
                                         dragIndex.parent());

  QRect rectText = visualRect(indexText);
  rectText.setWidth(rectText.width()-1);
  QBrush brush = qApp->palette().brush(QPalette::Highlight);

  QPainter painter;

  if (qAbs(rectText.top() - dragPos_.y()) < 3) {
    painter.begin(this->viewport());
    painter.setPen(QPen(brush, 2));
    painter.drawLine(rectText.topLeft().x()-2, rectText.top(),
                     viewport()->width()-2, rectText.top());
    painter.drawLine(rectText.topLeft().x()-2, rectText.top()-2,
                     rectText.topLeft().x()-2, rectText.top()+2);
    painter.drawLine(viewport()->width()-2, rectText.top()-2,
                     viewport()->width()-2, rectText.top()+2);
  }
  else if (qAbs(rectText.bottom() - dragPos_.y()) < 3) {
    painter.begin(this->viewport());
    painter.setPen(QPen(brush, 2));
    painter.drawLine(rectText.bottomLeft().x()-2, rectText.bottom(),
                     viewport()->width()-2, rectText.bottom());
    painter.drawLine(rectText.topLeft().x()-2, rectText.bottom()-2,
                     rectText.topLeft().x()-2, rectText.bottom()+2);
    painter.drawLine(viewport()->width()-2, rectText.bottom()-2,
                     viewport()->width()-2, rectText.bottom()+2);
  }
  else {
    if (!((FeedsTreeModel*)model())->isFolder(dragIndex))
      return;

    painter.begin(this->viewport());
    painter.setPen(QPen(brush, 1, Qt::DashLine));
    painter.setOpacity(0.5);
    painter.drawRect(rectText);

    painter.setPen(QPen());
    painter.setBrush(brush);
    painter.setOpacity(0.1);
    painter.drawRect(rectText);
  }

  painter.end();
}

QPersistentModelIndex FeedsTreeView::selectIndex()
{
  return ((FeedsTreeModel*)model())->getIndexById(selectId_, selectParentId_);
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
  QModelIndex dropIndex = indexAt(e->pos());

  QModelIndex indexWhat = currentIndex();
  QModelIndex indexWhere = dropIndex;

  int how = 0;
  QRect rectText = visualRect(dropIndex);
  if (qAbs(rectText.top() - e->pos().y()) < 3) {
    how = 0;
  } else if (qAbs(rectText.bottom() - e->pos().y()) < 3) {
    how = 1;
  } else {
    if (((FeedsTreeModel*)model())->isFolder(dropIndex)) {
      how = 2;
    } else {
      dropIndex = model()->index(dropIndex.row()+1,
                                  ((QyurSqlTreeModel*)model())->proxyColumnByOriginal("text"),
                                  dropIndex.parent());
      if (!dropIndex.isValid()) how = 1;
    }
  }

  emit signalDropped(indexWhat, indexWhere, how);
}
