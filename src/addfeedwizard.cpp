#include "addfeedwizard.h"

extern QString kCreateNewsTableQuery;

AddFeedWizard::AddFeedWizard(QWidget *parent, QSqlDatabase *db)
  : QWizard(parent),
    db_(db)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Add feed"));
  setWindowIcon(QIcon(":/images/feed"));
  setWizardStyle(QWizard::ModernStyle);
  setOptions(QWizard::HaveFinishButtonOnEarlyPages);

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

QWizardPage *AddFeedWizard::createUrlFeedPage()
{
  QWizardPage *page = new QWizardPage;
  page->setTitle(tr("Create new feed"));

  selectedPage = false;
  finishOn = false;

  urlFeedEdit_ = new LineEdit(this);

  QClipboard *clipboard_ = QApplication::clipboard();
  QString clipboardStr = clipboard_->text().left(8);
  if (clipboardStr.contains("http://", Qt::CaseInsensitive) ||
      clipboardStr.contains("https://", Qt::CaseInsensitive)) {
    urlFeedEdit_->setText(clipboard_->text());
    urlFeedEdit_->selectAll();
    urlFeedEdit_->setFocus();
  } else urlFeedEdit_->setText("http://");

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
  page->setTitle(tr("Create new feed"));
  page->setFinalPage(false);

  nameFeedEdit_ = new LineEdit(this);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(new QLabel(tr("Displayed name:")));
  layout->addWidget(nameFeedEdit_);
  page->setLayout(layout);

  connect(nameFeedEdit_, SIGNAL(textChanged(const QString&)),
          this, SLOT(nameFeedEditChanged(const QString&)));

  return page;
}

void AddFeedWizard::urlFeedEditChanged(const QString& text)
{
  button(QWizard::NextButton)->setEnabled(
        !text.isEmpty() && (urlFeedEdit_->text() != "http://"));

  bool buttonEnable = false;
  if (titleFeedAsName_->isChecked() && (urlFeedEdit_->text() != "http://") &&
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
  feedUrlString_ = urlFeedEdit_->text();

  QUrl feedUrl(feedUrlString_);
  if (feedUrl.host().isEmpty()) {
    textWarning->setText(tr("URL error!"));
    warningWidget_->setVisible(true);
    return;
  }

  QSqlQuery q(*db_);
  int duplicateFoundId = -1;
  q.exec(QString("select id from feeds where xmlUrl like '%1'").
         arg(feedUrlString_));
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

    QString qStr = QString("insert into feeds(xmlUrl) values (?)");
    q.prepare(qStr);
    q.addBindValue(feedUrlString_);
    q.exec();
    q.finish();

    persistentUpdateThread_->requestUrl(feedUrlString_, QDateTime());
  }
}

void AddFeedWizard::deleteFeed()
{
  int id = -1;

  QSqlQuery q(*db_);
  q.exec(QString("select id from feeds where xmlUrl like '%1'").
         arg(feedUrlString_));
  if (q.next()) id = q.value(0).toInt();
  if (id >= 0) {
    q.exec(QString("delete from feeds where id='%1'").arg(id));
    q.exec(QString("drop table feed_%1").arg(id));
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
          QString linkFeed = rx.cap(1);
          QUrl url(linkFeed);
          if (url.host().isEmpty()) {
            url.setScheme(url_.scheme());
            url.setHost(url_.host());
            if (url_.toString().indexOf('?') > -1) {
              str = url_.path();
              str = str.left(str.lastIndexOf('/')+1);
              url.setPath(str+url.path());
            }
          }
          linkFeed = url.toString();
          qDebug() << "Parse feed URL, valid:" << linkFeed;
          int parseFeedId = 0;

          QSqlQuery q(*db_);
          int duplicateFoundId = -1;
          q.exec(QString("select id from feeds where xmlUrl like '%1'").
                 arg(linkFeed));
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
            q.exec(QString("select id from feeds where xmlUrl like '%1'").
                   arg(feedUrlString_));
            if (q.next()) parseFeedId = q.value(0).toInt();

            feedUrlString_ = linkFeed;
            db_->exec(QString("update feeds set xmlUrl = '%1' where id == '%2'").
                      arg(linkFeed).
                      arg(parseFeedId));

            emit startGetUrlTimer();
            persistentUpdateThread_->requestUrl(linkFeed, QDateTime());
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
    QSqlQuery q = db_->exec(QString("update feeds set lastBuildDate = '%1' where xmlUrl == '%2'").
                            arg(dtReply.toString(Qt::ISODate)).
                            arg(url_.toString()));
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

void AddFeedWizard::slotUpdateFeed(const QUrl &url, const bool &)
{
  qDebug() << "ParseDone" << url.toString();
  selectedPage = true;

  if (titleFeedAsName_->isChecked()) {
    int parseFeedId = 0;
    QSqlQuery q(*db_);
    q.exec(QString("select id from feeds where xmlUrl like '%1'").
           arg(url.toString()));
    if (q.next()) parseFeedId = q.value(0).toInt();

    q.exec(QString("select title from feeds where id=='%1'").
           arg(parseFeedId));
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
  QSqlQuery q(*db_);
  q.exec(QString("select id from feeds where xmlUrl like '%1'").
         arg(feedUrlString_));
  if (q.next()) parseFeedId = q.value(0).toInt();

  q.exec(QString("select htmlUrl from feeds where id=='%1'").
         arg(parseFeedId));
  if (q.next()) htmlUrlString_ = q.value(0).toString();

  db_->exec(QString("update feeds set text = '%1' where id == '%2'").
            arg(nameFeedEdit_->text()).
            arg(parseFeedId));
  accept();
}
