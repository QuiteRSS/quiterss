#include <QtGui>
#include <QtSql>
#include <QDebug>

#include "treeitemdb.h"
#include "treemodeldb.h"

//!----------------------------------------------------------------------------
TreeModel::TreeModel(QObject *parent)
  : QAbstractItemModel(parent)
{
  tableModel = new QSqlTableModel(this);
  tableModel->setTable("feeds");
  tableModel->select();
  tableModel->setEditStrategy(QSqlTableModel::OnFieldChange);

  rootItem = new TreeItem(0, -1);
  setupModelData(rootItem);
}

//!----------------------------------------------------------------------------
TreeModel::~TreeModel()
{
  delete rootItem;
}

//!----------------------------------------------------------------------------
int TreeModel::columnCount(const QModelIndex &parent) const
{
  return tableModel->columnCount() - tableModel->fieldIndex("text");
}

//!----------------------------------------------------------------------------
QVariant TreeModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if ((role != Qt::DisplayRole) && (role != Qt::EditRole) &&
      (role != Qt::DecorationRole) &&
      (role != Qt::UserRole) && (role != Qt::UserRole+1)) {
    return QVariant();
  }

  TreeItem *item = getItem(index);

  if (role == Qt::UserRole)
    return item->id_;
  if (role == Qt::UserRole+1)
    return item->tableRow_;

  if (role == Qt::DecorationRole) {
    if (index.column() == 0) {
      if (!tableModel->index(item->tableRow_, tableModel->fieldIndex("xmlUrl")).
          data(Qt::EditRole).toString().isEmpty()) {
        QByteArray byteArray = tableModel->index(item->tableRow_, tableModel->fieldIndex("image")).
            data(Qt::EditRole).toByteArray();
        if (!byteArray.isNull()) {
          QPixmap icon;
          if (icon.loadFromData(QByteArray::fromBase64(byteArray))) {
            return icon;
          }
        }
        return QPixmap(":/images/feed");
      } else {
        return QPixmap(":/images/folder");
      }
    }
  }

  return tableModel->index(item->tableRow_, tableModel->fieldIndex("text")).data(role);
}

//!----------------------------------------------------------------------------
Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return 0;

  return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable |
      Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

//!----------------------------------------------------------------------------
TreeItem *TreeModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (item) return item;
    }
    return rootItem;
}

//!----------------------------------------------------------------------------
QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return tableModel->headerData(section + tableModel->fieldIndex("text"),
                                  orientation, role);

  return QVariant();
}

//!----------------------------------------------------------------------------
QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  TreeItem *parentItem;

  if (!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = getItem(parent);

  TreeItem *childItem = parentItem->child(row);
  if (childItem)
    return createIndex(row, column, childItem);
  else
    return QModelIndex();
}

//!----------------------------------------------------------------------------
QModelIndex TreeModel::parent(const QModelIndex &index) const
{
  if (!index.isValid())
    return QModelIndex();

  TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
  TreeItem *parentItem = childItem->parent();

  if (parentItem == rootItem)
    return QModelIndex();

  return createIndex(parentItem->row(), 0, parentItem);
}

//!----------------------------------------------------------------------------
int TreeModel::rowCount(const QModelIndex &parent) const
{
  TreeItem *parentItem;
  if (parent.column() > 0)
    return 0;

  if (!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<TreeItem*>(parent.internalPointer());

  return parentItem->childCount();
}

//!----------------------------------------------------------------------------
bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (role != Qt::EditRole)
      return false;

  TreeItem *item = getItem(index);

  bool result = tableModel->setData(
      tableModel->index(item->tableRow_, tableModel->fieldIndex("text")),
      value, role);

  if (result)
      emit dataChanged(index, index);

  return result;
}

//!----------------------------------------------------------------------------
bool TreeModel::setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role)
{
  if (role != Qt::EditRole || orientation != Qt::Horizontal)
      return false;

  bool result = tableModel-setHeaderData(section + tableModel->fieldIndex("text"),
                                         orientation, value, role);

  if (result)
      emit headerDataChanged(orientation, section, section);

  return result;
}

//!----------------------------------------------------------------------------
void TreeModel::setupModelData(TreeItem *parent)
{
  QMap<int, TreeItem*> parents;
  parents.insert(0, parent);

  //! регистрируем "родителей"
  // находим записи с родителем 0, затем идентификаторы записей загоняем в список
  // ищем записи с родителями из списка
  QQueue<int> parentIds;
  parentIds.enqueue(0);
  while (!parentIds.empty()) {

    int parentId = parentIds.dequeue();
    for (int i = 0; i < tableModel->rowCount(); ++i) {
      if ((tableModel->index(i, tableModel->fieldIndex("hasChildren")).data().toInt() == 1) &&
          (tableModel->index(i, tableModel->fieldIndex("parentId")).data().toInt() == parentId))
      {
        int id = tableModel->index(i, tableModel->fieldIndex("id")).data().toInt();

        TreeItem *item = new TreeItem(id, i, parents.value(parentId));
        parents.insert(id, item);

        parentIds.enqueue(id);
        qDebug() << parentIds;
      }
    }
  }

  //! регисрируем "детей"
  for (int i = 0; i < tableModel->rowCount(); ++i) {
    int id          = tableModel->index(i, tableModel->fieldIndex("id")).data().toInt();
    int hasChildren = tableModel->index(i, tableModel->fieldIndex("hasChildren")).data().toInt();
    int parentId    = tableModel->index(i, tableModel->fieldIndex("parentId")).data().toInt();
    int rowToParent = tableModel->index(i, tableModel->fieldIndex("rowToParent")).data().toInt();

    // "ребёнка" создаём и прикрепляем к "родителю"
    if (0 == hasChildren) {
      TreeItem *item = new TreeItem(id, i, parents.value(parentId));
      parents.value(parentId)->insertChild(rowToParent, item);
    }
    // "ребёнок" является чьим-то "родителем", который уже создан.
    // Прикрепляем его к своему "родителю".
    else {
      parents.value(parentId)->insertChild(rowToParent, parents.value(id));
    }
  }
}

//!-----------------------------------------------------------------------------
Qt::DropActions TreeModel::supportedDropActions() const
{
  return Qt::MoveAction;
}
