#include "newstabwidget.h"

#include "rsslisting.h"
#include "webpage.h"

NewsTabWidget::NewsTabWidget(QSettings *settings, QWidget *parent)
  : QWidget(parent),
    settings_(settings)
{
  createNewsList();
  createWebWidget();
  readSettings();
  retranslateStrings();

  QSplitter *centralSplitter = new QSplitter(Qt::Vertical);
  centralSplitter->addWidget(newsWidget_);
  centralSplitter->addWidget(webWidget_);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(centralSplitter, 0);
  setLayout(layout);
}

void NewsTabWidget::createNewsList()
{
  newsView_ = new NewsView(this);
  newsView_->setFrameStyle(QFrame::NoFrame);
  newsModel_ = new NewsModel(this, newsView_);
  newsModel_->setTable("news");
  newsModel_->setFilter("feedId=-1");
  newsHeader_ = new NewsHeader(newsModel_, newsView_);

  newsView_->setModel(newsModel_);
  newsView_->setHeader(newsHeader_);

  newsHeader_->init(settings_);

  newsToolBar_ = new QToolBar(this);
  newsToolBar_->setStyleSheet("QToolBar { border: none; padding: 0px; }");
  newsToolBar_->setIconSize(QSize(18, 18));

  QHBoxLayout *newsPanelLayout = new QHBoxLayout();
  newsPanelLayout->setMargin(0);
  newsPanelLayout->setSpacing(0);
  newsPanelLayout->addSpacing(5);
  newsPanelLayout->addWidget(newsToolBar_);
  newsPanelLayout->addStretch(1);

  QFrame *line = new QFrame(this);
  line->setFrameStyle(QFrame::HLine | QFrame::Sunken);

  QVBoxLayout *newsLayout = new QVBoxLayout();
  newsLayout->setMargin(0);
  newsLayout->setSpacing(0);
  newsLayout->addLayout(newsPanelLayout);
  newsLayout->addWidget(line);
  newsLayout->addWidget(newsView_);

  newsWidget_ = new QWidget(this);
  newsWidget_->setLayout(newsLayout);

//  //! Create title DockWidget
//  newsIconTitle_ = new QLabel(this);
//  newsIconTitle_->setPixmap(QPixmap(":/images/feed"));
//  newsTextTitle_ = new QLabel(this);
//  newsTextTitle_->setObjectName("newsTextTitle_");
//  QFont fontNewsTextTitle = newsTextTitle_->font();
//  fontNewsTextTitle.setBold(true);
//  newsTextTitle_->setFont(fontNewsTextTitle);

//  QHBoxLayout *newsTitleLayout = new QHBoxLayout();
//  newsTitleLayout->setMargin(4);
//  newsTitleLayout->addSpacing(3);
//  newsTitleLayout->addWidget(newsIconTitle_);
//  newsTitleLayout->addWidget(newsTextTitle_, 1);
//  newsTitleLayout->addSpacing(3);

//  newsTitleLabel_ = new QWidget(this);
//  newsTitleLabel_->setObjectName("newsTitleLabel_");
//  newsTitleLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
//  newsTitleLabel_->setLayout(newsTitleLayout);

//  newsToolBar_ = new QToolBar(this);
//  newsToolBar_->setStyleSheet("QToolBar { border: none; padding: 0px; }");
//  newsToolBar_->setIconSize(QSize(18, 18));

//  QHBoxLayout *newsPanelLayout = new QHBoxLayout();
//  newsPanelLayout->setMargin(0);
//  newsPanelLayout->setSpacing(0);

//  newsPanelLayout->addWidget(newsTitleLabel_);
//  newsPanelLayout->addStretch(1);
//  newsPanelLayout->addWidget(newsToolBar_);
//  newsPanelLayout->addSpacing(5);

//  QWidget *newsPanel = new QWidget(this);
//  newsPanel->setObjectName("newsPanel");
//  newsPanel->setLayout(newsPanelLayout);

  //! Create news DockWidget
//  newsDock_ = new QDockWidget(this);
//  newsDock_->setObjectName("newsDock");
//  newsDock_->setFeatures(QDockWidget::DockWidgetMovable);
//  newsDock_->setTitleBarWidget(newsPanel);
//  newsDock_->setWidget(newsView_);
//  addDockWidget(Qt::TopDockWidgetArea, newsDock_);

//  connect(newsView_, SIGNAL(pressed(QModelIndex)),
//          this, SLOT(slotNewsViewClicked(QModelIndex)));
//  connect(newsView_, SIGNAL(pressKeyUp()), this, SLOT(slotNewsUpPressed()));
//  connect(newsView_, SIGNAL(pressKeyDown()), this, SLOT(slotNewsDownPressed()));
//  connect(newsView_, SIGNAL(signalSetItemRead(QModelIndex, int)),
//          this, SLOT(slotSetItemRead(QModelIndex, int)));
//  connect(newsView_, SIGNAL(signalSetItemStar(QModelIndex,int)),
//          this, SLOT(slotSetItemStar(QModelIndex,int)));
//  connect(newsView_, SIGNAL(signalDoubleClicked(QModelIndex)),
//          this, SLOT(slotNewsViewDoubleClicked(QModelIndex)));
//  connect(newsView_, SIGNAL(customContextMenuRequested(QPoint)),
//          this, SLOT(showContextMenuNews(const QPoint &)));
//  connect(newsDock_, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
//          this, SLOT(slotNewsDockLocationChanged(Qt::DockWidgetArea)));
}

void NewsTabWidget::createWebWidget()
{
  webView_ = new WebView(this);

  webView_->pageAction(QWebPage::OpenLinkInNewWindow)->setVisible(false);
  webView_->pageAction(QWebPage::DownloadLinkToDisk)->setVisible(false);
  webView_->pageAction(QWebPage::OpenImageInNewWindow)->setVisible(false);
  webView_->pageAction(QWebPage::DownloadImageToDisk)->setVisible(false);

  webView_->setPage(new WebPage(this));

  webViewProgress_ = new QProgressBar(this);
  webViewProgress_->setObjectName("webViewProgress_");
  webViewProgress_->setFixedHeight(15);
  webViewProgress_->setMinimum(0);
  webViewProgress_->setMaximum(100);
  webViewProgress_->setVisible(true);
//  connect(webView_, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
//  connect(webView_, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished(bool)));
//  connect(webView_, SIGNAL(linkClicked(QUrl)), this, SLOT(slotLinkClicked(QUrl)));
//  connect(webView_->page(), SIGNAL(linkHovered(QString,QString,QString)),
//          this, SLOT(slotLinkHovered(QString,QString,QString)));
//  connect(webView_, SIGNAL(loadProgress(int)), this, SLOT(slotSetValue(int)));

  webViewProgressLabel_ = new QLabel(this);
  QHBoxLayout *progressLayout = new QHBoxLayout();
  progressLayout->setMargin(0);
  progressLayout->addWidget(webViewProgressLabel_, 0, Qt::AlignLeft|Qt::AlignVCenter);
  webViewProgress_->setLayout(progressLayout);

  //! Create web control panel
  webToolBar_ = new QToolBar(this);
  webToolBar_->setStyleSheet("QToolBar { border: none; padding: 0px; }");
  webToolBar_->setIconSize(QSize(16, 16));

//  webHomePageAct_ = new QAction(this);
//  webHomePageAct_->setIcon(QIcon(":/images/homePage"));
//  connect(webHomePageAct_, SIGNAL(triggered()),
//          this, SLOT(webHomePage()));

//  webToolBar_->addAction(webHomePageAct_);
//  QAction *webAction = webView_->pageAction(QWebPage::Back);
////  webAction->setIcon(QIcon(":/images/backPage"));
//  webToolBar_->addAction(webAction);
//  webAction = webView_->pageAction(QWebPage::Forward);
////  webAction->setIcon(QIcon(":/images/forwardPage"));
//  webToolBar_->addAction(webAction);
//  webAction = webView_->pageAction(QWebPage::Reload);
////  webAction->setIcon(QIcon(":/images/updateAllFeeds"));
//  webToolBar_->addAction(webAction);
//  webAction = webView_->pageAction(QWebPage::Stop);
////  webAction->setIcon(QIcon(":/images/delete"));
//  webToolBar_->addAction(webAction);
//  webToolBar_->addSeparator();

//  webExternalBrowserAct_ = new QAction(this);
//  webExternalBrowserAct_->setIcon(QIcon(":/images/openBrowser"));
//  webToolBar_->addAction(webExternalBrowserAct_);
//  connect(webExternalBrowserAct_, SIGNAL(triggered()),
//          this, SLOT(openPageInExternalBrowser()));

  QHBoxLayout *webControlPanelHLayout = new QHBoxLayout();
  webControlPanelHLayout->setMargin(0);
  webControlPanelHLayout->addSpacing(5);
  webControlPanelHLayout->addWidget(webToolBar_);

  QFrame *webControlPanelLine = new QFrame(this);
  webControlPanelLine->setFrameStyle(QFrame::HLine | QFrame::Sunken);

  QVBoxLayout *webControlPanelLayout = new QVBoxLayout();
  webControlPanelLayout->setMargin(0);
  webControlPanelLayout->setSpacing(0);
  webControlPanelLayout->addLayout(webControlPanelHLayout);
  webControlPanelLayout->addWidget(webControlPanelLine);

  webControlPanel_ = new QWidget(this);
  webControlPanel_->setObjectName("webControlPanel_");
  webControlPanel_->setLayout(webControlPanelLayout);
  webControlPanel_->setVisible(false);

  //! Create web panel
  webPanelTitleLabel_ = new QLabel(this);
  webPanelTitleLabel_->setCursor(Qt::PointingHandCursor);
  webPanelAuthorLabel_ = new QLabel(this);

  webPanelAuthor_ = new QLabel(this);
  webPanelAuthor_->setObjectName("webPanelAuthor_");
//  connect(webPanelAuthor_, SIGNAL(linkActivated(QString)),
//          this, SLOT(slotWebTitleLinkClicked(QString)));

  webPanelTitle_ = new QLabel(this);
  webPanelTitle_->setObjectName("webPanelTitle_");
//  connect(webPanelTitle_, SIGNAL(linkActivated(QString)),
//          this, SLOT(slotWebTitleLinkClicked(QString)));

  QGridLayout *webPanelLayout1 = new QGridLayout();
  webPanelLayout1->setMargin(5);
  webPanelLayout1->setSpacing(5);
  webPanelLayout1->setColumnStretch(1, 1);
  webPanelLayout1->addWidget(webPanelTitleLabel_, 0, 0, 1, 1);
  webPanelLayout1->addWidget(webPanelTitle_, 0, 1, 1, 1);
  webPanelLayout1->addWidget(webPanelAuthorLabel_, 1, 0, 1, 1);
  webPanelLayout1->addWidget(webPanelAuthor_, 1, 1, 1, 1);

  QFrame *webPanelLine = new QFrame(this);
  webPanelLine->setFrameStyle(QFrame::HLine | QFrame::Sunken);

  QVBoxLayout *webPanelLayout = new QVBoxLayout();
  webPanelLayout->setMargin(0);
  webPanelLayout->setSpacing(0);
  webPanelLayout->addLayout(webPanelLayout1);
  webPanelLayout->addWidget(webPanelLine);
  webPanelLayout->addWidget(webControlPanel_);

  webPanel_ = new QWidget(this);
  webPanel_->setObjectName("webPanel_");
  webPanel_->setLayout(webPanelLayout);

  QFrame *line = new QFrame(this);
  line->setFrameStyle(QFrame::HLine | QFrame::Sunken);

  //! Create web layout
  QVBoxLayout *webLayout = new QVBoxLayout();
  webLayout->setMargin(0);
  webLayout->setSpacing(0);
  webLayout->addWidget(line);
  webLayout->addWidget(webPanel_);
  webLayout->addWidget(webView_, 1);
  webLayout->addWidget(webViewProgress_);

  webWidget_ = new QWidget(this);
  webWidget_->setObjectName("webWidget_");
  webWidget_->setLayout(webLayout);
  webWidget_->setMinimumWidth(400);
}

/*! \brief Чтение настроек из ini-файла ***************************************/
void NewsTabWidget::readSettings()
{
  settings_->beginGroup("/Settings");

  QString fontFamily = settings_->value("/newsFontFamily", qApp->font().family()).toString();
  int fontSize = settings_->value("/newsFontSize", 8).toInt();
  newsView_->setFont(QFont(fontFamily, fontSize));
  fontFamily = settings_->value("/WebFontFamily", qApp->font().family()).toString();
  fontSize = settings_->value("/WebFontSize", 12).toInt();
  webView_->settings()->setFontFamily(QWebSettings::StandardFont, fontFamily);
  webView_->settings()->setFontSize(QWebSettings::DefaultFontSize, fontSize);

//  autoUpdatefeedsStartUp_ = settings_->value("autoUpdatefeedsStartUp", false).toBool();
//  autoUpdatefeeds_ = settings_->value("autoUpdatefeeds", false).toBool();
//  autoUpdatefeedsTime_ = settings_->value("autoUpdatefeedsTime", 10).toInt();
//  autoUpdatefeedsInterval_ = settings_->value("autoUpdatefeedsInterval", 0).toInt();

//  openingFeedAction_ = settings_->value("openingFeedAction", 0).toInt();
//  openNewsWebViewOn_ = settings_->value("openNewsWebViewOn", true).toBool();

//  markNewsReadOn_ = settings_->value("markNewsReadOn", true).toBool();
//  markNewsReadTime_ = settings_->value("markNewsReadTime", 0).toInt();

//  showDescriptionNews_ = settings_->value("showDescriptionNews", true).toBool();

  bool embeddedBrowserOn_ = settings_->value("embeddedBrowserOn", false).toBool();
  if (embeddedBrowserOn_) {
    webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
//    openInExternalBrowserAct_->setVisible(true);
  } else {
    webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
//    openInExternalBrowserAct_->setVisible(false);
  }
  bool javaScriptEnable_ = settings_->value("javaScriptEnable", true).toBool();
  webView_->settings()->setAttribute(
        QWebSettings::JavascriptEnabled, javaScriptEnable_);
  bool pluginsEnable_ = settings_->value("pluginsEnable", true).toBool();
  webView_->settings()->setAttribute(
        QWebSettings::PluginsEnabled, pluginsEnable_);

  settings_->endGroup();
}

void NewsTabWidget::retranslateStrings() {
  webViewProgress_->setFormat(tr("Loading... (%p%)"));
  webPanelTitleLabel_->setText(tr("Title:"));
  webPanelAuthorLabel_->setText(tr("Author:"));

  webView_->page()->action(QWebPage::OpenLink)->setText(tr("Open Link"));
  webView_->page()->action(QWebPage::OpenLinkInNewWindow)->setText(tr("Open in New Window"));
  webView_->page()->action(QWebPage::DownloadLinkToDisk)->setText(tr("Save Link..."));
  webView_->page()->action(QWebPage::CopyLinkToClipboard)->setText(tr("Copy Link"));
  webView_->page()->action(QWebPage::Copy)->setText(tr("Copy"));
  webView_->page()->action(QWebPage::Back)->setText(tr("Go Back"));
  webView_->page()->action(QWebPage::Forward)->setText(tr("Go Forward"));
  webView_->page()->action(QWebPage::Stop)->setText(tr("Stop"));
  webView_->page()->action(QWebPage::Reload)->setText(tr("Reload"));

  newsHeader_->retranslateStrings();
}
