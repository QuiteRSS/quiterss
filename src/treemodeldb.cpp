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
//  return tableModel->columnCount() - tableModel->fieldIndex("text");
  return 4;
}

//!----------------------------------------------------------------------------
QVariant TreeModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if ((role != Qt::FontRole) && (role != Qt::DisplayRole) &&
      (role != Qt::DecorationRole) && (role != Qt::TextColorRole) &&
      (role != Qt::UserRole) && (role != Qt::UserRole+1)) {
    return QVariant();
  }

  TreeItem *item = getItem(index);

  if (role == Qt::UserRole)
    return item->id_;
  if (role == Qt::UserRole+1)
    return item->tableRow_;

  if (role == Qt::FontRole) {
    QFont font = font_;
    if (fieldIndex("text") == index.column()) {
      if (0 < tableModel->index(item->tableRow_, tableModel->fieldIndex("unread")).data(Qt::EditRole).toInt())
        font.setBold(true);
    }
    return font;
  }
  else if (role == Qt::DisplayRole) {
    if (fieldIndex("unread") == index.column()) {
      int unread = tableModel->index(
          item->tableRow_, tableModel->fieldIndex("unread")).data(Qt::EditRole).toInt();
      return (0 == unread) ? QVariant() : QString("(%1)").arg(unread);
    }
    else if (fieldIndex("undeleteCount") == index.column()) {
      int undeleteCount = tableModel->index(
          item->tableRow_, tableModel->fieldIndex("undeleteCount")).data(Qt::EditRole).toInt();
      return (0 == undeleteCount) ? QVariant() : QString("(%1)").arg(undeleteCount);
    }
    else if (fieldIndex("updated") == index.column()) {
      QDateTime dtLocal;
      QString strDate = tableModel->index(
          item->tableRow_, tableModel->fieldIndex("updated")).data(Qt::EditRole).toString();

      if (!strDate.isNull()) {
        QDateTime dtLocalTime = QDateTime::currentDateTime();
        QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
        int nTimeShift = dtLocalTime.secsTo(dtUTC);

        QDateTime dt = QDateTime::fromString(strDate, Qt::ISODate);
        dtLocal = dt.addSecs(nTimeShift);

        if (QDateTime::currentDateTime().date() == dtLocal.date())
          return dtLocal.toString("hh:mm");
        else
          return dtLocal.toString("yyyy.MM.dd");
      } else {
        return QVariant();
      }
    }
  }
  else if (role == Qt::TextColorRole) {
    QBrush brush;
    brush = qApp->palette().brush(QPalette::WindowText);
    if (fieldIndex("unread") == index.column()) {
      brush = qApp->palette().brush(QPalette::Link);
    } else if (fieldIndex("text") == index.column()) {
      if (tableModel->index(item->tableRow_, tableModel->fieldIndex("newCount")).data(Qt::EditRole).toInt() > 0) {
        brush = qApp->palette().brush(QPalette::Link);
      }
    }
    return brush;
  }
  else if (role == Qt::DecorationRole) {
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

  return tableModel->index(
      item->tableRow_, tableModel->fieldIndex(getDBFieldName(index.column()))).data(role);
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
QString TreeModel::getDBFieldName(int col) const
{
  QString fieldName;
  switch (col) {
    case 0 : fieldName = "text"; break;
    case 1 : fieldName = "unread"; break;
    case 2 : fieldName = "undeleteCount"; break;
    case 3 : fieldName = "updated"; break;
    case 4 : fieldName = "id"; break;
    case 5 : fieldName = "title"; break;
    case 6 : fieldName = "description"; break;
    case 7 : fieldName = "currentNews"; break;
    case 8 : fieldName = "image"; break;
    case 9 : fieldName = "newCount"; break;
  }
  return fieldName;
}

//!----------------------------------------------------------------------------
QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return tableModel->headerData(section, orientation, role);

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
      tableModel->index(item->tableRow_, tableModel->fieldIndex(getDBFieldName(index.column()))),
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

//!-----------------------------------------------------------------------------
int TreeModel::fieldIndex(const QString &fieldName) const
{
//  return tableModel->fieldIndex(fieldName);
  // Замена первых 4-х индексов не необходимые
  if (fieldName == "text")          return 0;
  if (fieldName == "unread")        return 1;
  if (fieldName == "undeleteCount") return 2;
  if (fieldName == "updated")       return 3;
  if (fieldName == "id")            return 4;
  if (fieldName == "title")         return 5;
  if (fieldName == "description")   return 6;
  if (fieldName == "currentNews")   return 7;
  if (fieldName == "image")         return 8;
  if (fieldName == "newCount")      return 9;

  return -1;
}

//!-----------------------------------------------------------------------------
QSqlRecord TreeModel::record(int row) const
{
  return tableModel->record(row);
}

//!-----------------------------------------------------------------------------
bool TreeModel::select(QString filter)
{
  tableModel->setFilter(filter);

//  bool result = tableModel->select();
  return tableModel->select();

//  TreeItem *newRootItem = new TreeItem(0, -1);
//  setupModelData(newRootItem);

//  delete rootItem;
//  rootItem = newRootItem;

//  return result;
  return true;
}

//!-----------------------------------------------------------------------------
void TreeModel::setFilter(const QString &filter)
{
  select(filter);
}

