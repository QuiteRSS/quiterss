#include "dialog.h"

Dialog::Dialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
  pageLayout = new QVBoxLayout();
  pageLayout->setMargin(10);
  pageLayout->setSpacing(5);

  buttonBox = new QDialogButtonBox();

  buttonsLayout = new QHBoxLayout();
  buttonsLayout->setMargin(10);
  buttonsLayout->addWidget(buttonBox);

  QFrame *line = new QFrame();
  line->setFrameStyle(QFrame::HLine | QFrame::Sunken);

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setMargin(0);
  mainLayout->setSpacing(0);
  mainLayout->addLayout(pageLayout, 1);
  mainLayout->addWidget(line);
  mainLayout->addLayout(buttonsLayout);
  setLayout(mainLayout);

  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}
