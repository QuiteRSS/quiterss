#include "newsfiltersdialog.h"

NewsFiltersDialog::NewsFiltersDialog(QWidget *parent) :
  QDialog(parent)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("News filters"));
  setMinimumWidth(400);
  setMinimumHeight(250);

  filtersTree = new QTreeWidget();
  filtersTree->setObjectName("filtersTree");
  filtersTree->setColumnHidden(0, true);
  filtersTree->header()->setResizeMode(QHeaderView::Stretch);
  filtersTree->header()->setMovable(false);

  QStringList treeItem;
  treeItem << tr("Id") << tr("Name");
  filtersTree->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << QString::number(filtersTree->topLevelItemCount())
           << tr("Test filter %1").arg(filtersTree->topLevelItemCount());
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setCheckState(1, Qt::Checked);
  filtersTree->addTopLevelItem(treeWidgetItem);

  QPushButton *newButton = new QPushButton(tr("New..."), this);
  connect(newButton, SIGNAL(clicked()), this, SLOT(newFilter()));
  QPushButton *editButton = new QPushButton(tr("Edite..."), this);
  connect(editButton, SIGNAL(clicked()), this, SLOT(editeFilter()));
  QPushButton *deleteButton = new QPushButton(tr("Delete..."), this);
  connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteFilter()));

  QPushButton *moveUpButton = new QPushButton(tr("Move up"), this);
  connect(moveUpButton, SIGNAL(clicked()), this, SLOT(moveUpFilter()));
  QPushButton *moveDownButton = new QPushButton(tr("Move down"), this);
  connect(moveDownButton, SIGNAL(clicked()), this, SLOT(moveDownFilter()));

  QVBoxLayout *buttonsLayout = new QVBoxLayout();
  buttonsLayout->setAlignment(Qt::AlignCenter);
  buttonsLayout->setSpacing(5);
  buttonsLayout->addWidget(newButton);
  buttonsLayout->addWidget(editButton);
  buttonsLayout->addWidget(deleteButton);
  buttonsLayout->addSpacing(20);
  buttonsLayout->addWidget(moveUpButton);
  buttonsLayout->addWidget(moveDownButton);
  buttonsLayout->addStretch();

  QHBoxLayout *layoutH1 = new QHBoxLayout();
  layoutH1->setMargin(0);
  layoutH1->addWidget(filtersTree);
  layoutH1->addLayout(buttonsLayout);

  QPushButton *closeButton = new QPushButton(tr("&Close"), this);
  closeButton->setDefault(true);
  closeButton->setFocus(Qt::OtherFocusReason);
  connect(closeButton, SIGNAL(clicked()), SLOT(close()));
  QHBoxLayout *closeLayout = new QHBoxLayout();
  closeLayout->setAlignment(Qt::AlignRight);
  closeLayout->addWidget(closeButton);

  QVBoxLayout *mainlayout = new QVBoxLayout(this);
  mainlayout->setAlignment(Qt::AlignCenter);
  mainlayout->setMargin(10);
  mainlayout->setSpacing(10);
  mainlayout->addLayout(layoutH1);
  mainlayout->addLayout(closeLayout);

  connect(filtersTree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this, SLOT(slotCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
}

void NewsFiltersDialog::newFilter()
{
  QStringList treeItem;
  treeItem << QString::number(filtersTree->topLevelItemCount())
           << tr("Test filter %1").arg(filtersTree->topLevelItemCount());
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setCheckState(1, Qt::Checked);
  filtersTree->addTopLevelItem(treeWidgetItem);
}

void NewsFiltersDialog::editeFilter()
{

}

void NewsFiltersDialog::deleteFilter()
{
  filtersTree->takeTopLevelItem(filtersTree->currentIndex().row());
}

void NewsFiltersDialog::moveUpFilter()
{
  QTreeWidgetItem *treeWidgetItem = filtersTree->takeTopLevelItem(
        filtersTree->currentIndex().row()-1);
  filtersTree->insertTopLevelItem(filtersTree->currentIndex().row()+1,
                                  treeWidgetItem);
//  if (filtersTree->currentIndex().row() == 0)
}

void NewsFiltersDialog::moveDownFilter()
{
  QTreeWidgetItem *treeWidgetItem = filtersTree->takeTopLevelItem(
        filtersTree->currentIndex().row()+1);
  filtersTree->insertTopLevelItem(filtersTree->currentIndex().row(),
                                  treeWidgetItem);
}

void NewsFiltersDialog::slotCurrentItemChanged(QTreeWidgetItem *current,
                                               QTreeWidgetItem *previous)
{
  qDebug() << filtersTree->indexOfTopLevelItem(current)
           << filtersTree->indexOfTopLevelItem(previous);
//  if (filtersTree->indexOfTopLevelItem(current))
}
