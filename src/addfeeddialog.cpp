#include "addfeeddialog.h"

AddFeedDialog::AddFeedDialog(QWidget *parent) :
  QDialog(parent)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Add feed"));

  feedTitleEdit_ = new LineEdit(this);
  feedUrlEdit_ = new LineEdit(this);
  buttonBox_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox_, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox_, SIGNAL(rejected()), this, SLOT(reject()));

  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(new QLabel(tr("Feed Title:")), 0, 0, 1, 1);
  gridLayout->addWidget(feedTitleEdit_, 0, 1, 1, 1);
  gridLayout->addWidget(new QLabel(tr("Feed Url:")), 1, 0, 1, 1);
  gridLayout->addWidget(feedUrlEdit_, 1, 1, 1, 1);
  gridLayout->addWidget(buttonBox_, 2, 1, 1, 1);

  setLayout(gridLayout);

  setMinimumWidth(400);
}
