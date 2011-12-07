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

    persistentUpdateThread_ = new UpdateThread(this);
    persistentUpdateThread_->setObjectName("persistentUpdateThread_");
    connect(persistentUpdateThread_, SIGNAL(readedXml(QByteArray, QUrl)),
        this, SLOT(parseXml(QByteArray, QUrl)));
    connect(persistentUpdateThread_, SIGNAL(getUrlDone(int)),
        this, SLOT(getUrlDone(int)));


    feedsModel_ = new QSqlTableModel();
    feedsModel_->setTable("feeds");
    feedsModel_->select();

    feedsView_ = new QTreeView();
    feedsView_->setObjectName("feedsTreeView_");
    feedsView_->setModel(feedsModel_);
    feedsView_->header()->setResizeMode(QHeaderView::ResizeToContents);
    feedsView_->setUniformRowHeights(true);
    feedsView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    feedsView_->hideColumn(feedsModel_->fieldIndex("title"));
    feedsView_->hideColumn(feedsModel_->fieldIndex("description"));
    feedsView_->hideColumn(feedsModel_->fieldIndex("xmlurl"));
    feedsView_->hideColumn(feedsModel_->fieldIndex("htmlurl"));
    connect(feedsView_, SIGNAL(clicked(QModelIndex)),
            this, SLOT(slotFeedsTreeClicked(QModelIndex)));
    connect(feedsView_, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(updateFeed(QModelIndex)));
    connect(this, SIGNAL(signalFeedsTreeKeyUpDownPressed()),
            SLOT(slotFeedsTreeKeyUpDownPressed()), Qt::QueuedConnection);


    newsModel_ = new QSqlTableModel();
    newsView_ = new QTableView();
    newsView_->setObjectName("newsView_");
    newsView_->setModel(newsModel_);
    newsView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    newsView_->horizontalHeader()->setStretchLastSection(true);
    newsView_->horizontalHeader()->setHighlightSections(false);
    newsView_->verticalHeader()->setDefaultSectionSize(
        newsView_->verticalHeader()->minimumSectionSize());
    newsView_->verticalHeader()->setVisible(false);
    newsView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    newsView_->setShowGrid(false);
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

    //! Create feeds DocWidget
    feedsDoc_ = new QDockWidget(tr("Feeds"), this);
    feedsDoc_->setObjectName("feedsDoc");
    feedsDoc_->setWidget(feedsView_);
    feedsDoc_->setFeatures(QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::LeftDockWidgetArea, feedsDoc_);

    //! Create news layout
    QVBoxLayout *webLayout = new QVBoxLayout();
    webLayout->setMargin(1);  // Чтобы было видно границу виджета
    webLayout->addWidget(webView_);

    QWidget *webWidget = new QWidget();
    webWidget->setStyleSheet("border: 1px solid gray");
    webWidget->setLayout(webLayout);

    newsTabSplitter_ = new QSplitter(Qt::Vertical);
    newsTabSplitter_->addWidget(newsView_);
    newsTabSplitter_->addWidget(webWidget);

    newsTabWidget_ = new QTabWidget();
    newsTabWidget_->addTab(newsTabSplitter_, "");

    QSplitter *contentSplitter = new QSplitter(Qt::Vertical);
    contentSplitter->addWidget(treeWidget_);
    contentSplitter->addWidget(newsTabWidget_);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->addWidget(contentSplitter);

    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(layout);

    setCentralWidget(centralWidget);

    setWindowTitle(QString("QtRSS v") + QString(STRFILEVER).section('.', 0, 2));

    createActions();
    createMenu();
    createToolBar();

    feedsView_->installEventFilter(this);
    newsView_->installEventFilter(this);

    //! GIU tuning
    toggleQueryResults(false);


    progressBar_ = new QProgressBar();
    progressBar_->setFixedWidth(150);
    progressBar_->setFixedHeight(15);
    progressBar_->setMinimum(0);
    progressBar_->setMaximum(100);
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

    slotFeedsTreeClicked(feedsModel_->index(0, 0));  // загрузка новостей

    readSettings();

    //Установка шрифтов и их настроек для элементов
//    QFont font_ = newsTabWidget_->font();
//    font_.setBold(true);
//    newsTabWidget_->setFont(font_);
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
  exitAct_->setShortcut(QKeySequence::Quit);  // empty on windows :(
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

  menuBar()->addMenu(tr("&News"));

  toolsMenu_ = menuBar()->addMenu(tr("&Tools"));
  toolsMenu_->addAction(optionsAct_);

  menuBar()->addMenu(tr("&Help"));
}

/*! \brief Создание ToolBar ***************************************************/
void RSSListing::createToolBar()
{
  toolBar_ = addToolBar(tr("ToolBar"));
  toolBar_->setObjectName("ToolBar_General");
  toolBar_->addAction(addFeedAct_);
  toolBar_->addAction(deleteFeedAct_);
  toolBar_->addSeparator();
  toolBar_->addAction(updateFeedAct_);
  toolBar_->addAction(updateAllFeedsAct_);
  toolBar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  connect(toolBar_, SIGNAL(visibilityChanged(bool)), toolBarToggle_, SLOT(setChecked(bool)));
  connect(toolBarToggle_, SIGNAL(toggled(bool)), toolBar_, SLOT(setShown(bool)));
}

/*! \brief Чтение настроек из ini-файла ***************************************/
void RSSListing::readSettings()
{
  settings_->beginGroup("/Settings");

  QString fontFamily = settings_->value("/FontFamily", "Tahoma").toString();
  int fontSize = settings_->value("/FontSize", 8).toInt();
  qApp->setFont(QFont(fontFamily, fontSize));

  bool proxyOn = settings_->value("/Proxy", false).toBool();
  settings_->endGroup();

  setProxyAct_->setChecked(proxyOn);
  slotSetProxy();

  restoreGeometry(settings_->value("GeometryState").toByteArray());
  restoreState(settings_->value("ToolBarsState").toByteArray());

  // Загрузка ширины столбцов таблицы
  for (int i=0; i < newsModel_->columnCount(); ++i)
    newsView_->setColumnWidth(i, settings_->value(
         QString("newsView/columnWidth%1").arg(i), 100).toInt());

  QList<int> sizes;
  for (int i = 0 ; i < newsTabSplitter_->count() ; ++i) {
    sizes << settings_->value(QString("newsSplitter/size%1").arg(i)).toInt();
  }
  newsTabSplitter_->setSizes(sizes);
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

  // Сохранение ширины столбцов таблицы
  for (int i=0; i < newsModel_->columnCount(); ++i)
    settings_->setValue(QString("newsView/columnWidth%1").arg(i),
        newsView_->columnWidth(i));

  for (int i = 0 ; i < newsTabSplitter_->count() ; ++i) {
    settings_->setValue(QString("newsSplitter/size%1").arg(i), newsTabSplitter_->sizes().at(i));
  }
}

/*! \brief Добавление ленты в список лент *************************************/
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

/*! \brief Удаление ленты из списка лент с подтверждением *********************/
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

/*! \brief Разбор xml-файла ***************************************************/
void RSSListing::parseXml(const QByteArray &data, const QUrl &url)
{
  QXmlStreamReader xml;
  xml.addData(data);

  // поиск идентификатора ленты с таблице лент
  int parseFeedId = 0;
  QSqlQuery q(db_);
  q.exec(QString("select id from feeds where xmlurl like '%1'").
      arg(url.toString()));
  while (q.next()) {
    parseFeedId = q.value(q.record().indexOf("id")).toInt();
  }

  // идентификатор не найден (например, во время запроса удалили ленту)
  if (0 == parseFeedId) {
    qDebug() << QString("Feed '%1' not found").arg(url.toString());
    return;
  } else {
    qDebug() << QString("Feed '%1' found with id = %2").arg(url.toString()).
        arg(parseFeedId);
  }

  // собственно сам разбор
  db_.transaction();
  int itemCount = 0;
  while (!xml.atEnd()) {
    qApp->processEvents();
    xml.readNext();
    if (xml.isStartElement()) {
      if (xml.name() == "item")
        linkString = xml.attributes().value("rss:about").toString();
      currentTag = xml.name().toString();
//      qDebug() << itemCount << ": " << xml.namespaceUri().toString() << ": " << currentTag;
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
          qDebug() << "q.exec(" << q.lastQuery() << ")";
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
    QString str = QString("XML ERROR: Line=%1, ErrorString=%2").
        arg(xml.lineNumber()).arg(xml.errorString());
    statusBar()->showMessage(str, 3000);
  } else {
    statusBar()->showMessage(QString("Update done"), 3000);
  }
  db_.commit();
  slotFeedsTreeClicked(feedsModel_->index(feedsView_->currentIndex().row(), 0));
}

/*! \brief Обработка окончания запроса ****************************************/
void RSSListing::getUrlDone(const int &result)
{
  qDebug() << "getUrl result =" << result;
  progressBar_->hide();
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

  statusUnread_->setText(tr(" Unread: ") + QString::number(newsModel_->rowCount()) + " ");
  statusAll_->setText(tr(" All: ") + QString::number(newsModel_->rowCount()) + " ");
}

/*! \brief Запрос обновления ленты ********************************************/
void RSSListing::updateFeed(QModelIndex index)
{
//  addFeedAct_->setEnabled(false);
//  deleteFeedAct_->setEnabled(false);
//  feedsView_->setEnabled(false);
  treeWidget_->clear();

//  xml.clear();
//  currentUrl_.setUrl(feedsModel_->record(index.row()).field("xmlurl").value().toString());

  persistentUpdateThread_->getUrl(
      feedsModel_->record(index.row()).field("xmlurl").value().toString());

  statusBar()->showMessage(QString(tr("Update '%1 - %2' ...")).
      arg(feedsModel_->record(index.row()).field("id").value().toString()).
      arg(feedsModel_->record(index.row()).field("text").value().toString()),
      3000);
  progressBar_->setValue(progressBar_->minimum());
  progressBar_->show();
  QTimer::singleShot(400, this, SLOT(slotProgressBarUpdate()));
}

/*! \brief Обработка нажатия в дереве новостей ********************************/
void RSSListing::slotFeedViewClicked(QModelIndex index)
{
  webView_->setHtml(
      newsModel_->record(index.row()).field("description").value().toString());
}

/*! \brief Обработка клавиш Up/Down в дереве лент *****************************/
void RSSListing::slotFeedsTreeKeyUpDownPressed()
{
  slotFeedsTreeClicked(feedsView_->currentIndex());
}

/*! \brief Обработка клавиш Up/Down в дереве новостей *************************/
void RSSListing::slotFeedKeyUpDownPressed()
{
  slotFeedViewClicked(newsView_->currentIndex());
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
  trayMenu_->addAction(updateAllFeedsAct_);
  trayMenu_->addSeparator();

  setProxyAct_ = new QAction(this);
  setProxyAct_->setText(tr("Proxy enabled"));
  setProxyAct_->setCheckable(true);
  setProxyAct_->setChecked(false);
  connect(setProxyAct_, SIGNAL(triggered(bool)), this, SLOT(slotSetProxy()));
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
    persistentUpdateThread_->setProxyType(QNetworkProxy::HttpProxy);
  } else {
    persistentUpdateThread_->setProxyType(QNetworkProxy::NoProxy);
  }
  settings_->beginGroup("/Settings");
  settings_->setValue("/Proxy", on);
  settings_->endGroup();
}

/*! \brief Обновление ленты (действие) ****************************************/
void RSSListing::slotUpdateFeed()
{
  QModelIndex index = feedsView_->currentIndex();
  updateFeed(index);
}

/*! \brief Обновление ленты (действие) ****************************************/
void RSSListing::slotUpdateAllFeeds()
{
  statusBar()->showMessage("Feature 'Update all' is under construction", 3000);
//  for (int i = 0; i < feedsModel_->rowCount(); ++i) {
//    QModelIndex index = feedsModel_->index(i, 0);
//    updateFeed(index);
//  }
  updateThread_ = new UpdateThread(this);
  updateThread_->setObjectName("updateThread_");
  connect(updateThread_, SIGNAL(finished()), updateThread_, SLOT(deleteLater()));
  updateThread_->start(QThread::LowPriority);
}

void RSSListing::slotProgressBarUpdate()
{
  if (progressBar_->value() + 10 < progressBar_->maximum())
    progressBar_->setValue(progressBar_->value() + 10);
  else
    progressBar_->setValue(progressBar_->minimum());
  progressBar_->update();

  if (progressBar_->isVisible())
    QTimer::singleShot(400, this, SLOT(slotProgressBarUpdate()));
}
