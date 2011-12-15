#include "newsheader.h"
#include "rsslisting.h"

extern NewsModel *newsModel_;

NewsHeader::NewsHeader(Qt::Orientation orientation, QWidget * parent) :
    QHeaderView(orientation, parent)
{
  setObjectName("newsHeader");
  setContextMenuPolicy(Qt::CustomContextMenu);
  setMovable(true);
  setDefaultAlignment(Qt::AlignLeft);
  setMinimumSectionSize(25);
  setStretchLastSection(true);

  viewMenu_ = new QMenu(this);
  QAction *pAct_ = new QAction(tr("Test"), this);
  viewMenu_->addAction(pAct_);

  buttonColumnView = new QPushButton();
  buttonColumnView->setIcon(QIcon(":/images/images/triangleT.png"));
  buttonColumnView->setObjectName("buttonColumnView");
  buttonColumnView->setMaximumWidth(30);
  connect(buttonColumnView, SIGNAL(clicked()), this, SLOT(slotButtonColumnView()));

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->setMargin(0);
  buttonLayout->addWidget(buttonColumnView, 0, Qt::AlignRight|Qt::AlignVCenter);
  setLayout(buttonLayout);

  this->installEventFilter(this);
}

void NewsHeader::init()
{

}

bool NewsHeader::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::Resize) {
    QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event);
    bool minSize = false;
    int oldWidth = resizeEvent->oldSize().width();
    int newWidth = resizeEvent->size().width();
    if (oldWidth > 0) {
      int size = 0;
      int widthCol[count()];
      memset(widthCol, 0, sizeof(widthCol));
      static int idxColSize = count()-1;
      if (oldWidth > newWidth) {
        minSize = true;
        size = oldWidth - newWidth;
      } else {
        size = newWidth - oldWidth;
      }

      int countCol = 0;
      bool sizeOne = false;
      while (size) {
        int lIdx = logicalIndex(idxColSize);
        if (!isSectionHidden(lIdx)) {
          if (((sectionSize(lIdx) >= 40) && !minSize) ||
              ((sectionSize(lIdx) - widthCol[idxColSize] > 40) && minSize)) {
            widthCol[idxColSize]++;
            size--;
            sizeOne = true;
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
    } else return false;
  } else {
    return false;
  }
}

/*virtual*/ void NewsHeader::mousePressEvent(QMouseEvent *event)
{
  QPoint nPos = event->pos();
  nPos.setX(nPos.x() + 5);
  idxCol = visualIndex(logicalIndexAt(nPos));
  posX = event->pos().x();
  nPos = event->pos();
  nPos.setX(nPos.x() - 5);
  QHeaderView::mousePressEvent(event);
}

/*virtual*/ void NewsHeader::mouseMoveEvent(QMouseEvent *event)
{
  bool sizeMin = false;
  if (event->buttons() & Qt::LeftButton) {
    int oldWidth = width();
    int newWidth = 0;
    int stopColFix;
    for (int i = count()-1; i >= 0; i--) {
      if (!isSectionHidden(i)) {
        stopColFix = visualIndex(i);
        break;
      }
    }
    for (int i = 0; i < count(); i++) newWidth += sectionSize(i);
    if (posX > event->pos().x()) sizeMin =  true;
    if (!sizeMin) {
      if (event->pos().x() < oldWidth) {
        for (int i = count()-1; i >= 0; i--) {
          int lIdx = logicalIndex(i);
          if (!isSectionHidden(lIdx)) {
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
      int sectionWidth = sectionSize(logicalIndex(stopColFix)) + oldWidth - newWidth;
      if ((sectionWidth > 40)) {
        resizeSection(logicalIndex(stopColFix), sectionWidth);
      }
    }
    if (!sizeMin) {
      if (posX > event->pos().x()) posX = event->pos().x();
      event->ignore();
      return;
    }
  }
  if (posX > event->pos().x()) posX = event->pos().x();
  QHeaderView::mouseMoveEvent(event);
}

void NewsHeader::slotButtonColumnView()
{
  viewMenu_->setFocus();
  viewMenu_->show();
  QPoint pPoint;
  pPoint.setX(mapToGlobal(QPoint(0,0)).x() + width() - viewMenu_->width());
  pPoint.setY(mapToGlobal(QPoint(0,0)).y() + height());
  viewMenu_->popup(pPoint);
}
