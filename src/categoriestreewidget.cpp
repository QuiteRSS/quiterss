/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "categoriestreewidget.h"
#include "delegatewithoutfocus.h"
#include "newstabwidget.h"

#include <QSqlQuery>

CategoriesTreeWidget::CategoriesTreeWidget(QWidget * parent)
  : QTreeWidget(parent)
  , itemClicked_(0)
{
  setObjectName("newsCategoriesTree_");
  setFrameStyle(QFrame::NoFrame);
  setContextMenuPolicy(Qt::CustomContextMenu);
  setStyleSheet(
        QString("#newsCategoriesTree_ {border-top: 1px solid %1;}").
        arg(qApp->palette().color(QPalette::Dark).name()));
  setColumnCount(5);
  setColumnHidden(1, true);
  setColumnHidden(2, true);
  setColumnHidden(3, true);
  header()->hide();
  header()->setResizeMode(0, QHeaderView::Stretch);
  header()->setResizeMode(4, QHeaderView::ResizeToContents);
  header()->setStretchLastSection(false);

  DelegateWithoutFocus *itemDelegate = new DelegateWithoutFocus(this);
  setItemDelegate(itemDelegate);

  QStringList treeItem;
  treeItem.clear();
  treeItem << "Categories" << "Type" << "Id" << "CurrentNews" << "";
  setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << tr("Unread") << QString::number(TAB_CAT_UNREAD) << "-1";
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setIcon(0, QIcon(":/images/images/folder_unread.png"));
  addTopLevelItem(treeWidgetItem);
  treeItem.clear();
  treeItem << tr("Starred") << QString::number(TAB_CAT_STAR) << "-1";
  treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setIcon(0, QIcon(":/images/images/folder_star.png"));
  addTopLevelItem(treeWidgetItem);
  treeItem.clear();
  treeItem << tr("Deleted") << QString::number(TAB_CAT_DEL) << "-1";
  treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setIcon(0, QIcon(":/images/images/trash.png"));
  addTopLevelItem(treeWidgetItem);
  treeItem.clear();
  treeItem << tr("Labels") << QString::number(TAB_CAT_LABEL) << "0";
  treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setIcon(0, QIcon(":/images/label_3"));
  addTopLevelItem(treeWidgetItem);

  QSqlQuery q;
  q.exec("SELECT id, name, image, currentNews FROM labels ORDER BY num");
  while (q.next()) {
    int idLabel = q.value(0).toInt();
    QString nameLabel = q.value(1).toString();
    QByteArray byteArray = q.value(2).toByteArray();
    QString currentNews = q.value(3).toString();
    QPixmap imageLabel;
    if (!byteArray.isNull())
      imageLabel.loadFromData(byteArray);
    treeItem.clear();
    treeItem << nameLabel << QString::number(TAB_CAT_LABEL)
             << QString::number(idLabel) << currentNews;
    QTreeWidgetItem *childItem = new QTreeWidgetItem(treeItem);
    childItem->setIcon(0, QIcon(imageLabel));
    treeWidgetItem->addChild(childItem);
  }

  connect(this, SIGNAL(customContextMenuRequested(QPoint)),
          SLOT(showContextMenuCategory(const QPoint &)));
}

void CategoriesTreeWidget::mousePressEvent(QMouseEvent *event)
{
  QModelIndex index = indexAt(event->pos());
  QRect rectText = visualRect(index);

  if (!index.isValid()) return;
  if (!(event->pos().x() >= rectText.x())) {
    QTreeWidget::mousePressEvent(event);
    return;
  }

  if ((event->buttons() == Qt::MiddleButton)) {
    setCurrentIndex(index);
    emit signalMiddleClicked();
  }
  else if (event->buttons() & Qt::RightButton) {
    return;
  }
  else if (event->buttons() & Qt::LeftButton) {
    QTreeWidget::mousePressEvent(event);
  }
}

/** @brief Call context menu of the categories tree
 *----------------------------------------------------------------------------*/
void CategoriesTreeWidget::showContextMenuCategory(const QPoint &pos)
{
  QModelIndex index = indexAt(pos);
  if (index.isValid()) {
    QRect rectText = visualRect(index);
    if (pos.x() >= rectText.x()) {
      itemClicked_ = itemAt(pos);
      QMenu menu;
      menu.addAction(tr("Open in New Tab"), this, SLOT(openCategoryNewTab()));
      if (itemClicked_ == topLevelItem(2)) {
        menu.addSeparator();
        menu.addAction(tr("Clear 'Deleted'"), this, SIGNAL(signalClearDeleted()));
      }
      menu.exec(viewport()->mapToGlobal(pos));
    }
  }
}

void CategoriesTreeWidget::openCategoryNewTab()
{
  setCurrentItem(itemClicked_);
  emit signalMiddleClicked();
}
