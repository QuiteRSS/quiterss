#include "addfolderdialog.h"

AddFolderDialog::AddFolderDialog(QWidget *parent, QSqlDatabase *db)
  : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint),
    db_(db)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Add Folder"));
  setMinimumWidth(400);
  setMinimumHeight(300);

  nameFeedEdit_ = new LineEdit(this);

  foldersTree_ = new QTreeWidget(this);
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

  QSqlQuery q(*db_);
  QQueue<int> parentIds;
  parentIds.enqueue(0);
  while (!parentIds.empty()) {
    int parentId = parentIds.dequeue();
    QString qStr = QString("SELECT text, id FROM feeds WHERE parentId='%1' AND xmlUrl=''").
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
      parentIds.enqueue(folderId.toInt());
    }
  }

  foldersTree_->expandAll();
  foldersTree_->sortByColumn(0, Qt::AscendingOrder);

  buttonBox_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(false);

  QVBoxLayout *layoutMain = new QVBoxLayout(this);
  layoutMain->addWidget(new QLabel(tr("Name:")));
  layoutMain->addWidget(nameFeedEdit_);
  layoutMain->addWidget(new QLabel(tr("Location:")));
  layoutMain->addWidget(foldersTree_);
  layoutMain->addSpacing(5);
  layoutMain->addWidget(buttonBox_);
  setLayout(layoutMain);

  connect(buttonBox_, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox_, SIGNAL(rejected()), this, SLOT(reject()));
  connect(nameFeedEdit_, SIGNAL(textChanged(const QString&)),
          this, SLOT(nameFeedEditChanged(const QString&)));
}

void AddFolderDialog::nameFeedEditChanged(const QString& text)
{
  buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
}