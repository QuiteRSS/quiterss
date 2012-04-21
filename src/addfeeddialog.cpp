#include "addfeeddialog.h"

AddFeedDialog::AddFeedDialog(QWidget *parent) :
  QWizard(parent)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Add feed"));
  setWindowIcon(QIcon(":/images/feed"));
  setWizardStyle(QWizard::ModernStyle);
  setOptions(QWizard::HaveFinishButtonOnEarlyPages);

  addPage(createUrlFeedPage());
  addPage(createNameFeedPage());

  connect(this, SIGNAL(currentIdChanged(int)),
          SLOT(slotCurrentIdChanged(int)),
          Qt::QueuedConnection);
  resize(400, 300);
}

QWizardPage *AddFeedDialog::createUrlFeedPage()
{
    QWizardPage *page = new QWizardPage;
    page->setTitle("Create a new feed");

    urlFeedEdit_ = new LineEdit(this);
    connect(urlFeedEdit_, SIGNAL(textChanged(const QString&)),
            this, SLOT(urlFeedEditChanged(const QString&)));

    QClipboard *clipboard_ = QApplication::clipboard();
    QString clipboardStr = clipboard_->text().left(8);
    if (clipboardStr.contains("http://", Qt::CaseInsensitive) ||
        clipboardStr.contains("https://", Qt::CaseInsensitive)) {
      urlFeedEdit_->setText(clipboard_->text());
      urlFeedEdit_->selectAll();
      urlFeedEdit_->setFocus();
    } else urlFeedEdit_->setText("http://");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel("Feed URL or website address:"));
    layout->addWidget(urlFeedEdit_);
    page->setLayout(layout);

    return page;
}

QWizardPage *AddFeedDialog::createNameFeedPage()
{
    QWizardPage *page = new QWizardPage;
    page->setTitle("Create a new feed");
    page->setFinalPage(false);

    nameFeedEdit_ = new LineEdit(this);
    connect(nameFeedEdit_, SIGNAL(textChanged(const QString&)),
            this, SLOT(nameFeedEditChanged(const QString&)));

    nameFeedEdit_->selectAll();
    nameFeedEdit_->setFocus();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel("Name:"));
    layout->addWidget(nameFeedEdit_);
    page->setLayout(layout);

    return page;
}

void AddFeedDialog::urlFeedEditChanged(const QString& text)
{
  button(QWizard::NextButton)->setEnabled(!text.isEmpty());
}

void AddFeedDialog::nameFeedEditChanged(const QString& text)
{
  button(QWizard::FinishButton)->setEnabled(!text.isEmpty());
}

void AddFeedDialog::slotCurrentIdChanged(int)
{
  button(QWizard::FinishButton)->setEnabled(false);
}

/*virtual*/ bool AddFeedDialog::validateCurrentPage()
{
  return true;
}
