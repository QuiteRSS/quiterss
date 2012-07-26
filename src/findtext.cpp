#include "findtext.h"

FindTextContent::FindTextContent(QWidget *parent)
  : QLineEdit(parent)
{
  findInNewsAct_ = new QAction(this);
  findInNewsAct_->setObjectName("findInNewsAct");
  findInNewsAct_->setIcon(QIcon(":/images/findInNews"));
  findInNewsAct_->setCheckable(true);
  findInNewsAct_->setChecked(true);
  findInBrowserAct_ = new QAction(this);
  findInBrowserAct_->setObjectName("findInBrowserAct");
  findInBrowserAct_->setIcon(QIcon(":/images/findText"));
  findInBrowserAct_->setCheckable(true);

  findButton = new QToolButton(this);
  QPixmap findPixmap(":/images/selectFindInNews");
  findButton->setIcon(QIcon(findPixmap));
  findButton->setIconSize(findPixmap.size());
  findButton->setCursor(Qt::ArrowCursor);
  findButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");

  findGroup_ = new QActionGroup(this);
  findGroup_->setExclusive(true);
  findMenu_ = new QMenu(this);

  findMenu_->addAction(findInNewsAct_);
  findMenu_->addAction(findInBrowserAct_);
  findGroup_->addAction(findInNewsAct_);
  findGroup_->addAction(findInBrowserAct_);

  connect(findButton, SIGNAL(clicked()), this, SLOT(slotMenuFind()));
  connect(findGroup_, SIGNAL(triggered(QAction*)),
          this, SLOT(slotSelectFind(QAction*)));

  clearButton = new QToolButton(this);
  QPixmap pixmap(":/images/editClear");
  clearButton->setIcon(QIcon(pixmap));
  clearButton->setIconSize(pixmap.size());
  clearButton->setCursor(Qt::ArrowCursor);
  clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  clearButton->hide();
  connect(clearButton, SIGNAL(clicked()), this, SLOT(slotClear()));
  connect(this, SIGNAL(textChanged(const QString&)),
          SLOT(updateClearButton(const QString&)));

  findLabel_ = new QLabel(tr("Filter news"), this);
  findLabel_->setStyleSheet("QLabel { color: gray; }");

  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  setStyleSheet(QString("QLineEdit { padding-right: %1px; padding-left: %2px; }").
                arg(clearButton->sizeHint().width() + frameWidth + 1).
                arg(findButton->sizeHint().width() + frameWidth + 1));
  QSize msz = minimumSizeHint();
  setMinimumSize(
        qMax(msz.width(), clearButton->sizeHint().height() + findButton->sizeHint().height() + frameWidth * 2 + 2),
        qMax(msz.height(), clearButton->sizeHint().height() + frameWidth * 2 + 2));
}

void FindTextContent::retranslateStrings()
{
  findInNewsAct_->setText(tr("Filter News"));
  findInBrowserAct_->setText(tr("Find in Browser"));
  findLabel_->setText(findGroup_->checkedAction()->text());
}

void FindTextContent::resizeEvent(QResizeEvent *)
{
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  QSize sz = findButton->sizeHint();
  findButton->move(frameWidth+3,
                   (rect().bottom() + 1 - sz.height())/2);
  sz = findLabel_->sizeHint();
  findLabel_->move(frameWidth+findButton->sizeHint().width()+5,
                   (rect().bottom() + 1 - sz.height())/2);
  sz = clearButton->sizeHint();
    clearButton->move(rect().right() - frameWidth - sz.width(),
                      (rect().bottom() + 1 - sz.height())/2);
}

void FindTextContent::focusInEvent(QFocusEvent *event)
{
  findLabel_->setVisible(false);
  QLineEdit::focusInEvent(event);
}

void FindTextContent::focusOutEvent(QFocusEvent *event)
{
  if (text().isEmpty())
    findLabel_->setVisible(true);
  QLineEdit::focusOutEvent(event);
}

void FindTextContent::updateClearButton(const QString& text)
{
  clearButton->setVisible(!text.isEmpty());
}

void FindTextContent::slotClear()
{
  clear();
  emit signalClear();
}

void FindTextContent::slotMenuFind()
{
  findMenu_->popup(mapToGlobal(QPoint(0, height()-1)));
}

void FindTextContent::slotSelectFind(QAction *act)
{
  if (act->objectName() == "findInBrowserAct") {
    findButton->setIcon(QIcon(":/images/selectFindInBrowser"));
  } else {
    findButton->setIcon(QIcon(":/images/selectFindInNews"));
  }
  findLabel_->setText(act->text());
  emit signalSelectFind();
}
