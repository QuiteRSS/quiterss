/*
    treeitem.cpp

    A container for items of data supplied by the simple tree model.
*/

#include <QStringList>
#include <QDebug>

#include "treeitemdb.h"

//!----------------------------------------------------------------------------
TreeItem::TreeItem(const int id, const int tableRow, TreeItem *parent)
{
  id_ = id;
  tableRow_ = tableRow;
  parentItem = parent;
}

//!----------------------------------------------------------------------------
TreeItem::~TreeItem()
{
  qDeleteAll(childItems);
  childItems.clear();
}

//!----------------------------------------------------------------------------
void TreeItem::appendChild(TreeItem *item)
{
//  childItems.append(item);
}

//!----------------------------------------------------------------------------
void TreeItem::insertChild(int i, TreeItem *item)
{
  childItems.insert(i, item);
}

//!----------------------------------------------------------------------------
TreeItem *TreeItem::child(int row)
{
  return childItems.value(row);
}

//!----------------------------------------------------------------------------
int TreeItem::childCount() const
{
//  return childItems.count();
  return childItems.size();
}

//!----------------------------------------------------------------------------
TreeItem *TreeItem::parent()
{
  return parentItem;
}

//!----------------------------------------------------------------------------
int TreeItem::row() const
{
  if (parentItem)
//    return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));
    return parentItem->childItems.key(const_cast<TreeItem*>(this));

  return 0;
}

