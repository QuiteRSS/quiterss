/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2021 QuiteRSS Team <quiterssteam@gmail.com>
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
#ifndef CATEGORIESTREEWIDGET_H
#define CATEGORIESTREEWIDGET_H

#include <QtGui>
#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif

class CategoriesTreeWidget : public QTreeWidget
{
  Q_OBJECT
public:
  explicit CategoriesTreeWidget(QWidget *parent = 0);

  enum Items {UnreadItem, StarredItem, DeletedItem, LabelsItem};
  enum LabelRole {ImageRole = Qt::UserRole+1, NumRole, colorBgRole, colorTextRole};

  QList<QTreeWidgetItem *> getLabelListItems() const {
    QList<QTreeWidgetItem *> items;
    for (int i = 0; i < topLevelItem(LabelsItem)->childCount(); ++i) {
      items.append(topLevelItem(LabelsItem)->child(i));
    }
    return items;
  }
  QTreeWidgetItem* getLabelItem(int id) const {
    for (int i = 0; i < topLevelItem(LabelsItem)->childCount(); ++i) {
      QTreeWidgetItem *item = topLevelItem(LabelsItem)->child(i);
      if (item->text(2).toInt() == id)
        return item;
    }
    return 0;
  }
  int labelsCount() const {
    return topLevelItem(LabelsItem)->childCount();
  }

signals:
  void signalMiddleClicked();
  void signalClearDeleted();
  void signalMarkRead(QTreeWidgetItem *item);
  void signalViewAllYoutubeVideos();
//  void pressKeyUp();
//  void pressKeyDown();

protected:
  virtual void mousePressEvent(QMouseEvent*);
//  virtual void keyPressEvent(QKeyEvent*);

private slots:
  void showContextMenuCategory(const QPoint &pos);
  void openCategoryNewTab();

  void slotMarkRead();

private:
  QTreeWidgetItem *itemClicked_;

};

#endif // CATEGORIESTREEWIDGET_H
