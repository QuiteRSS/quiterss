#include <QtDebug>
#include <QtCore>
#include <windows.h>
#include <Psapi.h>

#include "addfeeddialog.h"
#include "optionsdialog.h"
#include "rsslisting.h"
#include "VersionNo.h"

/*!****************************************************************************/
const QString kCreateNewsTableQuery(
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
        "new integer default 1, "
        "read integer default 0, "
        "sticky integer default 0, "
        "deleted integer default 0, "
        "attachment varchar, "
        "feed varchar, "
        "location varchar, "
        "link varchar"
    ")");

/*!****************************************************************************/
RSSListing::RSSListing(QWidget *parent)
    : QMainWindow(parent)
{
    QString AppFileName = qApp->applicationFilePath();
    AppFileName.replace(".exe", ".ini");
    settings_ = new QSettings(AppFileName, QSettings::IniFormat);

    QString dbFileName(qApp->applicationFilePath());
    dbFileName.replace(".exe", ".db");
    db_ = QSqlDatabase::addDatabase("QSQLITE");
    db_.setDatabaseName(dbFileName);
    if (QFile(dbFileName).exists()) {
      db_.open();
    } else {  // Инициализация базы
      db_.open();
      db_.exec("create table feeds(id integer primary key, text varchar,"
          " title varchar, description varchar, xmlurl varchar, "
          "htmlurl varchar, unread integer)");
      db_.exec("insert into feeds(text, xmlurl) "
          "values ('Qt Labs', 'http://labs.qt.nokia.com/blogs/feed')");
      db_.exec("insert into feeds(text, xmlurl) "
          "values ('Qt Russian Forum', 'http://www.prog.org.ru/index.php?type=rss;action=.xml')");
      db_.exec("create table info(id integer primary key, name varchar, value varchar)");
      db_.exec("insert into info(name, value) values ('version', '1.0')");
      db_.exec(kCreateNewsTableQuery.arg(1));
      db_.exec(kCreateNewsTableQuery.arg(2));
    }

    persistentUpdateThread_ = new UpdateThread(this);
    persistentUpdateThread_->setObjectName("persistentUpdateThread_");
    connect(persistentUpdateThread_, SIGNAL(readedXml(QByteArray, QUrl)),
        this, SLOT(receiveXml(QByteArray, QUrl)));
    connect(persistentUpdateThread_, SIGNAL(getUrlDone(int)),
        this, SLOT(getUrlDone(int)));


    feedsModel_ = new FeedsModel(this);
    feedsModel_->setTable("feeds");
    feedsModel_->select();

    feedsView_ = new QTreeView();
    feedsView_->setObjectName("feedsTreeView_");
    feedsView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    feedsView_->setModel(feedsModel_);
    feedsView_->header()->setStretchLastSection(false);
    feedsView_->header()->setResizeMode(feedsModel_->fieldIndex("id"), QHeaderView::ResizeToContents);
    feedsView_->header()->setResizeMode(feedsModel_->fieldIndex("text"), QHeaderView::Stretch);
    feedsView_->header()->setResizeMode(feedsModel_->fieldIndex("unread"), QHeaderView::ResizeToContents);

    feedsView_->header()->setVisible(false);
    feedsView_->setUniformRowHeights(true);
    feedsView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    feedsView_->hideColumn(feedsModel_->fieldIndex("title"));
    feedsView_->hideColumn(feedsModel_->fieldIndex("description"));
    feedsView_->hideColumn(feedsModel_->fieldIndex("xmlurl"));
    feedsView_->hideColumn(feedsModel_->fieldIndex("htmlurl"));
    connect(feedsView_, SIGNAL(clicked(QModelIndex)),
            this, SLOT(slotFeedsTreeClicked(QModelIndex)));
    connect(this, SIGNAL(signalFeedsTreeKeyUpDownPressed()),
            SLOT(slotFeedsTreeKeyUpDownPressed()), Qt::QueuedConnection);

    newsModel_ = new NewsModel(this);
    newsModel_->setEditStrategy(QSqlTableModel::OnFieldChange);
    newsView_ = new QTreeView();
    newsView_->setObjectName("newsView_");
    newsView_->setModel(newsModel_);
    newsView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    newsView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    newsView_->setMinimumWidth(120);
    newsView_->setSortingEnabled(true);

    newsHeader_ = new NewsHeader(Qt::Horizontal, newsView_);
    newsView_->setHeader(newsHeader_);

    connect(newsView_, SIGNAL(clicked(QModelIndex)),
            this, SLOT(slotNewsViewClicked(QModelIndex)));
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

    setContextMenuPolicy(Qt::CustomContextMenu);

    //! Create feeds DockWidget
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
    setDockOptions(QMainWindow::AnimatedDocks|QMainWindow::AllowNestedDocks);

    feedsDock_ = new QDockWidget(tr("Feeds"), this);
    feedsDock_->setObjectName("feedsDock");
    feedsDock_->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea|Qt::TopDockWidgetArea);
    feedsDock_->setWidget(feedsView_);
    feedsDock_->setFeatures(QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::LeftDockWidgetArea, feedsDock_);

    toolBarNull_ = new QToolBar(this);
    toolBarNull_->setObjectName("toolBarNull");
    toolBarNull_->setMovable(false);
    toolBarNull_->setFixedWidth(6);
    addToolBar(Qt::LeftToolBarArea, toolBarNull_);

    pushButtonNull_ = new QPushButton(this);
    pushButtonNull_->setObjectName("pushButtonNull");
    pushButtonNull_->setIcon(QIcon(":/images/images/triangleL.png"));
    pushButtonNull_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolBarNull_->addWidget(pushButtonNull_);
    connect(pushButtonNull_, SIGNAL(clicked()), this, SLOT(slotVisibledFeedsDock()));

    //! Create news DockWidget
    QSplitter *newsSplitter = new QSplitter(Qt::Vertical);
    newsSplitter->addWidget(treeWidget_);
    newsSplitter->addWidget(newsView_);

    newsDock_ = new QDockWidget(tr("News"), this);
    newsDock_->setObjectName("newsDock");
    newsDock_->setFeatures(QDockWidget::DockWidgetMovable);
    newsDock_->setWidget(newsSplitter);
    addDockWidget(Qt::TopDockWidgetArea, newsDock_);

    //! Create web layout
    QVBoxLayout *webLayout = new QVBoxLayout();
    webLayout->setMargin(1);  // Чтобы было видно границу виджета
    webLayout->addWidget(webView_);

    QWidget *webWidget = new QWidget();
    webWidget->setStyleSheet("border: 1px solid gray");
    webWidget->setLayout(webLayout);

    setCentralWidget(webWidget);

    setWindowTitle(QString("QtRSS v") + QString(STRFILEVER).section('.', 0, 2));

    createActions();
    createMenu();
    createToolBar();

    feedsView_->installEventFilter(this);
    newsView_->installEventFilter(this);
    toolBarNull_->installEventFilter(this);

    //! GIU tuning
    toggleQueryResults(false);


    progressBar_ = new QProgressBar();
    progressBar_->setObjectName("progressBar_");
    progressBar_->setFixedWidth(200);
    progressBar_->setFixedHeight(15);
    progressBar_->setMinimum(0);
    progressBar_->setFormat(tr("Update feeds... (%p%)"));
    progressBar_->setVisible(false);
    statusBar()->addPermanentWidget(progressBar_);
    statusUnread_ = new QLabel(tr(" Unread: "));
    statusBar()->addPermanentWidget(statusUnread_);
    statusAll_ = new QLabel(tr(" All: "));
    statusBar()->addPermanentWidget(statusAll_);
    statusBar()->setVisible(true);

    //! testing
    webView_->load(QUrl("qrc:/html/test1.html"));
    webView_->show();

    traySystem = new QSystemTrayIcon(QIcon(":/images/images/QtRSS16.png"),this);
    connect(traySystem,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(slotActivationTray(QSystemTrayIcon::ActivationReason)));
    connect(this,SIGNAL(signalPlaceToTray()),this,SLOT(slotPlaceToTray()),Qt::QueuedConnection);
    traySystem->setToolTip("QtRSS");
    createTrayMenu();
    traySystem->show();

    connect(this, SIGNAL(signalCloseApp()),
            SLOT(slotCloseApp()), Qt::QueuedConnection);
    connect(feedsView_, SIGNAL(doubleClicked(QModelIndex)),
            updateFeedAct_, SLOT(trigger()));

    feedsView_->setCurrentIndex(feedsModel_->index(0, 0));
    slotFeedsTreeClicked(feedsModel_->index(0, 0));  // загрузка новостей

    readSettings();

    newsHeader_->init();

    //Установка шрифтов и их настроек для элементов
    QFont font_ = newsDock_->font();
    font_.setBold(true);
    newsDock_->setFont(font_);
}

/*!****************************************************************************/
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

/*virtual*/ void RSSListing::showEvent(QShowEvent* event)
{
  connect(feedsDock_, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
          this, SLOT(slotDockLocationChanged(Qt::DockWidgetArea)), Qt::UniqueConnection);
}

/*!****************************************************************************/
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
  } else if (obj == toolBarNull_) {
    if (event->type() == QEvent::MouseButtonRelease) {
      slotVisibledFeedsDock();
    }
    return false;
  } else {
    // pass the event on to the parent class
    return QMainWindow::eventFilter(obj, event);
  }
}

/*! \brief ОБработка событий закрытия окна ************************************/
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

/*! \brief Создание действий **************************************************
 * \details Которые будут использоваться в главном меню и ToolBar
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
  exitAct_->setShortcut(Qt::CTRL+Qt::Key_Q);  // standart on other OS
  connect(exitAct_, SIGNAL(triggered()), this, SLOT(slotClose()));

  toolBarToggle_ = new QAction(tr("&ToolBar"), this);
  toolBarToggle_->setCheckable(true);
  toolBarToggle_->setStatusTip(tr("Show ToolBar"));

  treeWidgetToggle_ = new QAction(tr("&Query results"), this);
  treeWidgetToggle_->setCheckable(true);
  treeWidgetToggle_->setStatusTip(tr("Show table with query results"));
  connect(treeWidgetToggle_, SIGNAL(toggled(bool)), this, SLOT(toggleQueryResults(bool)));

  updateFeedAct_ = new QAction(QIcon(":/images/updateFeed"), tr("Update"), this);
  updateFeedAct_->setStatusTip(tr("Update current feed"));
  updateFeedAct_->setShortcut(Qt::Key_F5);
  connect(updateFeedAct_, SIGNAL(triggered()), this, SLOT(slotUpdateFeed()));

  updateAllFeedsAct_ = new QAction(QIcon(":/images/updateAllFeeds"), tr("Update all"), this);
  updateAllFeedsAct_->setStatusTip(tr("Update all feeds"));
  updateAllFeedsAct_->setShortcut(Qt::CTRL + Qt::Key_F5);
  connect(updateAllFeedsAct_, SIGNAL(triggered()), this, SLOT(slotUpdateAllFeeds()));

  markNewsRead_ = new QAction(QIcon(":/images/markRead"), tr("Mark Read"), this);
  markNewsRead_->setStatusTip(tr("Mark current news read"));
  connect(markNewsRead_, SIGNAL(triggered()), this, SLOT(markNewsRead()));

  markAllNewsRead_ = new QAction(QIcon(":/images/markReadAll"), tr("Mark all news Read"), this);
  markAllNewsRead_->setStatusTip(tr("Mark all news read"));
  connect(markAllNewsRead_, SIGNAL(triggered()), this, SLOT(markAllNewsRead()));

  markNewsUnread_ = new QAction(QIcon(":/images/newsUnread"), tr("Mark Unread"), this);
  markNewsUnread_->setStatusTip(tr("Mark current news unread"));
  connect(markNewsUnread_, SIGNAL(triggered()), this, SLOT(markNewsUnread()));

  optionsAct_ = new QAction(tr("Options..."), this);
  optionsAct_->setStatusTip(tr("Open options gialog"));
  optionsAct_->setShortcut(Qt::Key_F8);
  connect(optionsAct_, SIGNAL(triggered()), this, SLOT(showOptionDlg()));
}

/*! \brief Создание главного меню *********************************************/
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
  feedMenu_->addAction(updateFeedAct_);
  feedMenu_->addAction(updateAllFeedsAct_);

  newsMenu_ = menuBar()->addMenu(tr("&News"));
  newsMenu_->addAction(markNewsRead_);
  newsMenu_->addAction(markAllNewsRead_);
  newsMenu_->addSeparator();
  newsMenu_->addAction(markNewsUnread_);

  toolsMenu_ = menuBar()->addMenu(tr("&Tools"));
  toolsMenu_->addAction(optionsAct_);

  menuBar()->addMenu(tr("&Help"));
}

/*! \brief Создание ToolBar ***************************************************/
void RSSListing::createToolBar()
{
  toolBar_ = addToolBar(tr("ToolBar"));
  toolBar_->setObjectName("ToolBar_General");
  toolBar_->setAllowedAreas(Qt::TopToolBarArea);
  toolBar_->setMovable(false);
  toolBar_->addAction(addFeedAct_);
  toolBar_->addAction(deleteFeedAct_);
  toolBar_->addSeparator();
  toolBar_->addAction(updateFeedAct_);
  toolBar_->addAction(updateAllFeedsAct_);
  toolBar_->addSeparator();
  toolBar_->addAction(markNewsRead_);
  toolBar_->addAction(markAllNewsRead_);
  toolBar_->addAction(markNewsUnread_);
  toolBar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  connect(toolBar_, SIGNAL(visibilityChanged(bool)), toolBarToggle_, SLOT(setChecked(bool)));
  connect(toolBarToggle_, SIGNAL(toggled(bool)), toolBar_, SLOT(setVisible(bool)));
}

/*! \brief Чтение настроек из ini-файла ***************************************/
void RSSListing::readSettings()
{
  settings_->beginGroup("/Settings");

  QString fontFamily = settings_->value("/FontFamily", "Tahoma").toString();
  int fontSize = settings_->value("/FontSize", 8).toInt();
  qApp->setFont(QFont(fontFamily, fontSize));

  settings_->endGroup();

  restoreGeometry(settings_->value("GeometryState").toByteArray());
  restoreState(settings_->value("ToolBarsState").toByteArray());
  newsHeader_->restoreGeometry(settings_->value("NewsViewGeometry").toByteArray());
  newsHeader_->restoreState(settings_->value("NewsViewState").toByteArray());

  setProxyAct_->setChecked(settings_->value("networkProxy/Enabled", false).toBool());
  networkProxy_.setType(static_cast<QNetworkProxy::ProxyType>
      (settings_->value("networkProxy/type", QNetworkProxy::HttpProxy).toInt()));
  networkProxy_.setHostName(settings_->value("networkProxy/hostName", "10.0.0.172").toString());
  networkProxy_.setPort(    settings_->value("networkProxy/port",     3150).toUInt());
  networkProxy_.setUser(    settings_->value("networkProxy/user",     "").toString());
  networkProxy_.setPassword(settings_->value("networkProxy/password", "").toString());
  slotSetProxy();
}

/*! \brief Запись настроек в ini-файл *****************************************/
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
  settings_->setValue("NewsViewGeometry", newsHeader_->saveGeometry());
  settings_->setValue("NewsViewState", newsHeader_->saveState());

  settings_->setValue("networkProxy/Enabled",  setProxyAct_->isChecked());
  settings_->setValue("networkProxy/type",     networkProxy_.type());
  settings_->setValue("networkProxy/hostName", networkProxy_.hostName());
  settings_->setValue("networkProxy/port",     networkProxy_.port());
  settings_->setValue("networkProxy/user",     networkProxy_.user());
  settings_->setValue("networkProxy/password", networkProxy_.password());
}

/*! \brief Добавление ленты в список лент *************************************/
void RSSListing::addFeed()
{
  AddFeedDialog *addFeedDialog = new AddFeedDialog(this);
  addFeedDialog->setWindowTitle(tr("Add feed"));
  if (addFeedDialog->exec() == QDialog::Rejected) return;

  QSqlQuery q(db_);
  QString qStr = QString("insert into feeds(link) values (%1)").
      arg(addFeedDialog->feedEdit_->text());
  q.exec(qStr);
  q.exec(kCreateNewsTableQuery.arg(q.lastInsertId().toString()));
  q.finish();
  feedsModel_->select();
}

/*! \brief Удаление ленты из списка лент с подтверждением *********************/
void RSSListing::deleteFeed()
{
  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle(tr("Delete feed"));
  msgBox.setText(QString(tr("Are you sure to delete the feed '%1'?")).
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

/*! \brief Импорт лент из OPML-файла ******************************************/
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
        q.exec(kCreateNewsTableQuery.arg(q.lastInsertId().toString()));
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

/*! \brief приём xml-файла ****************************************************/
void RSSListing::receiveXml(const QByteArray &data, const QUrl &url)
{
  url_ = url;
  xml_.addData(data);
}

/*! \brief Разбор xml-файла ***************************************************/
void RSSListing::parseXml()
{
  qDebug() << "=================== parseXml:start ============================";
  // поиск идентификатора ленты с таблице лент
  int parseFeedId = 0;
  QSqlQuery q(db_);
  q.exec(QString("select id from feeds where xmlurl like '%1'").
      arg(url_.toString()));
  while (q.next()) {
    parseFeedId = q.value(q.record().indexOf("id")).toInt();
  }

  // идентификатор не найден (например, во время запроса удалили ленту)
  if (0 == parseFeedId) {
    qDebug() << QString("Feed '%1' not found").arg(url_.toString());
    return;
  } else {
    qDebug() << QString("Feed '%1' found with id = %2").arg(url_.toString()).
        arg(parseFeedId);
  }

  // собственно сам разбор
  db_.transaction();
  int itemCount = 0;
  while (!xml_.atEnd()) {
    qApp->processEvents();
    xml_.readNext();
    if (xml_.isStartElement()) {
      if (xml_.name() == "item")
        linkString = xml_.attributes().value("rss:about").toString();
      currentTag = xml_.name().toString();
//      qDebug() << itemCount << ": " << currentTag;
    } else if (xml_.isEndElement()) {
      if (xml_.name() == "item") {

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
        QString qStr;
        qDebug() << "guid:" << guidString;
        if (!guidString.isEmpty())
          qStr = QString("select * from feed_%1 where guid == '%2'").
              arg(parseFeedId).arg(guidString);
        else
//          qStr = QString("select * from feed_%1 "
//              "where title == '%2' and description == '%3' and published == '%4'").
//              arg(parseFeedId).arg(titleString).arg(descriptionString).arg(pubDateString);
          qStr = QString("select * from feed_%1 "
              "where title == '%2' and published == '%3'").
              arg(parseFeedId).arg(titleString).arg(pubDateString);
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
          qDebug() << "q.exec(" << q.lastQuery() << ")";
          qDebug() << "       " << descriptionString;
          qDebug() << "       " << guidString;
          qDebug() << "       " << titleString;
          qDebug() << "       " << pubDateString;
          qDebug() << "       " << QDateTime::currentDateTime().toString();
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
    } else if (xml_.isCharacters() && !xml_.isWhitespace()) {
      if (currentTag == "item")
        itemString += xml_.text().toString();
      else if (currentTag == "title")
        titleString += xml_.text().toString();
      else if (currentTag == "link")
        linkString += xml_.text().toString();
      else if (currentTag == "description")
        descriptionString += xml_.text().toString();
      else if (currentTag == "comments")
        commentsString += xml_.text().toString();
      else if (currentTag == "pubDate")
        pubDateString += xml_.text().toString();
      else if (currentTag == "guid")
        guidString += xml_.text().toString();
    }
  }
  if (xml_.error() && xml_.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
    QString str = QString("XML ERROR: Line=%1, ErrorString=%2").
        arg(xml_.lineNumber()).arg(xml_.errorString());
    statusBar()->showMessage(str, 3000);
    qDebug() << str;
  }
  db_.commit();
  slotFeedsTreeClicked(feedsModel_->index(feedsView_->currentIndex().row(), 0));
  qDebug() << "=================== parseXml:finish ===========================";
}

/*! \brief Обработка окончания запроса ****************************************/
void RSSListing::getUrlDone(const int &result)
{
  qDebug() << "getUrl result =" << result;

  parseXml();
  xml_.clear();
  url_.clear();

  // очередь запросов пуста
  if (0 == result) {
    updateAllFeedsAct_->setEnabled(true);
    progressBar_->hide();
    statusBar()->showMessage(QString("Update done"), 3000);
  }
  // в очереди запросов осталось _result_ запросов
  else if (0 < result) {
    progressBar_->setValue(progressBar_->maximum() - result);
  }
}

/*! \brief Обработка события активации элемента в таблице результатов последнего запроса
 ******************************************************************************/
void RSSListing::itemActivated(QTreeWidgetItem * item)
{
    webView_->setHtml(item->text(3));
}

/*! \brief Обработка нажатия в дереве лент ************************************/
void RSSListing::slotFeedsTreeClicked(QModelIndex index)
{
  newsModel_->setTable(QString("feed_%1").arg(feedsModel_->index(index.row(), 0).data().toString()));
  newsModel_->select();
  newsView_->setColumnHidden(newsModel_->fieldIndex("id"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("guid"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("description"), true);
  // title
  // published
  newsView_->setColumnHidden(newsModel_->fieldIndex("modified"), true);
  // received
  newsView_->setColumnHidden(newsModel_->fieldIndex("author"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("category"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("label"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("new"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("sticky"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("deleted"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("attachment"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("feed"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("location"), true);
  newsView_->setColumnHidden(newsModel_->fieldIndex("link"), true);
//  newsView_->setSortingEnabled(true);
//  newsView_->sortByColumn(newsModel_->fieldIndex("published"));
  // Переименование колонок новостей
  newsModel_->setHeaderData(newsModel_->fieldIndex("title"), Qt::Horizontal, tr("Title"));
  newsModel_->setHeaderData(newsModel_->fieldIndex("published"), Qt::Horizontal, tr("Date"));
  newsModel_->setHeaderData(newsModel_->fieldIndex("received"), Qt::Horizontal, tr("Received"));
  newsModel_->setHeaderData(newsModel_->fieldIndex("read"), Qt::Horizontal, "");
  newsModel_->setHeaderData(newsModel_->fieldIndex("read"), Qt::Horizontal, QIcon(":/images/markRead"), Qt::DecorationRole);

  newsView_->setCurrentIndex(newsModel_->index(0, 0));
  slotNewsViewClicked(newsModel_->index(0, 0));

  newsDock_->setWindowTitle(feedsModel_->index(index.row(), 1).data().toString());

  updateStatus();
}

/*! \brief Запрос обновления ленты ********************************************/
void RSSListing::updateFeed(QModelIndex index)
{
  treeWidget_->clear();

  persistentUpdateThread_->getUrl(
      feedsModel_->record(index.row()).field("xmlurl").value().toString());

  statusBar()->showMessage(QString(tr("Update '%1 - %2' ...")).
      arg(feedsModel_->record(index.row()).field("id").value().toString()).
      arg(feedsModel_->record(index.row()).field("text").value().toString()),
      3000);
  progressBar_->setValue(progressBar_->minimum());
  progressBar_->show();
  QTimer::singleShot(150, this, SLOT(slotProgressBarUpdate()));
}

/*! \brief Обработка нажатия в дереве новостей ********************************/
void RSSListing::slotNewsViewClicked(QModelIndex index)
{
  static QModelIndex oldIndex;
  if (index.column() == newsModel_->fieldIndex("read")) {
    int read;
    if (newsModel_->index(index.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() == 0) {
      read = 1;
    } else {
      read = 0;
    }
    newsModel_->setData(
        newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
        read);
    newsView_->setCurrentIndex(oldIndex);
    updateStatus();
  } else {
    webView_->setHtml(
        newsModel_->record(index.row()).field("description").value().toString());
    markNewsRead();
    oldIndex = index;
  }
}

/*! \brief Обработка клавиш Up/Down в дереве лент *****************************/
void RSSListing::slotFeedsTreeKeyUpDownPressed()
{
  slotFeedsTreeClicked(feedsView_->currentIndex());
}

/*! \brief Обработка клавиш Up/Down в дереве новостей *************************/
void RSSListing::slotFeedKeyUpDownPressed()
{
  slotNewsViewClicked(newsView_->currentIndex());
}

/*! \brief Обработка переключения отображения таблицы результатов последнего запроса
 ******************************************************************************/
void RSSListing::toggleQueryResults(bool checked)
{
  treeWidget_->setVisible(checked);
}

/*! \brief Вызов окна настроек ************************************************/
void RSSListing::showOptionDlg()
{
  OptionsDialog *optionsDialog = new OptionsDialog(this);
  optionsDialog->setWindowTitle(tr("Options"));
  optionsDialog->restoreGeometry(settings_->value("options/geometry").toByteArray());
  optionsDialog->setProxy(networkProxy_);
  int result = optionsDialog->exec();
  settings_->setValue("options/geometry", optionsDialog->saveGeometry());

  if (result == QDialog::Rejected) return;

  networkProxy_ = optionsDialog->proxy();
  setProxyAct_->setChecked(networkProxy_.type() == QNetworkProxy::HttpProxy);
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
  trayMenu_->addAction(updateAllFeedsAct_);
  trayMenu_->addSeparator();

  setProxyAct_ = new QAction(this);
  setProxyAct_->setText(tr("Proxy enabled"));
  setProxyAct_->setCheckable(true);
  setProxyAct_->setChecked(false);
  connect(setProxyAct_, SIGNAL(toggled(bool)), this, SLOT(slotSetProxy()));
  trayMenu_->addAction(setProxyAct_);
  trayMenu_->addSeparator();

  trayMenu_->addAction(exitAct_);
  traySystem->setContextMenu(trayMenu_);
//  QNetworkProxy::NoProxy
}

/*! \brief Освобождение памяти ************************************************/
void RSSListing::myEmptyWorkingSet()
{
  EmptyWorkingSet(GetCurrentProcess());
}

/*! \brief Обработка переключения использования прокси сервера ****************/
void RSSListing::slotSetProxy()
{
  bool on = setProxyAct_->isChecked();
  if (on) {
    networkProxy_.setType(QNetworkProxy::HttpProxy);
  } else {
    networkProxy_.setType(QNetworkProxy::NoProxy);
  }
  persistentUpdateThread_->setProxy(networkProxy_);
}

/*! \brief Обновление ленты (действие) ****************************************/
void RSSListing::slotUpdateFeed()
{
  progressBar_->setMaximum(1);
  QModelIndex index = feedsView_->currentIndex();
  updateFeed(index);
}

/*! \brief Обновление ленты (действие) ****************************************/
void RSSListing::slotUpdateAllFeeds()
{
  progressBar_->setMaximum(feedsModel_->rowCount()-1);
  updateAllFeedsAct_->setEnabled(false);
  for (int i = 0; i < feedsModel_->rowCount(); ++i) {
    QModelIndex index = feedsModel_->index(i, 0);
    updateFeed(index);
  }
}

void RSSListing::slotProgressBarUpdate()
{
  progressBar_->update();

  if (progressBar_->isVisible())
    QTimer::singleShot(150, this, SLOT(slotProgressBarUpdate()));
}

void RSSListing::slotVisibledFeedsDock()
{
  if (feedsDock_->isVisible()){
    feedsDock_->setVisible(false);
  } else feedsDock_->setVisible(true);
}

void RSSListing::slotDockLocationChanged(Qt::DockWidgetArea area)
{
  if (area == Qt::LeftDockWidgetArea) {
    pushButtonNull_->setIcon(QIcon(":/images/images/triangleL.png"));
    toolBarNull_->show();
    addToolBar(Qt::LeftToolBarArea, toolBarNull_);
  } else if (area == Qt::RightDockWidgetArea) {
    toolBarNull_->show();
    pushButtonNull_->setIcon(QIcon(":/images/images/triangleR.png"));
    addToolBar(Qt::RightToolBarArea, toolBarNull_);
  } else {
    toolBarNull_->hide();
  }
}

void RSSListing::setItemRead(QModelIndex index, int read)
{
  if (!index.isValid()) return;

  newsModel_->setData(
      newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
      read);
}

void RSSListing::markNewsRead()
{
  QModelIndex index = newsView_->currentIndex();
  setItemRead(newsView_->currentIndex(), 1);
  newsView_->setCurrentIndex(index);
  updateStatus();
}

void RSSListing::markAllNewsRead()
{
  QString qStr = QString("update %1 set read=1").
      arg(newsModel_->tableName());
  QSqlQuery q(db_);
  q.exec(qStr);
  newsModel_->select();
  updateStatus();
}

void RSSListing::markNewsUnread()
{
  QModelIndex index = newsView_->currentIndex();
  setItemRead(newsView_->currentIndex(), 0);
  newsView_->setCurrentIndex(index);
  updateStatus();
}

void RSSListing::updateStatus()
{
  int allCount = 0;
  QString qStr = QString("select count(id) from %1").
      arg(newsModel_->tableName());
  QSqlQuery q(db_);
  q.exec(qStr);
  if (q.next()) allCount = q.value(0).toInt();

  int unreadCount = 0;
  qStr = QString("select count(read) from %1 where read==0").
      arg(newsModel_->tableName());
  q.exec(qStr);
  if (q.next()) unreadCount = q.value(0).toInt();

  qStr = QString("update feeds set unread='%1' where id=='%2'").
      arg(unreadCount).arg(newsModel_->tableName().remove("feed_"));
  q.exec(qStr);

  QModelIndex index = feedsView_->currentIndex();
  feedsModel_->select();
  feedsView_->setCurrentIndex(index);

  statusUnread_->setText(tr(" Unread: ") + QString::number(unreadCount) + " ");

  statusAll_->setText(tr(" All: ") + QString::number(allCount) + " ");
}
