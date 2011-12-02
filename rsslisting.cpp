#include <QtDebug>
#include <QtCore>
#include <windows.h>
#include <Psapi.h>

#include "addfeeddialog.h"
#include "optionsdialog.h"
#include "rsslisting.h"
#include "VersionNo.h"

/*!****************************************************************************/
const QString kCreateFeedTableQuery(
    "create table feed_%1("
        "id integer primary key, "
        "guid varchar, "
        "description varchar, "
        "title varchar, "
        "published varchar, "
        "modified varchar, "
        "received varchar, "
        "author varchar, "
        "category varchar, "
        "label varchar, "
        "status integer default 0, "
        "sticky integer default 0, "
        "deleted integer default 0, "
        "attachment varchar, "
        "feed varchar, "
        "location varchar, "
        "link varchar"
    ")");

/*! \fn RSSListing::RSSListing() **********************************************/
RSSListing::RSSListing(QWidget *parent)
    : QMainWindow(parent), currentReply_(0)
{
    QString AppFileName = qApp->applicationDirPath()+"/QtRSS.ini";
    settings_ = new QSettings(AppFileName, QSettings::IniFormat);

    db_ = QSqlDatabase::addDatabase("QSQLITE");
    db_.setDatabaseName("data.db");
    if (QFile("data.db").exists()) {
      db_.open();
    } else {  // Инициализация базы
      db_.open();
      db_.exec("create table feeds(id integer primary key, text varchar,"
          " title varchar, description varchar, xmlurl varchar, htmlurl varchar)");
      db_.exec("insert into feeds(text, xmlurl) "
          "values ('Qt Labs', 'http://labs.qt.nokia.com/blogs/feed')");
      db_.exec("insert into feeds(text, xmlurl) "
          "values ('Qt Russian Forum', 'http://www.prog.org.ru/index.php?type=rss;action=.xml')");
      db_.exec("create table info(id integer primary key, name varchar, value varchar)");
      db_.exec("insert into info(name, value) values ('version', '1.0')");
      db_.exec(kCreateFeedTableQuery.arg(1));
      db_.exec(kCreateFeedTableQuery.arg(2));
    }

    feedsModel_ = new QSqlTableModel();
    feedsModel_->setTable("feeds");
    feedsModel_->select();

    feedsView_ = new QTreeView();
    feedsView_->setObjectName("feedsTreeView_");
    feedsView_->setModel(feedsModel_);
    feedsView_->header()->setResizeMode(QHeaderView::ResizeToContents);
    feedsView_->setUniformRowHeights(true);
    feedsView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(feedsView_, SIGNAL(clicked(QModelIndex)),
            this, SLOT(slotFeedsTreeClicked(QModelIndex)));
    connect(feedsView_, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(slotFeedsTreeDoubleClicked(QModelIndex)));
    connect(this, SIGNAL(signalFeedsTreeKeyUpDownPressed()),
            SLOT(slotFeedsTreeKeyUpDownPressed()), Qt::QueuedConnection);


    newsModel_ = new QSqlTableModel();
    newsView_ = new QTableView();
    newsView_->setObjectName("newsView_");
    newsView_->setModel(newsModel_);
    newsView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    newsView_->horizontalHeader()->setStretchLastSection(true);
    newsView_->verticalHeader()->setDefaultSectionSize(
        newsView_->verticalHeader()->minimumSectionSize());
    newsView_->verticalHeader()->setVisible(false);
    newsView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    newsView_->setShowGrid(false);
    newsView_->horizontalHeader()->setHighlightSections(false);
//    feedView_->setFocusPolicy(Qt::NoFocus);

    connect(newsView_, SIGNAL(clicked(QModelIndex)),
            this, SLOT(slotFeedViewClicked(QModelIndex)));
    connect(this, SIGNAL(signalFeedKeyUpDownPressed()),
            SLOT(slotFeedKeyUpDownPressed()), Qt::QueuedConnection);

    QStringList headerLabels;
    headerLabels << tr("Item") << tr("Title") << tr("Link") << tr("Description")
                 << tr("Comments") << tr("pubDate") << tr("guid");
    treeWidget_ = new QTreeWidget();
    connect(treeWidget_, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            this, SLOT(itemActivated(QTreeWidgetItem*)));
    treeWidget_->setHeaderLabels(headerLabels);
    treeWidget_->header()->setResizeMode(QHeaderView::ResizeToContents);

    webView_ = new QWebView();

    networkProxy_.setType(QNetworkProxy::HttpProxy);
    networkProxy_.setHostName("10.0.0.172");
    networkProxy_.setPort(3150);
    manager_.setProxy(networkProxy_);
    QNetworkProxy::setApplicationProxy(networkProxy_);

    connect(&manager_, SIGNAL(finished(QNetworkReply*)),
             this, SLOT(finished(QNetworkReply*)));

    //! Create feed layout
    feedsTabWidget_ = new QTabWidget();
    feedsTabWidget_->addTab(feedsView_, tr("Feeds"));

    //! Create news layout
    QVBoxLayout *webLayout = new QVBoxLayout();
    webLayout->setMargin(1);  // Чтобы было видно границу виджета
    webLayout->addWidget(webView_);

    QWidget *webWidget = new QWidget();
    webWidget->setStyleSheet("border: 1px solid gray");
    webWidget->setLayout(webLayout);

    QSplitter *feedTabSplitter = new QSplitter(Qt::Vertical);
    feedTabSplitter->addWidget(newsView_);
    feedTabSplitter->addWidget(webWidget);

    newsTabWidget_ = new QTabWidget();
    newsTabWidget_->addTab(feedTabSplitter, "");

    QSplitter *contentSplitter = new QSplitter(Qt::Vertical);
    contentSplitter->addWidget(treeWidget_);
    contentSplitter->addWidget(newsTabWidget_);

    //! Combine layouts
    QSplitter *feedSplitter = new QSplitter();
    feedSplitter->addWidget(feedsTabWidget_);
    feedSplitter->addWidget(contentSplitter);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(4);
    layout->addWidget(feedSplitter);

    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(layout);

    setCentralWidget(centralWidget);


    setWindowTitle(QString("QtRSS v") +
                   QString(STRFILEVER).left(QString(STRFILEVER).lastIndexOf('.')));

    createActions();
    createMenu();
    createToolBar();

    feedsView_->installEventFilter(this);
    newsView_->installEventFilter(this);

    //! GIU tuning
    toggleQueryResults(false);
    toggleToolBar(false);

    statusBar()->setVisible(true);

    //! testing
    webView_->load(QUrl("qrc:/html/test1.html"));
    webView_->show();

    traySystem = new QSystemTrayIcon(QIcon(":/images/images/QtRSS32.png"),this);
    connect(traySystem,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(slotActivationTray(QSystemTrayIcon::ActivationReason)));
    connect(this,SIGNAL(signalPlaceToTray()),this,SLOT(slotPlaceToTray()),Qt::QueuedConnection);
    traySystem->setToolTip("QtRSS");
    createTrayMenu();
    traySystem->show();

    connect(this, SIGNAL(signalCloseApp()),
            SLOT(slotCloseApp()), Qt::QueuedConnection);

    readSettings();

    //Установка шрифтов и их настроек для элементов
//    QFont font_ = newsTabWidget_->font();
//    font_.setBold(true);
//    newsTabWidget_->setFont(font_);
}

/*! \fn RSSListing::~RSSListing() *********************************************/
RSSListing::~RSSListing()
{
  qDebug("App_Close");
  delete newsView_;
  delete feedsView_;
  delete newsModel_;
  delete feedsModel_;

  db_.close();

  QSqlDatabase::removeDatabase(QString());
}

/*! \ат RSSListing::eventFilter(QObject *obj, QEvent *event) ******************/
bool RSSListing::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == feedsView_) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
      if ((keyEvent->key() == Qt::Key_Up) ||
          (keyEvent->key() == Qt::Key_Down)) {
        emit signalFeedsTreeKeyUpDownPressed();
      }
      return false;
    } else {
      return false;
    }
  } else if (obj == newsView_) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
      if ((keyEvent->key() == Qt::Key_Up) ||
          (keyEvent->key() == Qt::Key_Down)) {
        emit signalFeedKeyUpDownPressed();
      }
      return false;
    } else {
      return false;
    }
  } else {
    // pass the event on to the parent class
    return QMainWindow::eventFilter(obj, event);
  }
}

/*! \fn void RSSListing::closeEvent(QCloseEvent* pe) **************************
 * \brief ОБработка событий закрытия окна
 ******************************************************************************/
/*virtual*/ void RSSListing::closeEvent(QCloseEvent* event)
{
  oldState = windowState();
  event->ignore();
  emit signalPlaceToTray();
}

/*! \brief Обработка события выхода из приложения *****************************/
void RSSListing::slotClose()
{
  traySystem->hide();
  writeSettings();
  emit signalCloseApp();
}

/*! \brief Завершение приложения **********************************************/
void RSSListing::slotCloseApp()
{
  qApp->quit();
}

/*! \brief Обработка события изменения состояния окна *************************/
/*virtual*/ void RSSListing::changeEvent(QEvent *event)
{
  if(event->type() == QEvent::WindowStateChange) {
    oldState = ((QWindowStateChangeEvent*)event)->oldState();
    if(isMinimized()) {
      event->ignore();
      emit signalPlaceToTray();
      return;
    }
  }
  QMainWindow::changeEvent(event);
}

/*! \brief Обработка события помещения программы в трей ***********************/
void RSSListing::slotPlaceToTray()
{
  QTimer::singleShot(10000, this, SLOT(myEmptyWorkingSet()));
  hide();
}

/*! \brief Обработка событий трея *********************************************/
void RSSListing::slotActivationTray(QSystemTrayIcon::ActivationReason reason)
{
  switch (reason) {
  case QSystemTrayIcon::Unknown:
    break;
  case QSystemTrayIcon::Context:
    trayMenu_->activateWindow();
    break;
  case QSystemTrayIcon::DoubleClick:
    slotShowWindows();
    break;
  case QSystemTrayIcon::Trigger:
    break;
  case QSystemTrayIcon::MiddleClick:
    break;
  }
}

/*! \brief Отображение окна по событию ****************************************/
void RSSListing::slotShowWindows()
{
  if (oldState == Qt::WindowMaximized) {
    showMaximized();
  } else {
    showNormal();
  }
  activateWindow();
}

/*! \fn void RSSListing::createActions() **************************************
 * \brief Создание действий

   Которые будут использоваться в главном меню и ToolBar
 ******************************************************************************/
void RSSListing::createActions()
{
  addFeedAct_ = new QAction(QIcon(":/images/addFeed"), tr("&Add..."), this);
  addFeedAct_->setStatusTip(tr("Add new feed"));
  addFeedAct_->setShortcut(QKeySequence::New);
  connect(addFeedAct_, SIGNAL(triggered()), this, SLOT(addFeed()));

  deleteFeedAct_ = new QAction(QIcon(":/images/deleteFeed"), tr("&Delete..."), this);
  deleteFeedAct_->setStatusTip(tr("Delete selected feed"));
  connect(deleteFeedAct_, SIGNAL(triggered()), this, SLOT(deleteFeed()));

  importFeedsAct_ = new QAction(tr("&Import feeds..."), this);
  importFeedsAct_->setStatusTip(tr("Import feeds from OPML file"));
  connect(importFeedsAct_, SIGNAL(triggered()), this, SLOT(importFeeds()));

  exitAct_ = new QAction(tr("E&xit"), this);
  exitAct_->setShortcut(QKeySequence::Quit);  // empty on windows :(
  connect(exitAct_, SIGNAL(triggered()), this, SLOT(slotClose()));

  toolBarToggle_ = new QAction(tr("&ToolBar"), this);
  toolBarToggle_->setCheckable(true);
  toolBarToggle_->setStatusTip(tr("Show ToolBar"));
  connect(toolBarToggle_, SIGNAL(toggled(bool)), this, SLOT(toggleToolBar(bool)));

  treeWidgetToggle_ = new QAction(tr("&Query results"), this);
  treeWidgetToggle_->setCheckable(true);
  treeWidgetToggle_->setStatusTip(tr("Show table with query results"));
  connect(treeWidgetToggle_, SIGNAL(toggled(bool)), this, SLOT(toggleQueryResults(bool)));

  optionsAct_ = new QAction(tr("Options..."), this);
  optionsAct_->setStatusTip(tr("Open options gialog"));
  optionsAct_->setShortcut(Qt::Key_F8);
  connect(optionsAct_, SIGNAL(triggered()), this, SLOT(showOptionDlg()));
}

/*! \fn void RSSListing::createMenu() *****************************************
 * \brief Создание главного меню
 ******************************************************************************/
void RSSListing::createMenu()
{
  fileMenu_ = menuBar()->addMenu(tr("&File"));
  fileMenu_->addAction(addFeedAct_);
  fileMenu_->addAction(deleteFeedAct_);
  fileMenu_->addSeparator();
  fileMenu_->addAction(importFeedsAct_);
  fileMenu_->addSeparator();
  fileMenu_->addAction(exitAct_);

  menuBar()->addMenu(tr("&Edit"));

  viewMenu_ = menuBar()->addMenu(tr("&View"));
  viewMenu_->addAction(toolBarToggle_);
  viewMenu_->addAction(treeWidgetToggle_);

  feedMenu_ = menuBar()->addMenu(tr("Fee&ds"));
  menuBar()->addMenu(tr("&News"));

  toolsMenu_ = menuBar()->addMenu(tr("&Tools"));
  toolsMenu_->addAction(optionsAct_);

  menuBar()->addMenu(tr("&Help"));
}

/*! \fn void RSSListing::createToolBar() **************************************
 * \brief Создание ToolBar
 ******************************************************************************/
void RSSListing::createToolBar()
{
  toolBar_ = addToolBar(tr("General"));
  toolBar_->setObjectName("ToolBar_General");
  toolBar_->addAction(addFeedAct_);
  toolBar_->addAction(deleteFeedAct_);
  toolBar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

/*! \fn void RSSListing::get(const QUrl &url) *********************************
 * \brief Starts the network request and connects the needed signals
 ******************************************************************************/
void RSSListing::get(const QUrl &url)
{
    qDebug() << "get:" << url;
    QNetworkRequest request(url);
    if (currentReply_) {
        currentReply_->disconnect(this);
        currentReply_->deleteLater();
    }
    currentReply_ = manager_.get(request);
    connect(currentReply_, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(currentReply_, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
    connect(currentReply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
    statusBar()->showMessage(QString("Fetching start..."), 3000);
}

/*! \fn void RSSListing::readSettings() ***************************************
 * \brief Чтение настроек из ini-файла
 ******************************************************************************/
void RSSListing::readSettings()
{
  settings_->beginGroup("/Settings");

  QString fontFamily = settings_->value("/FontFamily", "Tahoma").toString();
  int fontSize = settings_->value("/FontSize", 8).toInt();
  qApp->setFont(QFont(fontFamily, fontSize));

  settings_->endGroup();

  restoreGeometry(settings_->value("GeometryState").toByteArray());
  restoreState(settings_->value("ToolBarsState").toByteArray());

  // Загрузка ширины столбцов таблицы
  for (int i=0; i < newsModel_->columnCount(); ++i)
    newsView_->setColumnWidth(i, settings_->value(
         QString("newsView/columnWidth%1").arg(i), 100).toInt());
}

/*! \fn void RSSListing::writeSettings() **************************************
 * \brief Запись настроек в ini-файл
 ******************************************************************************/
void RSSListing::writeSettings()
{
  settings_->beginGroup("/Settings");

  QString strLocalLang = QLocale::system().name();
  QString lang = settings_->value("/Lang", strLocalLang).toString();
  settings_->setValue("/Lang", lang);

  QString fontFamily = settings_->value("/FontFamily", "Tahoma").toString();
  settings_->setValue("/FontFamily", fontFamily);
  int fontSize = settings_->value("/FontSize", 8).toInt();
  settings_->setValue("/FontSize", fontSize);

  settings_->endGroup();

  settings_->setValue("GeometryState", saveGeometry());
  settings_->setValue("ToolBarsState", saveState());

  // Сохранение ширины столбцов таблицы
  for (int i=0; i < newsModel_->columnCount(); ++i)
    settings_->setValue(QString("newsView/columnWidth%1").arg(i),
        newsView_->columnWidth(i));

}

/*! \fn void RSSListing::addFeed() ********************************************
 * \brief Добавление ленты в список лент
 ******************************************************************************/
void RSSListing::addFeed()
{
  AddFeedDialog *addFeedDialog = new AddFeedDialog(this);
  if (addFeedDialog->exec() == QDialog::Rejected) return;

  QSqlQuery q(db_);
  QString qStr = QString("insert into feeds(link) values (%1)").
      arg(addFeedDialog->feedEdit_->text());
  q.exec(qStr);
  q.exec(kCreateFeedTableQuery.arg(q.lastInsertId().toString()));
  q.finish();
  feedsModel_->select();
}

/*! \fn void RSSListing::deleteFeed() *****************************************
 * \brief Удаление ленты из списка лент с подтверждением
 ******************************************************************************/
void RSSListing::deleteFeed()
{
  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setText(QString("Are you sure to delete the feed '%1'?").
                 arg(feedsModel_->record(feedsView_->currentIndex().row()).field("text").value().toString()));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::No);

  if (msgBox.exec() == QMessageBox::No) return;

  QSqlQuery q(db_);
  QString str = QString("delete from feeds where text='%1'").
      arg(feedsModel_->record(feedsView_->currentIndex().row()).field("text").value().toString());
  q.exec(str);
  q.exec(QString("drop table feed_%1").
      arg(feedsModel_->record(feedsView_->currentIndex().row()).field("id").value().toString()));
  q.finish();
  feedsModel_->select();
}

/*! \brief Импорт лент из OPML-файла ******************************************
 * \fn void RSSListing::deleteFeed()
 ******************************************************************************/
void RSSListing::importFeeds()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select OPML-file"),
      qApp->applicationDirPath(),
      tr("OPML-files (*.opml)"));

  if (fileName.isNull()) {
    statusBar()->showMessage(tr("Import canceled"), 3000);
    return;
  }

  qDebug() << "process file:" << fileName;

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    statusBar()->showMessage(tr("Import: can't open a file"), 3000);
    return;
  }

  db_.transaction();

  QXmlStreamReader xml(&file);

  QString currentString;
  int elementCount = 0;
  int outlineCount = 0;
  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isStartElement()) {
      statusBar()->showMessage(QVariant(elementCount).toString(), 3000);
      // Выбираем одни outline'ы
      if (xml.name() == "outline") {
        currentTag = xml.name().toString();
        qDebug() << outlineCount << "+:" << xml.prefix().toString() << ":" << currentTag;
        QSqlQuery q(db_);
        QString qStr = QString("insert into feeds(text, title, description, xmlurl, htmlurl) "
                       "values(?, ?, ?, ?, ?)");
        q.prepare(qStr);
        q.addBindValue(xml.attributes().value("text").toString());
        q.addBindValue(xml.attributes().value("title").toString());
        q.addBindValue(xml.attributes().value("description").toString());
        q.addBindValue(xml.attributes().value("xmlUrl").toString());
        q.addBindValue(xml.attributes().value("htmlUrl").toString());
        q.exec();
        qDebug() << q.lastError().number() << ": " << q.lastError().text();
        q.exec(kCreateFeedTableQuery.arg(q.lastInsertId().toString()));
        q.finish();
      }
    } else if (xml.isEndElement()) {
      if (xml.name() == "outline") {
        ++outlineCount;
      }
      ++elementCount;
    } else if (xml.isCharacters() && !xml.isWhitespace()) {
      if (currentTag == "item")
        itemString += xml.text().toString();
      else if (currentTag == "title")
        titleString += xml.text().toString();
      else if (currentTag == "link")
        linkString += xml.text().toString();
      else if (currentTag == "description")
        descriptionString += xml.text().toString();
      else if (currentTag == "comments")
        commentsString += xml.text().toString();
      else if (currentTag == "pubDate")
        pubDateString += xml.text().toString();
      else if (currentTag == "guid")
        guidString += xml.text().toString();
      else currentString += xml.text().toString();
    }
  }
  if (xml.error()) {
    statusBar()->showMessage(QString("Import error: Line=%1, ErrorString=%2").
        arg(xml.lineNumber()).arg(xml.errorString()), 3000);
  } else {
    statusBar()->showMessage(QString("Import: file read done"), 3000);
  }
  db_.commit();
  feedsModel_->select();
}

/*! \brief Обработка события изменения метаданных интернет-запроса ************/
void RSSListing::metaDataChanged()
{
    QUrl redirectionTarget = currentReply_->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectionTarget.isValid()) {
        qDebug() << "get redirect...";
        get(redirectionTarget);
    }
}

/*! \fn void RSSListing::readyRead() ******************************************
 * \brief Reads data received from the RDF source.
 *
 *   We read all the available data, and pass it to the XML
 *   stream reader. Then we call the XML parsing function.
 ******************************************************************************/
void RSSListing::readyRead()
{
    int statusCode = currentReply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode >= 200 && statusCode < 300) {
        QByteArray data = currentReply_->readAll();
        xml.addData(data);
        parseXml();
    }
}

/*! \fn void RSSListing::finished(QNetworkReply *reply) ***********************
 * \brief Finishes processing an HTTP request.

    The default behavior is to keep the text edit read only.

    If an error has occurred, the user interface is made available
    to the user for further input, allowing a new fetch to be
    started.

    If the HTTP get request has finished, we make the
    user interface available to the user for further input.
 ******************************************************************************/
void RSSListing::finished(QNetworkReply *reply)
{
    Q_UNUSED(reply);
    addFeedAct_->setEnabled(true);
    deleteFeedAct_->setEnabled(true);
    feedsView_->setEnabled(true);
}

/*! \fn void RSSListing::parseXml() *******************************************
 * \brief Разбор xml-файла
 ******************************************************************************/
void RSSListing::parseXml()
{
  // поиск идентификатора ленты с таблице лент
  int parseFeedId = 0;
  QSqlQuery q(db_);
  q.exec(QString("select id from feeds where xmlurl like '%1'").
      arg(currentUrl_.toString()));
  while (q.next()) {
    parseFeedId = q.value(q.record().indexOf("id")).toInt();
  }

  // идентификатор не найден (например, во время запроса удалили ленту)
  if (0 == parseFeedId) {
    qDebug() << QString("Feed '%1' not found").arg(currentUrl_.toString());
    return;
  }

  // собственно сам разбор
  db_.transaction();
  int itemCount = 0;
  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isStartElement()) {
      if (xml.name() == "item")
        linkString = xml.attributes().value("rss:about").toString();
      currentTag = xml.name().toString();
      qDebug() << itemCount << ": " << xml.namespaceUri().toString() << ": " << currentTag;
    } else if (xml.isEndElement()) {
      if (xml.name() == "item") {

        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setText(0, itemString);
        item->setText(1, titleString);
        item->setText(2, linkString);
        item->setText(3, descriptionString);
        item->setText(4, commentsString);
        item->setText(5, pubDateString);
        item->setText(6, guidString);
        treeWidget_->addTopLevelItem(item);

        // поиск статей с giud в базе
        QSqlQuery q(db_);
        QString qStr = QString("select * from feed_%1 where guid == '%2'").
            arg(parseFeedId).arg(guidString);
        q.exec(qStr);
        // если статей с таким giud нет, добавляем статью в базу
        if (!q.next()) {
          qStr = QString("insert into feed_%1("
                         "description, guid, title, published, received) "
                         "values(?, ?, ?, ?, ?)").
              arg(parseFeedId);
          q.prepare(qStr);
          q.addBindValue(descriptionString);
          q.addBindValue(guidString);
          q.addBindValue(titleString);
          q.addBindValue(pubDateString);
          q.addBindValue(QDateTime::currentDateTime().toString());
          q.exec();
          qDebug() << q.lastError().number() << ": " << q.lastError().text();
        }
        q.finish();

        itemString.clear();
        titleString.clear();
        linkString.clear();
        descriptionString.clear();
        commentsString.clear();
        pubDateString.clear();
        guidString.clear();
        ++itemCount;
      }
    } else if (xml.isCharacters() && !xml.isWhitespace()) {
      if (currentTag == "item")
        itemString += xml.text().toString();
      else if (currentTag == "title")
        titleString += xml.text().toString();
      else if (currentTag == "link")
        linkString += xml.text().toString();
      else if (currentTag == "description")
        descriptionString += xml.text().toString();
      else if (currentTag == "comments")
        commentsString += xml.text().toString();
      else if (currentTag == "pubDate")
        pubDateString += xml.text().toString();
      else if (currentTag == "guid")
        guidString += xml.text().toString();
    }
  }
  if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
    statusBar()->showMessage(QString("XML ERROR: Line=%1, ErrorString=%2").
                             arg(xml.lineNumber()).arg(xml.errorString()), 3000);
  } else {
    statusBar()->showMessage(QString("Fetching done"), 3000);
  }
  db_.commit();
  slotFeedsTreeClicked(feedsModel_->index(feedsView_->currentIndex().row(), 0));
}

/*! \fn void RSSListing::itemActivated(QTreeWidgetItem * item) ****************
 * \brief Обработка события активации элемента в таблице результатов последнего запроса
 ******************************************************************************/
void RSSListing::itemActivated(QTreeWidgetItem * item)
{
    webView_->setHtml(item->text(3));
}

/*! \fn void RSSListing::error(QNetworkReply::NetworkError) *******************
 * \brief Обработка ошибки html-запроса
 ******************************************************************************/
void RSSListing::error(QNetworkReply::NetworkError)
{
    statusBar()->showMessage("error retrieving RSS feed", 3000);
    currentReply_->disconnect(this);
    currentReply_->deleteLater();
    currentReply_ = 0;
}

/*! \fn void RSSListing::slotFeedsTreeClicked(QModelIndex index) **************
 * \brief Обработка нажатия в дереве лент
 ******************************************************************************/
void RSSListing::slotFeedsTreeClicked(QModelIndex index)
{
  newsModel_->setTable(QString("feed_%1").arg(feedsModel_->index(index.row(), 0).data().toString()));
  newsModel_->select();
  newsView_->setColumnHidden(newsModel_->fieldIndex("id"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("guid"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("description"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("modified"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("author"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("category"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("label"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("status"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("sticky"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("deleted"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("attachment"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("feed"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("location"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("link"), true);
  newsView_->setSortingEnabled(true);
  newsView_->sortByColumn(newsModel_->fieldIndex("published"));
  newsTabWidget_->setTabText(0, feedsModel_->index(index.row(), 1).data().toString());
}

/*! \fn void RSSListing::slotFeedsTreeDoubleClicked(QModelIndex index) ********
 * \brief Обработка двойного нажатия в дереве лент
 ******************************************************************************/
void RSSListing::slotFeedsTreeDoubleClicked(QModelIndex index)
{
  addFeedAct_->setEnabled(false);
  deleteFeedAct_->setEnabled(false);
  feedsView_->setEnabled(false);
  treeWidget_->clear();

  xml.clear();

  currentUrl_.setUrl(feedsModel_->record(index.row()).field("xmlurl").value().toString());
  get(currentUrl_);
}

/*! \fn void RSSListing::slotFeedViewClicked(QModelIndex index) ***************
 * \brief Обработка нажатия в дереве новостей
 ******************************************************************************/
void RSSListing::slotFeedViewClicked(QModelIndex index)
{
  webView_->setHtml(
      newsModel_->record(index.row()).field("description").value().toString());
}

/*! \fn void RSSListing::slotFeedsTreeKeyUpDownPressed() **********************
 * \brief Обработка клавиш Up/Down в дереве лент
 ******************************************************************************/
void RSSListing::slotFeedsTreeKeyUpDownPressed()
{
  slotFeedsTreeClicked(feedsView_->currentIndex());
}

/*! \fn void RSSListing::slotFeedKeyUpDownPressed() ***************************
 * \brief Обработка клавиш Up/Down в дереве новостей
 ******************************************************************************/
void RSSListing::slotFeedKeyUpDownPressed()
{
  slotFeedViewClicked(newsView_->currentIndex());
}

/*! \brief Обработка переключения отображения тулбара *************************
 * \fn void RSSListing::toggleToolBar(bool checked)
 ******************************************************************************/
void RSSListing::toggleToolBar(bool checked)
{
  toolBar_->setVisible(checked);
}

/*! \fn void RSSListing::toggleQueryResults(bool checked) *********************
 * \brief Обработка переключения отображения таблицы результатов последнего запроса
 ******************************************************************************/
void RSSListing::toggleQueryResults(bool checked)
{
  treeWidget_->setVisible(checked);
}

/*! \brief Вызов окна настроек ************************************************
 * \fn void RSSListing::showOptionDlg()
 ******************************************************************************/
void RSSListing::showOptionDlg()
{
  OptionsDialog *optionsDialog = new OptionsDialog(this);
  optionsDialog->restoreGeometry(settings_->value("options/geometry").toByteArray());
  optionsDialog->exec();
  settings_->setValue("options/geometry", optionsDialog->saveGeometry());
}

/*! \brief Обработка сообщений полученных из запущщеной копии программы *******/
void RSSListing::receiveMessage(const QString& message)
{
  qDebug() << QString("Received message: '%1'").arg(message);
  if (!message.isEmpty()){
    QStringList params = message.split('\n');
    foreach (QString param, params) {
      if (param == "--show") slotShowWindows();
      if (param == "--exit") slotClose();
    }
  }
}

/*! \brief Создание меню трея *************************************************/
void RSSListing::createTrayMenu()
{
  trayMenu_ = new QMenu(this);
  QAction *showWindowAct_ = new QAction(tr("Show window"), this);
  connect(showWindowAct_, SIGNAL(triggered()), this, SLOT(slotShowWindows()));
  QFont font_ = showWindowAct_->font();
  font_.setBold(true);
  showWindowAct_->setFont(font_);
  trayMenu_->addAction(showWindowAct_);
  trayMenu_->addSeparator();
  trayMenu_->addAction(exitAct_);
  traySystem->setContextMenu(trayMenu_);
}

/*! \brief Освобождение памяти ************************************************/
void RSSListing::myEmptyWorkingSet()
{
  EmptyWorkingSet(GetCurrentProcess());
}
