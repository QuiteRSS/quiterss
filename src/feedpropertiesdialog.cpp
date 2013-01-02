/*This file is prepared for Doxygen automatic documentation generation.*/
#include "feedpropertiesdialog.h"

FeedPropertiesDialog::FeedPropertiesDialog(bool isFeed, QWidget *parent) :
  Dialog(parent, Qt::MSWindowsFixedSizeDialogHint),
  isFeed_(isFeed)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Properties"));
  setMinimumWidth(400);
  setMinimumHeight(300);

  tabWidget = new QTabWidget();
  tabWidget->addTab(CreateGeneralTab(), tr("General"));
  tabWidget->addTab(CreateStatusTab(), tr("Status"));
  pageLayout->addWidget(tabWidget);

  buttonBox->addButton(QDialogButtonBox::Ok);
  buttonBox->addButton(QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
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
  QPushButton *btnLoadTitle = new QPushButton(QIcon(":/images/updateFeed"), "");
  btnLoadTitle->setToolTip(tr("Load Feed Title"));
  btnLoadTitle->setIconSize(QSize(16, 16));
  btnLoadTitle->setFocusPolicy(Qt::NoFocus);
  layoutGeneralTitle->addWidget(editTitle, 1);
  layoutGeneralTitle->addWidget(btnLoadTitle);
  editURL = new LineEdit();

  starredOn_ = new QCheckBox(tr("Starred"));
  loadImagesOn = new QCheckBox(tr("Load images"));
  displayOnStartup = new QCheckBox(tr("Display feed in new tab on startup"));
  showDescriptionNews_ = new QCheckBox(tr("Show news' description instead of loading web page"));

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
  layoutGeneralMain->addWidget(starredOn_);
  layoutGeneralMain->addWidget(loadImagesOn);
  layoutGeneralMain->addWidget(displayOnStartup);
  layoutGeneralMain->addWidget(showDescriptionNews_);
  layoutGeneralMain->addStretch();

  connect(btnLoadTitle, SIGNAL(clicked()), this, SLOT(slotLoadTitle()));

  if (!isFeed_) {
    btnLoadTitle->hide();
    labelURLCapt->hide();
    editURL->hide();
    labelHomepageCapt->hide();
    labelHomepage->hide();
    starredOn_->hide();
    loadImagesOn->hide();
    displayOnStartup->hide();
    showDescriptionNews_->hide();
  }

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
  displayOnStartup->setChecked(feedProperties.general.displayOnStartup);
  starredOn_->setChecked(feedProperties.general.starred);
  loadImagesOn->setChecked(feedProperties.display.displayEmbeddedImages);
  showDescriptionNews_->setChecked(!feedProperties.display.displayNews);

  descriptionText_->setText(feedProperties.status.description);
  if (feedProperties.status.createdTime.isValid())
    createdFeed_->setText(feedProperties.status.createdTime.toString("dd.MM.yy hh:mm"));
  else
    createdFeed_->setText(tr("Long ago ;-)"));

  lastUpdateFeed_->setText(feedProperties.status.lastUpdate.toString("dd.MM.yy hh:mm"));
  newsCount_->setText(QString("%1 (%2 %3, %4 %5)").
                      arg(feedProperties.status.undeleteCount).
                      arg(feedProperties.status.newCount).
                      arg(tr("new")).
                      arg(feedProperties.status.unreadCount).
                      arg(tr("unread")));
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::slotLoadTitle()
{
  editTitle->setText(feedProperties.general.title);
  emit signalLoadTitle(feedProperties.general.homepage, feedProperties.general.url);
  emit startGetUrlTimer();
}
//------------------------------------------------------------------------------
FEED_PROPERTIES FeedPropertiesDialog::getFeedProperties()
{
  feedProperties.general.text = editTitle->text();
  feedProperties.general.url = editURL->text();
  feedProperties.general.displayOnStartup = displayOnStartup->isChecked();
  feedProperties.general.starred = starredOn_->isChecked();
  feedProperties.display.displayEmbeddedImages = loadImagesOn->isChecked();
  feedProperties.display.displayNews = !showDescriptionNews_->isChecked();
  return(feedProperties);
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::setFeedProperties(FEED_PROPERTIES properties)
{
  feedProperties = properties;
}

