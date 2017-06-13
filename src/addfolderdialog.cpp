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
#include "addfolderdialog.h"

AddFolderDialog::AddFolderDialog(QWidget *parent, int curFolderId)
  : Dialog(parent, Qt::MSWindowsFixedSizeDialogHint)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Add Folder"));
  setMinimumWidth(400);
  setMinimumHeight(300);

  nameFeedEdit_ = new LineEdit(this);

  foldersTree_ = new QTreeWidget(this);
  foldersTree_->setObjectName("foldersTree_");
  foldersTree_->setColumnCount(2);
  foldersTree_->setColumnHidden(1, true);
  foldersTree_->header()->hide();

  QStringList treeItem;
  treeItem << tr("Feeds") << "Id";
  foldersTree_->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << tr("All Feeds") << "0";
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setIcon(0, QIcon(":/images/folder"));
  foldersTree_->addTopLevelItem(treeWidgetItem);
  foldersTree_->setCurrentItem(treeWidgetItem);

  QSqlQuery q;
  QQueue<int> parentIds;
  parentIds.enqueue(0);
  while (!parentIds.empty()) {
    int parentId = parentIds.dequeue();
    QString qStr = QString("SELECT text, id FROM feeds WHERE parentId='%1' AND (xmlUrl='' OR xmlUrl IS NULL)").
        arg(parentId);
    q.exec(qStr);
    while (q.next()) {
      QString folderText = q.value(0).toString();
      QString folderId = q.value(1).toString();

      QStringList treeItem;
      treeItem << folderText << folderId;
      QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);

      treeWidgetItem->setIcon(0, QIcon(":/images/folder"));

      QList<QTreeWidgetItem *> treeItems =
            foldersTree_->findItems(QString::number(parentId),
                                    Qt::MatchFixedString | Qt::MatchRecursive,
                                    1);
      treeItems.at(0)->addChild(treeWidgetItem);
      if (folderId.toInt() == curFolderId)
        foldersTree_->setCurrentItem(treeWidgetItem);
      parentIds.enqueue(folderId.toInt());
    }
  }

  foldersTree_->expandAll();
  foldersTree_->sortByColumn(0, Qt::AscendingOrder);

  pageLayout->addWidget(new QLabel(tr("Name:")));
  pageLayout->addWidget(nameFeedEdit_);
  pageLayout->addWidget(new QLabel(tr("Location:")));
  pageLayout->addWidget(foldersTree_);

  buttonBox->addButton(QDialogButtonBox::Ok);
  buttonBox->addButton(QDialogButtonBox::Cancel);
  buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

  connect(nameFeedEdit_, SIGNAL(textChanged(const QString&)),
          this, SLOT(nameFeedEditChanged(const QString&)));
}

void AddFolderDialog::nameFeedEditChanged(const QString& text)
{
  buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
}
