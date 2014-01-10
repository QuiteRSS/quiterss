/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2014 QuiteRSS Team <quiterssteam@gmail.com>
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
#include "customizetoolbardialog.h"
#include "rsslisting.h"

CustomizeToolbarDialog::CustomizeToolbarDialog(QWidget *parent, QToolBar *toolbar)
  : Dialog(parent),
    toolbar_(toolbar)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setMinimumWidth(300);
  setMinimumHeight(400);

  if (toolbar_->objectName() == "ToolBar_General")
    setWindowTitle(tr("Customize Main Toolbar"));
  else if (toolbar_->objectName() == "feedsToolBar")
    setWindowTitle(tr("Customize Feeds Toolbar"));
  else if (toolbar_->objectName() == "newsToolBar")
    setWindowTitle(tr("Customize News Toolbar"));

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  settings_ = rssl_->settings_;

  shortcutTree_ = new QTreeWidget(this);
  shortcutTree_->setObjectName("actionTree");
  shortcutTree_->setIndentation(0);
  shortcutTree_->setColumnCount(2);
  shortcutTree_->setColumnHidden(1, true);
  shortcutTree_->setSortingEnabled(false);
  shortcutTree_->setHeaderHidden(true);
#ifdef HAVE_QT5
  shortcutTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
#else
  shortcutTree_->header()->setResizeMode(0, QHeaderView::Stretch);
#endif

  QStringList treeItem;
  treeItem << "Action" << "ObjectName";
  shortcutTree_->setHeaderLabels(treeItem);

  QListIterator<QAction *> iter(toolbar_->actions());
  while (iter.hasNext()) {
    QAction *pAction = iter.next();

    if ((pAction->objectName() == "restoreNewsAct") ||
        (pAction->objectName() == "separatorRAct")) {
      continue;
    }

    treeItem.clear();
    treeItem << pAction->text().remove("&") << pAction->objectName();
    QTreeWidgetItem *item = new QTreeWidgetItem(treeItem);

    if (pAction->icon().isNull())
      item->setIcon(0, QIcon(":/images/images/noicon.png"));
    else {
      if (pAction->objectName() == "autoLoadImagesToggle") {
        item->setIcon(0, QIcon(":/images/imagesOn"));
        item->setText(0, tr("Load images"));
      } else if ((pAction->objectName() == "feedsFilter") ||
                 (pAction->objectName() == "newsFilter")) {
        item->setIcon(0, QIcon(":/images/filterOn"));
      } else item->setIcon(0, pAction->icon());
    }

    if (pAction->objectName().isEmpty()) {
      item->setText(0, tr("Separator"));
      item->setText(1, "Separator");
      item->setBackground(0, qApp->palette().brush(QPalette::Midlight));
    }

    shortcutTree_->addTopLevelItem(item);
  }

  styleBox_ = new QComboBox(this);
  treeItem.clear();
  treeItem << tr("Icon") << tr("Text") << tr("Text Beside Icon") << tr("Text Under Icon");
  styleBox_->addItems(treeItem);
  QString styleStr = settings_->value("Settings/toolBarStyle", "toolBarStyleTuI_").toString();
  if (styleStr == "toolBarStyleI_") {
    styleBox_->setCurrentIndex(0);
  } else if (styleStr == "toolBarStyleT_") {
    styleBox_->setCurrentIndex(1);
  } else if (styleStr == "toolBarStyleTbI_") {
    styleBox_->setCurrentIndex(2);
  } else {
    styleBox_->setCurrentIndex(3);
  }

  iconBox_ = new QComboBox(this);
  treeItem.clear();
  treeItem << tr("Big") << tr("Normal") << tr("Small");
  iconBox_->addItems(treeItem);
  QString iconStr = settings_->value("Settings/toolBarIconSize", "toolBarIconNormal_").toString();
  if (iconStr == "toolBarIconBig_") {
    iconBox_->setCurrentIndex(0);
  } else if (iconStr == "toolBarIconSmall_") {
    iconBox_->setCurrentIndex(2);
  } else {
    iconBox_->setCurrentIndex(1);
  }

  QHBoxLayout *styleLayout = new QHBoxLayout();
  styleLayout->setMargin(0);
  styleLayout->addWidget(new QLabel(tr("Style:")));
  styleLayout->addWidget(styleBox_);
  styleLayout->addSpacing(10);
  styleLayout->addWidget(new QLabel(tr("Icon Size:")));
  styleLayout->addWidget(iconBox_);
  styleLayout->addStretch();

  QWidget *styleWidget = new QWidget(this);
  styleWidget->setLayout(styleLayout);

  if (toolbar_->objectName() != "ToolBar_General") {
    styleWidget->hide();
  }

  QVBoxLayout *mainVLayout = new QVBoxLayout();
  mainVLayout->addWidget(shortcutTree_, 1);
  mainVLayout->addWidget(styleWidget);

  addButtonMenu_ = new QMenu(this);
  addButton_ = new QPushButton(tr("Add"));
  addButton_->setMenu(addButtonMenu_);
  connect(addButtonMenu_, SIGNAL(aboutToShow()),
          this, SLOT(showMenuAddButton()));
  connect(addButtonMenu_, SIGNAL(triggered(QAction*)),
          this, SLOT(addShortcut(QAction*)));

  removeButton_ = new QPushButton(tr("Remove"));
  removeButton_->setEnabled(false);
  connect(removeButton_, SIGNAL(clicked()), this, SLOT(removeShortcut()));

  moveUpButton_ = new QPushButton(tr("Move up"));
  moveUpButton_->setEnabled(false);
  connect(moveUpButton_, SIGNAL(clicked()), this, SLOT(moveUpShortcut()));
  moveDownButton_ = new QPushButton(tr("Move down"));
  moveDownButton_->setEnabled(false);
  connect(moveDownButton_, SIGNAL(clicked()), this, SLOT(moveDownShortcut()));

  QPushButton *defaultButton = new QPushButton(tr("Default"));
  connect(defaultButton, SIGNAL(clicked()), this, SLOT(defaultShortcut()));

  QVBoxLayout *buttonsVLayout = new QVBoxLayout();
  buttonsVLayout->addWidget(addButton_);
  buttonsVLayout->addWidget(removeButton_);
  buttonsVLayout->addSpacing(10);
  buttonsVLayout->addWidget(moveUpButton_);
  buttonsVLayout->addWidget(moveDownButton_);
  buttonsVLayout->addStretch();

  QHBoxLayout *mainlayout = new QHBoxLayout();
  mainlayout->setMargin(0);
  mainlayout->addLayout(mainVLayout);
  mainlayout->addLayout(buttonsVLayout);

  pageLayout->addLayout(mainlayout);

  buttonsLayout->insertWidget(0, defaultButton);

  buttonBox->addButton(QDialogButtonBox::Ok);
  buttonBox->addButton(QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptDialog()));

  connect(shortcutTree_, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this, SLOT(slotCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(this, SIGNAL(finished(int)), this, SLOT(closeDialog()));

  restoreGeometry(settings_->value("customizeToolbarDlg/geometry").toByteArray());
}

void CustomizeToolbarDialog::acceptDialog()
{
  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());

  if (toolbar_->objectName() == "newsToolBar") {
    for (int i = 0; i < rssl_->stackedWidget_->count(); i++) {
      NewsTabWidget *widget = (NewsTabWidget*)rssl_->stackedWidget_->widget(i);
      QListIterator<QAction *> iter(widget->newsToolBar_->actions());
      while (iter.hasNext()) {
        QAction *pAction = iter.next();
        if (pAction->objectName().isEmpty()) {
          delete pAction;
        }
      }
      widget->newsToolBar_->clear();
    }
  } else {
    QListIterator<QAction *> iter(toolbar_->actions());
    while (iter.hasNext()) {
      QAction *pAction = iter.next();
      if (pAction->objectName().isEmpty()) {
        delete pAction;
      }
    }
    toolbar_->clear();
  }

  QString str;
  for (int i = 0; i < shortcutTree_->topLevelItemCount(); i++) {
    if (!str.isEmpty()) str.append(",");

    if (shortcutTree_->topLevelItem(i)->text(1) == "Separator") {
      str.append("Separator");
      continue;
    }

    QListIterator<QAction *> iter(rssl_->actions());
    while (iter.hasNext()) {
      QAction *pAction = iter.next();
      if (!pAction->icon().isNull()) {
        if (pAction->objectName() == shortcutTree_->topLevelItem(i)->text(1)) {
          str.append(pAction->objectName());
          break;
        }
      }
    }
  }

  if (toolbar_->objectName() != "newsToolBar") {
    foreach (QString actionStr, str.split(",", QString::SkipEmptyParts)) {
      if (actionStr == "Separator") {
        toolbar_->addSeparator();
      } else {
        QListIterator<QAction *> iter(rssl_->actions());
        while (iter.hasNext()) {
          QAction *pAction = iter.next();
          if (!pAction->icon().isNull()) {
            if (pAction->objectName() == actionStr) {
              toolbar_->addAction(pAction);
              break;
            }
          }
        }
      }
    }
  }

  if (toolbar_->objectName() == "ToolBar_General") {
    settings_->setValue("Settings/mainToolBar", str);

    switch (styleBox_->currentIndex()) {
    case 0: str = "toolBarStyleI_"; break;
    case 1: str = "toolBarStyleT_"; break;
    case 2: str = "toolBarStyleTbI_"; break;
    default: str = "toolBarStyleTuI_";
    }
    settings_->setValue("Settings/toolBarStyle", str);
    rssl_->setToolBarStyle(str);

    switch (iconBox_->currentIndex()) {
    case 0: str = "toolBarIconBig_"; break;
    case 2: str = "toolBarIconSmall_"; break;
    default: str = "toolBarIconNormal_";
    }
    settings_->setValue("Settings/toolBarIconSize", str);
    rssl_->setToolBarIconSize(str);
  } else if (toolbar_->objectName() == "feedsToolBar") {
    settings_->setValue("Settings/feedsToolBar", str);
  } else if (toolbar_->objectName() == "newsToolBar") {
    settings_->setValue("Settings/newsToolBar", str);

    for (int i = 0; i < rssl_->stackedWidget_->count(); i++) {
      NewsTabWidget *widget = (NewsTabWidget*)rssl_->stackedWidget_->widget(i);
      foreach (QString actionStr, str.split(",", QString::SkipEmptyParts)) {
        if (actionStr == "Separator") {
          widget->newsToolBar_->addSeparator();
        } else {
          QListIterator<QAction *> iter(rssl_->actions());
          while (iter.hasNext()) {
            QAction *pAction = iter.next();
            if (!pAction->icon().isNull()) {
              if (pAction->objectName() == actionStr) {
                widget->newsToolBar_->addAction(pAction);
                break;
              }
            }
          }
        }
      }
      widget->newsToolBar_->addAction(widget->separatorRAct_);
      widget->newsToolBar_->addAction(rssl_->restoreNewsAct_);
    }
  }

  accept();
}

void CustomizeToolbarDialog::closeDialog()
{
  settings_->setValue("customizeToolbarDlg/geometry", saveGeometry());
}

void CustomizeToolbarDialog::slotCurrentItemChanged(QTreeWidgetItem *current,
                                               QTreeWidgetItem *)
{
  if (shortcutTree_->indexOfTopLevelItem(current) == 0)
    moveUpButton_->setEnabled(false);
  else moveUpButton_->setEnabled(true);

  if (shortcutTree_->indexOfTopLevelItem(current) == (shortcutTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(false);
  else moveDownButton_->setEnabled(true);

  if (shortcutTree_->indexOfTopLevelItem(current) < 0) {
    removeButton_->setEnabled(false);
    moveUpButton_->setEnabled(false);
    moveDownButton_->setEnabled(false);
  } else {
    removeButton_->setEnabled(true);
  }
}

void CustomizeToolbarDialog::showMenuAddButton()
{
  addButtonMenu_->clear();

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
  QListIterator<QAction *> iter(rssl_->actions());
  while (iter.hasNext()) {
    QAction *pAction = iter.next();
    if ((pAction->objectName() == "restoreNewsAct") ||
        pAction->objectName().contains("labelAction_")) continue;

    if (!pAction->icon().isNull()) {
      QList<QTreeWidgetItem *> treeItems = shortcutTree_->findItems(pAction->objectName(),
                                                                    Qt::MatchFixedString,
                                                                    1);
      if (!treeItems.count()) {
        QAction *action = addButtonMenu_->addAction(pAction->icon(), pAction->text());
        action->setObjectName(pAction->objectName());

        if (pAction->objectName() == "autoLoadImagesToggle") {
          action->setIcon(QIcon(":/images/imagesOn"));
          action->setText(tr("Load images"));
        } else if ((pAction->objectName() == "feedsFilter") ||
                   (pAction->objectName() == "newsFilter")) {
          action->setIcon(QIcon(":/images/filterOn"));
        } else action->setIcon(pAction->icon());
      }
    }
  }
  addButtonMenu_->addSeparator();
  QAction *action = addButtonMenu_->addAction(QIcon(":/images/images/noicon.png"),
                                              tr("Separator"));
  action->setObjectName("Separator");
}

void CustomizeToolbarDialog::addShortcut(QAction *action)
{
  QStringList treeItem;
  treeItem << action->text().remove("&") << action->objectName();
  QTreeWidgetItem *item = new QTreeWidgetItem(treeItem);
  item->setIcon(0, action->icon());

  if (action->objectName() == "Separator")
    item->setBackground(0, qApp->palette().brush(QPalette::Midlight));

  shortcutTree_->addTopLevelItem(item);
}

void CustomizeToolbarDialog::removeShortcut()
{
  int row = shortcutTree_->currentIndex().row();
  shortcutTree_->takeTopLevelItem(row);

  if (shortcutTree_->currentIndex().row() == 0)
    moveUpButton_->setEnabled(false);
  if (shortcutTree_->currentIndex().row() == (shortcutTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(false);
}

void CustomizeToolbarDialog::moveUpShortcut()
{
  int row = shortcutTree_->currentIndex().row();
  QTreeWidgetItem *treeWidgetItem = shortcutTree_->takeTopLevelItem(row-1);
  shortcutTree_->insertTopLevelItem(row, treeWidgetItem);

  if (shortcutTree_->currentIndex().row() == 0)
    moveUpButton_->setEnabled(false);
  if (shortcutTree_->currentIndex().row() != (shortcutTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(true);
}

void CustomizeToolbarDialog::moveDownShortcut()
{
  int row = shortcutTree_->currentIndex().row();
  QTreeWidgetItem *treeWidgetItem = shortcutTree_->takeTopLevelItem(row+1);
  shortcutTree_->insertTopLevelItem(row, treeWidgetItem);

  if (shortcutTree_->currentIndex().row() != 0)
    moveUpButton_->setEnabled(true);
  if (shortcutTree_->currentIndex().row() == (shortcutTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(false);
}

void CustomizeToolbarDialog::defaultShortcut()
{
  shortcutTree_->clear();

  QString actionListStr;
  if (toolbar_->objectName() == "ToolBar_General") {
    actionListStr = "newAct,Separator,updateFeedAct,updateAllFeedsAct,"
        "Separator,markFeedRead,Separator,autoLoadImagesToggle";
  } else if (toolbar_->objectName() == "feedsToolBar") {
    actionListStr = "findFeedAct,feedsFilter";
  } else if (toolbar_->objectName() == "newsToolBar") {
    actionListStr = "markNewsRead,markAllNewsRead,Separator,markStarAct,"
        "newsLabelAction,shareMenuAct,openInExternalBrowserAct,Separator,"
        "nextUnreadNewsAct,prevUnreadNewsAct,Separator,"
        "newsFilter,Separator,deleteNewsAct";
  }

  foreach (QString actionStr, actionListStr.split(",", QString::SkipEmptyParts)) {
    QStringList treeItem;
    if (actionStr == "Separator") {
      QTreeWidgetItem *item = new QTreeWidgetItem(treeItem);
      item->setText(0, tr("Separator"));
      item->setText(1, "Separator");
      item->setIcon(0, QIcon(":/images/images/noicon.png"));
      item->setBackground(0, qApp->palette().brush(QPalette::Midlight));
      shortcutTree_->addTopLevelItem(item);
    } else {
      RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());
      QListIterator<QAction *> iter(rssl_->actions());
      while (iter.hasNext()) {
        QAction *pAction = iter.next();
        if (!pAction->icon().isNull()) {
          if (pAction->objectName() == actionStr) {
            treeItem << pAction->text().remove("&") << pAction->objectName();
            QTreeWidgetItem *item = new QTreeWidgetItem(treeItem);

            if (pAction->icon().isNull())
              item->setIcon(0, QIcon(":/images/images/noicon.png"));
            else {
              if (pAction->objectName() == "autoLoadImagesToggle") {
                item->setIcon(0, QIcon(":/images/imagesOn"));
                item->setText(0, tr("Load images"));
              } else if ((pAction->objectName() == "feedsFilter") ||
                         (pAction->objectName() == "newsFilter")) {
                item->setIcon(0, QIcon(":/images/filterOn"));
              } else item->setIcon(0, pAction->icon());
            }
            shortcutTree_->addTopLevelItem(item);
            break;
          }
        }
      }
    }
  }

  styleBox_->setCurrentIndex(3);
  iconBox_->setCurrentIndex(1);
}
