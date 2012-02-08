#include "feedpropertiesdialog.h"

FeedPropertiesDialog::FeedPropertiesDialog(QWidget *parent) :
    QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Feed properties"));

  titleEdit_ =  new QLineEdit();
  loadTitleButton_ = new QPushButton();
  loadTitleButton_->setIcon(QIcon(":/images/updateFeed"));
  loadTitleButton_->setToolTip(tr("Load feed title"));
  loadTitleButton_->setMaximumHeight(22);
  loadTitleButton_->setFocusPolicy(Qt::NoFocus);
  urlEdit_ =  new QLineEdit();
  homepageLabel_ =  new QLabel();
  homepageLabel_->setOpenExternalLinks(true);

  QGridLayout *generalGridLayout = new QGridLayout();
  generalGridLayout->addWidget(new QLabel(tr("Title:")), 0, 0);
  generalGridLayout->addWidget(titleEdit_, 0, 1);
  generalGridLayout->addWidget(loadTitleButton_, 0, 2);
  generalGridLayout->addWidget(new QLabel(tr("Feed URL:")), 1, 0);
  generalGridLayout->addWidget(urlEdit_, 1, 1, 1, 2);
  generalGridLayout->addWidget(new QLabel(tr("Homepage:")), 2, 0);
  generalGridLayout->addWidget(homepageLabel_, 2, 1, 1, 2);

  QVBoxLayout *generalLayout = new QVBoxLayout();
  generalLayout->addLayout(generalGridLayout);
  generalLayout->addStretch(1);

  QWidget *generalWidget_ = new QWidget();
  generalWidget_->setLayout(generalLayout);

  tabWidget_ = new QTabWidget();
  tabWidget_->addTab(generalWidget_, tr("General"));

  buttonBox_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setMargin(5);
  mainLayout->addWidget(tabWidget_);
  mainLayout->addWidget(buttonBox_);

  setLayout(mainLayout);

  connect(buttonBox_, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox_, SIGNAL(rejected()), this, SLOT(reject()));
  connect(loadTitleButton_, SIGNAL(clicked()), this, SLOT(slotLoadTitle()));

  setMinimumWidth(400);
  setMinimumHeight(300);
}

void FeedPropertiesDialog::slotLoadTitle()
{
  titleEdit_->setText(titleString_);
  emit signalLoadTitle(homePage_, feedUrl_);
  emit startGetUrlTimer();
}
