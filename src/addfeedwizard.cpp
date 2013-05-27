/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "addfeedwizard.h"
#include "addfolderdialog.h"
#include "authenticationdialog.h"

#include <QDomDocument>

extern QString kCreateNewsTableQuery;

AddFeedWizard::AddFeedWizard(QWidget *parent, const QString &dataDirPath, int curFolderId)
  : QWizard(parent),
    curFolderId_(curFolderId)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Add Feed"));
  setWizardStyle(QWizard::ModernStyle);
  setOptions(QWizard::HaveFinishButtonOnEarlyPages |
             QWizard::NoBackButtonOnStartPage);

  addPage(createUrlFeedPage());
  addPage(createNameFeedPage());

  persistentUpdateThread_ = new UpdateThread(this);
  persistentParseThread_ = new ParseThread(this, dataDirPath);

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
        clipboardStr.contains("https://", Qt::CaseInsensitive) ||
        clipboardStr.contains("www.", Qt::CaseInsensitive)) {
      urlFeedEdit_->setText(clipboard_->text());
      urlFeedEdit_->selectAll();
      urlFeedEdit_->setFocus();
    }
  }
}

QWizardPage *AddFeedWizard::createUrlFeedPage()
{
  QWizardPage *page = new QWizardPage;
  page->setTitle(tr("Create New Feed"));

  selectedPage = false;
  finishOn = false;

  urlFeedEdit_ = new LineEdit(this);
  urlFeedEdit_->setText("http://");

  titleFeedAsName_ = new QCheckBox(
        tr("Use title of the feed as displayed name"), this);
  titleFeedAsName_->setChecked(true);

  authentication_ = new QGroupBox(this);
  authentication_->setTitle(tr("Server requires authentication:"));
  authentication_->setCheckable(true);
  authentication_->setChecked(false);

  user_ = new LineEdit(this);
  pass_ = new LineEdit(this);
  pass_->setEchoMode(QLineEdit::Password);

  QGridLayout *authenticationLayout = new QGridLayout();
  authenticationLayout->addWidget(new QLabel(tr("Username:")), 0, 0);
  authenticationLayout->addWidget(user_, 0, 1);
  authenticationLayout->addWidget(new QLabel(tr("Password:")), 1, 0);
  authenticationLayout->addWidget(pass_, 1, 1);

  authentication_->setLayout(authenticationLayout);

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
  layout->addSpacing(10);
  layout->addWidget(authentication_);
  layout->addStretch(1);
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

  QSqlQuery q;
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
      if (folderId.toInt() == curFolderId_)
        foldersTree_->setCurrentItem(treeWidgetItem);
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
  if (feedUrl.scheme().isEmpty()) {
    feedUrlString_ = "http://" + feedUrlString_;
    feedUrl.setUrl(feedUrlString_);
    urlFeedEdit_->setText(feedUrlString_);
  }

  if (feedUrl.host().isEmpty()) {
    textWarning->setText(tr("URL error!"));
    warningWidget_->setVisible(true);
    return;
  }

  QSqlQuery q;
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

    // Calculate row's number to insert new feed
    int rowToParent = 0;
    q.exec("SELECT count(id) FROM feeds WHERE parentId=0");
    if (q.next()) rowToParent = q.value(0).toInt();

    int auth = 0;
    QString userInfo;
    if (authentication_->isChecked()) {
      auth = 1;
      userInfo = QString("%1:%2").arg(user_->text()).arg(pass_->text());
    }

    // Insert feed
    q.prepare("INSERT INTO feeds(xmlUrl, created, rowToParent, authentication) "
              "VALUES (:feedUrl, :feedCreateTime, :rowToParent, :authentication)");
    q.bindValue(":feedUrl", feedUrlString_);
    q.bindValue(":feedCreateTime",
        QLocale::c().toString(QDateTime::currentDateTimeUtc(), "yyyy-MM-ddTHH:mm:ss"));
    q.bindValue(":rowToParent", rowToParent);
    q.bindValue(":authentication", auth);
    q.exec();

    q.finish();

    emit signalRequestUrl(feedUrlString_, QDateTime(), userInfo);
  }
}

void AddFeedWizard::deleteFeed()
{
  int id = -1;

  QSqlQuery q;
  q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
  q.bindValue(":xmlUrl", feedUrlString_);
  q.exec();
  if (q.next()) id = q.value(0).toInt();
  if (id >= 0) {
    q.exec(QString("DELETE FROM feeds WHERE id='%1'").arg(id));
    q.exec(QString("DELETE FROM news WHERE feedId='%1'").arg(id));
  }

  // Correct rowToParent field
  QList<int> idList;
  q.exec("SELECT id FROM feeds WHERE parentId=0 ORDER BY rowToParent");
  while (q.next()) {
    idList << q.value(0).toInt();
  }
  for (int i = 0; i < idList.count(); i++) {
    q.exec(QString("UPDATE feeds SET rowToParent='%1' WHERE id=='%2'").
           arg(i).arg(idList.at(i)));
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
}

void AddFeedWizard::slotProgressBarUpdate()
{
  progressBar_->update();
  if (progressBar_->isVisible())
    QTimer::singleShot(250, this, SLOT(slotProgressBarUpdate()));
}

void AddFeedWizard::getUrlDone(const int &result, const QString &feedUrlStr,
                               const QByteArray &data, const QDateTime &dtReply)
{
  if (!data.isEmpty()) {
    bool isFeed = false;
    QString errorStr;
    int errorLine;
    int errorColumn;
    QDomDocument doc("parseDoc");
    if (!doc.setContent(data, false, &errorStr, &errorLine, &errorColumn)) {
      qDebug() << QString("Parse data error: line %1, column %2: %3").
                  arg(errorLine).arg(errorColumn).arg(errorStr);
    } else {
      QDomElement docElem = doc.documentElement();
      if ((docElem.tagName() == "rss") || (docElem.tagName() == "feed") ||
          (docElem.tagName() == "rdf:RDF"))
        isFeed = true;
    }

    if (!isFeed) {
      QString str = QString::fromUtf8(data);

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
          QUrl feedUrl(feedUrlStr);
          if (url.host().isEmpty()) {
            url.setScheme(feedUrl.scheme());
            url.setHost(feedUrl.host());
            if (feedUrl.toString().indexOf('?') > -1) {
              str = feedUrl.path();
              str = str.left(str.lastIndexOf('/')+1);
              url.setPath(str+url.path());
            }
          }
          linkFeedString = url.toString();
          qDebug() << "Parse feed URL, valid:" << linkFeedString;
          int parseFeedId = 0;

          QSqlQuery q;
          int duplicateFoundId = -1;
          q.prepare("SELECT id FROM feeds WHERE xmlUrl LIKE :xmlUrl");
          q.bindValue(":xmlUrl", linkFeedString);
          q.exec();
          if (q.next()) duplicateFoundId = q.value(0).toInt();

          if (0 <= duplicateFoundId) {
            if (feedUrlString_ != linkFeedString)
              textWarning->setText(tr("Duplicate feed!"));
            else
              textWarning->setText(tr("Can't find feed URL!"));
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

            authentication_->setChecked(false);

            emit signalRequestUrl(linkFeedString, QDateTime(), "");
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
      return;
    }

    emit xmlReadyParse(data, feedUrlStr, dtReply);
  }

  if ((result < 0) || data.isEmpty()) {
    if (result == -1)
      textWarning->setText(tr("URL error!"));
    else if (result == -2)
      textWarning->setText(tr("Server requires authentication!"));
    else if (result == -3)
      textWarning->setText(tr("Request timeout!"));
    else if (result == -4)
      textWarning->setText(tr("Redirect error!"));
    else
      textWarning->setText(tr("Request failed!"));
    warningWidget_->setVisible(true);

    deleteFeed();
    progressBar_->hide();
    page(0)->setEnabled(true);
    selectedPage = false;
    button(QWizard::CancelButton)->setEnabled(true);
  }
}

void AddFeedWizard::slotUpdateFeed(const QString &feedUrl, const bool &, int newCount)
{
  qDebug() << "ParseDone" << feedUrl;
  selectedPage = true;
  newCount_ = newCount;

  if (titleFeedAsName_->isChecked()) {
    QSqlQuery q;
    q.prepare("SELECT title FROM feeds WHERE xmlUrl LIKE :xmlUrl");
    q.bindValue(":xmlUrl", feedUrl);
    q.exec();
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

  QSqlQuery q;
  q.prepare("SELECT id, htmlUrl FROM feeds WHERE xmlUrl LIKE :xmlUrl");
  q.bindValue(":xmlUrl", feedUrlString_);
  q.exec();
  if (q.next()) {
    parseFeedId = q.value(0).toInt();
    htmlUrlString_ = q.value(1).toString();
  }

  if (foldersTree_->currentItem()->text(1) != "0")
    parentId = foldersTree_->currentItem()->text(1).toInt();

  // Correct rowToParent field
  QList<int> idList;
  q.exec("SELECT id FROM feeds WHERE parentId=0 ORDER BY rowToParent");
  while (q.next()) {
    if (parseFeedId != q.value(0).toInt())
      idList << q.value(0).toInt();
  }
  for (int i = 0; i < idList.count(); i++) {
    q.exec(QString("UPDATE feeds SET rowToParent='%1' WHERE id=='%2'").
           arg(i).arg(idList.at(i)));
  }

  // Calculate row number to insert feed
  int rowToParent = 0;
  q.exec(QString("SELECT count(id) FROM feeds WHERE parentId='%1' AND id!='%2'").
         arg(parentId).arg(parseFeedId));
  if (q.next()) rowToParent = q.value(0).toInt();

  int auth = 0;
  if (authentication_->isChecked()) auth = 1;

  q.prepare("UPDATE feeds SET text = ?, parentId = ?, rowToParent = ?, authentication = ? WHERE id == ?");
  q.addBindValue(nameFeedEdit_->text());
  q.addBindValue(parentId);
  q.addBindValue(rowToParent);
  q.addBindValue(auth);
  q.addBindValue(parseFeedId);
  q.exec();

  if (auth) {
    QUrl url(feedUrlString_);
    QString server = url.host();

    q.prepare("SELECT * FROM passwords WHERE server=?");
    q.addBindValue(server);
    q.exec();
    if (!q.next()) {
      q.prepare("INSERT INTO passwords (server, username, password) "
                "VALUES (:server, :username, :password)");
      q.bindValue(":server", server);
      q.bindValue(":username", user_->text());
      q.bindValue(":password", pass_->text().toUtf8().toBase64());
      q.exec();
    }
  }

  feedId_ = parseFeedId;
  feedParentId_ = parentId;

  accept();
}

/*! \brief Adding new folder **************************************************/
void AddFeedWizard::newFolder()
{
  AddFolderDialog *addFolderDialog = new AddFolderDialog(this);
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

  QSqlQuery q;

  // Calculate row number to insert folder
  int rowToParent = 0;
  q.exec(QString("SELECT count(id) FROM feeds WHERE parentId='%1'").
         arg(parentId));
  if (q.next()) rowToParent = q.value(0).toInt();

  // Add folder
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

void AddFeedWizard::slotAuthentication(QNetworkReply *reply, QAuthenticator *auth)
{
  AuthenticationDialog *authenticationDialog =
      new AuthenticationDialog(this, reply->url(), auth);

  if (!authenticationDialog->save_->isChecked())
    authenticationDialog->exec();

  delete authenticationDialog;
}

void AddFeedWizard::slotFeedCountsUpdate(FeedCountStruct)
{
}
