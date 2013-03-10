#include "customizetoolbardialog.h"
#include "rsslisting.h"

CustomizeToolbarDialog::CustomizeToolbarDialog(QWidget *parent, QToolBar *toolbar)
  : Dialog(parent),
    toolbar_(toolbar)
{
  setWindowTitle(tr("Customize Toolbar"));
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setMinimumWidth(300);
  setMinimumHeight(400);

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  settings_ = rssl_->settings_;

  shortcutTree_ = new QTreeWidget(this);
  shortcutTree_->setObjectName("actionTree");
  shortcutTree_->setColumnCount(4);
  shortcutTree_->setColumnHidden(0, true);
  shortcutTree_->setColumnHidden(2, true);
  shortcutTree_->setColumnHidden(3, true);
  shortcutTree_->setSortingEnabled(false);
  shortcutTree_->setHeaderHidden(true);
  shortcutTree_->header()->setResizeMode(1, QHeaderView::Stretch);

  QStringList treeItem;
  treeItem << "Id" << "Action" << "ObjectName" << "Data";
  shortcutTree_->setHeaderLabels(treeItem);

  QListIterator<QAction *> iter(toolbar_->actions());
  while (iter.hasNext()) {
    QAction *pAction = iter.next();

    treeItem.clear();
    treeItem << QString::number(shortcutTree_->topLevelItemCount())
             << pAction->text().remove("&")
             << pAction->objectName() << pAction->data().toString();
    QTreeWidgetItem *item = new QTreeWidgetItem(treeItem);

    if (pAction->icon().isNull())
      item->setIcon(1, QIcon(":/images/images/noicon.png"));
    else {
      if (pAction->objectName() == "autoLoadImagesToggle") {
        item->setIcon(1, QIcon(":/images/imagesOn"));
        item->setText(1, tr("Load images"));
      } else item->setIcon(1, pAction->icon());
    }

    if (pAction->objectName().isEmpty()) {
      item->setText(1, tr("Separator"));
      item->setText(2, "Separator");
      item->setBackground(1, qApp->palette().brush(QPalette::Midlight));
    }

    shortcutTree_->addTopLevelItem(item);
  }

  addButtonMenu_ = new QMenu(this);
  addButton_ = new QPushButton(tr("Add"));
  addButton_->setMenu(addButtonMenu_);
  connect(addButtonMenu_, SIGNAL(aboutToShow()),
          this, SLOT(showMenuAddButton()));
  connect(addButtonMenu_, SIGNAL(triggered(QAction*)),
          this, SLOT(addShortcut(QAction*)));

  removeButton_ = new QPushButton(tr("Remove"));
  removeButton_->setEnabled(false);
  connect(removeButton_, SIGNAL(clicked()), this, SLOT(removeShortcut()));

  moveUpButton_ = new QPushButton(tr("Move up"));
  moveUpButton_->setEnabled(false);
  connect(moveUpButton_, SIGNAL(clicked()), this, SLOT(moveUpShortcut()));
  moveDownButton_ = new QPushButton(tr("Move down"));
  moveDownButton_->setEnabled(false);
  connect(moveDownButton_, SIGNAL(clicked()), this, SLOT(moveDownShortcut()));

//  QPushButton *defaultButton;

  QVBoxLayout *buttonsVLayout = new QVBoxLayout();
  buttonsVLayout->addWidget(addButton_);
  buttonsVLayout->addWidget(removeButton_);
  buttonsVLayout->addSpacing(10);
  buttonsVLayout->addWidget(moveUpButton_);
  buttonsVLayout->addWidget(moveDownButton_);
  buttonsVLayout->addStretch();

  QHBoxLayout *mainlayout = new QHBoxLayout();
  mainlayout->setMargin(0);
  mainlayout->addWidget(shortcutTree_);
  mainlayout->addLayout(buttonsVLayout);

  pageLayout->addLayout(mainlayout);

  buttonBox->addButton(QDialogButtonBox::Ok);
  buttonBox->addButton(QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptDialog()));

  connect(shortcutTree_, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this, SLOT(slotCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(this, SIGNAL(finished(int)), this, SLOT(closeDialog()));

  restoreGeometry(settings_->value("customizeToolbarDlg/geometry").toByteArray());
}

void CustomizeToolbarDialog::acceptDialog()
{
  toolbar_->clear();

  QString str;
  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  for (int i = 0; i < shortcutTree_->topLevelItemCount(); i++) {
    if (!str.isEmpty()) str.append(",");

    if (shortcutTree_->topLevelItem(i)->text(2) == "Separator") {
      toolbar_->addSeparator();
      str.append("Separator");
      continue;
    }

    QListIterator<QAction *> iter(rssl_->actions());
    while (iter.hasNext()) {
      QAction *pAction = iter.next();
      if (!pAction->icon().isNull()) {
        if (pAction->objectName() == shortcutTree_->topLevelItem(i)->text(2)) {
          toolbar_->addAction(pAction);
          str.append(pAction->objectName());
          break;
        }
      }
    }
  }

  settings_->setValue("Settings/mainToolBar", str);

  accept();
}

void CustomizeToolbarDialog::closeDialog()
{
  settings_->setValue("customizeToolbarDlg/geometry", saveGeometry());
}

void CustomizeToolbarDialog::slotCurrentItemChanged(QTreeWidgetItem *current,
                                               QTreeWidgetItem *)
{
  if (shortcutTree_->indexOfTopLevelItem(current) == 0)
    moveUpButton_->setEnabled(false);
  else moveUpButton_->setEnabled(true);

  if (shortcutTree_->indexOfTopLevelItem(current) == (shortcutTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(false);
  else moveDownButton_->setEnabled(true);

  if (shortcutTree_->indexOfTopLevelItem(current) < 0) {
    removeButton_->setEnabled(false);
    moveUpButton_->setEnabled(false);
    moveDownButton_->setEnabled(false);
  } else {
    removeButton_->setEnabled(true);
  }
}

void CustomizeToolbarDialog::showMenuAddButton()
{
  addButtonMenu_->clear();

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  QListIterator<QAction *> iter(rssl_->actions());
  while (iter.hasNext()) {
    QAction *pAction = iter.next();
    if (!pAction->icon().isNull()) {
      QList<QTreeWidgetItem *> treeItems = shortcutTree_->findItems(pAction->objectName(),
                                                                    Qt::MatchFixedString,
                                                                    2);
      if (!treeItems.count()) {
        QAction *action = addButtonMenu_->addAction(pAction->icon(), pAction->text());
        action->setObjectName(pAction->objectName());

        if (pAction->objectName() == "autoLoadImagesToggle") {
          action->setIcon(QIcon(":/images/imagesOn"));
          action->setText(tr("Load images"));
        } else action->setIcon(pAction->icon());
      }
    }
  }
  addButtonMenu_->addSeparator();
  QAction *action = addButtonMenu_->addAction(QIcon(":/images/images/noicon.png"),
                                              tr("Separator"));
  action->setObjectName("Separator");
}

void CustomizeToolbarDialog::addShortcut(QAction *action)
{
  QStringList treeItem;
  treeItem << QString::number(shortcutTree_->topLevelItemCount())
           << action->text().remove("&")
           << action->objectName() << action->data().toString();
  QTreeWidgetItem *item = new QTreeWidgetItem(treeItem);
  item->setIcon(1, action->icon());

  if (action->objectName() == "Separator")
    item->setBackground(1, qApp->palette().brush(QPalette::Midlight));

  shortcutTree_->addTopLevelItem(item);
}

void CustomizeToolbarDialog::removeShortcut()
{
  int row = shortcutTree_->currentIndex().row();
  shortcutTree_->takeTopLevelItem(row);

  if (shortcutTree_->currentIndex().row() == 0)
    moveUpButton_->setEnabled(false);
  if (shortcutTree_->currentIndex().row() == (shortcutTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(false);
}

void CustomizeToolbarDialog::moveUpShortcut()
{
  int row = shortcutTree_->currentIndex().row();
  QTreeWidgetItem *treeWidgetItem = shortcutTree_->takeTopLevelItem(row-1);
  shortcutTree_->insertTopLevelItem(row, treeWidgetItem);

  if (shortcutTree_->currentIndex().row() == 0)
    moveUpButton_->setEnabled(false);
  if (shortcutTree_->currentIndex().row() != (shortcutTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(true);
}

void CustomizeToolbarDialog::moveDownShortcut()
{
  int row = shortcutTree_->currentIndex().row();
  QTreeWidgetItem *treeWidgetItem = shortcutTree_->takeTopLevelItem(row+1);
  shortcutTree_->insertTopLevelItem(row, treeWidgetItem);

  if (shortcutTree_->currentIndex().row() != 0)
    moveUpButton_->setEnabled(true);
  if (shortcutTree_->currentIndex().row() == (shortcutTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(false);
}
