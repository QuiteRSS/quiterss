/*This file is prepared for Doxygen automatic documentation generation.*/
#include "feedpropertiesdialog.h"

FeedPropertiesDialog::FeedPropertiesDialog(QWidget *parent) :
    QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Feed properties"));

  // Основное окно
  layoutMain = new QVBoxLayout(this);
  layoutMain->setMargin(2);
  tabWidget = new QTabWidget();
  buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  layoutMain->addWidget(tabWidget);
  layoutMain->addWidget(buttonBox);

  tabGeneral = CreateGeneralTab();

  tabWidget->addTab(tabGeneral, tr("General"));

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  setMinimumWidth(400);
  setMinimumHeight(300);
}
//------------------------------------------------------------------------------
QWidget *FeedPropertiesDialog::CreateGeneralTab(void)
{
  QWidget *tab = new QWidget();
  layoutGeneralMain = new QVBoxLayout(tab);
  layoutGeneralGrid = new QGridLayout();

  layoutGeneralTitle = new QHBoxLayout();
  editTitle = new QLineEdit();
  btnLoadTitle = new QPushButton(QIcon(":/images/updateFeed"), tr(""));
  btnLoadTitle->setToolTip(tr("Load feed title"));
  btnLoadTitle->setMaximumHeight(22);
  btnLoadTitle->setFocusPolicy(Qt::NoFocus);
  layoutGeneralTitle->addWidget(editTitle);
  layoutGeneralTitle->addWidget(btnLoadTitle);
  layoutGeneralTitle->setStretch(0, 0);
  layoutGeneralTitle->setStretch(0, 2);
  editURL = new QLineEdit();
  labelHomepage = new QLabel();
  labelHomepage->setOpenExternalLinks(true);
  layoutGeneralMain->addLayout(layoutGeneralTitle);

  labelTitleCapt = new QLabel(tr("Title:"));
  labelHomepageCapt = new QLabel(tr("Homepage:"));
  labelURLCapt = new QLabel(tr("Feed URL:"));

  layoutGeneralGrid->addWidget(labelURLCapt, 0, 0);
  layoutGeneralGrid->addWidget(editURL, 0, 1);
  layoutGeneralGrid->addWidget(labelHomepageCapt, 1, 0);
  layoutGeneralGrid->addWidget(labelHomepage, 1, 1);

  layoutGeneralMain->addLayout(layoutGeneralGrid);
  spacerGeneral = new QSpacerItem(0,0, QSizePolicy::Minimum, QSizePolicy::Expanding);
  layoutGeneralMain->addSpacerItem(spacerGeneral);

  connect(btnLoadTitle, SIGNAL(clicked()), this, SLOT(slotLoadTitle()));

  return tab;
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::showEvent(QShowEvent *event)
{
  editTitle->setText(feedProperties.general.title);
  editURL->setText(feedProperties.general.url);
  labelHomepage->setText(feedProperties.general.homepage);

  editTitle->setCursorPosition(0);
  editTitle->setFocus();
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
  return(feedProperties);
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::setFeedProperties(FEED_PROPERTIES properties)
{
  feedProperties = properties;
}

