#include "filterrulesdialog.h"
#include "newsfiltersdialog.h"
#include "rsslisting.h"

NewsFiltersDialog::NewsFiltersDialog(QWidget *parent, QSettings *settings)
  : QDialog(parent),
    settings_(settings)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("News Filters"));
  setMinimumWidth(500);
  setMinimumHeight(250);

  filtersTree = new QTreeWidget(this);
  filtersTree->setObjectName("filtersTree");
  filtersTree->setColumnCount(4);
  filtersTree->setColumnHidden(0, true);
  filtersTree->setColumnHidden(3, true);
  filtersTree->setSortingEnabled(false);
  filtersTree->header()->resizeSection(1, 150);
  filtersTree->header()->setMovable(false);

  QStringList treeItem;
  treeItem << "Id" << tr("Name Filter") << tr("Feeds") << "Num";
  filtersTree->setHeaderLabels(treeItem);

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  QSqlQuery q(rssl_->db_);
  QString qStr = QString("SELECT id, name, feeds, enable, num FROM filters ORDER BY num");
  q.exec(qStr);
  while (q.next()) {
    QSqlQuery q1(rssl_->db_);
    QString strNameFeeds;
    QStringList strIdFeeds = q.value(2).toString().split(",", QString::SkipEmptyParts);
    foreach (QString strIdFeed, strIdFeeds) {
      if (!strNameFeeds.isNull()) strNameFeeds.append("; ");
      qStr = QString("SELECT text FROM feeds WHERE id==%1 AND xmlUrl!=''").
          arg(strIdFeed);
      q1.exec(qStr);
      if (q1.next()) strNameFeeds.append(q1.value(0).toString());
    }

    treeItem.clear();
    treeItem << q.value(0).toString()
             << q.value(1).toString()
             << strNameFeeds
             << q.value(4).toString();
    QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
    if (q.value(3).toInt() == 1)
      treeWidgetItem->setCheckState(1, Qt::Checked);
    else
      treeWidgetItem->setCheckState(1, Qt::Unchecked);
    filtersTree->addTopLevelItem(treeWidgetItem);

    if (q.value(4).toInt() == 0) {
      qStr = QString("UPDATE filters SET num='%1' WHERE id=='%1'").
          arg(q.value(0).toInt());
      q1.exec(qStr);
      treeWidgetItem->setText(3, q.value(0).toString());
    }
  }

  QPushButton *newButton = new QPushButton(tr("New..."), this);
  connect(newButton, SIGNAL(clicked()), this, SLOT(newFilter()));
  editButton = new QPushButton(tr("Edit..."), this);
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
  buttonsLayout->addSpacing(10);
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

  applyFilterButton = new QPushButton(tr("Apply Selected Filter"), this);
  applyFilterButton->setEnabled(false);
  connect(applyFilterButton, SIGNAL(clicked()), SLOT(applyFilter()));

  QHBoxLayout *mainButtonsLayout = new QHBoxLayout();
  mainButtonsLayout->addWidget(applyFilterButton);
  mainButtonsLayout->addStretch();
  mainButtonsLayout->addWidget(closeButton);

  QVBoxLayout *mainlayout = new QVBoxLayout();
  mainlayout->setMargin(5);
  mainlayout->addLayout(layoutH1);
  mainlayout->addLayout(mainButtonsLayout);
  setLayout(mainlayout);

  connect(filtersTree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this, SLOT(slotCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(filtersTree, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(editFilter()));
  connect(filtersTree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
          this, SLOT(slotItemChanged(QTreeWidgetItem*,int)));
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
        parentWidget(), -1);

  int result = filterRulesDialog->exec();
  if (result == QDialog::Rejected) {
    delete filterRulesDialog;
    return;
  }

  int filterId = filterRulesDialog->filterId_;
  delete filterRulesDialog;

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  QSqlQuery q(rssl_->db_);
  QString qStr = QString("SELECT name, feeds, enable FROM filters WHERE id=='%1'").
      arg(filterId);
  q.exec(qStr);
  if (q.next()) {
    QSqlQuery q1(rssl_->db_);
    QString strNameFeeds;
    QStringList strIdFeeds = q.value(1).toString().split(",", QString::SkipEmptyParts);
    foreach (QString strIdFeed, strIdFeeds) {
      if (!strNameFeeds.isNull()) strNameFeeds.append("; ");
      qStr = QString("SELECT text FROM feeds WHERE id==%1 AND xmlUrl!=''").
          arg(strIdFeed);
      q1.exec(qStr);
      if (q1.next()) strNameFeeds.append(q1.value(0).toString());
    }

    QStringList treeItem;
    treeItem << QString::number(filterId)
             << q.value(0).toString()
             << strNameFeeds;
    QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
    if (q.value(2).toInt() == 1)
      treeWidgetItem->setCheckState(1, Qt::Checked);
    else
      treeWidgetItem->setCheckState(1, Qt::Unchecked);
    filtersTree->addTopLevelItem(treeWidgetItem);

    treeWidgetItem->setText(3, q.value(0).toString());
  }

  if (((filtersTree->currentIndex().row() != (filtersTree->topLevelItemCount()-1))) &&
      filtersTree->currentIndex().isValid())
    moveDownButton->setEnabled(true);

  filtersTree->setCurrentItem(
        filtersTree->topLevelItem(filtersTree->topLevelItemCount()-1));
}

void NewsFiltersDialog::editFilter()
{
  int filterRow = filtersTree->currentIndex().row();
  int filterId = filtersTree->topLevelItem(filterRow)->text(0).toInt();

  FilterRulesDialog *filterRulesDialog = new FilterRulesDialog(
        parentWidget(), filterId);

  int result = filterRulesDialog->exec();
  if (result == QDialog::Rejected) {
    delete filterRulesDialog;
    return;
  }

  delete filterRulesDialog;

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  QSqlQuery q(rssl_->db_);
  QString qStr = QString("SELECT name, feeds FROM filters WHERE id=='%1'").
      arg(filterId);
  q.exec(qStr);
  if (q.next()) {
    QSqlQuery q1(rssl_->db_);
    QString strNameFeeds;
    QStringList strIdFeeds = q.value(1).toString().split(",", QString::SkipEmptyParts);
    foreach (QString strIdFeed, strIdFeeds) {
      if (!strNameFeeds.isNull()) strNameFeeds.append("; ");
      qStr = QString("SELECT text FROM feeds WHERE id==%1 AND xmlUrl!=''").
          arg(strIdFeed);
      q1.exec(qStr);
      if (q1.next()) strNameFeeds.append(q1.value(0).toString());
    }

    filtersTree->topLevelItem(filterRow)->setText(0, QString::number(filterId));
    filtersTree->topLevelItem(filterRow)->setText(1, q.value(0).toString());
    filtersTree->topLevelItem(filterRow)->setText(2, strNameFeeds);
  }
}

void NewsFiltersDialog::deleteFilter()
{
  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle(tr("Delete Filter"));
  msgBox.setText(QString(tr("Are you sure to delete the filter '%1'?")).
                 arg(filtersTree->currentItem()->text(1)));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::No);

  if (msgBox.exec() == QMessageBox::No) return;

  int filterRow = filtersTree->currentIndex().row();
  int filterId = filtersTree->topLevelItem(filterRow)->text(0).toInt();

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  QSqlQuery q(rssl_->db_);
  q.exec(QString("DELETE FROM filters WHERE id='%1'").arg(filterId));
  q.exec(QString("DELETE FROM filterConditions WHERE idFilter='%1'").arg(filterId));
  q.exec(QString("DELETE FROM filterActions WHERE idFilter='%1'").arg(filterId));
  q.exec("VACUUM");
  q.finish();

  filtersTree->takeTopLevelItem(filterRow);

  if (filtersTree->currentIndex().row() == 0)
    moveUpButton->setEnabled(false);
  if (filtersTree->currentIndex().row() == (filtersTree->topLevelItemCount()-1))
    moveDownButton->setEnabled(false);
}

void NewsFiltersDialog::moveUpFilter()
{
  int filterRow = filtersTree->currentIndex().row();

  int num1 = filtersTree->topLevelItem(filterRow)->text(3).toInt();
  int num2 = filtersTree->topLevelItem(filterRow-1)->text(3).toInt();
  filtersTree->topLevelItem(filterRow-1)->setText(3, QString::number(num1));
  filtersTree->topLevelItem(filterRow)->setText(3, QString::number(num2));

  QTreeWidgetItem *treeWidgetItem = filtersTree->takeTopLevelItem(filterRow-1);
  filtersTree->insertTopLevelItem(filterRow, treeWidgetItem);

  if (filtersTree->currentIndex().row() == 0)
    moveUpButton->setEnabled(false);
  if (filtersTree->currentIndex().row() != (filtersTree->topLevelItemCount()-1))
    moveDownButton->setEnabled(true);

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  QSqlQuery q(rssl_->db_);
  int filterId = filtersTree->topLevelItem(filterRow)->text(0).toInt();
  int filterNum = filtersTree->topLevelItem(filterRow)->text(3).toInt();
  QString qStr = QString("UPDATE filters SET num='%1' WHERE id=='%2'").
      arg(filterNum).arg(filterId);
  q.exec(qStr);

  filterId = filtersTree->topLevelItem(filterRow-1)->text(0).toInt();
  filterNum = filtersTree->topLevelItem(filterRow-1)->text(3).toInt();
  qStr = QString("UPDATE filters SET num='%1' WHERE id=='%2'").
      arg(filterNum).arg(filterId);
  q.exec(qStr);
}

void NewsFiltersDialog::moveDownFilter()
{
  int filterRow = filtersTree->currentIndex().row();

  int num1 = filtersTree->topLevelItem(filterRow)->text(3).toInt();
  int num2 = filtersTree->topLevelItem(filterRow+1)->text(3).toInt();
  filtersTree->topLevelItem(filterRow+1)->setText(3, QString::number(num1));
  filtersTree->topLevelItem(filterRow)->setText(3, QString::number(num2));

  QTreeWidgetItem *treeWidgetItem = filtersTree->takeTopLevelItem(filterRow+1);
  filtersTree->insertTopLevelItem(filterRow, treeWidgetItem);

  if (filtersTree->currentIndex().row() == (filtersTree->topLevelItemCount()-1))
    moveDownButton->setEnabled(false);
  if (filtersTree->currentIndex().row() != 0)
    moveUpButton->setEnabled(true);

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  QSqlQuery q(rssl_->db_);
  int filterId = filtersTree->topLevelItem(filterRow)->text(0).toInt();
  int filterNum = filtersTree->topLevelItem(filterRow)->text(3).toInt();
  QString qStr = QString("UPDATE filters SET num='%1' WHERE id=='%2'").
      arg(filterNum).arg(filterId);
  q.exec(qStr);

  filterId = filtersTree->topLevelItem(filterRow+1)->text(0).toInt();
  filterNum = filtersTree->topLevelItem(filterRow+1)->text(3).toInt();
  qStr = QString("UPDATE filters SET num='%1' WHERE id=='%2'").
      arg(filterNum).arg(filterId);
  q.exec(qStr);
}

void NewsFiltersDialog::slotCurrentItemChanged(QTreeWidgetItem *current,
                                               QTreeWidgetItem *)
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
    applyFilterButton->setEnabled(false);
  } else {
    editButton->setEnabled(true);
    deleteButton->setEnabled(true);
    if (current->checkState(1) == Qt::Checked)
      applyFilterButton->setEnabled(true);
    else
      applyFilterButton->setEnabled(false);
  }
}

void NewsFiltersDialog::applyFilter()
{
  int filterRow = filtersTree->currentIndex().row();
  int filterId = filtersTree->topLevelItem(filterRow)->text(0).toInt();
  int feedId = -1;

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  QSqlQuery q(rssl_->db_);
  QString qStr = QString("SELECT feeds FROM filters WHERE id='%1'").
      arg(filterId);
  q.exec(qStr);
  if (q.next()) {
    QStringList strIdFeeds = q.value(0).toString().split(",", QString::SkipEmptyParts);
    foreach (QString strIdFeed, strIdFeeds) {
      rssl_->setUserFilter(strIdFeed.toInt(), filterId);
      NewsTabWidget *widget = qobject_cast<NewsTabWidget*>(rssl_->tabWidget_->currentWidget());
      if (widget->feedId_ == strIdFeed.toInt()) feedId = strIdFeed.toInt();
    }
  }

  if (feedId == -1)
    rssl_->slotUpdateStatus(false);
  else {
    rssl_->slotUpdateNews();
    rssl_->slotUpdateStatus();
  }
}

void NewsFiltersDialog::slotItemChanged(QTreeWidgetItem *item, int column)
{
  if (column == 1) {
    int enable = 0;
    if (item->checkState(1) == Qt::Checked) enable = 1;

    RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
    QSqlQuery q(rssl_->db_);
    QString qStr = QString("UPDATE filters SET enable='%1' WHERE id=='%2'").
        arg(enable).arg(item->text(0).toInt());
    q.exec(qStr);

    if (filtersTree->currentItem() == item)
      applyFilterButton->setEnabled(enable);
  }
}
