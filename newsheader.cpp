#include "newsheader.h"

NewsHeader::NewsHeader(Qt::Orientation orientation, QWidget * parent) :
    QHeaderView(orientation, parent)
{
  setObjectName("newsHeader");
  setContextMenuPolicy(Qt::CustomContextMenu);
  setMovable(true);
  setDefaultAlignment(Qt::AlignLeft);
  setMinimumSectionSize(40);

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
  for (int i = 0; i < count(); i++) {
    if (!isSectionHidden(i)) {
      startColFix = i;
      break;
    }
  }
  for (int i = count()-1; i >= 0; i--) {
    if (!isSectionHidden(i)) {
      stopColFix = i;
      break;
    }
  }
  setResizeMode(stopColFix, QHeaderView::Fixed);

}

bool NewsHeader::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::Resize) {
    QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event);
    bool minSize = false;
    bool sizeOk = false;
    int oldWidth = resizeEvent->oldSize().width();
    int newWidth = resizeEvent->size().width();
    if (oldWidth > newWidth) minSize = true;

    for (int i = count()-1; i >= 0; i--) {
      if (i != startColFix) {
        if ((sectionSize(i) > 40) && minSize) {
          int width = newWidth - sectionSize(startColFix);
          for (int y = 0; y < count(); y++) {
            if (!((y == startColFix) || (y == i)))
              width = width - sectionSize(y);
          }
          if (width > 40) resizeSection(i, width);
          else resizeSection(i, 40);
          sizeOk = true;
        }
      }
    }

    if ((sectionSize(startColFix) > 40) || !minSize) {
      if (sizeOk) newWidth = newWidth;
      int width = newWidth - sectionSize(4) - sectionSize(stopColFix);
      if (width > 40) resizeSection(startColFix, width);
      else resizeSection(startColFix, 40);
    }
    event->ignore();
    return true;
  } else if (event->type() == QEvent::MouseMove) {
    qDebug() << "1";
    return false;
  } else {
    return false;
  }
}

/*virtual*/ void NewsHeader::mousePressEvent(QMouseEvent *event)
{
  QPoint nPos = event->pos();
  nPos.setX(nPos.x() + 5);
  idxCol = logicalIndexAt(nPos);
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
    for (int i = 0; i < count(); i++) newWidth += sectionSize(i);
    if (posX > event->pos().x()) sizeMin =  true;
    if (!sizeMin) {
      if (event->pos().x() < oldWidth) {
        for (int i = count()-1; i >= 0; i--) {
          if (!isSectionHidden(i)) {
            int sectionWidth = sectionSize(i) + oldWidth - newWidth;
            if (sectionWidth > 40) {
              if (i >= idxCol) {
                resizeSection(i, sectionWidth);
                sizeMin = true;
                break;
              }
            }
          }
        }
      }
      int tWidth = 0;
      for (int i = idxCol; i < count(); i++) {
        if (!isSectionHidden(i)) {
          tWidth += 40;
        }
      }
      if (event->pos().x()+tWidth > oldWidth) {
        sizeMin = false;
      }
    } else {
      int sectionWidth = sectionSize(stopColFix) + oldWidth - newWidth;
      if ((sectionWidth > 40)) {
        resizeSection(stopColFix, sectionWidth);
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
