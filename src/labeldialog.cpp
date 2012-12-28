#include "labeldialog.h"

LabelDialog::LabelDialog(QWidget *parent)
  : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("New Label"));

  nameEdit_ = new LineEdit(this);

  QMenu *iconMenu = new QMenu();
  QStringList strNameLabels;
  strNameLabels << tr("Important") << tr("Work") << tr("Personal")
                << tr("To Do") << tr("Later") << tr("Amusingly");
  for (int i = 0; i < 6; i++) {
    iconMenu->addAction(QIcon(QString(":/images/label_%1").arg(i+1)),
                        strNameLabels.at(i));
  }
  iconMenu->addSeparator();
  QAction *newIcon = new QAction(tr("Load icon..."), this);
  iconMenu->addAction(newIcon);

  iconButton_ = new QToolButton(this);
  iconButton_->setIconSize(QSize(16, 16));
  iconButton_->setPopupMode(QToolButton::MenuButtonPopup);
  iconButton_->setMenu(iconMenu);

  QPixmap pixmap(14, 14);
  pixmap.fill(QColor("#555555"));
  colorTextButton_ = new QToolButton(this);
  colorTextButton_->setIconSize(QSize(16, 16));
  colorTextButton_->setIcon(pixmap);

  colorBgButton_ = new QToolButton(this);
  colorBgButton_->setIconSize(QSize(16, 16));

  buttonBox_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(false);

  QHBoxLayout *layoutH1 = new QHBoxLayout();
  layoutH1->addWidget(new QLabel(tr("Name:")));
  layoutH1->addWidget(nameEdit_, 1);

  QHBoxLayout *layoutH2 = new QHBoxLayout();
  layoutH2->addWidget(new QLabel(tr("Icon:")));
  layoutH2->addWidget(iconButton_);
  layoutH2->addSpacing(10);
  layoutH2->addWidget(new QLabel(tr("Color text:")));
  layoutH2->addWidget(colorTextButton_);
  layoutH2->addSpacing(10);
  layoutH2->addWidget(new QLabel(tr("Color background:")));
  layoutH2->addWidget(colorBgButton_);
  layoutH2->addStretch(1);

  QVBoxLayout *layoutMain = new QVBoxLayout();
  layoutMain->setMargin(5);
  layoutMain->addLayout(layoutH1);
  layoutMain->addLayout(layoutH2);
  layoutMain->addWidget(buttonBox_);
  setLayout(layoutMain);

  connect(buttonBox_, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox_, SIGNAL(rejected()), this, SLOT(reject()));
  connect(nameEdit_, SIGNAL(textChanged(const QString&)),
          this, SLOT(nameEditChanged(const QString&)));
  connect(iconButton_, SIGNAL(clicked()),
          iconButton_, SLOT(showMenu()));
  connect(iconMenu, SIGNAL(triggered(QAction*)),
          this, SLOT(selectIcon(QAction*)));
}

void LabelDialog::nameEditChanged(const QString& text)
{
  buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
}

void LabelDialog::selectIcon(QAction *action)
{
  iconButton_->setIcon(action->icon());
}
