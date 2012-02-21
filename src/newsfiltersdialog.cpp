#include "filterrulesdialog.h"
#include "newsfiltersdialog.h"

NewsFiltersDialog::NewsFiltersDialog(QWidget *parent, QSettings *settings)
  : QDialog(parent),
    settings_(settings)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("News filters"));
  setMinimumWidth(500);
  setMinimumHeight(250);

  filtersTree = new QTreeWidget(this);
  filtersTree->setObjectName("filtersTree");
  filtersTree->setColumnCount(3);
  filtersTree->setColumnHidden(0, true);
  filtersTree->header()->resizeSection(1, 150);
  filtersTree->header()->setMovable(false);

  QStringList treeItem;
  treeItem << tr("Id") << tr("Name filter") << tr("Locations");
  filtersTree->setHeaderLabels(treeItem);

//  treeItem.clear();
//  treeItem << QString::number(filtersTree->topLevelItemCount())
//           << tr("Test filter %1").arg(filtersTree->topLevelItemCount())
//           << "Feed 1";
//  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
//  treeWidgetItem->setCheckState(1, Qt::Checked);
//  filtersTree->addTopLevelItem(treeWidgetItem);

  QPushButton *newButton = new QPushButton(tr("New..."), this);
  connect(newButton, SIGNAL(clicked()), this, SLOT(newFilter()));
  editButton = new QPushButton(tr("Edit.."), this);
  editButton->setEnabled(false);
  connect(editButton, SIGNAL(clicked()), this, SLOT(editFilter()));
  deleteButton = new QPushButton(tr("Delete..."), this);
  deleteButton->setEnabled(false);
  connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteFilter()));

  moveUpButton = new QPushButton(tr("Move up"), this);
  moveUpButton->setEnabled(false);
  connect(moveUpButton, SIGNAL(clicked()), this, SLOT(moveUpFilter()));
  moveDownButton = new QPushButton(tr("Move down"), this);
  moveDownButton->setEnabled(false);
  connect(moveDownButton, SIGNAL(clicked()), this, SLOT(moveDownFilter()));

  QVBoxLayout *buttonsLayout = new QVBoxLayout();
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

  QVBoxLayout *mainlayout = new QVBoxLayout();
  mainlayout->setMargin(5);
  mainlayout->addLayout(layoutH1);
  mainlayout->addLayout(closeLayout);
  setLayout(mainlayout);

  connect(filtersTree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this, SLOT(slotCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(filtersTree, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(editFilter()));
  connect(this, SIGNAL(finished(int)), this, SLOT(closeDialog()));

  restoreGeometry(settings_->value("newsFiltersDlg/geometry").toByteArray());
}

void NewsFiltersDialog::closeDialog()
{
  settings_->setValue("newsFiltersDlg/geometry", saveGeometry());
}

void NewsFiltersDialog::newFilter()
{
  FilterRulesDialog *filterRulesDialog = new FilterRulesDialog(
        this, settings_, &feedsList_);

  int result = filterRulesDialog->exec();
  if (result == QDialog::Rejected) {
    delete filterRulesDialog;
    return;
  }

  QStringList treeItem;
  treeItem << QString::number(filtersTree->topLevelItemCount())
           << filterRulesDialog->filterName->text()
           << "All news";
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setCheckState(1, Qt::Checked);
  filtersTree->addTopLevelItem(treeWidgetItem);

  if (((filtersTree->currentIndex().row() != (filtersTree->topLevelItemCount()-1))) &&
      filtersTree->currentIndex().isValid())
    moveDownButton->setEnabled(true);

  delete filterRulesDialog;
}

void NewsFiltersDialog::editFilter()
{
  FilterRulesDialog *filterRulesDialog = new FilterRulesDialog(
        this, settings_, &feedsList_);

  filterRulesDialog->filterName->setText(filtersTree->currentItem()->text(1));
  filterRulesDialog->filterName->selectAll();
  filterRulesDialog->feedsList_ = feedsList_;

  int result = filterRulesDialog->exec();
  if (result == QDialog::Rejected) {
    delete filterRulesDialog;
    return;
  }

  filtersTree->currentItem()->setText(1, filterRulesDialog->filterName->text());

  delete filterRulesDialog;
}

void NewsFiltersDialog::deleteFilter()
{
  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle(tr("Delete filter"));
  msgBox.setText(QString(tr("Are you sure to delete the filter '%1'?")).
                 arg(filtersTree->currentItem()->text(1)));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::No);

  if (msgBox.exec() == QMessageBox::No) return;

  filtersTree->takeTopLevelItem(filtersTree->currentIndex().row());

  if (filtersTree->currentIndex().row() == 0)
    moveUpButton->setEnabled(false);
  if (filtersTree->currentIndex().row() == (filtersTree->topLevelItemCount()-1))
    moveDownButton->setEnabled(false);

}

void NewsFiltersDialog::moveUpFilter()
{
  QTreeWidgetItem *treeWidgetItem = filtersTree->takeTopLevelItem(
        filtersTree->currentIndex().row()-1);
  filtersTree->insertTopLevelItem(filtersTree->currentIndex().row()+1,
                                  treeWidgetItem);
  if (filtersTree->currentIndex().row() == 0)
    moveUpButton->setEnabled(false);
  if (filtersTree->currentIndex().row() != (filtersTree->topLevelItemCount()-1))
    moveDownButton->setEnabled(true);
}

void NewsFiltersDialog::moveDownFilter()
{
  QTreeWidgetItem *treeWidgetItem = filtersTree->takeTopLevelItem(
        filtersTree->currentIndex().row()+1);
  filtersTree->insertTopLevelItem(filtersTree->currentIndex().row(),
                                  treeWidgetItem);
  if (filtersTree->currentIndex().row() == (filtersTree->topLevelItemCount()-1))
    moveDownButton->setEnabled(false);
  if (filtersTree->currentIndex().row() != 0)
    moveUpButton->setEnabled(true);
}

void NewsFiltersDialog::slotCurrentItemChanged(QTreeWidgetItem *current,
                                               QTreeWidgetItem *previous)
{
  if (filtersTree->indexOfTopLevelItem(current) == 0)
    moveUpButton->setEnabled(false);
  else moveUpButton->setEnabled(true);

  if (filtersTree->indexOfTopLevelItem(current) == (filtersTree->topLevelItemCount()-1))
    moveDownButton->setEnabled(false);
  else moveDownButton->setEnabled(true);

  if (filtersTree->indexOfTopLevelItem(current) < 0) {
    editButton->setEnabled(false);
    deleteButton->setEnabled(false);
    moveUpButton->setEnabled(false);
    moveDownButton->setEnabled(false);
  } else {
    editButton->setEnabled(true);
    deleteButton->setEnabled(true);
  }
}
