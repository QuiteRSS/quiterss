/*This file is prepared for Doxygen automatic documentation generation.*/
#include <QDebug>
#include "feedpropertiesdialog.h"

FeedPropertiesDialog::FeedPropertiesDialog(QWidget *parent) :
    QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Feed properties"));

  // Основное окно
  layoutMain = new QVBoxLayout(this);
  layoutMain->setMargin(5);
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
  labelTitleCapt = new QLabel(tr("Title:"));
  labelHomepageCapt = new QLabel(tr("Homepage:"));
  labelURLCapt = new QLabel(tr("Feed URL:"));

  layoutGeneralTitle = new QHBoxLayout();
  editTitle = new QLineEdit();
  btnLoadTitle = new QPushButton(QIcon(":/images/updateFeed"), tr(""));
  btnLoadTitle->setToolTip(tr("Load feed title"));
  btnLoadTitle->setMaximumHeight(22);
  btnLoadTitle->setFocusPolicy(Qt::NoFocus);
  layoutGeneralTitle->addWidget(editTitle);
  layoutGeneralTitle->addWidget(btnLoadTitle);
  layoutGeneralTitle->setStretch(0, 2);
  layoutGeneralTitle->setStretch(1, 0);
  editURL = new QLineEdit();

  layoutGeneralHomepage = new QHBoxLayout();
  labelHomepage = new QLabel();
  labelHomepage->setOpenExternalLinks(true);
  layoutGeneralHomepage->addWidget(labelHomepageCapt);
  layoutGeneralHomepage->addWidget(labelHomepage);
  layoutGeneralHomepage->setStretch(0,0);
  layoutGeneralHomepage->setStretch(1,2);

  layoutGeneralGrid->addWidget(labelTitleCapt, 0, 0);
  layoutGeneralGrid->addLayout(layoutGeneralTitle, 0 ,1);
  layoutGeneralGrid->addWidget(labelURLCapt, 1, 0);
  layoutGeneralGrid->addWidget(editURL, 1, 1);

  layoutGeneralMain->addLayout(layoutGeneralGrid);
  layoutGeneralMain->addLayout(layoutGeneralHomepage);
  spacerGeneral = new QSpacerItem(0,0, QSizePolicy::Minimum, QSizePolicy::Expanding);
  layoutGeneralMain->addSpacerItem(spacerGeneral);

  connect(btnLoadTitle, SIGNAL(clicked()), this, SLOT(slotLoadTitle()));

  return tab;
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::showEvent(QShowEvent *event)
{
  editTitle->setText(feedProperties.general.text);
  editURL->setText(feedProperties.general.url);
  editURL->selectAll();
  editURL->setFocus();
  labelHomepage->setText(QString("<a href='%1'>%1</a>").arg(feedProperties.general.homepage));
  editTitle->setCursorPosition(0);
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
  return(feedProperties);
}
//------------------------------------------------------------------------------
void FeedPropertiesDialog::setFeedProperties(FEED_PROPERTIES properties)
{
  feedProperties = properties;
}

