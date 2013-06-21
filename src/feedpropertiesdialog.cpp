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
/*This file is prepared for Doxygen automatic documentation generation.*/
#include "feedpropertiesdialog.h"

FeedPropertiesDialog::FeedPropertiesDialog(bool isFeed, QWidget *parent)
  : Dialog(parent)
  , isFeed_(isFeed)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Properties"));
  setMinimumWidth(450);
  setMinimumHeight(350);

  tabWidget = new QTabWidget();
  tabWidget->addTab(CreateGeneralTab(), tr("General"));
  tabWidget->addTab(CreateColumnsTab(), tr("Columns"));
  tabWidget->addTab(CreateAuthenticationTab(), tr("Authentication"));
  tabWidget->addTab(CreateStatusTab(), tr("Status"));
  pageLayout->addWidget(tabWidget);

  if (!isFeed_) {
    tabWidget->removeTab(2);
  }

  buttonBox->addButton(QDialogButtonBox::Ok);
  buttonBox->addButton(QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

  connect(this, SIGNAL(signalLoadIcon(QString,QString)),
          parent, SIGNAL(faviconRequestUrl(QString,QString)));
  connect(parent, SIGNAL(signalIconFeedReady(QString, const QByteArray&)),
          this, SLOT(slotFaviconUpdate(QString, const QByteArray&)));
}
//------------------------------------------------------------------------------
QWidget *FeedPropertiesDialog::CreateGeneralTab()
{
  QWidget *tab = new QWidget();
  QVBoxLayout *layoutGeneralMain = new QVBoxLayout(tab);
  QGridLayout *layoutGeneralGrid = new QGridLayout();
  QLabel *labelTitleCapt = new QLabel(tr("Title:"));
  QLabel *labelHomepageCapt = new QLabel(tr("Homepage:"));
  QLabel *labelURLCapt = new QLabel(tr("Feed URL:"));

  QHBoxLayout *layoutGeneralTitle = new QHBoxLayout();
  editTitle = new LineEdit();
  QToolButton *loadTitleButton = new QToolButton();
  loadTitleButton->setIcon(QIcon(":/images/updateFeed"));
  loadTitleButton->setIconSize(QSize(16, 16));
  loadTitleButton->setToolTip(tr("Load Title"));
  loadTitleButton->setFocusPolicy(Qt::NoFocus);

  QMenu *selectIconMenu = new QMenu();
  selectIconMenu->addAction(tr("Load Favicon"));
  selectIconMenu->addSeparator();
  selectIconMenu->addAction(tr("Select Icon..."));
  selectIconButton_ = new QToolButton(this);
  selectIconButton_->setIconSize(QSize(16, 16));
  selectIconButton_->setToolTip(tr("Select Icon"));
  selectIconButton_->setFocusPolicy(Qt::NoFocus);
  selectIconButton_->setPopupMode(QToolButton::MenuButtonPopup);
  selectIconButton_->setMenu(selectIconMenu);

  layoutGeneralTitle->addWidget(editTitle, 1);
  layoutGeneralTitle->addWidget(loadTitleButton);
  layoutGeneralTitle->addWidget(selectIconButton_);
  editURL = new LineEdit();

  updateEnable_ = new QCheckBox(tr("Automatically update every"));
  updateInterval_ = new QSpinBox();
  updateInterval_->setEnabled(false);
  updateInterval_->setRange(1, 9999);
  connect(updateEnable_, SIGNAL(toggled(bool)),
          updateInterval_, SLOT(setEnabled(bool)));

  updateIntervalType_ = new QComboBox(this);
  updateIntervalType_->setEnabled(false);
  QStringList intervalTypeList;
  intervalTypeList << tr("seconds") << tr("minutes")  << tr("hours");
  updateIntervalType_->addItems(intervalTypeList);
  connect(updateEnable_, SIGNAL(toggled(bool)),
          updateIntervalType_, SLOT(setEnabled(bool)));

  QHBoxLayout *updateFeedsLayout = new QHBoxLayout();
  updateFeedsLayout->setMargin(0);
  updateFeedsLayout->addWidget(updateEnable_);
  updateFeedsLayout->addWidget(updateInterval_);
  updateFeedsLayout->addWidget(updateIntervalType_);
  updateFeedsLayout->addStretch();

  starredOn_ = new QCheckBox(tr("Starred"));
  loadImagesOn = new QCheckBox(tr("Load images"));
  displayOnStartup = new QCheckBox(tr("Display in new tab on startup"));
  showDescriptionNews_ = new QCheckBox(tr("Show news' description instead of loading web page"));
  duplicateNewsMode_ = new QCheckBox(tr("Automatically delete duplicate news"));

  QHBoxLayout *layoutGeneralHomepage = new QHBoxLayout();
  labelHomepage = new QLabel();
  labelHomepage->setOpenExternalLinks(true);
  layoutGeneralHomepage->addWidget(labelHomepageCapt);
  layoutGeneralHomepage->addWidget(labelHomepage, 1);

  layoutGeneralGrid->addWidget(labelTitleCapt, 0, 0);
  layoutGeneralGrid->addLayout(layoutGeneralTitle, 0 ,1);
  layoutGeneralGrid->addWidget(labelURLCapt, 1, 0);
  layoutGeneralGrid->addWidget(editURL, 1, 1);

  layoutGeneralMain->addLayout(layoutGeneralGrid);
  layoutGeneralMain->addLayout(layoutGeneralHomepage);
  layoutGeneralMain->addSpacing(15);
  layoutGeneralMain->addLayout(updateFeedsLayout);
  layoutGeneralMain->addWidget(starredOn_);
  layoutGeneralMain->addWidget(loadImagesOn);
  layoutGeneralMain->addWidget(displayOnStartup);
  layoutGeneralMain->addWidget(showDescriptionNews_);
  layoutGeneralMain->addWidget(duplicateNewsMode_);
  layoutGeneralMain->addStretch();

  connect(loadTitleButton, SIGNAL(clicked()), this, SLOT(setDefaultTitle()));
  connect(selectIconButton_, SIGNAL(clicked()),
          this, SLOT(selectIcon()));
  connect(selectIconMenu->actions().at(0), SIGNAL(triggered()),
          this, SLOT(loadDefaultIcon()));
  connect(selectIconMenu->actions().at(2), SIGNAL(triggered()),
          this, SLOT(selectIcon()));

  if (!isFeed_) {
    loadTitleButton->hide();
    selectIconButton_->hide();
    labelURLCapt->hide();
    editURL->hide();
    labelHomepageCapt->hide();
    labelHomepage->hide();
    starredOn_->hide();
    duplicateNewsMode_->hide();
  }

  return tab;
}
//------------------------------------------------------------------------------
QWidget *FeedPropertiesDialog::CreateColumnsTab()
{
  columnsTree_ = new QTreeWidget(this);
  columnsTree_->setObjectName("columnsTree");
  columnsTree_->setIndentation(0);
  columnsTree_->setColumnCount(2);
  columnsTree_->setColumnHidden(1, true);
  columnsTree_->setSortingEnabled(false);
  columnsTree_->setHeaderHidden(true);
#ifdef HAVE_QT5
  columnsTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
#else
  columnsTree_->header()->setResizeMode(0, QHeaderView::Stretch);
#endif

  QStringList treeItem;
  treeItem << "Name" << "Index";
  columnsTree_->setHeaderLabels(treeItem);

  sortByColumnBox_ = new QComboBox(this);

  sortOrderBox_ = new QComboBox(this);
  treeItem.clear();
  treeItem << tr("Ascending") << tr("Descending");
  sortOrderBox_->addItems(treeItem);

  QHBoxLayout *styleLayout = new QHBoxLayout();
  styleLayout->setMargin(0);
  styleLayout->addWidget(new QLabel(tr("Sort by:")));
  styleLayout->addWidget(sortByColumnBox_);
  styleLayout->addSpacing(10);
  styleLayout->addWidget(sortOrderBox_);
  styleLayout->addStretch();

  QWidget *styleWidget = new QWidget(this);
  styleWidget->setLayout(styleLayout);

  QVBoxLayout *mainVLayout = new QVBoxLayout();
  mainVLayout->addWidget(columnsTree_, 1);
  mainVLayout->addWidget(styleWidget);

  addButtonMenu_ = new QMenu(this);
  addButton_ = new QPushButton(tr("Add"));
  addButton_->setMenu(addButtonMenu_);
  connect(addButtonMenu_, SIGNAL(aboutToShow()),
          this, SLOT(showMenuAddButton()));
  connect(addButtonMenu_, SIGNAL(triggered(QAction*)),
          this, SLOT(addColumn(QAction*)));

  removeButton_ = new QPushButton(tr("Remove"));
  removeButton_->setEnabled(false);
  connect(removeButton_, SIGNAL(clicked()), this, SLOT(removeColumn()));

  moveUpButton_ = new QPushButton(tr("Move up"));
  moveUpButton_->setEnabled(false);
  connect(moveUpButton_, SIGNAL(clicked()), this, SLOT(moveUpColumn()));
  moveDownButton_ = new QPushButton(tr("Move down"));
  moveDownButton_->setEnabled(false);
  connect(moveDownButton_, SIGNAL(clicked()), this, SLOT(moveDownColumn()));

  QPushButton *defaultButton = new QPushButton(tr("Default"));
  connect(defaultButton, SIGNAL(clicked()), this, SLOT(defaultColumns()));

  QVBoxLayout *buttonsVLayout = new QVBoxLayout();
  buttonsVLayout->addWidget(addButton_);
  buttonsVLayout->addWidget(removeButton_);
  buttonsVLayout->addSpacing(10);
  buttonsVLayout->addWidget(moveUpButton_);
  buttonsVLayout->addWidget(moveDownButton_);
  buttonsVLayout->addSpacing(10);
  buttonsVLayout->addWidget(defaultButton);
  buttonsVLayout->addStretch();

  QHBoxLayout *mainlayout = new QHBoxLayout();
  mainlayout->addLayout(mainVLayout);
  mainlayout->addLayout(buttonsVLayout);

  connect(columnsTree_, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this, SLOT(slotCurrentColumnChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

  QWidget *tab = new QWidget();
  tab->setLayout(mainlayout);

  return tab;
}
//------------------------------------------------------------------------------
QWidget *FeedPropertiesDialog::CreateAuthenticationTab()
{
  authentication_ = new QGroupBox(this);
  authentication_->setTitle(tr("Server requires authentication:"));
  authentication_->setCheckable(true);
  authentication_->setChecked(false);

  user_ = new LineEdit(this);
  pass_ = new LineEdit(this);
  pass_->setEchoMode(QLineEdit::Password);

  QGridLayout *authenticationLayout = new QGridLayout();
  authenticationLayout->addWidget(new QLabel(tr("Username:")), 2, 0);
  authenticationLayout->addWidget(user_, 2, 1);
  authenticationLayout->addWidget(new QLabel(tr("Password:")), 3, 0);
  authenticationLayout->addWidget(pass_, 3, 1);

  authentication_->setLayout(authenticationLayout);

  QVBoxLayout *layoutMain = new QVBoxLayout();
  layoutMain->addWidget(authentication_);
  layoutMain->addStretch(1);

  QWidget *tab = new QWidget();
  tab->setLayout(layoutMain);

  return tab;
}
//------------------------------------------------------------------------------
QWidget *FeedPropertiesDialog::CreateStatusTab()
{
  createdFeed_ = new QLabel();
  lastUpdateFeed_ = new QLabel();
  newsCount_ = new QLabel();

  QLabel *descriptionLabel = new QLabel(tr("Description:"));

  descriptionText_ = new QTextEdit();
  descriptionText_->setReadOnly(true);

  QGridLayout *layoutGrid = new QGridLayout();
  layoutGrid->setColumnStretch(1,1);
  layoutGrid->addWidget(new QLabel(tr("Created:")), 0, 0);
  layoutGrid->addWidget(createdFeed_, 0, 1);
  layoutGrid->addWidget(new QLabel(tr("Last update:")), 1, 0);
  layoutGrid->addWidget(lastUpdateFeed_, 1, 1);
  layoutGrid->addWidget(new QLabel(tr("News count:")), 2, 0);
  layoutGrid->addWidget(newsCount_, 2, 1);
  layoutGrid->addWidget(descriptionLabel, 3, 0, 1, 1, Qt::AlignTop);
  layoutGrid->addWidget(descriptionText_, 3, 1, 1, 1, Qt::AlignTop);

  QVBoxLayout *layoutMain = new QVBoxLayout();
  layoutMain->addLayout(layoutGrid);
  layoutMain->addStretch(1);

  QWidget *tab = new QWidget();
  tab->setLayout(layoutMain);

  if (!isFeed_) {
    descriptionLabel->hide();
    descriptionText_->hide();
  }

  return tab;
}
//------------------------------------------------------------------------------
/*virtual*/ void FeedPropertiesDialog::showEvent(QShowEvent *event)
{
  Q_UNUSED(event)
  editTitle->setText(feedProperties.general.text);
  editURL->setText(feedProperties.general.url);
  editURL->selectAll();
  editURL->setFocus();
  labelHomepage->setText(QString("<a href='%1'>%1</a>").arg(feedProperties.general.homepage));
  selectIconButton_->setIcon(windowIcon());

  updateEnable_->setChecked(feedProperties.general.updateEnable);
  updateInterval_->setValue(feedProperties.general.updateInterval);
  updateIntervalType_->setCurrentIndex(feedProperties.general.intervalType + 1);

  displayOnStartup->setChecked(feedProperties.general.displayOnStartup);
  starredOn_->setChecked(feedProperties.general.starred);
  loadImagesOn->setChecked(feedProperties.display.displayEmbeddedImages);
  showDescriptionNews_->setChecked(!feedProperties.display.displayNews);
  duplicateNewsMode_->setChecked(feedProperties.general.duplicateNewsMode);

  for (int i = 0; i < feedProperties.column.columns.count(); ++i) {
    int index = feedProperties.column.indexList.indexOf(feedProperties.column.columns.at(i));
    QString name = feedProperties.column.nameList.at(index);
    QStringList treeItem;
    treeItem << name
             << QString::number(feedProperties.column.columns.at(i));
    QTreeWidgetItem *item = new QTreeWidgetItem(treeItem);
    columnsTree_->addTopLevelItem(item);
  }
  for (int i = 0; i < feedProperties.column.indexList.count(); ++i) {
    sortByColumnBox_->addItem(feedProperties.column.nameList.at(i),
                              feedProperties.column.indexList.at(i));
    if (feedProperties.column.sortBy == feedProperties.column.indexList.at(i))
      sortByColumnBox_->setCurrentIndex(i);
  }
  sortOrderBox_->setCurrentIndex(feedProperties.column.sortType);

  authentication_->setChecked(feedProperties.authentication.on);
  user_->setText(feedProperties.authentication.user);
  pass_->setText(feedProperties.authentication.pass);

  descriptionText_->setText(feedProperties.status.description);
  if (feedProperties.status.createdTime.isValid())
    createdFeed_->setText(feedProperties.status.createdTime.toString("dd.MM.yy hh:mm"));
  else
    createdFeed_->setText(tr("Long ago ;-)"));

  lastUpdateFeed_->setText(QString("%1 (%2)").
                           arg(feedProperties.status.lastUpdate.toString("dd.MM.yy hh:mm")).
                           arg(feedProperties.status.lastBuildDate.toString("dd.MM.yy hh:mm"))
                           );
  newsCount_->setText(QString("%1 (%2 %3, %4 %5)").
                      arg(feedProperties.status.undeleteCount).
                      arg(feedProperties.status.newCount).
                      arg(tr("new")).
                      arg(feedProperties.status.unreadCount).
                      arg(tr("unread")));
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::setDefaultTitle()
{
  editTitle->setText(feedProperties.general.title);
}

void FeedPropertiesDialog::loadDefaultIcon()
{
  emit signalLoadIcon(feedProperties.general.homepage, feedProperties.general.url);
}

void FeedPropertiesDialog::selectIcon()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select Image"),
                                                  QDir::homePath(),
                                                  tr("Image files (*.bmp *.gif *.ico *.jpg *.jpeg *.mng *.png *.pbm *.pgm *.ppm *.tif *.tiff *.tga *.svg *.xbm *.xpm)"));

  if (fileName.isNull()) return;

  QMessageBox msgBox;
  msgBox.setText(tr("Load icon: can't open a file!"));
  msgBox.setIcon(QMessageBox::Warning);

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly)) {
    msgBox.exec();
    return;
  }

  QPixmap pixmap;
  if (pixmap.loadFromData(file.readAll())) {
    pixmap = pixmap.scaled(16, 16, Qt::IgnoreAspectRatio,
                           Qt::SmoothTransformation);
    QByteArray faviconData;
    QBuffer    buffer(&faviconData);
    buffer.open(QIODevice::WriteOnly);
    if (pixmap.save(&buffer, "ICO")) {
      slotFaviconUpdate(feedProperties.general.url, faviconData);
    }
  } else {
    msgBox.exec();
  }

  file.close();
}

void FeedPropertiesDialog::slotFaviconUpdate(const QString &feedUrl, const QByteArray &faviconData)
{
  if (feedUrl == feedProperties.general.url) {
    feedProperties.general.image = faviconData;
    if (!faviconData.isNull()) {
      QPixmap icon;
      icon.loadFromData(faviconData);
      setWindowIcon(icon);
    } else if (isFeed_) {
      setWindowIcon(QPixmap(":/images/feed"));
    } else {
      setWindowIcon(QPixmap(":/images/folder"));
    }
    selectIconButton_->setIcon(windowIcon());
  }
}
//------------------------------------------------------------------------------
FEED_PROPERTIES FeedPropertiesDialog::getFeedProperties()
{
  feedProperties = feedProperties;

  feedProperties.general.text = editTitle->text();
  feedProperties.general.url = editURL->text();

  feedProperties.general.updateEnable = updateEnable_->isChecked();
  feedProperties.general.updateInterval = updateInterval_->value();
  feedProperties.general.intervalType = updateIntervalType_->currentIndex() - 1;

  feedProperties.general.displayOnStartup = displayOnStartup->isChecked();
  feedProperties.general.starred = starredOn_->isChecked();
  feedProperties.display.displayEmbeddedImages = loadImagesOn->isChecked();
  feedProperties.display.displayNews = !showDescriptionNews_->isChecked();
  feedProperties.general.duplicateNewsMode = duplicateNewsMode_->isChecked();

  feedProperties.column.columns.clear();
  for (int i = 0; i < columnsTree_->topLevelItemCount(); ++i) {
    int index = columnsTree_->topLevelItem(i)->text(1).toInt();
    feedProperties.column.columns.append(index);
  }
  feedProperties.column.sortBy =
      sortByColumnBox_->itemData(sortByColumnBox_->currentIndex()).toInt();
  feedProperties.column.sortType = sortOrderBox_->currentIndex();

  feedProperties.authentication.on = authentication_->isChecked();
  feedProperties.authentication.user = user_->text();
  feedProperties.authentication.pass = pass_->text();

  return(feedProperties);
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::setFeedProperties(FEED_PROPERTIES properties)
{
  feedProperties = properties;
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::slotCurrentColumnChanged(QTreeWidgetItem *current,
                                                  QTreeWidgetItem *)
{
  if (columnsTree_->indexOfTopLevelItem(current) == 0)
    moveUpButton_->setEnabled(false);
  else moveUpButton_->setEnabled(true);

  if (columnsTree_->indexOfTopLevelItem(current) == (columnsTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(false);
  else moveDownButton_->setEnabled(true);

  if (columnsTree_->indexOfTopLevelItem(current) < 0) {
    removeButton_->setEnabled(false);
    moveUpButton_->setEnabled(false);
    moveDownButton_->setEnabled(false);
  } else {
    removeButton_->setEnabled(true);
  }
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::showMenuAddButton()
{
  QListIterator<QAction *> iter(addButtonMenu_->actions());
  while (iter.hasNext()) {
    QAction *nextAction = iter.next();
    delete nextAction;
  }

  for (int i = 0; i < feedProperties.column.indexList.count(); ++i) {
    int index = feedProperties.column.indexList.at(i);
    QList<QTreeWidgetItem *> treeItems = columnsTree_->findItems(QString::number(index),
                                                                 Qt::MatchFixedString,
                                                                 1);
    if (!treeItems.count()) {
      QAction *action = addButtonMenu_->addAction(feedProperties.column.nameList.at(i));
      action->setData(index);
    }
  }
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::addColumn(QAction *action)
{
  QStringList treeItem;
  treeItem << action->text() << action->data().toString();
  QTreeWidgetItem *item = new QTreeWidgetItem(treeItem);
  columnsTree_->addTopLevelItem(item);
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::removeColumn()
{
  int row = columnsTree_->currentIndex().row();
  columnsTree_->takeTopLevelItem(row);

  if (columnsTree_->currentIndex().row() == 0)
    moveUpButton_->setEnabled(false);
  if (columnsTree_->currentIndex().row() == (columnsTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(false);
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::moveUpColumn()
{
  int row = columnsTree_->currentIndex().row();
  QTreeWidgetItem *treeWidgetItem = columnsTree_->takeTopLevelItem(row-1);
  columnsTree_->insertTopLevelItem(row, treeWidgetItem);

  if (columnsTree_->currentIndex().row() == 0)
    moveUpButton_->setEnabled(false);
  if (columnsTree_->currentIndex().row() != (columnsTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(true);
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::moveDownColumn()
{
  int row = columnsTree_->currentIndex().row();
  QTreeWidgetItem *treeWidgetItem = columnsTree_->takeTopLevelItem(row+1);
  columnsTree_->insertTopLevelItem(row, treeWidgetItem);

  if (columnsTree_->currentIndex().row() != 0)
    moveUpButton_->setEnabled(true);
  if (columnsTree_->currentIndex().row() == (columnsTree_->topLevelItemCount()-1))
    moveDownButton_->setEnabled(false);
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::defaultColumns()
{
  columnsTree_->clear();
  for (int i = 0; i < feedProperties.columnDefault.columns.count(); ++i) {
    int index = feedProperties.column.indexList.indexOf(feedProperties.columnDefault.columns.at(i));
    QString name = feedProperties.column.nameList.at(index);
    QStringList treeItem;
    treeItem << name
             << QString::number(feedProperties.columnDefault.columns.at(i));
    QTreeWidgetItem *item = new QTreeWidgetItem(treeItem);
    columnsTree_->addTopLevelItem(item);
  }
  for (int i = 0; i < feedProperties.column.indexList.count(); ++i) {
    if (feedProperties.columnDefault.sortBy == feedProperties.column.indexList.at(i))
      sortByColumnBox_->setCurrentIndex(i);
  }
  sortOrderBox_->setCurrentIndex(feedProperties.columnDefault.sortType);
}
