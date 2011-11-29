#include "addfeeddialog.h"

AddFeedDialog::AddFeedDialog(QWidget *parent) :
    QDialog(parent)
{
  feedEdit_ = new QLineEdit();
  buttonBox_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox_, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox_, SIGNAL(rejected()), this, SLOT(reject()));

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->addWidget(feedEdit_);
  mainLayout->addWidget(buttonBox_);

  setLayout(mainLayout);
}
