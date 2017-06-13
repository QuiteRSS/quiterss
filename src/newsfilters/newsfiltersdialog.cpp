/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "newsfiltersdialog.h"

#include "mainapplication.h"
#include "filterrulesdialog.h"
#include "parseobject.h"
#include "settings.h"

NewsFiltersDialog::NewsFiltersDialog(QWidget *parent)
  : Dialog(parent, Qt::WindowMinMaxButtonsHint)
{
  setWindowTitle(tr("News Filters"));
  setMinimumWidth(500);
  setMinimumHeight(300);

  filtersTree_ = new QTreeWidget(this);
  filtersTree_->setObjectName("filtersTree");
  filtersTree_->setColumnCount(4);
  filtersTree_->setColumnHidden(0, true);
  filtersTree_->setColumnHidden(3, true);
  filtersTree_->setSortingEnabled(false);
  filtersTree_->header()->resizeSection(1, 150);
#ifdef HAVE_QT5
  filtersTree_->header()->setSectionsMovable(false);
#else
  filtersTree_->header()->setMovable(false);
#endif

  QStringList treeItem;
  treeItem << "Id" << tr("Filter Name") << tr("Feeds") << "Num";
  filtersTree_->setHeaderLabels(treeItem);

  QSqlQuery q;
  QString qStr = QString("SELECT id, name, feeds, enable, num FROM filters ORDER BY num");
  q.exec(qStr);
  while (q.next()) {
    QSqlQuery q1;
    bool isFolder = false;
    QString strNameFeeds;
    QStringList strIdFeeds = q.value(2).toString().split(",", QString::SkipEmptyParts);
    foreach (QString strIdFeed, strIdFeeds) {
      if (isFolder) strNameFeeds.append("; ");
      qStr = QString("SELECT text FROM feeds WHERE id==%1 AND xmlUrl!=''").
          arg(strIdFeed);
      q1.exec(qStr);
      if (q1.next()) {
        strNameFeeds.append(q1.value(0).toString());
        isFolder = true;
      } else {
        isFolder = false;
      }
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
    treeWidgetItem->setToolTip(2, strNameFeeds);
    filtersTree_->addTopLevelItem(treeWidgetItem);

    if (q.value(4).toInt() == 0) {
      qStr = QString("UPDATE filters SET num='%1' WHERE id=='%1'").
          arg(q.value(0).toInt());
      q1.exec(qStr);
      treeWidgetItem->setText(3, q.value(0).toString());
    }
  }

  QPushButton *newButton = new QPushButton(tr("New..."), this);
  connect(newButton, SIGNAL(clicked()), this, SLOT(newFilter()));
  editButton_ = new QPushButton(tr("Edit..."), this);
  editButton_->setEnabled(false);
  connect(editButton_, SIGNAL(clicked()), this, SLOT(editFilter()));
  deleteButton_ = new QPushButton(tr("Delete..."), this);
  deleteButton_->setEnabled(false);
  connect(deleteButton_, SIGNAL(clicked()), this, SLOT(deleteFilter()));

  moveUpButton_ = new QPushButton(tr("Move up"), this);
  moveUpButton_->setEnabled(false);
  connect(moveUpButton_, SIGNAL(clicked()), this, SLOT(moveUpFilter()));
  moveDownButton_ = new QPushButton(tr("Move down"), this);
  moveDownButton_->setEnabled(false);
  connect(moveDownButton_, SIGNAL(clicked()), this, SLOT(moveDownFilter()));

  runFilterButton_ = new QPushButton(tr("Run Filter"), this);
  runFilterButton_->setEnabled(false);
  buttonsLayout->insertWidget(0, runFilterButton_);
  connect(runFilterButton_, SIGNAL(clicked()), SLOT(applyFilter()));

  QVBoxLayout *buttonsVLayout = new QVBoxLayout();
  buttonsVLayout->addWidget(newButton);
  buttonsVLayout->addWidget(editButton_);
  buttonsVLayout->addWidget(deleteButton_);
  buttonsVLayout->addSpacing(10);
  buttonsVLayout->addWidget(moveUpButton_);
  buttonsVLayout->addWidget(moveDownButton_);
  buttonsVLayout->addStretch();
  buttonsVLayout->addWidget(runFilterButton_);

  QHBoxLayout *mainlayout = new QHBoxLayout();
  mainlayout->setMargin(0);
  mainlayout->addWidget(filtersTree_);
  mainlayout->addLayout(buttonsVLayout);

  pageLayout->addLayout(mainlayout);

  buttonBox->addButton(QDialogButtonBox::Close);

  filtersTree_->setCurrentIndex(QModelIndex());

  connect(filtersTree_, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this, SLOT(slotCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(filtersTree_, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(editFilter()));
  connect(filtersTree_, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
          this, SLOT(slotItemChanged(QTreeWidgetItem*,int)));
  connect(this, SIGNAL(finished(int)), this, SLOT(closeDialog()));

  Settings settings;
  restoreGeometry(settings.value("newsFiltersDlg/geometry").toByteArray());
}

void NewsFiltersDialog::closeDialog()
{
  Settings settings;
  settings.setValue("newsFiltersDlg/geometry", saveGeometry());
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

  QSqlQuery q;
  QString qStr = QString("SELECT name, feeds, enable FROM filters WHERE id=='%1'").
      arg(filterId);
  q.exec(qStr);
  if (q.next()) {
    QSqlQuery q1;
    bool isFolder = false;
    QString strNameFeeds;
    QStringList strIdFeeds = q.value(1).toString().split(",", QString::SkipEmptyParts);
    foreach (QString strIdFeed, strIdFeeds) {
      if (isFolder) strNameFeeds.append("; ");
      qStr = QString("SELECT text FROM feeds WHERE id==%1 AND xmlUrl!=''").
          arg(strIdFeed);
      q1.exec(qStr);
      if (q1.next()) {
        strNameFeeds.append(q1.value(0).toString());
        isFolder = true;
      } else {
        isFolder = false;
      }
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
    treeWidgetItem->setToolTip(2, strNameFeeds);
    filtersTree_->addTopLevelItem(treeWidgetItem);

    treeWidgetItem->setText(3, q.value(0).toString());
  }

  if (((filtersTree_->currentIndex().row() != (filtersTree_->topLevelItemCount()-1))) &&
      filtersTree_->currentIndex().isValid())
    moveDownButton_->setEnabled(true);

  filtersTree_->setCurrentItem(
        filtersTree_->topLevelItem(filtersTree_->topLevelItemCount()-1));
}

void NewsFiltersDialog::editFilter()
{
  int filterRow = filtersTree_->currentIndex().row();
  int filterId = filtersTree_->topLevelItem(filterRow)->text(0).toInt();

  FilterRulesDialog *filterRulesDialog = new FilterRulesDialog(
        parentWidget(), filterId);

  int result = filterRulesDialog->exec();
  if (result == QDialog::Rejected) {
    delete filterRulesDialog;
    return;
  }

  delete filterRulesDialog;

  QSqlQuery q;
  QString qStr = QString("SELECT name, feeds FROM filters WHERE id=='%1'").
      arg(filterId);
  q.exec(qStr);
  if (q.next()) {
    QSqlQuery q1;
    bool isFolder = false;
    QString strNameFeeds;
    QStringList strIdFeeds = q.value(1).toString().split(",", QString::SkipEmptyParts);
    foreach (QString strIdFeed, strIdFeeds) {
      if (isFolder) strNameFeeds.append("; ");
      qStr = QString("SELECT text FROM feeds WHERE id==%1 AND xmlUrl!=''").
          arg(strIdFeed);
      q1.exec(qStr);
      if (q1.next()) {
        strNameFeeds.append(q1.value(0).toString());
        isFolder = true;
      } else {
        isFolder = false;
      }
    }

    filtersTree_->topLevelItem(filterRow)->setText(0, QString::number(filterId));
    filtersTree_->topLevelItem(filterRow)->setText(1, q.value(0).toString());
    filtersTree_->topLevelItem(filterRow)->setText(2, strNameFeeds);
    filtersTree_->topLevelItem(filterRow)->setToolTip(2, strNameFeeds);
  }
}

void NewsFiltersDialog::deleteFilter()
{
  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle(tr("Delete Filter"));
  msgBox.setText(QString(tr("Are you sure you want to delete the filter '%1'?")).
                 arg(filtersTree_->currentItem()->text(1)));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::No);

  if (msgBox.exec() == QMessageBox::No) return;

  int filterRow = filtersTree_->currentIndex().row();
  int filterId = filtersTree_->topLevelItem(filterRow)->text(0).toInt();

  QSqlQuery q;
  q.exec(QString("DELETE FROM filters WHERE id='%1'").arg(filterId));
  q.exec(QString("DELETE FROM filterConditions WHERE idFilter='%1'").arg(filterId));
  q.exec(QString("DELETE FROM filterActions WHERE idFilter='%1'").arg(filterId));
  q.finish();

  filtersTree_->takeTopLevelItem(filterRow);

  if (filtersTree_->currentIndex().row() == 0)
    moveUpButton_->setEnabled(false);
  if (filtersTree_->currentIndex().row() == (filtersTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(false);
}

void NewsFiltersDialog::moveUpFilter()
{
  int filterRow = filtersTree_->currentIndex().row();

  int num1 = filtersTree_->topLevelItem(filterRow)->text(3).toInt();
  int num2 = filtersTree_->topLevelItem(filterRow-1)->text(3).toInt();
  filtersTree_->topLevelItem(filterRow-1)->setText(3, QString::number(num1));
  filtersTree_->topLevelItem(filterRow)->setText(3, QString::number(num2));

  QTreeWidgetItem *treeWidgetItem = filtersTree_->takeTopLevelItem(filterRow-1);
  filtersTree_->insertTopLevelItem(filterRow, treeWidgetItem);

  if (filtersTree_->currentIndex().row() == 0)
    moveUpButton_->setEnabled(false);
  if (filtersTree_->currentIndex().row() != (filtersTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(true);

  QSqlQuery q;
  int filterId = filtersTree_->topLevelItem(filterRow)->text(0).toInt();
  int filterNum = filtersTree_->topLevelItem(filterRow)->text(3).toInt();
  QString qStr = QString("UPDATE filters SET num='%1' WHERE id=='%2'").
      arg(filterNum).arg(filterId);
  q.exec(qStr);

  filterId = filtersTree_->topLevelItem(filterRow-1)->text(0).toInt();
  filterNum = filtersTree_->topLevelItem(filterRow-1)->text(3).toInt();
  qStr = QString("UPDATE filters SET num='%1' WHERE id=='%2'").
      arg(filterNum).arg(filterId);
  q.exec(qStr);
}

void NewsFiltersDialog::moveDownFilter()
{
  int filterRow = filtersTree_->currentIndex().row();

  int num1 = filtersTree_->topLevelItem(filterRow)->text(3).toInt();
  int num2 = filtersTree_->topLevelItem(filterRow+1)->text(3).toInt();
  filtersTree_->topLevelItem(filterRow+1)->setText(3, QString::number(num1));
  filtersTree_->topLevelItem(filterRow)->setText(3, QString::number(num2));

  QTreeWidgetItem *treeWidgetItem = filtersTree_->takeTopLevelItem(filterRow+1);
  filtersTree_->insertTopLevelItem(filterRow, treeWidgetItem);

  if (filtersTree_->currentIndex().row() == (filtersTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(false);
  if (filtersTree_->currentIndex().row() != 0)
    moveUpButton_->setEnabled(true);

  QSqlQuery q;
  int filterId = filtersTree_->topLevelItem(filterRow)->text(0).toInt();
  int filterNum = filtersTree_->topLevelItem(filterRow)->text(3).toInt();
  QString qStr = QString("UPDATE filters SET num='%1' WHERE id=='%2'").
      arg(filterNum).arg(filterId);
  q.exec(qStr);

  filterId = filtersTree_->topLevelItem(filterRow+1)->text(0).toInt();
  filterNum = filtersTree_->topLevelItem(filterRow+1)->text(3).toInt();
  qStr = QString("UPDATE filters SET num='%1' WHERE id=='%2'").
      arg(filterNum).arg(filterId);
  q.exec(qStr);
}

void NewsFiltersDialog::slotCurrentItemChanged(QTreeWidgetItem *current,
                                               QTreeWidgetItem *)
{
  if (filtersTree_->indexOfTopLevelItem(current) == 0)
    moveUpButton_->setEnabled(false);
  else moveUpButton_->setEnabled(true);

  if (filtersTree_->indexOfTopLevelItem(current) == (filtersTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(false);
  else moveDownButton_->setEnabled(true);

  if (filtersTree_->indexOfTopLevelItem(current) < 0) {
    editButton_->setEnabled(false);
    deleteButton_->setEnabled(false);
    moveUpButton_->setEnabled(false);
    moveDownButton_->setEnabled(false);
    runFilterButton_->setEnabled(false);
  } else {
    editButton_->setEnabled(true);
    deleteButton_->setEnabled(true);
    runFilterButton_->setEnabled(true);
  }
}

void NewsFiltersDialog::applyFilter()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  int filterRow = filtersTree_->currentIndex().row();
  int filterId = filtersTree_->topLevelItem(filterRow)->text(0).toInt();
  int feedId = -1;

  MainWindow *mainWindow = mainApp->mainWindow();
  QSqlQuery q;
  QString qStr = QString("SELECT feeds FROM filters WHERE id='%1'").
      arg(filterId);
  q.exec(qStr);
  if (q.first()) {
    QStringList strIdFeeds = q.value(0).toString().split(",", QString::SkipEmptyParts);
    q.finish();
    foreach (QString strIdFeed, strIdFeeds) {
      mainApp->runUserFilter(strIdFeed.toInt(), filterId);
      NewsTabWidget *widget = qobject_cast<NewsTabWidget*>(mainWindow->stackedWidget_->currentWidget());
      if (widget->feedId_ == strIdFeed.toInt()) feedId = strIdFeed.toInt();
    }
  }

  if (feedId != -1)
    mainWindow->slotUpdateNews(NewsTabWidget::RefreshAll);
  mainWindow->slotUpdateStatus(feedId);
  mainWindow->recountCategoryCounts();

  QApplication::restoreOverrideCursor();
}

void NewsFiltersDialog::slotItemChanged(QTreeWidgetItem *item, int column)
{
  if (column == 1) {
    int enable = 0;
    if (item->checkState(1) == Qt::Checked) enable = 1;

    QSqlQuery q;
    QString qStr = QString("UPDATE filters SET enable='%1' WHERE id=='%2'").
        arg(enable).arg(item->text(0).toInt());
    q.exec(qStr);
  }
}
