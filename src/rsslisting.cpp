#include <QtCore>

#if defined(Q_WS_WIN)
#include <windows.h>
#include <Psapi.h>
#endif

#include "aboutdialog.h"
#include "addfeeddialog.h"
#include "delegatewithoutfocus.h"
#include "feedpropertiesdialog.h"
#include "optionsdialog.h"
#include "rsslisting.h"
#include "sqlite/sqlite3.h"
#include "updateappdialog.h"
#include "VersionNo.h"

/*!***************************************************************************/
const QString kDbName = "feeds.db";

const QString kCreateFeedsTableQuery(
    "create table feeds("
        "id integer primary key, "
        "text varchar, "             // Текст ленты (сейчас заменяет имя)
        "title varchar, "            // Имя ленты
        "description varchar, "      // Описание ленты
        "xmlUrl varchar, "           // интернет-адрес самой ленты
        "htmlUrl varchar, "          // интернет-адрес сайта, с которого забираем ленту
        "language varchar, "         // язык, на котором написана лента
        "copyrights varchar, "       // права
        "author_name varchar, "      // автор лента: имя
        "author_email varchar, "     //              е-мейл
        "author_uri varchar, "       //              личная страница
        "webMaster varchar, "        // е-мейл адрес ответственного за технические неполядки ленты
        "pubdate varchar, "          // Дата публикации содержимого ленты
        "lastBuildDate varchar, "    // Последняя дата изменения содержимого ленты
        "category varchar, "         // категории содержимого, освещаемые в ленте
        "contributor varchar, "      // участник (через табы)
        "generator varchar, "        // программа, используемая для генерации содержимого
        "docs varchar, "             // ссылка на документ, описывающий стандарт RSS
        "cloud_domain varchar, "     // Веб-сервис, предоставляющий rssCloud интерфейс
        "cloud_port varchar, "       //   .
        "cloud_path varchar, "       //   .
        "cloud_procedure varchar, "  //   .
        "cloud_protocal varchar, "   //   .
        "ttl integer, "              // Время в минутах, в течение которого канал может быть кеширован
        "skipHours varchar, "        // Подсказка аггрегаторам, когда не нужно обновлять ленту (указываются часы)
        "skipDays varchar, "         // Подсказка аггрегаторам, когда не нужно обновлять ленту (указываются дни недели)
        "image blob, "               // gif, jpeg, png рисунок, который может быть ассоциирован с каналом
        "unread integer, "           // количество непрочитанных новостей
        "newCount integer, "         // количество новых новостей
        "currentNews integer, "      // отображаемая новость
        "label varchar"              // выставляется пользователем
    ")");

const QString kCreateNewsTableQuery(
    "create table feed_%1("
        "id integer primary key, "
        "feed integer, "                       // идентификатор ленты из таблицы feeds
        "guid varchar, "                       // уникальный номер
        "guidislink varchar default 'true', "  // флаг того, что уникальный номер является ссылкой на новость
        "description varchar, "                // краткое содержание
        "content varchar, "                    // полное содержание (atom)
        "title varchar, "                      // заголовок
        "published varchar, "                  // дата публикащии
        "modified varchar, "                   // дата модификации
        "received varchar, "                   // дата приёма новости (выставляется при приёме)
        "author_name varchar, "                // имя автора
        "author_uri varchar, "                 // страничка автора (atom)
        "author_email varchar, "               // почта автора (atom)
        "category varchar, "                   // категория, может содержать несколько категорий (например через знак табуляции)
        "label varchar, "                      // метка (выставляется пользователем)
        "new integer default 1, "              // Флаг "новая". Устанавливается при приёме, снимается при закрытии программы
        "read integer default 0, "             // Флаг "прочитанная". Устанавливается после выбора новости
        "sticky integer default 0, "           // Флаг "отличная". Устанавливается пользователем
        "deleted integer default 0, "          // Флаг "удалённая". Новость помечается удалённой, но физически из базы не удаляется, 
                                               //   чтобы при обновлении новостей она не появлялась вновь. 
                                               //   Физическое удаление новость будет производится при общей чистке базы
        "attachment varchar, "                 // ссылка на прикрепленные файлы (ссылки могут быть разделены табами)
        "comments varchar, "                   // интернел-ссылка на страницу, содержащую комментарии(ответы) к новости
        "enclosure_length, "                   // медиа-объект, ассоциированный с новостью:
        "enclosure_type, "                     //   длина, тип,
        "enclosure_url, "                      //   адрес.
        "source varchar, "                     // источник, если это перепубликация  (atom: <link via>)
        "link_href varchar, "                  // интернет-ссылка на новость (atom: <link self>)
        "link_enclosure varchar, "             // интернет-ссылка на потенциально большой объём информации,
                                               //   который нереально передать в новости (atom)
        "link_related varchar, "               // интернет-ссылка на сопутствующие данный для новости  (atom)
        "link_alternate varchar, "             // интернет-ссылка на альтернативное представление новости
        "contributor varchar, "                // участник (через табы)
        "rights varchar "                      // права
    ")");

/*!****************************************************************************/
RSSListing::RSSListing(QWidget *parent)
    : QMainWindow(parent)
{
    QString AppFileName = qApp->applicationFilePath();
    AppFileName.replace(".exe", ".ini");
    settings_ = new QSettings(AppFileName, QSettings::IniFormat);

    dbFileName_ = qApp->applicationDirPath() + "/" + kDbName;
    if (!QFile(dbFileName_).exists()) {  // Инициализация базы
      QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "dbFileName_");
      db.setDatabaseName(dbFileName_);
      db.open();
      db.transaction();
      db.exec(kCreateFeedsTableQuery);
      db.exec("create table info(id integer primary key, name varchar, value varchar)");
      db.exec("insert into info(name, value) values ('version', '1.0')");
      db.commit();
      db.close();
    }
    QSqlDatabase::removeDatabase("dbFileName_");

    db_ = QSqlDatabase::addDatabase("QSQLITE");
    db_.setDatabaseName(":memory:");
    db_.open();
    sqliteDBMemFile(db_, dbFileName_, false);

    persistentUpdateThread_ = new UpdateThread(this);
    persistentUpdateThread_->setObjectName("persistentUpdateThread_");
    connect(persistentUpdateThread_, SIGNAL(readedXml(QByteArray, QUrl)),
        this, SLOT(receiveXml(QByteArray, QUrl)));
    connect(persistentUpdateThread_, SIGNAL(getUrlDone(int,QDateTime)),
        this, SLOT(getUrlDone(int,QDateTime)));

    persistentParseThread_ = new ParseThread(this, &db_);
    persistentParseThread_->setObjectName("persistentParseThread_");
    connect(this, SIGNAL(xmlReadyParse(QByteArray,QUrl)),
        persistentParseThread_, SLOT(parseXml(QByteArray,QUrl)),
        Qt::QueuedConnection);

    feedsModel_ = new FeedsModel(this);
    feedsModel_->setTable("feeds");
    feedsModel_->select();

    feedsView_ = new QTreeView();
    feedsView_->setObjectName("feedsTreeView_");
    feedsView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    feedsView_->setModel(feedsModel_);
    for (int i = 0; i < feedsModel_->columnCount(); ++i)
      feedsView_->hideColumn(i);
    feedsView_->showColumn(feedsModel_->fieldIndex("text"));
    feedsView_->showColumn(feedsModel_->fieldIndex("unread"));
    feedsView_->header()->setStretchLastSection(false);
    feedsView_->header()->setResizeMode(feedsModel_->fieldIndex("text"), QHeaderView::Stretch);
    feedsView_->header()->setResizeMode(feedsModel_->fieldIndex("unread"), QHeaderView::ResizeToContents);

    feedsView_->header()->setVisible(false);
    feedsView_->setUniformRowHeights(true);
    feedsView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    feedsView_->setContextMenuPolicy(Qt::CustomContextMenu);

    feedsModel_->font_ = feedsView_->font();

    connect(feedsView_, SIGNAL(clicked(QModelIndex)),
            this, SLOT(slotFeedsTreeClicked(QModelIndex)));
    connect(this, SIGNAL(signalFeedsTreeKeyUpDownPressed()),
            SLOT(slotFeedsTreeKeyUpDownPressed()), Qt::QueuedConnection);
    connect(feedsView_, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showContextMenuFeed(const QPoint &)));

    newsModel_ = new NewsModel(this);
    newsModel_->setEditStrategy(QSqlTableModel::OnFieldChange);
    newsView_ = new NewsView(this);
    newsView_->setModel(newsModel_);
    newsView_->model_ = newsModel_;
    newsModel_->view_ = newsView_;

    newsHeader_ = new NewsHeader(Qt::Horizontal, newsView_);
    newsHeader_->model_ = newsModel_;
    newsHeader_->view_ = newsView_;
    newsView_->setHeader(newsHeader_);

    connect(newsView_, SIGNAL(clicked(QModelIndex)),
            this, SLOT(slotNewsViewClicked(QModelIndex)));
    connect(this, SIGNAL(signalFeedKeyUpDownPressed()),
            SLOT(slotNewsKeyUpDownPressed()), Qt::QueuedConnection);
    connect(newsView_, SIGNAL(signalSetItemRead(QModelIndex, int)),
            this, SLOT(slotSetItemRead(QModelIndex, int)));
    connect(newsView_, SIGNAL(signalSetItemStar(QModelIndex,int)),
            this, SLOT(slotSetItemStar(QModelIndex,int)));
    connect(newsView_, SIGNAL(signalDoubleClicked(QModelIndex)),
            this, SLOT(slotNewsViewDoubleClicked(QModelIndex)));
    connect(newsView_, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showContextMenuNews(const QPoint &)));

    webView_ = new QWebView();
    webView_->setObjectName("webView_");
    webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    webViewProgress_ = new QProgressBar(this);
    webViewProgress_->setObjectName("webViewProgress_");
    webViewProgress_->setFixedHeight(15);
    webViewProgress_->setMinimum(0);
    webViewProgress_->setMaximum(100);
    webViewProgress_->setVisible(true);
    connect(webView_, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
    connect(webView_, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished(bool)));
    connect(webView_, SIGNAL(loadProgress(int)), webViewProgress_, SLOT(setValue(int)));
    connect(webView_, SIGNAL(linkClicked(QUrl)), this, SLOT(slotLinkClicked(QUrl)));

    setContextMenuPolicy(Qt::CustomContextMenu);

    feedsTitleLabel_ = new QLabel(this);
    feedsTitleLabel_->setObjectName("feedsTitleLabel_");
    feedsTitleLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
    feedsTitleLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    feedsToolBar_ = new QToolBar(this);
    feedsToolBar_->setObjectName("feedsToolBar_");
    feedsToolBar_->setIconSize(QSize(16, 16));

    QHBoxLayout *feedsPanelLayout = new QHBoxLayout();
    feedsPanelLayout->setMargin(0);
    feedsPanelLayout->setSpacing(0);

    feedsPanelLayout->addWidget(feedsTitleLabel_, 0);
    feedsPanelLayout->addStretch(1);
    feedsPanelLayout->addWidget(feedsToolBar_, 0);
    feedsPanelLayout->addSpacing(5);

    QWidget *feedsPanel = new QWidget(this);
    feedsPanel->setObjectName("feedsPanel");
    feedsPanel->setLayout(feedsPanelLayout);

    QVBoxLayout *feedsWidgetLayout = new QVBoxLayout();
    feedsWidgetLayout->setMargin(1);
    feedsWidgetLayout->setSpacing(0);
    feedsWidgetLayout->addWidget(feedsView_);

    QWidget *feedsWidget = new QWidget(this);
    feedsWidget->setObjectName("feedsWidget");
    feedsWidget->setLayout(feedsWidgetLayout);

    //! Create feeds DockWidget
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
    setDockOptions(QMainWindow::AnimatedDocks|QMainWindow::AllowNestedDocks);

    feedsDock_ = new QDockWidget(this);
    feedsDock_->setObjectName("feedsDock");
    feedsDock_->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea|Qt::TopDockWidgetArea);
    feedsDock_->setFeatures(QDockWidget::DockWidgetMovable);
    feedsDock_->setTitleBarWidget(feedsPanel);
    feedsDock_->setWidget(feedsWidget);
    addDockWidget(Qt::LeftDockWidgetArea, feedsDock_);
    connect(feedsDock_, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
        this, SLOT(slotFeedsDockLocationChanged(Qt::DockWidgetArea)));

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


    newsTitleLabel_ = new QLabel(this);
    newsTitleLabel_->setObjectName("newsTitleLabel_");
    newsTitleLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
    newsTitleLabel_->setMinimumWidth(10);

    newsToolBar_ = new QToolBar(this);
    newsToolBar_->setObjectName("newsToolBar_");
    newsToolBar_->setIconSize(QSize(16, 16));

    QHBoxLayout *newsPanelLayout = new QHBoxLayout();
    newsPanelLayout->setMargin(0);
    newsPanelLayout->setSpacing(0);

    newsPanelLayout->addWidget(newsTitleLabel_);
    newsPanelLayout->addStretch(1);
    newsPanelLayout->addWidget(newsToolBar_);
    newsPanelLayout->addSpacing(5);

    QWidget *newsPanel = new QWidget(this);
    newsPanel->setObjectName("newsPanel");
    newsPanel->setLayout(newsPanelLayout);

    QVBoxLayout *newsWidgetLayout = new QVBoxLayout();
    newsWidgetLayout->setMargin(1);
    newsWidgetLayout->setSpacing(0);
    newsWidgetLayout->addWidget(newsView_);

    QWidget *newsWidget = new QWidget(this);
    newsWidget->setObjectName("newsWidget");
    newsWidget->setLayout(newsWidgetLayout);

    //! Create news DockWidget
    newsDock_ = new QDockWidget(this);
    newsDock_->setObjectName("newsDock");
    newsDock_->setFeatures(QDockWidget::DockWidgetMovable);
    newsDock_->setTitleBarWidget(newsPanel);
    newsDock_->setWidget(newsWidget);
    addDockWidget(Qt::TopDockWidgetArea, newsDock_);
    connect(newsDock_, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
        this, SLOT(slotNewsDockLocationChanged(Qt::DockWidgetArea)));

    webPanelTitleLabel_ = new QLabel(this);
    webPanelAuthorLabel_ = new QLabel(this);

    QVBoxLayout *webPanelLabelLayout = new QVBoxLayout();
    webPanelLabelLayout->addWidget(webPanelTitleLabel_);
    webPanelLabelLayout->addWidget(webPanelAuthorLabel_);

    webPanelAuthor_ = new QLabel("");
    webPanelAuthor_->setObjectName("webPanelAuthor_");
    webPanelAuthor_->setOpenExternalLinks(true);

    webPanelTitle_ = new QLabel("");
    webPanelTitle_->setObjectName("webPanelTitle_");
    webPanelTitle_->setOpenExternalLinks(true);

    QVBoxLayout *webPanelTitleLayout = new QVBoxLayout();
    webPanelTitleLayout->addWidget(webPanelTitle_);
    webPanelTitleLayout->addWidget(webPanelAuthor_);

    QHBoxLayout *webPanelLayout = new QHBoxLayout();
    webPanelLayout->addLayout(webPanelLabelLayout, 0);
    webPanelLayout->addLayout(webPanelTitleLayout, 1);

    webPanel_ = new QWidget();
    webPanel_->setObjectName("webPanel_");
    webPanel_->setLayout(webPanelLayout);

    //! Create web layout
    QVBoxLayout *webLayout = new QVBoxLayout();
    webLayout->setMargin(1);  // Чтобы было видно границу виджета
    webLayout->setSpacing(0);
    webLayout->addWidget(webPanel_, 0);
    webLayout->addWidget(webView_, 1);
    webLayout->addWidget(webViewProgress_, 0);

    webWidget_ = new QWidget();
    webWidget_->setObjectName("webWidget_");
    webWidget_->setLayout(webLayout);
    webWidget_->setMinimumWidth(400);

    setCentralWidget(webWidget_);

    setWindowTitle(QString("QuiteRSS v") + QString(STRFILEVER).section('.', 0, 2));

    createActions();
    createMenu();
    createToolBar();
    createMenuNews();
    createMenuFeed();

    feedsView_->installEventFilter(this);
    newsView_->installEventFilter(this);
    toolBarNull_->installEventFilter(this);

    //! GIU tuning
    progressBar_ = new QProgressBar();
    progressBar_->setObjectName("progressBar_");
    progressBar_->setFixedWidth(100);
    progressBar_->setFixedHeight(14);
    progressBar_->setMinimum(0);
    progressBar_->setTextVisible(false);
    progressBar_->setVisible(false);
    statusBar()->setMinimumHeight(22);
    statusBar()->addPermanentWidget(progressBar_);
    statusUnread_ = new QLabel(this);
    statusBar()->addPermanentWidget(statusUnread_);
    statusAll_ = new QLabel(this);
    statusBar()->addPermanentWidget(statusAll_);
    statusBar()->setVisible(true);

    traySystem = new QSystemTrayIcon(QIcon(":/images/images/QuiteRSS16.png"),this);
    connect(traySystem,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(slotActivationTray(QSystemTrayIcon::ActivationReason)));
    connect(this,SIGNAL(signalPlaceToTray()),this,SLOT(slotPlaceToTray()),Qt::QueuedConnection);
    traySystem->setToolTip("QuiteRSS");
    createTrayMenu();
    traySystem->show();

    connect(this, SIGNAL(signalCloseApp()),
            SLOT(slotCloseApp()), Qt::QueuedConnection);
    connect(feedsView_, SIGNAL(doubleClicked(QModelIndex)),
            updateFeedAct_, SLOT(trigger()));

    loadSettingsFeeds();
    int row = newsView_->currentIndex().row();

    readSettings();

    newsHeader_->createMenu();
    newsView_->setCurrentIndex(newsModel_->index(row, 0));

    //Установка шрифтов и их настроек для элементов
//    QFont font_ = newsTitleLabel_->font();
//    font_.setBold(true);
//    newsTitleLabel_->setFont(font_);

    if (autoUpdatefeedsStartUp_) {
      slotGetAllFeeds();
    }
    updateFeedsTimer_.start(autoUpdatefeedsTime_*60000, this);

    translator_ = new QTranslator(this);
    if (translator_->load("quiterss_" + langFileName_, qApp->applicationDirPath() + QString("/lang"))) {
      qApp->installTranslator(translator_);
    } else retranslateStrings();
}

/*!****************************************************************************/
RSSListing::~RSSListing()
{
  qDebug("App_Closing");

  db_.transaction();
  QSqlQuery q(db_);
  q.exec("select id from feeds");
  while (q.next()) {
    QSqlQuery qt(db_);
    QString qStr = QString("UPDATE feed_%1 SET new=0 WHERE new=1")
        .arg(q.value(0).toString());
    qt.exec(qStr);
  }
  db_.commit();

  QString  qStr = QString("update feeds set newCount=0");
  q.exec(qStr);

  persistentUpdateThread_->quit();
  persistentParseThread_->quit();

  delete newsView_;
  delete feedsView_;
  delete newsModel_;
  delete feedsModel_;

  sqliteDBMemFile(db_, dbFileName_, true);
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
      } else if (keyEvent->key() == Qt::Key_Delete) {
        deleteFeed();
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
      } else if (keyEvent->key() == Qt::Key_Delete) {
        deleteNews();
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
  event->ignore();
  if (closingTray_) {
    oldState = windowState();
    emit signalPlaceToTray();
  } else {
    slotClose();
  }
}

/*! \brief Обработка события выхода из приложения *****************************/
void RSSListing::slotClose()
{
  traySystem->hide();
  hide();
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
    if(isMinimized()) {
      oldState = ((QWindowStateChangeEvent*)event)->oldState();
      if (minimizingTray_) {
        event->ignore();
        emit signalPlaceToTray();
        return;
      }
    } else {
      oldState = windowState();
    }
  } else if(event->type() == QEvent::ActivationChange) {
    if (isActiveWindow())
      traySystem->setIcon(QIcon(":/images/images/QuiteRSS16.png"));
  } else if(event->type() == QEvent::LanguageChange) {
    retranslateStrings();
  }
  QMainWindow::changeEvent(event);
}

/*! \brief Обработка события помещения программы в трей ***********************/
void RSSListing::slotPlaceToTray()
{
  if (emptyWorking_)
    QTimer::singleShot(10000, this, SLOT(myEmptyWorkingSet()));
  hide();
  sqliteDBMemFile(db_, dbFileName_, true);
  writeSettings();
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
    if (!singleClickTray_) slotShowWindows();
    break;
  case QSystemTrayIcon::Trigger:
    if (singleClickTray_) slotShowWindows();
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

void RSSListing::timerEvent(QTimerEvent *event)
{
  if (event->timerId() == updateFeedsTimer_.timerId()) {
    updateFeedsTimer_.stop();
    updateFeedsTimer_.start(autoUpdatefeedsTime_*60000, this);
    slotTimerUpdateFeeds();
  }
}

/*! \brief Создание действий **************************************************
 * \details Которые будут использоваться в главном меню и ToolBar
 ******************************************************************************/
void RSSListing::createActions()
{
  addFeedAct_ = new QAction(this);
  addFeedAct_->setIcon(QIcon(":/images/addFeed"));
  addFeedAct_->setShortcut(QKeySequence::New);
  connect(addFeedAct_, SIGNAL(triggered()), this, SLOT(addFeed()));

  deleteFeedAct_ = new QAction(this);
  deleteFeedAct_->setIcon(QIcon(":/images/deleteFeed"));
  connect(deleteFeedAct_, SIGNAL(triggered()), this, SLOT(deleteFeed()));

  importFeedsAct_ = new QAction(this);
  importFeedsAct_->setIcon(QIcon(":/images/importFeeds"));
  connect(importFeedsAct_, SIGNAL(triggered()), this, SLOT(slotImportFeeds()));

  exitAct_ = new QAction(this);
  exitAct_->setShortcut(Qt::CTRL+Qt::Key_Q);  // standart on other OS
  connect(exitAct_, SIGNAL(triggered()), this, SLOT(slotClose()));

  toolBarToggle_ = new QAction(this);
  toolBarToggle_->setCheckable(true);
  toolBarStyleI_ = new QAction(this);
  toolBarStyleI_->setObjectName("toolBarStyleI_");
  toolBarStyleI_->setCheckable(true);
  toolBarStyleT_ = new QAction(this);
  toolBarStyleT_->setObjectName("toolBarStyleT_");
  toolBarStyleT_->setCheckable(true);
  toolBarStyleTbI_ = new QAction(this);
  toolBarStyleTbI_->setObjectName("toolBarStyleTbI_");
  toolBarStyleTbI_->setCheckable(true);
  toolBarStyleTuI_ = new QAction(this);
  toolBarStyleTuI_->setObjectName("toolBarStyleTuI_");
  toolBarStyleTuI_->setCheckable(true);
  toolBarStyleTuI_->setChecked(true);

  toolBarIconBig_ = new QAction(this);
  toolBarIconBig_->setObjectName("toolBarIconBig_");
  toolBarIconBig_->setCheckable(true);
  toolBarIconNormal_ = new QAction(this);
  toolBarIconNormal_->setObjectName("toolBarIconNormal_");
  toolBarIconNormal_->setCheckable(true);
  toolBarIconNormal_->setChecked(true);
  toolBarIconSmall_ = new QAction(this);
  toolBarIconSmall_->setObjectName("toolBarIconSmall_");
  toolBarIconSmall_->setCheckable(true);

  autoLoadImagesToggle_ = new QAction(this);

  updateFeedAct_ = new QAction(this);
  updateFeedAct_->setIcon(QIcon(":/images/updateFeed"));
  updateFeedAct_->setShortcut(Qt::Key_F5);
  connect(updateFeedAct_, SIGNAL(triggered()), this, SLOT(slotGetFeed()));

  updateAllFeedsAct_ = new QAction(this);
  updateAllFeedsAct_->setIcon(QIcon(":/images/updateAllFeeds"));
  updateAllFeedsAct_->setShortcut(Qt::CTRL + Qt::Key_F5);
  connect(updateAllFeedsAct_, SIGNAL(triggered()), this, SLOT(slotGetAllFeeds()));

  markNewsRead_ = new QAction(this);
  markNewsRead_->setIcon(QIcon(":/images/markRead"));
  connect(markNewsRead_, SIGNAL(triggered()), this, SLOT(markNewsRead()));

  markAllNewsRead_ = new QAction(this);
  markAllNewsRead_->setIcon(QIcon(":/images/markReadAll"));
  connect(markAllNewsRead_, SIGNAL(triggered()), this, SLOT(markAllNewsRead()));

  optionsAct_ = new QAction(this);
  optionsAct_->setIcon(QIcon(":/images/options"));
  optionsAct_->setShortcut(Qt::Key_F8);
  connect(optionsAct_, SIGNAL(triggered()), this, SLOT(showOptionDlg()));

  feedsFilter_ = new QAction(this);
  feedsFilter_->setIcon(QIcon(":/images/filterOff"));
  filterFeedsAll_ = new QAction(this);
  filterFeedsAll_->setObjectName("filterFeedsAll_");
  filterFeedsAll_->setCheckable(true);
  filterFeedsAll_->setChecked(true);
  filterFeedsNew_ = new QAction(this);
  filterFeedsNew_->setObjectName("filterFeedsNew_");
  filterFeedsNew_->setCheckable(true);
  filterFeedsUnread_ = new QAction(this);
  filterFeedsUnread_->setObjectName("filterFeedsUnread_");
  filterFeedsUnread_->setCheckable(true);

  newsFilter_ = new QAction(this);
  newsFilter_->setIcon(QIcon(":/images/filterOff"));
  filterNewsAll_ = new QAction(this);
  filterNewsAll_->setObjectName("filterNewsAll_");
  filterNewsAll_->setCheckable(true);
  filterNewsAll_->setChecked(true);
  filterNewsNew_ = new QAction(this);
  filterNewsNew_->setObjectName("filterNewsNew_");
  filterNewsNew_->setCheckable(true);
  filterNewsUnread_ = new QAction( this);
  filterNewsUnread_->setObjectName("filterNewsUnread_");
  filterNewsUnread_->setCheckable(true);
  filterNewsStar_ = new QAction(this);
  filterNewsStar_->setObjectName("filterNewsStar_");
  filterNewsStar_->setCheckable(true);

  aboutAct_ = new QAction(this);
  aboutAct_->setObjectName("AboutAct_");
  connect(aboutAct_, SIGNAL(triggered()), this, SLOT(slotShowAboutDlg()));

  updateAppAct_ = new QAction(this);
  updateAppAct_->setObjectName("UpdateApp_");
  connect(updateAppAct_, SIGNAL(triggered()), this, SLOT(slotShowUpdateAppDlg()));

  openInBrowserAct_ = new QAction(this);
  connect(openInBrowserAct_, SIGNAL(triggered()), this, SLOT(openInBrowserNews()));
  markStarAct_ = new QAction(this);
  markStarAct_->setIcon(QIcon(":/images/starOn"));
  connect(markStarAct_, SIGNAL(triggered()), this, SLOT(markNewsStar()));
  deleteNewsAct_ = new QAction(this);
  deleteNewsAct_->setIcon(QIcon(":/images/deleteNews"));
  deleteNewsAct_->setShortcut(Qt::Key_Delete);
  connect(deleteNewsAct_, SIGNAL(triggered()), this, SLOT(deleteNews()));

  markFeedRead_ = new QAction(this);
  markFeedRead_->setIcon(QIcon(":/images/markRead"));
  connect(markFeedRead_, SIGNAL(triggered()), this, SLOT(markAllNewsRead()));
  feedProperties_ = new QAction(this);
  feedProperties_->setShortcut(Qt::CTRL+Qt::Key_E);
  connect(feedProperties_, SIGNAL(triggered()), this, SLOT(slotShowFeedPropertiesDlg()));
}

/*! \brief Создание главного меню *********************************************/
void RSSListing::createMenu()
{
  fileMenu_ = new QMenu(this);
  menuBar()->addMenu(fileMenu_);
  fileMenu_->addAction(addFeedAct_);
  fileMenu_->addAction(deleteFeedAct_);
  fileMenu_->addSeparator();
  fileMenu_->addAction(importFeedsAct_);
  fileMenu_->addSeparator();
  fileMenu_->addAction(exitAct_);

  editMenu_ = new QMenu(this);
  connect(editMenu_, SIGNAL(aboutToShow()), this, SLOT(slotEditMenuAction()));
  menuBar()->addMenu(editMenu_);
  editMenu_->addAction(feedProperties_);

  viewMenu_  = new QMenu(this);
  menuBar()->addMenu(viewMenu_ );

  toolBarMenu_ = new QMenu(this);
  toolBarStyleMenu_ = new QMenu(this);
  toolBarStyleMenu_->addAction(toolBarStyleI_);
  toolBarStyleMenu_->addAction(toolBarStyleT_);
  toolBarStyleMenu_->addAction(toolBarStyleTbI_);
  toolBarStyleMenu_->addAction(toolBarStyleTuI_);
  toolBarStyleGroup_ = new QActionGroup(this);
  toolBarStyleGroup_->addAction(toolBarStyleI_);
  toolBarStyleGroup_->addAction(toolBarStyleT_);
  toolBarStyleGroup_->addAction(toolBarStyleTbI_);
  toolBarStyleGroup_->addAction(toolBarStyleTuI_);
  connect(toolBarStyleGroup_, SIGNAL(triggered(QAction*)), this, SLOT(setToolBarStyle(QAction*)));

  toolBarIconSizeMenu_ = new QMenu(this);
  toolBarIconSizeMenu_->addAction(toolBarIconBig_);
  toolBarIconSizeMenu_->addAction(toolBarIconNormal_);
  toolBarIconSizeMenu_->addAction(toolBarIconSmall_);
  toolBarIconSizeGroup_ = new QActionGroup(this);
  toolBarIconSizeGroup_->addAction(toolBarIconBig_);
  toolBarIconSizeGroup_->addAction(toolBarIconNormal_);
  toolBarIconSizeGroup_->addAction(toolBarIconSmall_);
  connect(toolBarIconSizeGroup_, SIGNAL(triggered(QAction*)), this, SLOT(setToolBarIconSize(QAction*)));

  toolBarMenu_->addMenu(toolBarStyleMenu_);
  toolBarMenu_->addMenu(toolBarIconSizeMenu_);
  toolBarMenu_->addAction(toolBarToggle_);
  viewMenu_->addMenu(toolBarMenu_);

  feedMenu_ = new QMenu(this);
  menuBar()->addMenu(feedMenu_);
  feedMenu_->addAction(updateFeedAct_);
  feedMenu_->addAction(updateAllFeedsAct_);
  feedMenu_->addSeparator();

  feedsFilterGroup_ = new QActionGroup(this);
  feedsFilterGroup_->setExclusive(true);
  connect(feedsFilterGroup_, SIGNAL(triggered(QAction*)), this, SLOT(setFeedsFilter(QAction*)));

  feedsFilterMenu_ = new QMenu(this);
  feedsFilterMenu_->addAction(filterFeedsAll_);
  feedsFilterGroup_->addAction(filterFeedsAll_);
  feedsFilterMenu_->addSeparator();
  feedsFilterMenu_->addAction(filterFeedsNew_);
  feedsFilterGroup_->addAction(filterFeedsNew_);
  feedsFilterMenu_->addAction(filterFeedsUnread_);
  feedsFilterGroup_->addAction(filterFeedsUnread_);

  feedsFilter_->setMenu(feedsFilterMenu_);
  feedMenu_->addAction(feedsFilter_);
  feedsToolBar_->addAction(feedsFilter_);
  connect(feedsFilter_, SIGNAL(triggered()), this, SLOT(slotFeedsFilter()));

  newsMenu_ = new QMenu(this);
  menuBar()->addMenu(newsMenu_);
  newsMenu_->addAction(markNewsRead_);
  newsMenu_->addAction(markAllNewsRead_);
  newsMenu_->addSeparator();

  newsFilterGroup_ = new QActionGroup(this);
  newsFilterGroup_->setExclusive(true);
  connect(newsFilterGroup_, SIGNAL(triggered(QAction*)), this, SLOT(setNewsFilter(QAction*)));

  newsFilterMenu_ = new QMenu(this);
  newsFilterMenu_->addAction(filterNewsAll_);
  newsFilterGroup_->addAction(filterNewsAll_);
  newsFilterMenu_->addSeparator();
  newsFilterMenu_->addAction(filterNewsNew_);
  newsFilterGroup_->addAction(filterNewsNew_);
  newsFilterMenu_->addAction(filterNewsUnread_);
  newsFilterGroup_->addAction(filterNewsUnread_);
  newsFilterMenu_->addAction(filterNewsStar_);
  newsFilterGroup_->addAction(filterNewsStar_);

  newsFilter_->setMenu(newsFilterMenu_);
  newsMenu_->addAction(newsFilter_);
  newsToolBar_->addAction(newsFilter_);
  connect(newsFilter_, SIGNAL(triggered()), this, SLOT(slotNewsFilter()));

  newsMenu_->addSeparator();
  newsMenu_->addAction(autoLoadImagesToggle_);

  toolsMenu_ = new QMenu(this);
  menuBar()->addMenu(toolsMenu_);
  toolsMenu_->addAction(optionsAct_);

  helpMenu_ = new QMenu(this);
  menuBar()->addMenu(helpMenu_);
  helpMenu_->addAction(updateAppAct_);
  helpMenu_->addSeparator();
  helpMenu_->addAction(aboutAct_);
}

/*! \brief Создание ToolBar ***************************************************/
void RSSListing::createToolBar()
{
  toolBar_ = new QToolBar(this);
  addToolBar(toolBar_);
  toolBar_->setObjectName("ToolBar_General");
  toolBar_->setAllowedAreas(Qt::TopToolBarArea);
  toolBar_->setMovable(false);
  toolBar_->setContextMenuPolicy(Qt::CustomContextMenu);
  toolBar_->addAction(addFeedAct_);
  toolBar_->addAction(deleteFeedAct_);
  toolBar_->addSeparator();
  toolBar_->addAction(updateFeedAct_);
  toolBar_->addAction(updateAllFeedsAct_);
  toolBar_->addSeparator();
  toolBar_->addAction(markNewsRead_);
  toolBar_->addAction(markAllNewsRead_);
  toolBar_->addSeparator();
  toolBar_->addAction(autoLoadImagesToggle_);
  toolBar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  connect(toolBar_, SIGNAL(visibilityChanged(bool)), toolBarToggle_, SLOT(setChecked(bool)));
  connect(toolBarToggle_, SIGNAL(toggled(bool)), toolBar_, SLOT(setVisible(bool)));
  connect(autoLoadImagesToggle_, SIGNAL(triggered()), this, SLOT(setAutoLoadImages()));
  connect(toolBar_, SIGNAL(customContextMenuRequested(QPoint)),
          this, SLOT(showContextMenuToolBar(const QPoint &)));
}

/*! \brief Чтение настроек из ini-файла ***************************************/
void RSSListing::readSettings()
{
  settings_->beginGroup("/Settings");

  startingTray_ = settings_->value("startingTray", false).toBool();
  minimizingTray_ = settings_->value("minimizingTray", true).toBool();
  closingTray_ = settings_->value("closingTray", true).toBool();
  singleClickTray_ = settings_->value("singleClickTray", false).toBool();
  emptyWorking_ = settings_->value("emptyWorking", true).toBool();

  QString strLang("en");
  QString strLocalLang = QLocale::system().name().left(2);
  QDir langDir = qApp->applicationDirPath() + "/lang";
  foreach (QString file, langDir.entryList(QStringList("*.qm"), QDir::Files)) {
    if (strLocalLang == file.section('.', 0, 0).section('_', 1))
      strLang = strLocalLang;
  }
  langFileName_ = settings_->value("langFileName", strLang).toString();

  QString fontFamily = settings_->value("/feedsFontFamily", qApp->font().family()).toString();
  int fontSize = settings_->value("/feedsFontSize", qApp->font().pointSize()).toInt();
  feedsView_->setFont(QFont(fontFamily, fontSize));

  fontFamily = settings_->value("/newsFontFamily", qApp->font().family()).toString();
  fontSize = settings_->value("/newsFontSize", qApp->font().pointSize()).toInt();
  newsView_->setFont(QFont(fontFamily, fontSize));
qDebug() << qApp->font().pixelSize();
  fontFamily = settings_->value("/WebFontFamily", qApp->font().family()).toString();
  fontSize = settings_->value("/WebFontSize", qApp->font().pointSize()+4).toInt();
  webView_->settings()->setFontFamily(QWebSettings::StandardFont, fontFamily);
  webView_->settings()->setFontSize(QWebSettings::DefaultFontSize, fontSize);

  autoUpdatefeedsStartUp_ = settings_->value("autoUpdatefeedsStartUp", false).toBool();
  autoUpdatefeeds_ = settings_->value("autoUpdatefeeds", false).toBool();
  autoUpdatefeedsTime_ = settings_->value("autoUpdatefeedsTime", 10).toInt();

  QString str = settings_->value("toolBarStyle", "toolBarStyleTuI_").toString();
  QList<QAction*> listActions = toolBarStyleGroup_->actions();
  foreach(QAction *action, listActions) {
    if (action->objectName() == str) {
      action->setChecked(true);
      break;
    }
  }
  setToolBarStyle(toolBarStyleGroup_->checkedAction());
  str = settings_->value("toolBarIconSize", "toolBarIconNormal_").toString();
  listActions = toolBarIconSizeGroup_->actions();
  foreach(QAction *action, listActions) {
    if (action->objectName() == str) {
      action->setChecked(true);
      break;
    }
  }
  setToolBarIconSize(toolBarIconSizeGroup_->checkedAction());

  settings_->endGroup();

  restoreGeometry(settings_->value("GeometryState").toByteArray());
  restoreState(settings_->value("ToolBarsState").toByteArray());
  newsHeader_->restoreGeometry(settings_->value("NewsHeaderGeometry").toByteArray());
  newsHeader_->restoreState(settings_->value("NewsHeaderState").toByteArray());

  networkProxy_.setType(static_cast<QNetworkProxy::ProxyType>(
      settings_->value("networkProxy/type", QNetworkProxy::DefaultProxy).toInt()));
  networkProxy_.setHostName(settings_->value("networkProxy/hostName", "").toString());
  networkProxy_.setPort(    settings_->value("networkProxy/port",     "").toUInt());
  networkProxy_.setUser(    settings_->value("networkProxy/user",     "").toString());
  networkProxy_.setPassword(settings_->value("networkProxy/password", "").toString());
  persistentUpdateThread_->setProxy(networkProxy_);
}

/*! \brief Запись настроек в ini-файл *****************************************/
void RSSListing::writeSettings()
{
  settings_->beginGroup("/Settings");

  settings_->setValue("startingTray", startingTray_);
  settings_->setValue("minimizingTray", minimizingTray_);
  settings_->setValue("closingTray", closingTray_);
  settings_->setValue("singleClickTray", singleClickTray_);
  settings_->setValue("emptyWorking", emptyWorking_);

  settings_->setValue("langFileName", langFileName_);

  QString fontFamily = feedsView_->font().family();
  settings_->setValue("/feedsFontFamily", fontFamily);
  int fontSize = feedsView_->font().pointSize();
  settings_->setValue("/feedsFontSize", fontSize);

  fontFamily = newsView_->font().family();
  settings_->setValue("/newsFontFamily", fontFamily);
  fontSize = newsView_->font().pointSize();
  settings_->setValue("/newsFontSize", fontSize);

  fontFamily = webView_->settings()->fontFamily(QWebSettings::StandardFont);
  settings_->setValue("/WebFontFamily", fontFamily);
  fontSize = webView_->settings()->fontSize(QWebSettings::DefaultFontSize);
  settings_->setValue("/WebFontSize", fontSize);

  settings_->setValue("autoLoadImages", autoLoadImages_);
  settings_->setValue("autoUpdatefeedsStartUp", autoUpdatefeedsStartUp_);
  settings_->setValue("autoUpdatefeeds", autoUpdatefeeds_);
  settings_->setValue("autoUpdatefeedsTime", autoUpdatefeedsTime_);

  settings_->setValue("toolBarStyle",
                      toolBarStyleGroup_->checkedAction()->objectName());
  settings_->setValue("toolBarIconSize",
                      toolBarIconSizeGroup_->checkedAction()->objectName());

  settings_->endGroup();

  settings_->setValue("GeometryState", saveGeometry());
  settings_->setValue("ToolBarsState", saveState());
  if (newsModel_->columnCount()) {
    settings_->setValue("NewsHeaderGeometry", newsHeader_->saveGeometry());
    settings_->setValue("NewsHeaderState", newsHeader_->saveState());
  }

  settings_->setValue("networkProxy/type",     networkProxy_.type());
  settings_->setValue("networkProxy/hostName", networkProxy_.hostName());
  settings_->setValue("networkProxy/port",     networkProxy_.port());
  settings_->setValue("networkProxy/user",     networkProxy_.user());
  settings_->setValue("networkProxy/password", networkProxy_.password());

  settings_->setValue("feedSettings/currentId",
                      feedsModel_->index(feedsView_->currentIndex().row(), 0).data().toInt());
  settings_->setValue("feedSettings/filterName",
                      feedsFilterGroup_->checkedAction()->objectName());
  settings_->setValue("newsSettings/filterName",
                      newsFilterGroup_->checkedAction()->objectName());
}

/*! \brief Добавление ленты в список лент *************************************/
void RSSListing::addFeed()
{
  AddFeedDialog *addFeedDialog = new AddFeedDialog(this);

  QClipboard *clipboard_ = QApplication::clipboard();
  QString clipboardStr = clipboard_->text().left(7);
  if (clipboardStr.contains("http://", Qt::CaseInsensitive)) {
    addFeedDialog->feedUrlEdit_->setText(clipboard_->text());
    addFeedDialog->feedUrlEdit_->setCursorPosition(0);
  } else addFeedDialog->feedUrlEdit_->setText("http://");

  if (addFeedDialog->exec() == QDialog::Rejected) return;
  QSqlQuery q(db_);
  QString qStr = QString("insert into feeds(text, xmlUrl) values (?, ?)");
  q.prepare(qStr);
  q.addBindValue(addFeedDialog->feedTitleEdit_->text());
  q.addBindValue(addFeedDialog->feedUrlEdit_->text());
  q.exec();
  q.exec(kCreateNewsTableQuery.arg(q.lastInsertId().toString()));
  q.finish();

  QModelIndex index = feedsView_->currentIndex();
  feedsModel_->select();
  feedsView_->setCurrentIndex(index);

  progressBar_->setMaximum(1);
  getFeed(
      feedsModel_->record(feedsModel_->rowCount()-1).field("xmlUrl").value().toString(),
      QDateTime::fromString(feedsModel_->record(feedsModel_->rowCount()-1).field("lastBuildDate").value().toString(), Qt::ISODate)
  );
}

/*! \brief Удаление ленты из списка лент с подтверждением *********************/
void RSSListing::deleteFeed()
{
  if (feedsView_->currentIndex().row() >= 0) {
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

    int row = feedsView_->currentIndex().row();
    feedsModel_->select();
    if (feedsModel_->rowCount() == row) row--;
    feedsView_->setCurrentIndex(feedsModel_->index(row, 0));
  }
}

/*! \brief Импорт лент из OPML-файла ******************************************/
void RSSListing::slotImportFeeds()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select OPML-file"),
      qApp->applicationDirPath(),
      tr("OPML-files (*.opml)"));

  if (fileName.isNull()) {
    statusBar()->showMessage(tr("Import canceled"), 3000);
    return;
  }

  qDebug() << "import file:" << fileName;

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    statusBar()->showMessage(tr("Import: can't open a file"), 3000);
    return;
  }

  db_.transaction();

  QXmlStreamReader xml(&file);

  int elementCount = 0;
  int outlineCount = 0;
  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isStartElement()) {
      statusBar()->showMessage(QVariant(elementCount).toString(), 3000);
      // Выбираем одни outline'ы
      if (xml.name() == "outline") {
        qDebug() << outlineCount << "+:" << xml.prefix().toString()
            << ":" << xml.name().toString();;
        QSqlQuery q(db_);
        QString qStr = QString("insert into feeds(text, title, description, xmlUrl, htmlUrl) "
                       "values(?, ?, ?, ?, ?)");
        q.prepare(qStr);
        q.addBindValue(xml.attributes().value("text").toString());
        q.addBindValue(xml.attributes().value("title").toString());
        q.addBindValue(xml.attributes().value("description").toString());
        q.addBindValue(xml.attributes().value("xmlUrl").toString());
        q.addBindValue(xml.attributes().value("htmlUrl").toString());
        q.exec();
        qDebug() << q.lastQuery() << q.boundValues();
        qDebug() << q.lastError().number() << ": " << q.lastError().text();
        q.exec(kCreateNewsTableQuery.arg(q.lastInsertId().toString()));
        q.finish();
      }
    } else if (xml.isEndElement()) {
      if (xml.name() == "outline") {
        ++outlineCount;
      }
      ++elementCount;
    }
  }
  if (xml.error()) {
    statusBar()->showMessage(QString("Import error: Line=%1, ErrorString=%2").
        arg(xml.lineNumber()).arg(xml.errorString()), 3000);
  } else {
    statusBar()->showMessage(QString("Import: file read done"), 3000);
  }
  db_.commit();

  QModelIndex index = feedsView_->currentIndex();
  feedsModel_->select();
  feedsView_->setCurrentIndex(index);
}

/*! \brief приём xml-файла ****************************************************/
void RSSListing::receiveXml(const QByteArray &data, const QUrl &url)
{
  url_ = url;
  data_.append(data);
}

/*! \brief Обработка окончания запроса ****************************************/
void RSSListing::getUrlDone(const int &result, const QDateTime &dtReply)
{
  qDebug() << "getUrl result =" << result;

  if (!url_.isEmpty() && !data_.isEmpty()) {
    emit xmlReadyParse(data_, url_);
    QSqlQuery q = db_.exec(QString("update feeds set lastBuildDate = '%1' where xmlUrl == '%2'").
        arg(dtReply.toString(Qt::ISODate)).
        arg(url_.toString()));
    qDebug() << url_.toString() << dtReply.toString(Qt::ISODate);
    qDebug() << q.lastQuery() << q.lastError() << q.lastError().text();
  }
  data_.clear();
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
    statusBar()->showMessage(progressBar_->text());
  }
}

void RSSListing::slotUpdateFeed(const QUrl &url)
{
  // поиск идентификатора ленты с таблице лент
  int parseFeedId = 0;
  QSqlQuery q(db_);
  q.exec(QString("select id from feeds where xmlUrl like '%1'").
      arg(url.toString()));
  while (q.next()) {
    parseFeedId = q.value(q.record().indexOf("id")).toInt();
  }

  int unreadCount = 0;
  QString qStr = QString("select count(read) from feed_%1 where read==0").
      arg(parseFeedId);
  q.exec(qStr);
  if (q.next()) unreadCount = q.value(0).toInt();

  qStr = QString("update feeds set unread='%1' where id=='%2'").
      arg(unreadCount).arg(parseFeedId);
  q.exec(qStr);

  int newCount = 0;
  qStr = QString("select count(new) from feed_%1 where new==1").
      arg(parseFeedId);
  q.exec(qStr);
  if (q.next()) newCount = q.value(0).toInt();

  int newCountOld = 0;
  qStr = QString("select newCount from feeds where id=='%1'").
      arg(parseFeedId);
  q.exec(qStr);
  if (q.next()) newCountOld = q.value(0).toInt();

  qStr = QString("update feeds set newCount='%1' where id=='%2'").
      arg(newCount).arg(parseFeedId);
  q.exec(qStr);

  if (!isActiveWindow() && (newCount > newCountOld)) {
    traySystem->setIcon(QIcon(":/images/images/QuiteRSS16_NewNews.png"));
  }

  QModelIndex index = feedsView_->currentIndex();

  // если обновлена просматриваемая лента, кликаем по ней
  if (parseFeedId ==
      feedsModel_->index(index.row(), feedsModel_->fieldIndex("id")).data().toInt()) {
    slotFeedsTreeClicked(feedsModel_->index(index.row(), 0));
  }
  // иначе обновляем модель лент
  else {
    feedsModel_->select();
    feedsView_->setCurrentIndex(index);
  }
}

/*! \brief Обработка нажатия в дереве лент ************************************/
void RSSListing::slotFeedsTreeClicked(QModelIndex index)
{
  static QModelIndex indexOld = index;
  if (index.row() != indexOld.row()) {
    slotSetAllRead();
  }
  indexOld = index;
  setFeedsFilter(feedsFilterGroup_->checkedAction());
  bool initNo = false;
  if (newsModel_->columnCount() == 0) initNo = true;
  newsModel_->setTable(QString("feed_%1").arg(feedsModel_->index(index.row(), 0).data().toString()));
  newsModel_->select();
  setNewsFilter(newsFilterGroup_->checkedAction());

  newsHeader_->overload();

  if (initNo) {
    newsHeader_->initColumns();
    newsHeader_->createMenu();
    newsHeader_->restoreGeometry(settings_->value("NewsHeaderGeometry").toByteArray());
    newsHeader_->restoreState(settings_->value("NewsHeaderState").toByteArray());
  }

  int row = -1;
  for (int i = 0; i < newsModel_->rowCount(); i++) {
    if (newsModel_->index(i, newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt() ==
        feedsModel_->index(index.row(), feedsModel_->fieldIndex("currentNews")).data().toInt()) {
      row = i;
    }
  }
  newsView_->setCurrentIndex(newsModel_->index(row, 0));
  slotNewsViewClicked(newsModel_->index(row, 0));

  newsTitleLabel_->setText(feedsModel_->index(index.row(), 1).data().toString());
  if (index.isValid()) feedProperties_->setEnabled(true);
  else feedProperties_->setEnabled(false);
}

/*! \brief Запрос обновления ленты ********************************************/
void RSSListing::getFeed(QString urlString, QDateTime date)
{
  persistentUpdateThread_->getUrl(urlString, date);

  progressBar_->setValue(progressBar_->minimum());
  progressBar_->show();
  statusBar()->showMessage(progressBar_->text());
  QTimer::singleShot(150, this, SLOT(slotProgressBarUpdate()));
}

/*! \brief Обработка нажатия в дереве новостей ********************************/
void RSSListing::slotNewsViewClicked(QModelIndex index)
{
  if (QApplication::keyboardModifiers() == Qt::NoModifier) {
    static QModelIndex indexOld = index;
    if (!index.isValid()) {
      webView_->setHtml("");
      webPanel_->hide();
      slotUpdateStatus();  // необходимо, когда выбрана другая лента, но новость в ней не выбрана
      return;
    }

    webPanel_->show();

    QModelIndex indexNew = index;
    if (!((index.row() == indexOld.row()) &&
           newsModel_->index(index.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() == 1)) {
      updateWebView(index);
      slotSetItemRead(index, 1);

      QSqlQuery q(db_);
      int id = newsModel_->index(index.row(), 0).
          data(Qt::EditRole).toInt();
      QString qStr = QString("update feeds set currentNews='%1' where id=='%2'").
          arg(id).arg(newsModel_->tableName().remove("feed_"));
      q.exec(qStr);
    }
    indexOld = indexNew;
  }
}

/*! \brief Обработка клавиш Up/Down в дереве лент *****************************/
void RSSListing::slotFeedsTreeKeyUpDownPressed()
{
  slotFeedsTreeClicked(feedsView_->currentIndex());
}

/*! \brief Обработка клавиш Up/Down в дереве новостей *************************/
void RSSListing::slotNewsKeyUpDownPressed()
{
  slotNewsViewClicked(newsView_->currentIndex());
}

/*! \brief Вызов окна настроек ************************************************/
void RSSListing::showOptionDlg()
{
  static int index = 0;
  OptionsDialog *optionsDialog = new OptionsDialog(this);
  optionsDialog->restoreGeometry(settings_->value("options/geometry").toByteArray());
  optionsDialog->setCurrentItem(index);

  optionsDialog->startingTray_->setChecked(startingTray_);
  optionsDialog->minimizingTray_->setChecked(minimizingTray_);
  optionsDialog->closingTray_->setChecked(closingTray_);
  optionsDialog->singleClickTray_->setChecked(singleClickTray_);
  optionsDialog->emptyWorking_->setChecked(emptyWorking_);

  optionsDialog->setProxy(networkProxy_);

  optionsDialog->updateFeedsStartUp_->setChecked(autoUpdatefeedsStartUp_);
  optionsDialog->updateFeeds_->setChecked(autoUpdatefeeds_);
  optionsDialog->updateFeedsTime_->setValue(autoUpdatefeedsTime_);

  optionsDialog->setLanguage(langFileName_);

  QString strFont = QString("%1, %2").
      arg(feedsView_->font().family()).
      arg(feedsView_->font().pointSize());
  optionsDialog->fontTree->topLevelItem(0)->setText(2, strFont);
  strFont = QString("%1, %2").
      arg(newsView_->font().family()).
      arg(newsView_->font().pointSize());
  optionsDialog->fontTree->topLevelItem(1)->setText(2, strFont);
  strFont = QString("%1, %2").
      arg(webView_->settings()->fontFamily(QWebSettings::StandardFont)).
      arg(webView_->settings()->fontSize(QWebSettings::DefaultFontSize));
  optionsDialog->fontTree->topLevelItem(2)->setText(2, strFont);

  int result = optionsDialog->exec();
  settings_->setValue("options/geometry", optionsDialog->saveGeometry());
  index = optionsDialog->currentIndex();

  if (result == QDialog::Rejected) return;

  startingTray_ = optionsDialog->startingTray_->isChecked();
  minimizingTray_ = optionsDialog->minimizingTray_->isChecked();
  closingTray_ = optionsDialog->closingTray_->isChecked();
  singleClickTray_ = optionsDialog->singleClickTray_->isChecked();
  emptyWorking_ = optionsDialog->emptyWorking_->isChecked();

  networkProxy_ = optionsDialog->proxy();
  persistentUpdateThread_->setProxy(networkProxy_);

  autoUpdatefeedsStartUp_ = optionsDialog->updateFeedsStartUp_->isChecked();
  autoUpdatefeeds_ = optionsDialog->updateFeeds_->isChecked();
  autoUpdatefeedsTime_ = optionsDialog->updateFeedsTime_->value();
  if (updateFeedsTimer_.isActive() && !autoUpdatefeeds_) {
    updateFeedsTimer_.stop();
  } else if (!updateFeedsTimer_.isActive() && autoUpdatefeeds_) {
    updateFeedsTimer_.start(autoUpdatefeedsTime_*60000, this);
  }

  if (langFileName_ != optionsDialog->language()) {
    langFileName_ = optionsDialog->language();
    qApp->removeTranslator(translator_);
    if (translator_->load("quiterss_" + langFileName_, qApp->applicationDirPath() + QString("/lang"))) {
      qApp->installTranslator(translator_);
    } else retranslateStrings();
  }

  QFont font = feedsView_->font();
  font.setFamily(
        optionsDialog->fontTree->topLevelItem(0)->text(2).section(", ", 0, 0));
  font.setPointSize(
        optionsDialog->fontTree->topLevelItem(0)->text(2).section(", ", 1).toInt());
  feedsView_->setFont(font);
  feedsModel_->font_ = font;
  font = newsView_->font();
  font.setFamily(
        optionsDialog->fontTree->topLevelItem(1)->text(2).section(", ", 0, 0));
  font.setPointSize(
        optionsDialog->fontTree->topLevelItem(1)->text(2).section(", ", 1).toInt());
  newsView_->setFont(font);
  webView_->settings()->setFontFamily(QWebSettings::StandardFont,
        optionsDialog->fontTree->topLevelItem(2)->text(2).section(", ", 0, 0));
  webView_->settings()->setFontSize(QWebSettings::DefaultFontSize,
        optionsDialog->fontTree->topLevelItem(2)->text(2).section(", ", 1).toInt());
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
  showWindowAct_ = new QAction(this);
  connect(showWindowAct_, SIGNAL(triggered()), this, SLOT(slotShowWindows()));
  QFont font_ = showWindowAct_->font();
  font_.setBold(true);
  showWindowAct_->setFont(font_);
  trayMenu_->addAction(showWindowAct_);
  trayMenu_->addAction(updateAllFeedsAct_);
  trayMenu_->addSeparator();

  trayMenu_->addAction(optionsAct_);
  trayMenu_->addSeparator();

  trayMenu_->addAction(exitAct_);
  traySystem->setContextMenu(trayMenu_);
}

/*! \brief Освобождение памяти ************************************************/
void RSSListing::myEmptyWorkingSet()
{
#if defined(Q_WS_WIN)
  if (isHidden())
    EmptyWorkingSet(GetCurrentProcess());
#endif
}

/*! \brief Обновление ленты (действие) ****************************************/
void RSSListing::slotGetFeed()
{
  progressBar_->setMaximum(1);
  getFeed(
      feedsModel_->record(feedsView_->currentIndex().row()).field("xmlUrl").value().toString(),
      QDateTime::fromString(feedsModel_->record(feedsView_->currentIndex().row()).field("lastBuildDate").value().toString(), Qt::ISODate)
  );
}

/*! \brief Обновление ленты (действие) ****************************************/
void RSSListing::slotGetAllFeeds()
{
  updateAllFeedsAct_->setEnabled(false);

  int feedCount = 0;

  QSqlQuery q(db_);
  q.exec("select xmlUrl, lastBuildDate from feeds where xmlUrl is not null");
  qDebug() << q.lastError();
  while (q.next()) {
    getFeed(q.record().value(0).toString(), q.record().value(1).toDateTime());
    ++feedCount;
  }

  if (0 == feedCount) {
    updateAllFeedsAct_->setEnabled(true);
    return;
  }

  progressBar_->setMaximum(feedCount-1);
}

void RSSListing::slotProgressBarUpdate()
{
  progressBar_->update();

  if (progressBar_->isVisible())
    QTimer::singleShot(150, this, SLOT(slotProgressBarUpdate()));
}

void RSSListing::slotVisibledFeedsDock()
{
  feedsDock_->setVisible(!feedsDock_->isVisible());
  if (newsDockArea_ == feedsDockArea_)
    newsDock_->setVisible(feedsDock_->isVisible());
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

void RSSListing::slotSetItemRead(QModelIndex index, int read)
{
  if (!index.isValid()) return;

  QModelIndex curIndex = newsView_->currentIndex();
  if (newsModel_->index(index.row(), newsModel_->fieldIndex("new")).data(Qt::EditRole).toInt() == 1) {
    newsModel_->setData(
        newsModel_->index(index.row(), newsModel_->fieldIndex("new")),
        0);
//    QSqlQuery q(db_);
//    q.exec(QString("update %1 set new=0 where id=='%2'").
//        arg(newsModel_->tableName()).
//        arg(newsModel_->index(index.row(), newsModel_->fieldIndex("id")).data().toInt()));
//    qDebug() << q.lastQuery() << q.lastError();
  }
  if (newsModel_->index(index.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() != read) {
    newsModel_->setData(
        newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
        read);
//    QSqlQuery q(db_);
//    q.exec(QString("update %1 set read='%2' where id=='%3'").
//        arg(newsModel_->tableName()).arg(read).
//        arg(newsModel_->index(index.row(), newsModel_->fieldIndex("id")).data().toInt()));
//    qDebug() << q.lastQuery() << q.lastError();
  }
  newsView_->setCurrentIndex(curIndex);
  slotUpdateStatus();
}

void RSSListing::markNewsRead()
{
  QModelIndex index = newsView_->currentIndex();
  if (newsModel_->index(index.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() == 0) {
    slotSetItemRead(index, 1);
  } else {
    slotSetItemRead(index, 0);
  }
}

void RSSListing::markAllNewsRead()
{
  QString qStr = QString("update %1 set read=1 WHERE read=0").
      arg(newsModel_->tableName());
  QSqlQuery q(db_);
  q.exec(qStr);
  qStr = QString("UPDATE %1 SET new=0 WHERE new=1").
      arg(newsModel_->tableName());
  q.exec(qStr);
  newsModel_->select();
  setNewsFilter(newsFilterGroup_->checkedAction());
  slotUpdateStatus();
}

/*! \brief Подсчёт новостей
 *
 * Подсчёт всех новостей в фиде. (50мс)
 * Подсчёт всех не прочитанных новостей в фиде. (50мс)
 * Подсчёт всех новых новостей в фиде. (50мс)
 * Запись этих данных в таблицу лент (100мс)
 * Вывод этих данных в статусную строку
 */
void RSSListing::slotUpdateStatus()
{
  QString qStr;

  int id = feedsModel_->index(
        feedsView_->currentIndex().row(), feedsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

  int allCount = 0;
  qStr = QString("select count(id) from %1 where deleted=0").
      arg(newsModel_->tableName());
  QSqlQuery q(db_);
  q.exec(qStr);
  if (q.next()) allCount = q.value(0).toInt();

  int unreadCount = 0;
  qStr = QString("select count(read) from %1 where read==0").
      arg(newsModel_->tableName());
  q.exec(qStr);
  if (q.next()) unreadCount = q.value(0).toInt();

  int newCount = 0;
  qStr = QString("select count(new) from %1 where new==1").
      arg(newsModel_->tableName());
  q.exec(qStr);
  if (q.next()) newCount = q.value(0).toInt();

  int newCountOld = 0;
  qStr = QString("select newCount from feeds where id=='%1'").
      arg(newsModel_->tableName().remove("feed_"));
  q.exec(qStr);
  if (q.next()) newCountOld = q.value(0).toInt();

  qStr = QString("update feeds set unread='%1', newCount='%2' where id=='%3'").
      arg(unreadCount).arg(newCount).
      arg(newsModel_->tableName().remove("feed_"));
  q.exec(qStr);

  if (!isActiveWindow() && (newCount > newCountOld)) {
    traySystem->setIcon(QIcon(":/images/images/QuiteRSS16_NewNews.png"));
  }

  QModelIndex index = feedsView_->currentIndex();
  feedsModel_->select();
  if (feedsModel_->index(index.row(),
                         feedsModel_->fieldIndex("id")).data(Qt::EditRole).toInt() == id) {
    feedsView_->setCurrentIndex(index);
  }

  statusUnread_->setText(QString(tr(" Unread: %1 ")).arg(unreadCount));
  statusAll_->setText(QString(tr(" All: %1 ")).arg(allCount));
}

void RSSListing::slotLoadStarted()
{
  if (newsView_->currentIndex().isValid()) {
    webViewProgress_->setValue(0);
    webViewProgress_->show();
  }
}

void RSSListing::slotLoadFinished(bool ok)
{
  if (!ok) statusBar()->showMessage(tr("Error loading to WebVeiw"), 3000);
  webViewProgress_->hide();
}

void RSSListing::setFeedsFilter(QAction* pAct)
{
  QModelIndex index = feedsView_->currentIndex();

  int id = feedsModel_->index(
        index.row(), feedsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

  if (pAct->objectName() == "filterFeedsAll_") {
    feedsModel_->setFilter("");
  } else if (pAct->objectName() == "filterFeedsNew_") {
    feedsModel_->setFilter(QString("newCount > 0"));
  } else if (pAct->objectName() == "filterFeedsUnread_") {
    feedsModel_->setFilter(QString("unread > 0"));
  }

  if (pAct->objectName() == "filterFeedsAll_") feedsFilter_->setIcon(QIcon(":/images/filterOff"));
  else feedsFilter_->setIcon(QIcon(":/images/filterOn"));

  int rowFeeds = -1;
  for (int i = 0; i < feedsModel_->rowCount(); i++) {
    if (feedsModel_->index(i, feedsModel_->fieldIndex("id")).data(Qt::EditRole).toInt() == id) {
      rowFeeds = i;
    }
  }
  feedsView_->setCurrentIndex(feedsModel_->index(rowFeeds, 0));
}

void RSSListing::setNewsFilter(QAction* pAct)
{
  QModelIndex index = newsView_->currentIndex();

  int id = newsModel_->index(
        index.row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

  QString qStr = QString("UPDATE %1 SET read=2 WHERE read=1").
      arg(newsModel_->tableName());
  QSqlQuery q(db_);
  q.exec(qStr);

  if (pAct->objectName() == "filterNewsAll_") {
    newsModel_->setFilter("deleted = 0");
  } else if (pAct->objectName() == "filterNewsNew_") {
    newsModel_->setFilter(QString("new = 1 AND deleted = 0"));
  } else if (pAct->objectName() == "filterNewsUnread_") {
    newsModel_->setFilter(QString("read < 2 AND deleted = 0"));
  } else if (pAct->objectName() == "filterNewsStar_") {
    newsModel_->setFilter(QString("sticky = 1 AND deleted = 0"));
  }

  if (pAct->objectName() == "filterNewsAll_") newsFilter_->setIcon(QIcon(":/images/filterOff"));
  else newsFilter_->setIcon(QIcon(":/images/filterOn"));

  int row = -1;
  for (int i = 0; i < newsModel_->rowCount(); i++) {
    if (newsModel_->index(i, newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt() == id) {
      row = i;
    }
  }
  newsView_->setCurrentIndex(newsModel_->index(row, 0));
  if (row == -1) webView_->setHtml("");
}

void RSSListing::slotFeedsDockLocationChanged(Qt::DockWidgetArea area)
{
  feedsDockArea_ = area;
}

void RSSListing::slotNewsDockLocationChanged(Qt::DockWidgetArea area)
{
  newsDockArea_ = area;
}

void RSSListing::slotNewsViewDoubleClicked(QModelIndex index)
{
  QDesktopServices::openUrl(
        QUrl(newsModel_->index(index.row(), newsModel_->fieldIndex("link_href")).data(Qt::EditRole).toString()));
}

void RSSListing::slotSetAllRead()
{
  QString qStr = QString("UPDATE %1 SET read=2 WHERE read=1").
      arg(newsModel_->tableName());
  QSqlQuery q(db_);
  q.exec(qStr);
  qStr = QString("UPDATE %1 SET new=0 WHERE new=1").
      arg(newsModel_->tableName());
  q.exec(qStr);

  qStr = QString("update feeds set newCount=0 where id=='%2'").
      arg(newsModel_->tableName().remove("feed_"));
  q.exec(qStr);
}

void RSSListing::slotShowAboutDlg()
{
  AboutDialog *aboutDialog = new AboutDialog(this);
  aboutDialog->exec();
}

void RSSListing::deleteNews()
{
  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  qDebug() << indexes.count();

  if (indexes.count() == 0) return;

  int cnt = indexes.count();
  for (int i = cnt-1; i >= 0; --i) {
    curIndex = indexes.at(i);
    int row = curIndex.row();
    qDebug() << "*01" << row << i;
    newsModel_->setData(
          newsModel_->index(row, newsModel_->fieldIndex("read")),
          1);
    if (newsModel_->index(row, newsModel_->fieldIndex("deleted")).data(Qt::EditRole).toInt() == 0) {
      newsModel_->setData(
            newsModel_->index(row, newsModel_->fieldIndex("deleted")),
            1);
    }
  }

  if (curIndex.row() == newsModel_->rowCount())
    curIndex = newsModel_->index(curIndex.row()-1, curIndex.column());
  newsView_->setCurrentIndex(curIndex);
  slotNewsViewClicked(curIndex);
  slotUpdateStatus();
}

void RSSListing::createMenuNews()
{
  newsContextMenu_ = new QMenu(this);
  newsContextMenu_->addAction(openInBrowserAct_);
  newsContextMenu_->addSeparator();
  newsContextMenu_->addAction(markNewsRead_);
  newsContextMenu_->addAction(markAllNewsRead_);
  newsContextMenu_->addSeparator();
  newsContextMenu_->addAction(markStarAct_);
  newsContextMenu_->addSeparator();
  newsContextMenu_->addAction(updateFeedAct_);
  newsContextMenu_->addSeparator();
  newsContextMenu_->addAction(deleteNewsAct_);
}

void RSSListing::showContextMenuNews(const QPoint &p)
{
  if (newsView_->currentIndex().isValid())
    newsContextMenu_->popup(newsView_->viewport()->mapToGlobal(p));
}

void RSSListing::openInBrowserNews()
{
  slotNewsViewDoubleClicked(newsView_->currentIndex());
}

void RSSListing::slotSetItemStar(QModelIndex index, int sticky)
{
  if (!index.isValid()) return;

  QModelIndex curIndex = newsView_->currentIndex();
  newsModel_->setData(
      newsModel_->index(index.row(), newsModel_->fieldIndex("sticky")),
      sticky);
  newsView_->setCurrentIndex(curIndex);
}

void RSSListing::markNewsStar()
{
  QModelIndex index = newsView_->currentIndex();
  if (newsModel_->index(index.row(), newsModel_->fieldIndex("sticky")).data(Qt::EditRole).toInt() == 0) {
    slotSetItemStar(index, 1);
  } else {
    slotSetItemStar(index, 0);
  }
}

void RSSListing::createMenuFeed()
{
  feedContextMenu_ = new QMenu(this);
  feedContextMenu_->addAction(addFeedAct_);
  feedContextMenu_->addSeparator();
  feedContextMenu_->addAction(markFeedRead_);
  feedContextMenu_->addSeparator();
  feedContextMenu_->addAction(updateFeedAct_);
  feedContextMenu_->addSeparator();
  feedContextMenu_->addAction(deleteFeedAct_);
  feedContextMenu_->addSeparator();
  feedContextMenu_->addAction(feedProperties_);
}

void RSSListing::showContextMenuFeed(const QPoint &p)
{
  if (feedsView_->currentIndex().isValid())
    feedContextMenu_->popup(feedsView_->viewport()->mapToGlobal(p));
}

void RSSListing::slotLinkClicked(QUrl url)
{
  QDesktopServices::openUrl(url);
}

void RSSListing::setAutoLoadImages()
{
  autoLoadImages_ = !autoLoadImages_;
  if (autoLoadImages_) {
    autoLoadImagesToggle_->setText(tr("Load images"));
    autoLoadImagesToggle_->setToolTip(tr("Auto load images to news view"));
    autoLoadImagesToggle_->setIcon(QIcon(":/images/imagesOn"));
  } else {
    autoLoadImagesToggle_->setText(tr("No load images"));
    autoLoadImagesToggle_->setToolTip(tr("No load images to news view"));
    autoLoadImagesToggle_->setIcon(QIcon(":/images/imagesOff"));
  }
  webView_->settings()->setAttribute(QWebSettings::AutoLoadImages, autoLoadImages_);
  updateWebView(newsView_->currentIndex());
}

void RSSListing::loadSettingsFeeds()
{
  autoLoadImages_ = !settings_->value("Settings/autoLoadImages", false).toBool();
  setAutoLoadImages();

  QString filterName = settings_->value("feedSettings/filterName", "filterFeedsAll_").toString();
  QList<QAction*> listActions = feedsFilterGroup_->actions();
  foreach(QAction *action, listActions) {
    if (action->objectName() == filterName) {
      action->setChecked(true);
      break;
    }
  }
  filterName = settings_->value("newsSettings/filterName", "filterNewsAll_").toString();
  listActions = newsFilterGroup_->actions();
  foreach(QAction *action, listActions) {
    if (action->objectName() == filterName) {
      action->setChecked(true);
      break;
    }
  }

  setFeedsFilter(feedsFilterGroup_->checkedAction());
  int id = settings_->value("feedSettings/currentId", 0).toInt();
  int row = -1;
  for (int i = 0; i < feedsModel_->rowCount(); i++) {
    if (feedsModel_->index(i, 0).data().toInt() == id) {
      row = i;
    }
  }

  feedsView_->setCurrentIndex(feedsModel_->index(row, 0));
  slotFeedsTreeClicked(feedsModel_->index(row, 0));  // загрузка новостей
}

void RSSListing::updateWebView(QModelIndex index)
{
  if (!index.isValid()) {
    webView_->setHtml("");
    webPanel_->hide();
    return;
  }

  webPanel_->show();

  QString titleString = newsModel_->record(index.row()).field("title").value().toString();
  titleString = webPanelTitle_->fontMetrics().elidedText(
      titleString, Qt::ElideRight, webPanelTitle_->width());
  QString panelTitleString = QString("<a href='%1'>%2</a>").
      arg(newsModel_->record(index.row()).field("link_href").value().toString()).
      arg(titleString);
  webPanelTitle_->setText(panelTitleString);

  // Формируем панель автора из автора новости
  QString authorString;
  QString authorName = newsModel_->record(index.row()).field("author_name").value().toString();
  QString authorEmail = newsModel_->record(index.row()).field("author_email").value().toString();
  QString authorUri = newsModel_->record(index.row()).field("author_uri").value().toString();
//  qDebug() << "author_news:" << authorName << authorEmail << authorUri;
  authorString = authorName;
  if (!authorEmail.isEmpty()) authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
  if (!authorUri.isEmpty())   authorString.append(QString(" <a href='%1'>page</a>").arg(authorUri));

  // Если авора новости нет, формируем панель автора из автора ленты
  // @NOTE(arhohryakov:2012.01.03) Автор берётся из текущего фида, т.к. при
  //   новость обновляется именно у него
  if (authorString.isEmpty()) {
    authorName = feedsModel_->record(feedsView_->currentIndex().row()).field("author_name").value().toString();
    authorEmail = feedsModel_->record(feedsView_->currentIndex().row()).field("author_email").value().toString();
    authorUri = feedsModel_->record(feedsView_->currentIndex().row()).field("author_uri").value().toString();
//    qDebug() << "author_feed:" << authorName << authorEmail << authorUri;
    authorString = authorName;
    if (!authorEmail.isEmpty()) authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
    if (!authorUri.isEmpty())   authorString.append(QString(" <a href='%1'>page</a>").arg(authorUri));
  }

  webPanelAuthor_->setText(authorString);
  webPanelAuthorLabel_->setVisible(!authorString.isEmpty());
  webPanelAuthor_->setVisible(!authorString.isEmpty());

  QString content = newsModel_->record(index.row()).field("content").value().toString();
  if (content.isEmpty()) {
    webView_->setHtml(
          newsModel_->record(index.row()).field("description").value().toString());
//    qDebug() << "setHtml : description";
  }
  else {
    webView_->setHtml(content);
//    qDebug() << "setHtml : content";
  }
}

void RSSListing::slotFeedsFilter()
{
  static QAction *action = NULL;

  if (feedsFilterGroup_->checkedAction()->objectName() == "filterFeedsAll_") {
    if (action != NULL) {
      action->setChecked(true);
      setFeedsFilter(action);
    } else {
      feedsFilterMenu_->popup(
            feedsToolBar_->mapToGlobal(QPoint(0, feedsToolBar_->height()-1)));
    }
  } else {
    action = feedsFilterGroup_->checkedAction();
    filterFeedsAll_->setChecked(true);
    setFeedsFilter(filterFeedsAll_);
  }
}

void RSSListing::slotNewsFilter()
{
  static QAction *action = NULL;

  if (newsFilterGroup_->checkedAction()->objectName() == "filterNewsAll_") {
    if (action != NULL) {
      action->setChecked(true);
      setNewsFilter(action);
    } else {
      newsFilterMenu_->popup(
            newsToolBar_->mapToGlobal(QPoint(0, newsToolBar_->height()-1)));
    }
  } else {
    action = newsFilterGroup_->checkedAction();
    filterNewsAll_->setChecked(true);
    setNewsFilter(filterNewsAll_);
  }
}

void RSSListing::slotTimerUpdateFeeds()
{
  if (autoUpdatefeeds_ && updateAllFeedsAct_->isEnabled()) {
    slotGetAllFeeds();
  }
}

void RSSListing::slotShowUpdateAppDlg()
{
  UpdateAppDialog *updateAppDialog = new UpdateAppDialog(this);
  updateAppDialog->exec();
}

/*
** This function is used to load the contents of a database file on disk
** into the "main" database of open database connection pInMemory, or
** to save the current contents of the database opened by pInMemory into
** a database file on disk. pInMemory is probably an in-memory database,
** but this function will also work fine if it is not.
**
** Parameter zFilename points to a nul-terminated string containing the
** name of the database file on disk to load from or save to. If parameter
** isSave is non-zero, then the contents of the file zFilename are
** overwritten with the contents of the database opened by pInMemory. If
** parameter isSave is zero, then the contents of the database opened by
** pInMemory are replaced by data loaded from the file zFilename.
**
** If the operation is successful, SQLITE_OK is returned. Otherwise, if
** an error occurs, an SQLite error code is returned.
*/
bool RSSListing::sqliteDBMemFile(QSqlDatabase memdb, QString filename, bool save)
{
  qDebug() << "sqliteDBMemFile(): save =" << save;
  bool state = false;
  QVariant v = memdb.driver()->handle();
  if (v.isValid() && qstrcmp(v.typeName(),"sqlite3*") == 0) {
    // v.data() returns a pointer to the handle
    sqlite3 * handle = *static_cast<sqlite3 **>(v.data());
    if (handle != 0) {  // check that it is not NULL
      sqlite3 * pInMemory = handle;
      QByteArray array = filename.toLocal8Bit();
      const char * zFilename = array.data();
      int rc;                   /* Function return code */
      sqlite3 *pFile;           /* Database connection opened on zFilename */
      sqlite3_backup *pBackup;  /* Backup object used to copy data */
      sqlite3 *pTo;             /* Database to copy to (pFile or pInMemory) */
      sqlite3 *pFrom;           /* Database to copy from (pFile or pInMemory) */

      /* Open the database file identified by zFilename. Exit early if this fails
      ** for any reason. */
      rc = sqlite3_open( zFilename, &pFile );
      if (rc == SQLITE_OK) {

        /* If this is a 'load' operation (isSave==0), then data is copied
        ** from the database file just opened to database pInMemory.
        ** Otherwise, if this is a 'save' operation (isSave==1), then data
        ** is copied from pInMemory to pFile.  Set the variables pFrom and
        ** pTo accordingly. */
        pFrom = ( save ? pInMemory : pFile);
        pTo   = ( save ? pFile     : pInMemory);

        /* Set up the backup procedure to copy from the "main" database of
        ** connection pFile to the main database of connection pInMemory.
        ** If something goes wrong, pBackup will be set to NULL and an error
        ** code and  message left in connection pTo.
        **
        ** If the backup object is successfully created, call backup_step()
        ** to copy data from pFile to pInMemory. Then call backup_finish()
        ** to release resources associated with the pBackup object.  If an
        ** error occurred, then  an error code and message will be left in
        ** connection pTo. If no error occurred, then the error code belonging
        ** to pTo is set to SQLITE_OK.
        */
        pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
        if (pBackup) {
          (void)sqlite3_backup_step(pBackup, -1);
          (void)sqlite3_backup_finish(pBackup);
        }
        rc = sqlite3_errcode(pTo);
      }

      /* Close the database connection opened on database file zFilename
      ** and return the result of this function. */
      (void)sqlite3_close(pFile);

      if (rc == SQLITE_OK) state = true;
    }
  }
  qDebug() << "sqliteDBMemFile(): return =" << state;
  return state;
}

void RSSListing::retranslateStrings() {
  webViewProgress_->setFormat(tr("Loading... (%p%)"));
  feedsTitleLabel_->setText(tr("Feeds"));
  webPanelTitleLabel_->setText(tr("Title:"));
  webPanelAuthorLabel_->setText(tr("Author:"));
  progressBar_->setFormat(tr("Update feeds... (%p%)"));
  statusUnread_->setText(statusUnread_->text());
  statusAll_->setText(statusAll_->text());

  QString str = statusUnread_->text();
  str = str.right(str.length() - str.indexOf(':') - 1).replace(" ", "");
  statusUnread_->setText(QString(tr(" Unread: %1 ")).arg(str));
  str = statusAll_->text();
  str = str.right(str.length() - str.indexOf(':') - 1).replace(" ", "");
  statusAll_->setText(QString(tr(" All: %1 ")).arg(str));

  addFeedAct_->setText(tr("&Add..."));
  addFeedAct_->setToolTip(tr("Add new feed"));

  deleteFeedAct_->setText(tr("&Delete..."));
  deleteFeedAct_->setToolTip(tr("Delete selected feed"));

  importFeedsAct_->setText(tr("&Import feeds..."));
  importFeedsAct_->setToolTip(tr("Import feeds from OPML file"));

  exitAct_->setText(tr("E&xit"));

  if (autoLoadImages_) {
    autoLoadImagesToggle_->setText(tr("Load images"));
    autoLoadImagesToggle_->setToolTip(tr("Auto load images to news view"));
  } else {
    autoLoadImagesToggle_->setText(tr("No load images"));
    autoLoadImagesToggle_->setToolTip(tr("No load images to news view"));
  }

  updateFeedAct_->setText(tr("Update feed"));
  updateFeedAct_->setToolTip(tr("Update current feed"));

  updateAllFeedsAct_->setText(tr("Update all"));
  updateAllFeedsAct_->setToolTip(tr("Update all feeds"));

  markNewsRead_->setText(tr("Mark Read"));
  markNewsRead_->setToolTip(tr("Mark current news read"));

  markAllNewsRead_->setText(tr("Mark all news Read"));
  markAllNewsRead_->setToolTip(tr("Mark all news read"));

  optionsAct_->setText(tr("Options..."));
  optionsAct_->setToolTip(tr("Open options gialog"));

  feedsFilter_->setText(tr("Filter"));
  filterFeedsAll_->setText(tr("Show All"));
  filterFeedsNew_->setText(tr("Show New"));
  filterFeedsUnread_->setText(tr("Show Unread"));

  newsFilter_->setText( tr("Filter"));
  filterNewsAll_->setText(tr("Show All"));
  filterNewsNew_->setText(tr("Show New"));
  filterNewsUnread_->setText(tr("Show Unread"));
  filterNewsStar_->setText(tr("Show Star"));

  aboutAct_ ->setText(tr("About..."));
  aboutAct_->setToolTip(tr("Show 'About' dialog"));

  updateAppAct_->setText(tr("Check for updates..."));

  openInBrowserAct_->setText(tr("Open in Browser"));
  markStarAct_->setText(tr("Star"));
  deleteNewsAct_->setText( tr("Delete"));

  markFeedRead_->setText(tr("Mark Read"));
  markFeedRead_->setToolTip(tr("Mark feed read"));
  feedProperties_->setText(tr("Properties"));
  feedProperties_->setToolTip(tr("Properties feed"));

  fileMenu_->setTitle(tr("&File"));
  editMenu_->setTitle(tr("&Edit"));
  viewMenu_->setTitle(tr("&View"));
  feedMenu_->setTitle(tr("Fee&ds"));
  newsMenu_->setTitle(tr("&News"));
  toolsMenu_->setTitle(tr("&Tools"));
  helpMenu_->setTitle(tr("&Help"));

  toolBar_->setWindowTitle(tr("ToolBar"));
  toolBarMenu_->setTitle(tr("ToolBar"));
  toolBarStyleMenu_->setTitle(tr("Style"));
  toolBarStyleI_->setText(tr("Icon"));
  toolBarStyleT_->setText(tr("Text"));
  toolBarStyleTbI_->setText(tr("Text beside icon"));
  toolBarStyleTuI_->setText(tr("Text under icon"));
  toolBarIconSizeMenu_->setTitle(tr("Icon size"));
  toolBarToggle_->setText(tr("Show ToolBar"));

  toolBarIconSizeMenu_->setTitle(tr("Icon size"));
  toolBarIconBig_->setText(tr("Big"));
  toolBarIconNormal_->setText(tr("Normal"));
  toolBarIconSmall_->setText(tr("Small"));

  showWindowAct_->setText(tr("Show window"));

  newsHeader_->retranslateStrings();
}

void RSSListing::setToolBarStyle(QAction *pAct)
{
  if (pAct->objectName() == "toolBarStyleI_") {
    toolBar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
  } else if (pAct->objectName() == "toolBarStyleT_") {
    toolBar_->setToolButtonStyle(Qt::ToolButtonTextOnly);
  } else if (pAct->objectName() == "toolBarStyleTbI_") {
    toolBar_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  } else {
    toolBar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  }
}

void RSSListing::setToolBarIconSize(QAction *pAct)
{
  if (pAct->objectName() == "toolBarIconBig_") {
    toolBar_->setIconSize(QSize(32, 32));
  } else if (pAct->objectName() == "toolBarIconSmall_") {
    toolBar_->setIconSize(QSize(16, 16));
  } else {
    toolBar_->setIconSize(QSize(24, 24));
  }
}

void RSSListing::showContextMenuToolBar(const QPoint &p)
{
  toolBarMenu_->popup(toolBar_->mapToGlobal(p));
}

void RSSListing::slotShowFeedPropertiesDlg()
{
  if (!feedsView_->currentIndex().isValid()){
    feedProperties_->setEnabled(false);
    return;
  }

  FeedPropertiesDialog *feedPropertiesDialog = new FeedPropertiesDialog(this);
  feedPropertiesDialog->restoreGeometry(settings_->value("feedProperties/geometry").toByteArray());

  QModelIndex index = feedsView_->currentIndex();
  feedPropertiesDialog->titleEdit_->setText(
        feedsModel_->record(index.row()).field("text").value().toString());
  feedPropertiesDialog->titleEdit_->setCursorPosition(0);
  feedPropertiesDialog->urlEdit_->setText(
        feedsModel_->record(index.row()).field("xmlUrl").value().toString());
  feedPropertiesDialog->urlEdit_->setCursorPosition(0);

  feedPropertiesDialog->titleString_ = feedsModel_->record(index.row()).field("title").value().toString();

  if (feedsModel_->record(index.row()).field("htmlUrl").value().toString().isNull()) {
    feedPropertiesDialog->homepageLabel_->setText("None");
  } else {
    QString homepageString = QString("<a href='%1'>%1</a>").
        arg(feedsModel_->record(index.row()).field("htmlUrl").value().toString());
    feedPropertiesDialog->homepageLabel_->setText(homepageString);
  }

  int result = feedPropertiesDialog->exec();
  settings_->setValue("feedProperties/geometry", feedPropertiesDialog->saveGeometry());
  if (result == QDialog::Rejected) return;

  int id = feedsModel_->record(index.row()).field("id").value().toInt();

  db_.exec(QString("update feeds set text = '%1' where id == '%2'").
          arg(feedPropertiesDialog->titleEdit_->text()).
          arg(id));
  db_.exec(QString("update feeds set xmlUrl = '%1' where id == '%2'").
          arg(feedPropertiesDialog->urlEdit_->text()).
          arg(id));

  feedsModel_->select();
  feedsView_->setCurrentIndex(index);
  newsTitleLabel_->setText(feedsModel_->index(index.row(), 1).data().toString());
}

void RSSListing::slotEditMenuAction()
{
  if (feedsView_->currentIndex().isValid()) feedProperties_->setEnabled(true);
  else  feedProperties_->setEnabled(false);
}
