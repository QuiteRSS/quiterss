#include <QtGui>
#include <QtSql>
#include <QDebug>

#include "treeitemdb.h"
#include "treemodeldb.h"

//!----------------------------------------------------------------------------
TreeModel::TreeModel(QObject *parent)
  : QAbstractItemModel(parent)
{
  tableModel = new QSqlTableModel();
  tableModel->setTable("feeds");
  tableModel->select();
  tableModel->setEditStrategy(QSqlTableModel::OnFieldChange);

  rootItem = new TreeItem(0);
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
      (role != Qt::UserRole) && (role != Qt::UserRole+1))
    return QVariant();

  TreeItem *item = getItem(index);

  if (role == Qt::UserRole) {
    return item->id_;
  } else {
    // отфильтровываем строчку по id и берём дату
    QString filterStr = QString("id == '%1'").arg(item->id_);
    tableModel->setFilter(filterStr);
    return tableModel->index(0, index.column() + tableModel->fieldIndex("text")).
        data(role);
  }
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

  // отфильтровываем строчку по id
  QString filterStr = QString("id == '%1'").arg(item->id_);
  tableModel->setFilter(filterStr);

  qDebug() << tableModel->index(0, index.column() + tableModel->fieldIndex("text")).data();
  bool result = tableModel->setData(
      tableModel->index(0, index.column() + tableModel->fieldIndex("text")),
      value, role);
  qDebug() << "setData(): res =" << result << value << role;
  qDebug() << tableModel->index(0, index.column() + tableModel->fieldIndex("text")).data();

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

  QSqlQuery q;

  //! регистрируем "родителей"
  // находим записи с родителем 0, затем идентификаторы записей загоняем в список
  // ищем записи с родителями из списка
  QQueue<int> parentIds;
  parentIds.enqueue(0);
  while (!parentIds.empty()) {

    q.prepare("SELECT id, parentId FROM feeds "
              "WHERE hasChildren=1 AND parentId=:parentId");
    q.bindValue(":parentId", parentIds.dequeue());
    q.exec();
    qDebug() << __FUNCTION__ << q.lastQuery();
    qDebug() << __FUNCTION__ << q.boundValues();

    while (q.next()) {
      int id          = q.value(0).toInt();
      int parentId    = q.value(1).toInt();

      TreeItem *item = new TreeItem(id, parents.value(parentId));
      parents.insert(id, item);

      parentIds.enqueue(id);
      qDebug() << parentIds;
    }
  }

  //! регисрируем "детей"
  q.exec("SELECT id, hasChildren, parentId, rowToParent FROM feeds");
  qDebug() << __FUNCTION__ << q.lastQuery();
  qDebug() << __FUNCTION__ << q.boundValues();

  while (q.next()) {
    int id          = q.value(0).toInt();
    int hasChildren = q.value(1).toInt();
    int parentId    = q.value(2).toInt();
    int rowToParent = q.value(3).toInt();

    // "ребёнка" создаём и прикрепляем к "родителю"
    if (0 == hasChildren) {
      TreeItem *item = new TreeItem(id, parents.value(parentId));
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
