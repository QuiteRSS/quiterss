#include "newsfiltersdialog.h"

NewsFiltersDialog::NewsFiltersDialog(QWidget *parent) :
    QDialog(parent)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("News filters"));
  setMinimumWidth(400);
  setMinimumHeight(200);

  QTreeWidget *filtersTree = new QTreeWidget();
  filtersTree->setObjectName("filtersTree");
  filtersTree->setColumnCount(2);
  filtersTree->setColumnHidden(0, true);
  filtersTree->header()->setResizeMode(QHeaderView::Stretch);
  filtersTree->header()->setMovable(false);

  QStringList treeItem;
  treeItem << tr("Id") << tr("Name");
  filtersTree->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << "0" << tr("Test filter");
  filtersTree->addTopLevelItem(new QTreeWidgetItem(treeItem));

  QPushButton *newButton = new QPushButton(tr("New..."), this);
  QPushButton *editButton = new QPushButton(tr("Edite..."), this);
  QPushButton *deleteButton = new QPushButton(tr("Delete..."), this);

  QVBoxLayout *buttonsLayout = new QVBoxLayout();
  buttonsLayout->setAlignment(Qt::AlignCenter);
  buttonsLayout->setSpacing(5);
  buttonsLayout->addWidget(newButton);
  buttonsLayout->addWidget(editButton);
  buttonsLayout->addWidget(deleteButton);
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
}
