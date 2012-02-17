#include "filterrulesdialog.h"

FilterRulesDialog::FilterRulesDialog(QWidget *parent) :
  QDialog(parent)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Filter rules"));
  setMinimumWidth(400);
  setMinimumHeight(250);

//  filtersTree = new QTreeWidget();
//  filtersTree->setObjectName("filtersTree");
//  filtersTree->setColumnCount(2);
//  filtersTree->setColumnHidden(0, true);
//  filtersTree->header()->setResizeMode(QHeaderView::Stretch);
//  filtersTree->header()->setMovable(false);

//  QStringList treeItem;
//  treeItem << tr("Id") << tr("Name");
//  filtersTree->setHeaderLabels(treeItem);

//  treeItem.clear();
//  treeItem << QString::number(filtersTree->topLevelItemCount())
//           << tr("Test filter %1").arg(filtersTree->topLevelItemCount());
//  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
//  treeWidgetItem->setCheckState(1, Qt::Checked);
//  filtersTree->addTopLevelItem(treeWidgetItem);

//  QPushButton *newButton = new QPushButton(tr("New..."), this);
//  connect(newButton, SIGNAL(clicked()), this, SLOT(newFilter()));
//  editButton = new QPushButton(tr("Edite..."), this);
//  editButton->setEnabled(false);
//  connect(editButton, SIGNAL(clicked()), this, SLOT(editeFilter()));
//  deleteButton = new QPushButton(tr("Delete..."), this);
//  deleteButton->setEnabled(false);
//  connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteFilter()));

//  moveUpButton = new QPushButton(tr("Move up"), this);
//  moveUpButton->setEnabled(false);
//  connect(moveUpButton, SIGNAL(clicked()), this, SLOT(moveUpFilter()));
//  moveDownButton = new QPushButton(tr("Move down"), this);
//  moveDownButton->setEnabled(false);
//  connect(moveDownButton, SIGNAL(clicked()), this, SLOT(moveDownFilter()));

//  QVBoxLayout *buttonsLayout = new QVBoxLayout();
//  buttonsLayout->setAlignment(Qt::AlignCenter);
//  buttonsLayout->setSpacing(5);
//  buttonsLayout->addWidget(newButton);
//  buttonsLayout->addWidget(editButton);
//  buttonsLayout->addWidget(deleteButton);
//  buttonsLayout->addSpacing(20);
//  buttonsLayout->addWidget(moveUpButton);
//  buttonsLayout->addWidget(moveDownButton);
//  buttonsLayout->addStretch();

//  QHBoxLayout *layoutH1 = new QHBoxLayout();
//  layoutH1->setMargin(0);
//  layoutH1->addWidget(filtersTree);
//  layoutH1->addLayout(buttonsLayout);

  buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  QVBoxLayout *mainlayout = new QVBoxLayout(this);
  mainlayout->setAlignment(Qt::AlignCenter);
  mainlayout->addStretch();
  mainlayout->setMargin(5);
//  mainlayout->addLayout(layoutH1);
  mainlayout->addWidget(buttonBox);
}
