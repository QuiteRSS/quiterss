#include "addfeedwizard.h"
#include "addfolderdialog.h"

extern QString kCreateNewsTableQuery;

AddFeedWizard::AddFeedWizard(QWidget *parent, QSqlDatabase *db)
  : QWizard(parent),
    db_(db)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Add Feed"));
  setWizardStyle(QWizard::ModernStyle);
  setOptions(QWizard::HaveFinishButtonOnEarlyPages |
             QWizard::NoBackButtonOnStartPage);

  addPage(createUrlFeedPage());
  addPage(createNameFeedPage());

  persistentUpdateThread_ = new UpdateThread(this);
  persistentUpdateThread_->setObjectName("persistentUpdateThread_");
  connect(this, SIGNAL(startGetUrlTimer()),
          persistentUpdateThread_, SIGNAL(startGetUrlTimer()));
  connect(persistentUpdateThread_, SIGNAL(readedXml(QByteArray, QUrl)),
          this, SLOT(receiveXml(QByteArray, QUrl)));
  connect(persistentUpdateThread_, SIGNAL(getUrlDone(int,QDateTime)),
          this, SLOT(getUrlDone(int,QDateTime)));
  persistentParseThread_ = new ParseThread(this, db_);
  persistentParseThread_->setObjectName("persistentParseThread_");
  connect(this, SIGNAL(xmlReadyParse(QByteArray,QUrl)),
          persistentParseThread_, SLOT(parseXml(QByteArray,QUrl)),
          Qt::QueuedConnection);

  connect(button(QWizard::BackButton), SIGNAL(clicked()),
          this, SLOT(backButtonClicked()));
  connect(button(QWizard::NextButton), SIGNAL(clicked()),
          this, SLOT(nextButtonClicked()));
  connect(button(QWizard::FinishButton), SIGNAL(clicked()),
          this, SLOT(finishButtonClicked()));
  connect(this, SIGNAL(currentIdChanged(int)),
          SLOT(slotCurrentIdChanged(int)),
          Qt::QueuedConnection);
  resize(400, 300);
}

AddFeedWizard::~AddFeedWizard()
{
  persistentUpdateThread_->quit();
  persistentParseThread_->quit();
  while (persistentUpdateThread_->isRunning());
  while (persistentParseThread_->isRunning());
}

/*virtual*/ void AddFeedWizard::done(int result)
{
  if (result == QDialog::Rejected) {
    if (progressBar_->isVisible() || (currentId() == 1))
      deleteFeed();
  }
  QWizard::done(result);
}

void AddFeedWizard::changeEvent(QEvent *event)
{
  if ((event->type() == QEvent::ActivationChange) &&
      isActiveWindow() && (currentId() == 0) && urlFeedEdit_->isEnabled()) {
    QClipboard *clipboard_ = QApplication::clipboard();
    QString clipboardStr = clipboard_->text().left(8);
    if (clipboardStr.contains("http://", Qt::CaseInsensitive) ||
        clipboardStr.contains("https://", Qt::CaseInsensitive)) {
      urlFeedEdit_->setText(clipboard_->text());
      urlFeedEdit_->selectAll();
      urlFeedEdit_->setFocus();
    } else urlFeedEdit_->setText("http://");
  }
}

QWizardPage *AddFeedWizard::createUrlFeedPage()
{
  QWizardPage *page = new QWizardPage;
  page->setTitle(tr("Create New Feed"));

  selectedPage = false;
  finishOn = false;

  urlFeedEdit_ = new LineEdit(this);

  titleFeedAsName_ = new QCheckBox(
        tr("Use title of the feed as displayed name"), this);
  titleFeedAsName_->setChecked(true);

  QLabel *iconWarning = new QLabel(this);
  iconWarning->setPixmap(QPixmap(":/images/warning"));
  textWarning = new QLabel(this);
  QFont font = textWarning->font();
  font.setBold(true);
  textWarning->setFont(font);

  QHBoxLayout *warningLayout = new QHBoxLayout();
  warningLayout->setMargin(0);
  warningLayout->addWidget(iconWarning);
  warningLayout->addWidget(textWarning, 1);

  warningWidget_ = new QWidget(this);
  warningWidget_->setLayout(warningLayout);
  warningWidget_->setVisible(false);

  progressBar_ = new QProgressBar(this);
  progressBar_->setObjectName("progressBar_");
  progressBar_->setTextVisible(false);
  progressBar_->setFixedHeight(15);
  progressBar_->setMinimum(0);
  progressBar_->setMaximum(0);
  progressBar_->setVisible(false);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(new QLabel(tr("Feed URL or website address:")));
  layout->addWidget(urlFeedEdit_);
  layout->addWidget(titleFeedAsName_);
  layout->addStretch(0);
  layout->addWidget(warningWidget_);
  layout->addWidget(progressBar_);
  page->setLayout(layout);

  connect(urlFeedEdit_, SIGNAL(textChanged(const QString&)),
          this, SLOT(urlFeedEditChanged(const QString&)));
  connect(titleFeedAsName_, SIGNAL(stateChanged(int)),
          this, SLOT(titleFeedAsNameStateChanged(int)));

  return page;
}

QWizardPage *AddFeedWizard::createNameFeedPage()
{
  QWizardPage *page = new QWizardPage;
  page->setTitle(tr("Create New Feed"));
  page->setFinalPage(false);

  nameFeedEdit_ = new LineEdit(this);

  foldersTree_ = new QTreeWidget(this);
  foldersTree_->setObjectName("foldersTree_");
  foldersTree_->setColumnCount(2);
  foldersTree_->setColumnHidden(1, true);
  foldersTree_->header()->hide();

  QStringList treeItem;
  treeItem << tr("Feeds") << "Id";
  foldersTree_->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << tr("All Feeds") << "0";
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setIcon(0, QIcon(":/images/folder"));
  foldersTree_->addTopLevelItem(treeWidgetItem);
  foldersTree_->setCurrentItem(treeWidgetItem);

  QSqlQuery q(*db_);
  QQueue<int> parentIds;
  parentIds.enqueue(0);
  while (!parentIds.empty()) {
    int parentId = parentIds.dequeue();
    QString qStr = QString("SELECT text, id FROM feeds WHERE parentId='%1' AND (xmlUrl='' OR xmlUrl IS NULL)").
        arg(parentId);
    q.exec(qStr);
    while (q.next()) {
      QString folderText = q.value(0).toString();
      QString folderId = q.value(1).toString();

      QStringList treeItem;
      treeItem << folderText << folderId;
      QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);

      treeWidgetItem->setIcon(0, QIcon(":/images/folder"));

      QList<QTreeWidgetItem *> treeItems =
            foldersTree_->findItems(QString::number(parentId),
                                                       Qt::MatchFixedString | Qt::MatchRecursive,
                                                       1);
      treeItems.at(0)->addChild(treeWidgetItem);
      parentIds.enqueue(folderId.toInt());
    }
  }

  foldersTree_->expandAll();
  foldersTree_->sortByColumn(0, Qt::AscendingOrder);

  QToolButton *newFolderButton = new QToolButton(this);
  newFolderButton->setIcon(QIcon(":/images/addT"));
  newFolderButton->setToolTip(tr("New Folder..."));
  newFolderButton->setAutoRaise(true);

  QHBoxLayout *newFolderLayout = new QHBoxLayout;
  newFolderLayout->setMargin(0);
  newFolderLayout->addWidget(newFolderButton);
  newFolderLayout->addStretch();
  QVBoxLayout *newFolderVLayout = new QVBoxLayout;
  newFolderVLayout->setMargin(2);
  newFolderVLayout->addStretch();
  newFolderVLayout->addLayout(newFolderLayout);

  foldersTree_->setLayout(newFolderVLayout);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(new QLabel(tr("Displayed name:")));
  layout->addWidget(nameFeedEdit_);
  layout->addWidget(new QLabel(tr("Location:")));
  layout->addWidget(foldersTree_);
  page->setLayout(layout);

  connect(nameFeedEdit_, SIGNAL(textChanged(const QString&)),
          this, SLOT(nameFeedEditChanged(const QString&)));
  connect(newFolderButton, SIGNAL(clicked()),
          this, SLOT(newFolder()));

  return page;
}

void AddFeedWizard::urlFeedEditChanged(const QString& text)
{
  button(QWizard::NextButton)->setEnabled(
        !text.isEmpty() && (text != "http://"));

  bool buttonEnable = false;
  if (titleFeedAsName_->isChecked() && (text != "http://") &&
      !text.isEmpty()) {
    buttonEnable = true;
  }
  warningWidget_->setVisible(false);
  button(QWizard::FinishButton)->setEnabled(buttonEnable);
}

void AddFeedWizard::titleFeedAsNameStateChanged(int state)
{
  bool buttonEnable = false;
  if ((state == Qt::Checked) && (urlFeedEdit_->text() != "http://") &&
      !urlFeedEdit_->text().isEmpty()) {
    buttonEnable = true;
  }
  button(QWizard::FinishButton)->setEnabled(buttonEnable);
}

void AddFeedWizard::nameFeedEditChanged(const QString& text)
{
  button(QWizard::FinishButton)->setEnabled(!text.isEmpty());
}

void AddFeedWizard::backButtonClicked()
{
  deleteFeed();
  nameFeedEdit_->clear();
  page(0)->setEnabled(true);
  selectedPage = false;
}

void AddFeedWizard::nextButtonClicked()
{
  if (currentId() == 0)
    addFeed();
}

void AddFeedWizard::finishButtonClicked()
{
  if (currentId() == 0) {
    finishOn = true;
    addFeed();
  } else if (currentId() == 1) {
    finish();
  }
}

void AddFeedWizard::addFeed()
{
  feedUrlString_ = urlFeedEdit_->text().simplified();

  QUrl feedUrl(feedUrlString_);
  if (feedUrl.host().isEmpty()) {
    textWarning->setText(tr("URL error!"));
    warningWidget_->setVisible(true);
    return;
  }

  QSqlQuery q(*db_);
  int duplicateFoundId = -1;
  q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
  q.bindValue(":xmlUrl", feedUrlString_);
  q.exec();
  if (q.next()) duplicateFoundId = q.value(0).toInt();

  if (0 <= duplicateFoundId) {
    textWarning->setText(tr("Duplicate feed!"));
    warningWidget_->setVisible(true);
  } else {
    button(QWizard::NextButton)->setEnabled(false);
    button(QWizard::CancelButton)->setEnabled(false);
    button(QWizard::FinishButton)->setEnabled(false);
    page(0)->setEnabled(false);
    showProgressBar();

    // Вычисляем номер ряда для вставляемой ленты
    int rowToParent = 0;
    q.exec("SELECT max(rowToParent) FROM feeds WHERE parentId=0");
    qDebug() << q.lastQuery();
    qDebug() << q.lastError();
    if (q.next() && !q.value(0).isNull()) rowToParent = q.value(0).toInt() + 1;

    // Добавляем ленту
    q.prepare("INSERT INTO feeds(xmlUrl, created, rowToParent) "
              "VALUES (:feedUrl, :feedCreateTime, :rowToParent)");
    q.bindValue(":feedUrl", feedUrlString_);
    q.bindValue(":feedCreateTime",
        QLocale::c().toString(QDateTime::currentDateTimeUtc(), "yyyy-MM-ddTHH:mm:ss"));
    q.bindValue(":rowToParent", rowToParent);
    q.exec();
    q.finish();

    persistentUpdateThread_->requestUrl(feedUrlString_, QDateTime());
  }
}

void AddFeedWizard::deleteFeed()
{
  int id = -1;

  QSqlQuery q(*db_);
  q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
  q.bindValue(":xmlUrl", feedUrlString_);
  q.exec();
  if (q.next()) id = q.value(0).toInt();
  if (id >= 0) {
    q.exec(QString("DELETE FROM feeds WHERE id='%1'").arg(id));
    q.exec(QString("DELETE FROM news WHERE feedId='%1'").arg(id));
    q.exec("VACUUM");
  }
  q.finish();
}

void AddFeedWizard::slotCurrentIdChanged(int idPage)
{
  if (idPage == 0)
    urlFeedEditChanged(urlFeedEdit_->text());
  else if (idPage == 1)
    nameFeedEditChanged(nameFeedEdit_->text());
}

/*virtual*/ bool AddFeedWizard::validateCurrentPage()
{
  if (!selectedPage) {
    return false;
  }
  return true;
}

void AddFeedWizard::showProgressBar()
{
  progressBar_->show();
  QTimer::singleShot(250, this, SLOT(slotProgressBarUpdate()));
  emit startGetUrlTimer();
}

void AddFeedWizard::slotProgressBarUpdate()
{
  progressBar_->update();
  if (progressBar_->isVisible())
    QTimer::singleShot(250, this, SLOT(slotProgressBarUpdate()));
}

void AddFeedWizard::receiveXml(const QByteArray &data, const QUrl &url)
{
  url_ = url;
  data_.append(data);
}

void AddFeedWizard::getUrlDone(const int &result, const QDateTime &dtReply)
{
  if (!url_.isEmpty() && !data_.isEmpty()) {
    QString str = QString::fromUtf8(data_);

    if (str.contains("<html", Qt::CaseInsensitive)) {
      QRegExp rx("<link[^>]+(atom|rss)\\+xml[^>]+>",
                 Qt::CaseInsensitive, QRegExp::RegExp2);
      int pos = rx.indexIn(str);
      if (pos > -1) {
        str = rx.cap(0);
        rx.setPattern("href=\"([^\"]+)");
        pos = rx.indexIn(str);
        if (pos > -1) {
          QString linkFeedString = rx.cap(1);
          QUrl url(linkFeedString);
          if (url.host().isEmpty()) {
            url.setScheme(url_.scheme());
            url.setHost(url_.host());
            if (url_.toString().indexOf('?') > -1) {
              str = url_.path();
              str = str.left(str.lastIndexOf('/')+1);
              url.setPath(str+url.path());
            }
          }
          linkFeedString = url.toString();
          qDebug() << "Parse feed URL, valid:" << linkFeedString;
          int parseFeedId = 0;

          QSqlQuery q(*db_);
          int duplicateFoundId = -1;
          q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
          q.bindValue(":xmlUrl", linkFeedString);
          q.exec();
          if (q.next()) duplicateFoundId = q.value(0).toInt();

          if (0 <= duplicateFoundId) {
            textWarning->setText(tr("Duplicate feed!"));
            warningWidget_->setVisible(true);

            deleteFeed();
            progressBar_->hide();
            page(0)->setEnabled(true);
            selectedPage = false;
            button(QWizard::CancelButton)->setEnabled(true);
          } else {
            q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
            q.bindValue(":xmlUrl", feedUrlString_);
            q.exec();
            if (q.next()) parseFeedId = q.value(0).toInt();

            feedUrlString_ = linkFeedString;
            q.prepare("UPDATE feeds SET xmlUrl = :xmlUrl WHERE id == :id");
            q.bindValue(":xmlUrl", linkFeedString);
            q.bindValue(":id", parseFeedId);
            q.exec();

            emit startGetUrlTimer();
            persistentUpdateThread_->requestUrl(linkFeedString, QDateTime());
          }
        }
      }
      if (pos < 0) {
        textWarning->setText(tr("Can't find feed URL!"));
        warningWidget_->setVisible(true);

        deleteFeed();
        progressBar_->hide();
        page(0)->setEnabled(true);
        selectedPage = false;
        button(QWizard::CancelButton)->setEnabled(true);
      }

      data_.clear();
      url_.clear();
      return;
    }

    emit xmlReadyParse(data_, url_);
    QSqlQuery q(*db_);
    q.prepare("UPDATE feeds SET lastBuildDate = :lastBuildDate "
              "WHERE xmlUrl == :xmlUrl");
    q.bindValue(":lastBuildDate", dtReply.toString(Qt::ISODate));
    q.bindValue(":xmlUrl", url_.toString());
    q.exec();
    qDebug() << url_.toString() << dtReply.toString(Qt::ISODate);
    qDebug() << q.lastQuery() << q.lastError() << q.lastError().text();
  }
  data_.clear();
  url_.clear();

  if (result == -1) {
    textWarning->setText(tr("URL error!"));
    warningWidget_->setVisible(true);

    deleteFeed();
    progressBar_->hide();
    page(0)->setEnabled(true);
    selectedPage = false;
    button(QWizard::CancelButton)->setEnabled(true);
  }
}

void AddFeedWizard::slotUpdateFeed(int feedId, const bool &, int newCount)
{
  qDebug() << "ParseDone" << feedId;
  selectedPage = true;
  newCount_ = newCount;

  if (titleFeedAsName_->isChecked()) {
    QSqlQuery q(*db_);
    q.exec(QString("SELECT title FROM feeds WHERE id=='%1'").
           arg(feedId));
    if (q.next()) nameFeedEdit_->setText(q.value(0).toString());
    nameFeedEdit_->selectAll();
    nameFeedEdit_->setFocus();
  }
  if (!finishOn) {
    button(QWizard::CancelButton)->setEnabled(true);
    next();
  } else {
    finish();
  }
  progressBar_->hide();
}

void AddFeedWizard::finish()
{
  int parseFeedId = 0;
  int parentId = 0;
  QSqlQuery q(*db_);
  q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
  q.bindValue(":xmlUrl", feedUrlString_);
  q.exec();
  if (q.next()) parseFeedId = q.value(0).toInt();

  q.exec(QString("SELECT htmlUrl FROM feeds WHERE id=='%1'").
         arg(parseFeedId));
  if (q.next()) htmlUrlString_ = q.value(0).toString();

  if (foldersTree_->currentItem()->text(1) != "0")
    parentId = foldersTree_->currentItem()->text(1).toInt();

  q.prepare("UPDATE feeds SET text = ?, parentId = ? WHERE id == ?");
  q.addBindValue(nameFeedEdit_->text());
  q.addBindValue(parentId);
  q.addBindValue(parseFeedId);
  q.exec();

  feedId_ = parseFeedId;

  accept();
}

/*! \brief Добавление новой папки *********************************************/
void AddFeedWizard::newFolder()
{
  AddFolderDialog *addFolderDialog = new AddFolderDialog(this, db_);
  QList<QTreeWidgetItem *> treeItems =
      addFolderDialog->foldersTree_->findItems(foldersTree_->currentItem()->text(1),
                                               Qt::MatchFixedString | Qt::MatchRecursive,
                                               1);
  addFolderDialog->foldersTree_->setCurrentItem(treeItems.at(0));

  if (addFolderDialog->exec() == QDialog::Rejected) {
    delete addFolderDialog;
    return;
  }

  int folderId = 0;
  QString folderText = addFolderDialog->nameFeedEdit_->text();
  int parentId = addFolderDialog->foldersTree_->currentItem()->text(1).toInt();

  QSqlQuery q(*db_);

  // Вычисляем номер ряда для папки
  int rowToParent = 0;
  QString qStr = QString("SELECT max(rowToParent) FROM feeds WHERE parentId='%1'").
      arg(parentId);
  q.exec(qStr);
  if (q.next() && !q.value(0).isNull()) rowToParent = q.value(0).toInt() + 1;

  // Добавляем папку
  q.prepare("INSERT INTO feeds(text, created, parentId, rowToParent) "
            "VALUES (:text, :feedCreateTime, :parentId, :rowToParent)");
  q.bindValue(":text", folderText);
  q.bindValue(":feedCreateTime",
              QLocale::c().toString(QDateTime::currentDateTimeUtc(), "yyyy-MM-ddTHH:mm:ss"));
  q.bindValue(":parentId", parentId);
  q.bindValue(":rowToParent", rowToParent);
  q.exec();

  folderId = q.lastInsertId().toInt();

  treeItems = foldersTree_->findItems(QString::number(parentId),
                                      Qt::MatchFixedString | Qt::MatchRecursive,
                                      1);
  QStringList treeItem;
  treeItem << folderText << QString::number(folderId);
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeItems.at(0)->addChild(treeWidgetItem);
  foldersTree_->setCurrentItem(treeWidgetItem);

  delete addFolderDialog;
}
