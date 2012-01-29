#include "newsheader.h"

NewsHeader::NewsHeader(Qt::Orientation orientation, QWidget * parent) :
    QHeaderView(orientation, parent)
{
  setObjectName("newsHeader");
  setContextMenuPolicy(Qt::CustomContextMenu);
  setMovable(true);
  setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  setMinimumSectionSize(25);
  setStretchLastSection(false);

  pActGroup_ = NULL;

  viewMenu_ = new QMenu(this);

  buttonColumnView = new QPushButton();
  buttonColumnView->setIcon(QIcon(":/images/images/column.png"));
  buttonColumnView->setObjectName("buttonColumnView");
  buttonColumnView->setMaximumWidth(30);
  connect(buttonColumnView, SIGNAL(clicked()), this, SLOT(slotButtonColumnView()));

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->setMargin(0);
  buttonLayout->addWidget(buttonColumnView, 0, Qt::AlignRight|Qt::AlignVCenter);
  setLayout(buttonLayout);

  connect(this, SIGNAL(sectionMoved(int,int,int)), SLOT(slotSectionMoved(int, int, int)));

  this->installEventFilter(this);
}

void NewsHeader::initColumns()
{
  if (model_->columnCount() == 0) return;
  for (int i = 0; i < model_->columnCount(); ++i)
    hideSection(i);
  showSection(model_->fieldIndex("title"));
  showSection(model_->fieldIndex("published"));
  showSection(model_->fieldIndex("received"));
  showSection(model_->fieldIndex("author_name"));
  showSection(model_->fieldIndex("read"));
  showSection(model_->fieldIndex("sticky"));

  moveSection(visualIndex(model_->fieldIndex("sticky")), 0);
  resizeSection(model_->fieldIndex("sticky"), 25);
  setResizeMode(model_->fieldIndex("sticky"), QHeaderView::Fixed);
  moveSection(visualIndex(model_->fieldIndex("title")), 1);
  moveSection(visualIndex(model_->fieldIndex("read")), 2);
  resizeSection(model_->fieldIndex("read"), 25);
  setResizeMode(model_->fieldIndex("read"), QHeaderView::Fixed);
  moveSection(visualIndex(model_->fieldIndex("author_name")), 3);
  resizeSection(model_->fieldIndex("author_name"), 100);
  resizeSection(model_->fieldIndex("title"), 200);
  setSortIndicator(model_->fieldIndex("published"), Qt::DescendingOrder);
}

void NewsHeader::createMenu()
{
  if (model_->columnCount() == 0) return;
  if (pActGroup_) delete pActGroup_;
  pActGroup_ = new QActionGroup(viewMenu_);
  pActGroup_->setExclusive(false);
  connect(pActGroup_, SIGNAL(triggered(QAction*)), this, SLOT(columnVisible(QAction*)));

  for (int i = 0; i < model_->columnCount(); i++) {
    int lIdx = logicalIndex(i);
    if ((lIdx == model_->fieldIndex("title")) ||
        (lIdx == model_->fieldIndex("published")) ||
        (lIdx == model_->fieldIndex("received")) ||
        (lIdx == model_->fieldIndex("author_name")) ||
        (lIdx == model_->fieldIndex("read")) ||
        (lIdx == model_->fieldIndex("sticky"))) {
      QAction *action = pActGroup_->addAction(
            model_->headerData(lIdx,
            Qt::Horizontal, Qt::EditRole).toString());
      action->setData(lIdx);
      action->setCheckable(true);
      action->setChecked(!isSectionHidden(lIdx));
      viewMenu_->addAction(action);
    }
  }
}

void NewsHeader::overload()
{
  if (model_->columnCount() == 0) return;
  model_->setHeaderData(model_->fieldIndex("title"), Qt::Horizontal, tr("Title"));
  model_->setHeaderData(model_->fieldIndex("published"), Qt::Horizontal, tr("Date"));
  model_->setHeaderData(model_->fieldIndex("received"), Qt::Horizontal, tr("Received"));
  model_->setHeaderData(model_->fieldIndex("author_name"), Qt::Horizontal, tr("Author"));
  model_->setHeaderData(model_->fieldIndex("read"), Qt::Horizontal, tr("Read"));
  model_->setHeaderData(model_->fieldIndex("sticky"), Qt::Horizontal, tr("Star"));
  for (int i = 0; i < model_->columnCount(); i++) {
    model_->setHeaderData(i, Qt::Horizontal,
                          model_->headerData(i, Qt::Horizontal, Qt::DisplayRole),
                          Qt::EditRole);
  }
  model_->setHeaderData(model_->fieldIndex("read"), Qt::Horizontal, "", Qt::DisplayRole);
  model_->setHeaderData(model_->fieldIndex("read"), Qt::Horizontal,
                        QIcon(":/images/readSection"), Qt::DecorationRole);
  model_->setHeaderData(model_->fieldIndex("sticky"), Qt::Horizontal, "", Qt::DisplayRole);
  model_->setHeaderData(model_->fieldIndex("sticky"), Qt::Horizontal,
                        QIcon(":/images/starSection"), Qt::DecorationRole);
  setSortIndicator(sortIndicatorSection(), sortIndicatorOrder());
}

bool NewsHeader::eventFilter(QObject *obj, QEvent *event)
{
  Q_UNUSED(obj)
  if (event->type() == QEvent::Resize) {
    if (model_->columnCount() == 0) return false;

    if (buttonColumnView->height() != height())
      buttonColumnView->setFixedHeight(height());

    QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event);
    bool minSize = false;
    int newWidth = resizeEvent->size().width();
    int size = 0;
    int widthCol[count()];
    memset(widthCol, 0, sizeof(widthCol));
    static int idxColSize = count()-1;
    int tWidth = 0;
    for (int i = 0; i < count(); i++) tWidth += sectionSize(i);
    if (tWidth > newWidth) {
      minSize = true;
      size = tWidth - newWidth;
    } else {
      size = newWidth - tWidth;
    }
    int countCol = 0;
    bool sizeOne = false;
    while (size) {
      int lIdx = logicalIndex(idxColSize);
      if (!isSectionHidden(lIdx)) {
        if (!((model_->fieldIndex("read") == lIdx) || (model_->fieldIndex("sticky") == lIdx))) {
          if (((sectionSize(lIdx) >= 40) && !minSize) ||
              ((sectionSize(lIdx) - widthCol[idxColSize] > 40) && minSize)) {
            widthCol[idxColSize]++;
            size--;
            sizeOne = true;
          }
        }
      }
      if (idxColSize == 0) idxColSize = count()-1;
      else idxColSize--;

      if (++countCol == count()) {
        if (!sizeOne) break;
        sizeOne = false;
        countCol = 0;
      }
    }

    for (int i = count()-1; i >= 0; i--) {
      int lIdx = logicalIndex(i);
      if (!isSectionHidden(lIdx) && (sectionSize(lIdx) >= 40)) {
        if (!minSize) {
          resizeSection(lIdx, sectionSize(lIdx) + widthCol[i]);
        } else {
          resizeSection(lIdx, sectionSize(lIdx) - widthCol[i]);
        }
      }
    }
    event->ignore();
    return true;
  } else if ((event->type() == QEvent::HoverMove) ||
             (event->type() == QEvent::HoverEnter) ||
             (event->type() == QEvent::HoverLeave)) {
    QHoverEvent *hoverEvent = static_cast<QHoverEvent*>(event);
    if (hoverEvent->pos().x() > width() - buttonColumnView->width()) {
      if (event->type() == QEvent::HoverMove) {
        setCursor(QCursor(Qt::ArrowCursor));
        QHoverEvent* pe =
                    new QHoverEvent(QEvent::HoverLeave, hoverEvent->oldPos(), hoverEvent->pos());
        QApplication::sendEvent(this, pe);
      }
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

/*virtual*/ void NewsHeader::mousePressEvent(QMouseEvent *event)
{
  QPoint nPos = event->pos();
  nPos.setX(nPos.x() + 5);
  idxCol = visualIndex(logicalIndexAt(nPos));
  posX1 = event->pos().x();
  nPos = event->pos();
  nPos.setX(nPos.x() - 5);
  QHeaderView::mousePressEvent(event);
}

/*virtual*/ void NewsHeader::mouseMoveEvent(QMouseEvent *event)
{
  bool sizeMin = false;
  if ((event->buttons() & Qt::LeftButton) && cursor().shape() == Qt::SplitHCursor) {
    int oldWidth = width();
    int newWidth = 0;

    for (int i = 0; i < count(); i++) newWidth += sectionSize(i);
    if (posX1 > event->pos().x()) sizeMin =  true;
    if (!sizeMin) {
      if (event->pos().x() < oldWidth) {
        for (int i = count()-1; i >= 0; i--) {
          int lIdx = logicalIndex(i);
          if (!isSectionHidden(lIdx)) {
            if (!((model_->fieldIndex("read") == lIdx) || (model_->fieldIndex("sticky") == lIdx))) {
              int sectionWidth = sectionSize(lIdx) + oldWidth - newWidth;
              if (sectionWidth > 40) {
                if (i >= idxCol) {
                  resizeSection(lIdx, sectionWidth);
                  sizeMin = true;
                  break;
                }
              }
            }
          }
        }
      }
      int tWidth = 0;
      for (int i = idxCol; i < count(); i++) {
        if (!isSectionHidden(logicalIndex(i))) {
          tWidth += 40;
        }
      }
      if (event->pos().x()+tWidth > oldWidth) {
        sizeMin = false;
      }
    } else {
      int stopColFix = 0;
      for (int i = count()-1; i >= 0; i--) {
        int lIdx = logicalIndex(i);
        if (!isSectionHidden(lIdx)) {
          if (!((model_->fieldIndex("read") == lIdx) || (model_->fieldIndex("sticky") == lIdx))) {
            stopColFix = i;
            break;
          }
        }
      }

      int sectionWidth = sectionSize(logicalIndex(stopColFix)) + oldWidth - newWidth;
//      qDebug() << oldWidth << newWidth << sectionWidth << sectionSize(logicalIndex(stopColFix));
      if ((sectionWidth > 40)) {
        if (!((model_->fieldIndex("read") == logicalIndex(idxCol)) ||
              (model_->fieldIndex("sticky") == logicalIndex(idxCol))) || idxCol < stopColFix) {
          resizeSection(logicalIndex(stopColFix), sectionWidth);
        } else sizeMin = false;
      }
    }
    if (!sizeMin) {
      if (posX1 > event->pos().x()) posX1 = event->pos().x();
      event->ignore();
      return;
    }
  }
  if (posX1 > event->pos().x()) posX1 = event->pos().x();

  QHeaderView::mouseMoveEvent(event);
}

void NewsHeader::slotButtonColumnView()
{
  viewMenu_->setFocus();
  viewMenu_->show();
  QPoint pPoint;
  pPoint.setX(mapToGlobal(QPoint(0,0)).x() + width() - viewMenu_->width() - 1);
  pPoint.setY(mapToGlobal(QPoint(0,0)).y() + height() + 1);
  viewMenu_->popup(pPoint);
}

void NewsHeader::columnVisible(QAction *action)
{
  int idx = action->data().toInt();
  setSectionHidden(idx, !isSectionHidden(idx));
  QSize newSize = size();
  newSize.setWidth(newSize.width()+1);
  resize(newSize);
}

void NewsHeader::slotSectionMoved(int lIdx, int oldVIdx, int newVIdx)
{
  Q_UNUSED(oldVIdx)
  if ((model_->fieldIndex("read") == lIdx) ||
      (model_->fieldIndex("sticky") == lIdx)) {
    for (int i = count()-1; i >= 0; i--) {
      if (!isSectionHidden(logicalIndex(i))) {
        if (i == newVIdx) {
          resizeSection(lIdx, 45);
          break;
        } else {
          resizeSection(lIdx, 25);
          break;
        }
      }
    }
    QSize newSize = size();
    newSize.setWidth(newSize.width()+1);
    resize(newSize);
  }
  createMenu();
}

void NewsHeader::retranslateStrings()
{
  if (model_->columnCount() == 0) return;
  model_->setHeaderData(model_->fieldIndex("title"), Qt::Horizontal, tr("Title"));
  model_->setHeaderData(model_->fieldIndex("published"), Qt::Horizontal, tr("Date"));
  model_->setHeaderData(model_->fieldIndex("received"), Qt::Horizontal, tr("Received"));
  model_->setHeaderData(model_->fieldIndex("author_name"), Qt::Horizontal, tr("Author"));
  model_->setHeaderData(model_->fieldIndex("read"), Qt::Horizontal, tr("Read"));
  model_->setHeaderData(model_->fieldIndex("sticky"), Qt::Horizontal, tr("Star"));

  if (pActGroup_) delete pActGroup_;
  pActGroup_ = new QActionGroup(viewMenu_);
  pActGroup_->setExclusive(false);
  connect(pActGroup_, SIGNAL(triggered(QAction*)), this, SLOT(columnVisible(QAction*)));

  for (int i = 0; i < model_->columnCount(); i++) {
    int lIdx = logicalIndex(i);
    if ((lIdx == model_->fieldIndex("title")) ||
        (lIdx == model_->fieldIndex("published")) ||
        (lIdx == model_->fieldIndex("received")) ||
        (lIdx == model_->fieldIndex("author_name")) ||
        (lIdx == model_->fieldIndex("read")) ||
        (lIdx == model_->fieldIndex("sticky"))) {
      QAction *action = pActGroup_->addAction(
            model_->headerData(lIdx,
            Qt::Horizontal, Qt::EditRole).toString());
      action->setData(lIdx);
      action->setCheckable(true);
      action->setChecked(!isSectionHidden(lIdx));
      viewMenu_->addAction(action);
    }
  }

  model_->setHeaderData(model_->fieldIndex("read"), Qt::Horizontal, "", Qt::DisplayRole);
  model_->setHeaderData(model_->fieldIndex("read"), Qt::Horizontal,
                        QIcon(":/images/readSection"), Qt::DecorationRole);
  model_->setHeaderData(model_->fieldIndex("sticky"), Qt::Horizontal, "", Qt::DisplayRole);
  model_->setHeaderData(model_->fieldIndex("sticky"), Qt::Horizontal,
                        QIcon(":/images/starSection"), Qt::DecorationRole);
}
