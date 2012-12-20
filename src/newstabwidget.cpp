#include "newstabwidget.h"
#include "rsslisting.h"

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

RSSListing *rsslisting_;

NewsTabWidget::NewsTabWidget( QWidget *parent, int type, int feedId, int feedParId)
  : QWidget(parent), type_(type),
    feedId_(feedId), feedParId_(feedParId)
{
  rsslisting_ = qobject_cast<RSSListing*>(parent);
  db_ = &rsslisting_->db_;
  feedsTreeView_ = rsslisting_->feedsTreeView_;
  feedsTreeModel_ = rsslisting_->feedsTreeModel_;

  currentNewsIdOld = -1;

  newsIconTitle_ = new QLabel();
  newsIconMovie_ = new QMovie(":/images/loading");
  newsIconTitle_->setMovie(newsIconMovie_);
  newsTextTitle_ = new QLabel();

  closeButton_ = new QToolButton();
  closeButton_->setFixedSize(15, 15);
  closeButton_->setCursor(Qt::ArrowCursor);
  closeButton_->setStyleSheet(
        "QToolButton { background-color: transparent;"
        "border: none; padding: 0px;"
        "image: url(:/images/close); }"
        "QToolButton:hover {"
        "image: url(:/images/closeHover); }"
        );
  connect(closeButton_, SIGNAL(clicked()),
          this, SLOT(slotTabClose()));

  QHBoxLayout *newsTitleLayout = new QHBoxLayout();
  newsTitleLayout->setMargin(0);
  newsTitleLayout->setSpacing(0);
  newsTitleLayout->addWidget(newsIconTitle_);
  newsTitleLayout->addSpacing(3);
  newsTitleLayout->addWidget(newsTextTitle_, 1);
  newsTitleLayout->addWidget(closeButton_);

  newsTitleLabel_ = new QWidget();
  newsTitleLabel_->setObjectName("newsTitleLabel_");
  newsTitleLabel_->setStyleSheet("min-height: 16px;");
  newsTitleLabel_->setFixedWidth(148);
  newsTitleLabel_->setLayout(newsTitleLayout);
  newsTitleLabel_->setVisible(false);

  if (type_ != TAB_WEB) {
    createNewsList();
    createMenuNews();
  }
  createWebWidget();

  if (type_ != TAB_WEB) {
    newsTabWidgetSplitter_ = new QSplitter(this);
    newsTabWidgetSplitter_->setObjectName("newsTabWidgetSplitter");

    if ((rsslisting_->browserPosition_ == TOP_POSITION) ||
        (rsslisting_->browserPosition_ == LEFT_POSITION)) {
      newsTabWidgetSplitter_->addWidget(webWidget_);
      newsTabWidgetSplitter_->addWidget(newsWidget_);
    } else {
      newsTabWidgetSplitter_->addWidget(newsWidget_);
      newsTabWidgetSplitter_->addWidget(webWidget_);
    }
  }

  QVBoxLayout *layout = new QVBoxLayout();
  layout->setMargin(0);
  layout->setSpacing(0);
  if (type_ != TAB_WEB)
    layout->addWidget(newsTabWidgetSplitter_);
  else
    layout->addWidget(webWidget_);
  setLayout(layout);


  if (type_ != TAB_WEB) {
    newsTabWidgetSplitter_->restoreState(
          rsslisting_->settings_->value("NewsTabSplitterState").toByteArray());
    newsTabWidgetSplitter_->restoreGeometry(
          rsslisting_->settings_->value("NewsTabSplitterGeometry").toByteArray());

    if ((rsslisting_->browserPosition_ == RIGHT_POSITION) ||
        (rsslisting_->browserPosition_ == LEFT_POSITION)) {
      newsTabWidgetSplitter_->setOrientation(Qt::Horizontal);
    } else {
      newsTabWidgetSplitter_->setOrientation(Qt::Vertical);
    }
    newsTabWidgetSplitter_->setHandleWidth(1);
    newsTabWidgetSplitter_->setStyleSheet(
          QString("QSplitter::handle {background: %1;}").
          arg(qApp->palette().color(QPalette::Dark).name()));
  }
}

void NewsTabWidget::showEvent(QShowEvent *)
{
  QString titleStr, panelTitleStr;
  titleStr = webPanelTitle_->fontMetrics().elidedText(
        titleString_, Qt::ElideRight, webPanelTitle_->width());
  panelTitleStr = QString("<a href='%1'>%2</a>").arg(linkString_).arg(titleStr);
  webPanelTitle_->setText(panelTitleStr);
}

void NewsTabWidget::resizeEvent(QResizeEvent *)
{
  QString titleStr, panelTitleStr;
  titleStr = webPanelTitle_->fontMetrics().elidedText(
        titleString_, Qt::ElideRight, webPanelTitle_->width());
  panelTitleStr = QString("<a href='%1'>%2</a>").arg(linkString_).arg(titleStr);
  webPanelTitle_->setText(panelTitleStr);
}

//! Создание новостного списка и всех сопутствующих панелей
void NewsTabWidget::createNewsList()
{
  newsView_ = new NewsView(this);
  newsView_->setFrameStyle(QFrame::NoFrame);
  newsModel_ = new NewsModel(this, newsView_, db_);
  newsModel_->setTable("news");
  newsModel_->setFilter("feedId=-1");
  newsHeader_ = new NewsHeader(newsModel_, newsView_);

  newsView_->setModel(newsModel_);
  newsView_->setHeader(newsHeader_);

  newsHeader_->init(rsslisting_);

  newsToolBar_ = new QToolBar(this);
  newsToolBar_->setStyleSheet("QToolBar { border: none; padding: 0px; }");
  newsToolBar_->setIconSize(QSize(18, 18));

  newsToolBar_->addAction(rsslisting_->markNewsRead_);
  newsToolBar_->addAction(rsslisting_->markAllNewsRead_);
  newsToolBar_->addSeparator();
  newsToolBar_->addAction(rsslisting_->markStarAct_);

  newsToolBar_->addSeparator();
  newsToolBar_->addAction(rsslisting_->deleteNewsAct_);
  newsToolBar_->addSeparator();
  newsToolBar_->addAction(rsslisting_->newsFilter_);
  newsToolBar_->addSeparator();
  newsToolBar_->addAction(rsslisting_->restoreNewsAct_);

  findText_ = new FindTextContent(this);
  findText_->setFixedWidth(200);

  QHBoxLayout *newsPanelLayout = new QHBoxLayout();
  newsPanelLayout->setMargin(2);
  newsPanelLayout->setSpacing(2);
  newsPanelLayout->addWidget(newsToolBar_);
  newsPanelLayout->addStretch(1);
  newsPanelLayout->addWidget(findText_);

  QFrame *line = new QFrame(this);
  line->setFrameStyle(QFrame::HLine | QFrame::Sunken);

  QVBoxLayout *newsPanelLayoutV = new QVBoxLayout();
  newsPanelLayoutV->setMargin(0);
  newsPanelLayoutV->setSpacing(0);
  newsPanelLayoutV->addLayout(newsPanelLayout);
  newsPanelLayoutV->addWidget(line);

  newsPanelWidget_ = new QWidget(this);
  newsPanelWidget_->setLayout(newsPanelLayoutV);
  if (!rsslisting_->newsToolbarToggle_->isChecked())
    newsPanelWidget_->hide();

  QVBoxLayout *newsLayout = new QVBoxLayout();
  newsLayout->setMargin(0);
  newsLayout->setSpacing(0);
  newsLayout->addWidget(newsPanelWidget_);
  newsLayout->addWidget(newsView_);

  newsWidget_ = new QWidget(this);
  newsWidget_->setLayout(newsLayout);

  markNewsReadTimer_ = new QTimer(this);

  connect(newsView_, SIGNAL(pressed(QModelIndex)),
          this, SLOT(slotNewsViewClicked(QModelIndex)));
  connect(newsView_, SIGNAL(pressKeyUp()), this, SLOT(slotNewsUpPressed()));
  connect(newsView_, SIGNAL(pressKeyDown()), this, SLOT(slotNewsDownPressed()));
  connect(newsView_, SIGNAL(pressKeyHome()), this, SLOT(slotNewsHomePressed()));
  connect(newsView_, SIGNAL(pressKeyEnd()), this, SLOT(slotNewsEndPressed()));
  connect(newsView_, SIGNAL(pressKeyPageUp()), this, SLOT(slotNewsPageUpPressed()));
  connect(newsView_, SIGNAL(pressKeyPageDown()), this, SLOT(slotNewsPageDownPressed()));
  connect(newsView_, SIGNAL(signalSetItemRead(QModelIndex, int)),
          this, SLOT(slotSetItemRead(QModelIndex, int)));
  connect(newsView_, SIGNAL(signalSetItemStar(QModelIndex,int)),
          this, SLOT(slotSetItemStar(QModelIndex,int)));
  connect(newsView_, SIGNAL(signalDoubleClicked(QModelIndex)),
          this, SLOT(slotNewsViewDoubleClicked(QModelIndex)));
  connect(newsView_, SIGNAL(signalMiddleClicked(QModelIndex)),
          this, SLOT(slotNewsMiddleClicked(QModelIndex)));
  connect(markNewsReadTimer_, SIGNAL(timeout()),
          this, SLOT(slotReadTimer()));
  connect(newsView_, SIGNAL(customContextMenuRequested(QPoint)),
          this, SLOT(showContextMenuNews(const QPoint &)));

  connect(newsModel_, SIGNAL(signalSort(int,int)),
          this, SLOT(slotSort(int,int)));

  connect(findText_, SIGNAL(textChanged(QString)),
          this, SLOT(slotFindText(QString)));
  connect(findText_, SIGNAL(signalSelectFind()),
          this, SLOT(slotSelectFind()));
  connect(findText_, SIGNAL(returnPressed()),
          this, SLOT(slotSelectFind()));

  connect(rsslisting_->newsToolbarToggle_, SIGNAL(toggled(bool)),
          newsPanelWidget_, SLOT(setVisible(bool)));
}

void NewsTabWidget::createMenuNews()
{
  newsContextMenu_ = new QMenu(this);
  newsContextMenu_->addAction(rsslisting_->restoreNewsAct_);
  newsContextMenu_->addSeparator();
  newsContextMenu_->addAction(rsslisting_->openInBrowserAct_);
  newsContextMenu_->addAction(rsslisting_->openInExternalBrowserAct_);
  newsContextMenu_->addAction(rsslisting_->openNewsNewTabAct_);
  newsContextMenu_->addSeparator();
  newsContextMenu_->addAction(rsslisting_->markNewsRead_);
  newsContextMenu_->addAction(rsslisting_->markAllNewsRead_);
  newsContextMenu_->addSeparator();
  newsContextMenu_->addAction(rsslisting_->markStarAct_);
  newsContextMenu_->addSeparator();
  newsContextMenu_->addAction(rsslisting_->updateFeedAct_);
  newsContextMenu_->addSeparator();
  newsContextMenu_->addAction(rsslisting_->deleteNewsAct_);
  newsContextMenu_->addAction(rsslisting_->deleteAllNewsAct_);
}

void NewsTabWidget::showContextMenuNews(const QPoint &p)
{
  if (newsView_->currentIndex().isValid())
    newsContextMenu_->popup(newsView_->viewport()->mapToGlobal(p));
}

//! Создание веб-виджета и панели управления
void NewsTabWidget::createWebWidget()
{
  webView_ = new WebView(this);

  webMenu_ = new QMenu(webView_);

  webViewProgress_ = new QProgressBar(this);
  webViewProgress_->setObjectName("webViewProgress_");
  webViewProgress_->setFixedHeight(15);
  webViewProgress_->setMinimum(0);
  webViewProgress_->setMaximum(100);
  webViewProgress_->setVisible(true);

  webViewProgressLabel_ = new QLabel(this);
  QHBoxLayout *progressLayout = new QHBoxLayout();
  progressLayout->setMargin(0);
  progressLayout->addWidget(webViewProgressLabel_, 0, Qt::AlignLeft|Qt::AlignVCenter);
  webViewProgress_->setLayout(progressLayout);

  //! Create web control panel
  QToolBar *webToolBar_ = new QToolBar(this);
  webToolBar_->setStyleSheet("QToolBar { border: none; padding: 0px; }");
  webToolBar_->setIconSize(QSize(18, 18));

  webHomePageAct_ = new QAction(this);
  webHomePageAct_->setIcon(QIcon(":/images/homePage"));

  webToolBar_->addAction(webHomePageAct_);
  QAction *webAction = webView_->pageAction(QWebPage::Back);
  webToolBar_->addAction(webAction);
  webAction = webView_->pageAction(QWebPage::Forward);
  webToolBar_->addAction(webAction);
  webAction = webView_->pageAction(QWebPage::Reload);
  webToolBar_->addAction(webAction);
  webAction = webView_->pageAction(QWebPage::Stop);
  webToolBar_->addAction(webAction);
  webToolBar_->addSeparator();

  webExternalBrowserAct_ = new QAction(this);
  webExternalBrowserAct_->setIcon(QIcon(":/images/openBrowser"));
  webToolBar_->addAction(webExternalBrowserAct_);

  QHBoxLayout *webControlPanelHLayout = new QHBoxLayout();
  webControlPanelHLayout->setMargin(2);
  webControlPanelHLayout->setSpacing(2);
  webControlPanelHLayout->addWidget(webToolBar_);
  webControlPanelHLayout->addStretch(1);

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

  if (type_ != TAB_WEB)
    setWebToolbarVisible(false, false);
  else
    setWebToolbarVisible(true, false);

  //! Create web panel
  webPanelTitleLabel_ = new QLabel(this);
  webPanelAuthorLabel_ = new QLabel(this);

  webPanelAuthor_ = new QLabel(this);
  webPanelAuthor_->setObjectName("webPanelAuthor_");

  webPanelTitle_ = new QLabel(this);
  webPanelTitle_->setObjectName("webPanelTitle_");

  QGridLayout *webPanelLayout1 = new QGridLayout();
  webPanelLayout1->setMargin(5);
  webPanelLayout1->setSpacing(5);
  webPanelLayout1->setColumnStretch(1, 1);
  webPanelLayout1->addWidget(webPanelTitleLabel_, 0, 0);
  webPanelLayout1->addWidget(webPanelTitle_, 0, 1);
  webPanelLayout1->addWidget(webPanelAuthorLabel_, 1, 0);
  webPanelLayout1->addWidget(webPanelAuthor_, 1, 1);

  QFrame *webPanelLine = new QFrame(this);
  webPanelLine->setFrameStyle(QFrame::HLine | QFrame::Sunken);

  QVBoxLayout *webPanelLayout = new QVBoxLayout();
  webPanelLayout->setMargin(0);
  webPanelLayout->setSpacing(0);
  webPanelLayout->addLayout(webPanelLayout1);
  webPanelLayout->addWidget(webPanelLine);

  webPanel_ = new QWidget(this);
  webPanel_->setObjectName("webPanel_");
  webPanel_->setLayout(webPanelLayout);
  webPanel_->setVisible(false);

  //! Create web layout
  QVBoxLayout *webLayout = new QVBoxLayout();
  webLayout->setMargin(0);
  webLayout->setSpacing(0);
  webLayout->addWidget(webPanel_);
  webLayout->addWidget(webControlPanel_);
  webLayout->addWidget(webView_, 1);
  webLayout->addWidget(webViewProgress_);

  webWidget_ = new QWidget(this);
  webWidget_->setObjectName("webWidget_");
  webWidget_->setLayout(webLayout);
  webWidget_->setMinimumWidth(400);

  webView_->page()->action(QWebPage::OpenLinkInNewWindow)->disconnect();

  urlExternalBrowserAct_ = new QAction(this);
  urlExternalBrowserAct_->setIcon(QIcon(":/images/openBrowser"));

  webPanelTitle_->installEventFilter(this);

  if (type_ != TAB_WEB) {
    QSqlQuery q(*db_);
    q.exec(QString("SELECT displayEmbeddedImages FROM feeds WHERE id=='%1'").
           arg(feedId_));
    if (q.next()) autoLoadImages_ = q.value(0).toInt();
  }

  webDefaultFontSize_ =
      webView_->settings()->fontSize(QWebSettings::DefaultFontSize);
  webDefaultFixedFontSize_ =
      webView_->settings()->fontSize(QWebSettings::DefaultFixedFontSize);

  connect(webHomePageAct_, SIGNAL(triggered()),
          this, SLOT(webHomePage()));
  connect(webExternalBrowserAct_, SIGNAL(triggered()),
          this, SLOT(openPageInExternalBrowser()));
  connect(urlExternalBrowserAct_, SIGNAL(triggered()),
          this, SLOT(openUrlInExternalBrowser()));
  connect(webPanelAuthor_, SIGNAL(linkActivated(QString)),
          this, SLOT(slotWebTitleLinkClicked(QString)));
  connect(webPanelTitle_, SIGNAL(linkActivated(QString)),
          this, SLOT(slotWebTitleLinkClicked(QString)));
  connect(this, SIGNAL(signalWebViewSetContent(QString)),
                SLOT(slotWebViewSetContent(QString)), Qt::QueuedConnection);
  connect(webView_, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
  connect(webView_, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished(bool)));
  connect(webView_, SIGNAL(linkClicked(QUrl)), this, SLOT(slotLinkClicked(QUrl)));
  connect(webView_->page(), SIGNAL(linkHovered(QString,QString,QString)),
          this, SLOT(slotLinkHovered(QString,QString,QString)));
  connect(webView_, SIGNAL(loadProgress(int)), this, SLOT(slotSetValue(int)));

  connect(webView_, SIGNAL(titleChanged(QString)),
          this, SLOT(webTitleChanged(QString)));
  connect(webView_->page()->action(QWebPage::OpenLinkInNewWindow), SIGNAL(triggered()),
          this, SLOT(openLinkInNewTab()));

  connect(webView_, SIGNAL(customContextMenuRequested(QPoint)),
          this, SLOT(showContextWebPage(const QPoint &)));

  connect(rsslisting_->browserToolbarToggle_, SIGNAL(triggered()),
          this, SLOT(setWebToolbarVisible()));
}

/*! \brief Чтение настроек из ini-файла ***************************************/
void NewsTabWidget::setSettings(bool newTab)
{
  if (newTab) {
    if (type_ != TAB_WEB) {
      newsView_->setFont(
            QFont(rsslisting_->newsFontFamily_, rsslisting_->newsFontSize_));
      newsModel_->formatDateTime_ = rsslisting_->formatDateTime_;
    }

    webPanelTitleLabel_->setFont(
          QFont(rsslisting_->panelNewsFontFamily_, rsslisting_->panelNewsFontSize_));
    webPanelTitle_->setFont(
          QFont(rsslisting_->panelNewsFontFamily_, rsslisting_->panelNewsFontSize_));
    webPanelAuthorLabel_->setFont(
          QFont(rsslisting_->panelNewsFontFamily_, rsslisting_->panelNewsFontSize_));
    webPanelAuthor_->setFont(
          QFont(rsslisting_->panelNewsFontFamily_, rsslisting_->panelNewsFontSize_));

    webView_->settings()->setFontFamily(
          QWebSettings::StandardFont, rsslisting_->webFontFamily_);

    if ((!webView_->url().isValid() ||
        (webView_->url().toString() == "about:blank")) && (type_ != TAB_WEB)) {
      webView_->settings()->setFontSize(
            QWebSettings::DefaultFontSize, rsslisting_->webFontSize_);
    }

    if (!rsslisting_->externalBrowserOn_) {
      webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    } else {
      webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    }
    webView_->settings()->setAttribute(
          QWebSettings::JavascriptEnabled, rsslisting_->javaScriptEnable_);
    webView_->settings()->setAttribute(
          QWebSettings::PluginsEnabled, rsslisting_->pluginsEnable_);
  } else if (type_ != TAB_WEB) {
    QSqlQuery q(*db_);
    q.exec(QString("SELECT displayEmbeddedImages FROM feeds WHERE id=='%1'").
           arg(feedId_));
    if (q.next()) autoLoadImages_ = q.value(0).toInt();
  }

  if (type_ != TAB_WEB)
    rsslisting_->slotUpdateStatus(feedId_, false);

  rsslisting_->autoLoadImages_ = !autoLoadImages_;
  rsslisting_->setAutoLoadImages(false);
  webView_->settings()->setAttribute(
        QWebSettings::AutoLoadImages, autoLoadImages_);

  if (type_ != TAB_WEB) {
    newsToolBar_->actions().at(4)->setVisible(type_ != TAB_CAT_DEL);
    newsToolBar_->actions().at(5)->setVisible(type_ != TAB_CAT_DEL);
    newsToolBar_->actions().at(6)->setVisible(type_ == TAB_FEED);
    newsToolBar_->actions().at(7)->setVisible(type_ == TAB_FEED);
    newsToolBar_->actions().at(8)->setVisible(type_ == TAB_CAT_DEL);
    newsToolBar_->actions().at(9)->setVisible(type_ == TAB_CAT_DEL);

    if (type_ == TAB_CAT_DEL) {
      rsslisting_->deleteNewsAct_->setEnabled(false);
      rsslisting_->deleteAllNewsAct_->setEnabled(false);
      setVisibleAction(true);
    } else {
      rsslisting_->deleteNewsAct_->setEnabled(true);
      rsslisting_->deleteAllNewsAct_->setEnabled(true);
    }

    rsslisting_->newsFilter_->setEnabled(type_ == TAB_FEED);
  }
}

//! Перезагрузка перевода
void NewsTabWidget::retranslateStrings() {
  webViewProgress_->setFormat(tr("Loading... (%p%)"));
  webPanelTitleLabel_->setText(tr("Title:"));
  webPanelAuthorLabel_->setText(tr("Author:"));

  webHomePageAct_->setText(tr("Home"));
  webExternalBrowserAct_->setText(tr("Open Page in External Browser"));
  urlExternalBrowserAct_->setText(tr("Open Link in External Browser"));

  webView_->page()->action(QWebPage::OpenLink)->setText(tr("Open Link"));
  webView_->page()->action(QWebPage::OpenLinkInNewWindow)->setText(tr("Open in New Tab"));
  webView_->page()->action(QWebPage::DownloadLinkToDisk)->setText(tr("Save Link..."));
  webView_->page()->action(QWebPage::CopyLinkToClipboard)->setText(tr("Copy Link"));
  webView_->page()->action(QWebPage::Copy)->setText(tr("Copy"));
  webView_->page()->action(QWebPage::Back)->setText(tr("Go Back"));
  webView_->page()->action(QWebPage::Forward)->setText(tr("Go Forward"));
  webView_->page()->action(QWebPage::Stop)->setText(tr("Stop"));
  webView_->page()->action(QWebPage::Reload)->setText(tr("Reload"));

  if (type_ != TAB_WEB) {
    findText_->retranslateStrings();
    newsHeader_->retranslateStrings();
  }

  closeButton_->setToolTip(tr("Close Tab"));
}

bool NewsTabWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == webPanelTitle_) {
    if (event->type() == QEvent::MouseButtonRelease) {
      QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
      if (mouseEvent->button() & Qt::MiddleButton) {
        webView_->midButtonClick = true;
        QMouseEvent* pe =
            new QMouseEvent(QEvent::MouseButtonRelease, mouseEvent->pos(),
                            Qt::LeftButton, Qt::LeftButton,
                            Qt::ControlModifier);
        QApplication::sendEvent(webPanelTitle_, pe);
      }
    }
    return false;
  } else {
    return QWidget::eventFilter(obj, event);
  }
}

/*! \brief Обработка нажатия в дереве новостей ********************************/
void NewsTabWidget::slotNewsViewClicked(QModelIndex index)
{
  if (QApplication::keyboardModifiers() == Qt::NoModifier) {
    slotNewsViewSelected(index);
  }
}

void NewsTabWidget::slotNewsViewSelected(QModelIndex index, bool clicked)
{
  QElapsedTimer timer;
  timer.start();
  qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

  int newsId = newsModel_->index(index.row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

  if (rsslisting_->markNewsReadOn_ && rsslisting_->markPrevNewsRead_ &&
      (newsId != currentNewsIdOld)) {
    QModelIndex startIndex = newsModel_->index(0, newsModel_->fieldIndex("id"));
    QModelIndexList indexList = newsModel_->match(startIndex, Qt::EditRole, currentNewsIdOld);
    if (!indexList.isEmpty()) {
      slotSetItemRead(indexList.first(), 1);
    }
  }

  if (!index.isValid()) {
    hideWebContent();
    currentNewsIdOld = newsId;
    qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed() << "(invalid index)";
    return;
  }

  if (!((newsId == currentNewsIdOld) &&
        newsModel_->index(index.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() >= 1) ||
        clicked) {
    qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

    markNewsReadTimer_->stop();
    if (rsslisting_->markNewsReadOn_ && rsslisting_->markCurNewsRead_) {
      if (rsslisting_->markNewsReadTime_ == 0) {
        slotSetItemRead(newsView_->currentIndex(), 1);
      } else {
        markNewsReadTimer_->start(rsslisting_->markNewsReadTime_*1000);
      }
    }

    // Запись текущей новости в ленту
    QSqlQuery q(*db_);
    QString qStr = QString("UPDATE feeds SET currentNews='%1' WHERE id=='%2'").
        arg(newsId).arg(feedId_);
    q.exec(qStr);

    qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

    QModelIndex feedIndex = feedsTreeModel_->getIndexById(feedId_, feedParId_);
    feedsTreeModel_->setData(
          feedIndex.sibling(feedIndex.row(), feedsTreeModel_->proxyColumnByOriginal("currentNews")),
          newsId);

    qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

    QWebSettings::globalSettings()->clearMemoryCaches();

    qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

    updateWebView(index);

    qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();
  }
  currentNewsIdOld = newsId;
  qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();
}

//! Двойной клик в списке новостей
void NewsTabWidget::slotNewsViewDoubleClicked(QModelIndex index)
{
  if (!index.isValid()) return;

  QString linkString = newsModel_->record(
        index.row()).field("link_href").value().toString();
  if (linkString.isEmpty())
    linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();

  slotLinkClicked(linkString.simplified());
}

void NewsTabWidget::slotNewsMiddleClicked(QModelIndex index)
{
  if (!index.isValid()) return;

  QString linkString = newsModel_->record(
        index.row()).field("link_href").value().toString();
  if (linkString.isEmpty())
    linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();

  webView_->midButtonClick = true;
  slotLinkClicked(linkString.simplified());
}

/*! \brief Обработка клавиш Up/Down в дереве новостей *************************/
void NewsTabWidget::slotNewsUpPressed()
{
  if (type_ == TAB_WEB) return;

  if (!newsView_->currentIndex().isValid()) {
    if (newsModel_->rowCount() > 0) {
      newsView_->setCurrentIndex(newsModel_->index(0, newsModel_->fieldIndex("title")));
      slotNewsViewClicked(newsModel_->index(0, newsModel_->fieldIndex("title")));
    }
    return;
  }

  int row = newsView_->currentIndex().row();
  if (row == 0) return;
  else row--;

  int value = newsView_->verticalScrollBar()->value();
  int pageStep = newsView_->verticalScrollBar()->pageStep();
  if (row < (value + pageStep/2))
    newsView_->verticalScrollBar()->setValue(value-1);

  newsView_->setCurrentIndex(newsModel_->index(row, newsModel_->fieldIndex("title")));
  slotNewsViewClicked(newsModel_->index(row, newsModel_->fieldIndex("title")));
}

void NewsTabWidget::slotNewsDownPressed()
{
  if (type_ == TAB_WEB) return;

  if (!newsView_->currentIndex().isValid()) {
    if (newsModel_->rowCount() > 0) {
      newsView_->setCurrentIndex(newsModel_->index(0, newsModel_->fieldIndex("title")));
      slotNewsViewClicked(newsModel_->index(0, newsModel_->fieldIndex("title")));
    }
    return;
  }

  int row = newsView_->currentIndex().row();
  if ((row+1) == newsModel_->rowCount()) return;
  else row++;

  int value = newsView_->verticalScrollBar()->value();
  int pageStep = newsView_->verticalScrollBar()->pageStep();
  if (row > (value + pageStep/2))
    newsView_->verticalScrollBar()->setValue(value+1);

  newsView_->setCurrentIndex(newsModel_->index(row, newsModel_->fieldIndex("title")));
  slotNewsViewClicked(newsModel_->index(row, newsModel_->fieldIndex("title")));
}

/*! \brief Обработка клавиш Home/End в дереве новостей *************************/
void NewsTabWidget::slotNewsHomePressed()
{
  int row = 0;
  newsView_->setCurrentIndex(newsModel_->index(row, newsModel_->fieldIndex("title")));
  slotNewsViewClicked(newsModel_->index(row, newsModel_->fieldIndex("title")));
}

void NewsTabWidget::slotNewsEndPressed()
{
  int row = newsModel_->rowCount() - 1;
  newsView_->setCurrentIndex(newsModel_->index(row, newsModel_->fieldIndex("title")));
  slotNewsViewClicked(newsModel_->index(row, newsModel_->fieldIndex("title")));
}

/*! \brief Обработка клавиш PageUp/PageDown в дереве новостей *************************/
void NewsTabWidget::slotNewsPageUpPressed()
{
  if (!newsView_->currentIndex().isValid()) {
    if (newsModel_->rowCount() > 0) {
      newsView_->setCurrentIndex(newsModel_->index(0, newsModel_->fieldIndex("title")));
      slotNewsViewClicked(newsModel_->index(0, newsModel_->fieldIndex("title")));
    }
    return;
  }

  int row = newsView_->currentIndex().row() - newsView_->verticalScrollBar()->pageStep();
  if (row < 0) row = 0;
  newsView_->setCurrentIndex(newsModel_->index(row, newsModel_->fieldIndex("title")));
  slotNewsViewClicked(newsModel_->index(row, newsModel_->fieldIndex("title")));
}

void NewsTabWidget::slotNewsPageDownPressed()
{
  if (!newsView_->currentIndex().isValid()) {
    if (newsModel_->rowCount() > 0) {
      newsView_->setCurrentIndex(newsModel_->index(0, newsModel_->fieldIndex("title")));
      slotNewsViewClicked(newsModel_->index(0, newsModel_->fieldIndex("title")));
    }
    return;
  }

  int row = newsView_->currentIndex().row() + newsView_->verticalScrollBar()->pageStep();
  if ((row+1) > newsModel_->rowCount()) row = newsModel_->rowCount()-1;
  newsView_->setCurrentIndex(newsModel_->index(row, newsModel_->fieldIndex("title")));
  slotNewsViewClicked(newsModel_->index(row, newsModel_->fieldIndex("title")));
}

//! Пометка новости прочитанной
void NewsTabWidget::slotSetItemRead(QModelIndex index, int read)
{
  if (!index.isValid() || (newsModel_->rowCount() == 0)) return;

  bool changed = false;
  int newsId = newsModel_->index(index.row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();
  QSqlQuery q(*db_);

  if (read == 1) {
    if (newsModel_->index(index.row(), newsModel_->fieldIndex("new")).data(Qt::EditRole).toInt() == 1) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("new")),
            0);
      q.exec(QString("UPDATE news SET new=0 WHERE id=='%1'").arg(newsId));
    }
    if (newsModel_->index(index.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() == 0) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
            1);
      q.exec(QString("UPDATE news SET read=1 WHERE id=='%1'").arg(newsId));
      changed = true;
    }
  } else {
    if (newsModel_->index(index.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() != 0) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
            0);
      q.exec(QString("UPDATE news SET read=0 WHERE id=='%1'").arg(newsId));
      changed = true;
    }
  }

  if (changed) {
    newsView_->viewport()->update();
    rsslisting_->slotUpdateStatus(feedId_);
  }
}

//! Пометка новости звездочкой (избранная)
void NewsTabWidget::slotSetItemStar(QModelIndex index, int starred)
{
  if (!index.isValid()) return;

  newsModel_->setData(index, starred);

  int newsId = newsModel_->index(index.row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();
  QSqlQuery q(*db_);
  q.exec(QString("UPDATE news SET starred='%1' WHERE id=='%2'").
         arg(starred).arg(newsId));
}

void NewsTabWidget::slotReadTimer()
{
  markNewsReadTimer_->stop();
  slotSetItemRead(newsView_->currentIndex(), 1);
}

//! Отметить выделенные новости прочитанными
void NewsTabWidget::markNewsRead()
{
  if (type_ == TAB_WEB) return;

  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  int cnt = indexes.count();
  if (cnt == 0) return;

  if (cnt == 1) {
    curIndex = indexes.at(0);
    if (newsModel_->index(curIndex.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() == 0) {
      slotSetItemRead(curIndex, 1);
    } else {
      slotSetItemRead(curIndex, 0);
    }
  } else {
    bool markRead = false;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      if (newsModel_->index(curIndex.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() == 0) {
        markRead = true;
        break;
      }
    }

    db_->transaction();
    QSqlQuery q(*db_);
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      int newsId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();
      q.exec(QString("UPDATE news SET new=0, read='%1' WHERE id=='%2'").
             arg(markRead).arg(newsId));
    }
    db_->commit();

    newsModel_->select();

    while (newsModel_->canFetchMore())
      newsModel_->fetchMore();

    int newsIdCur =
        feedsTreeModel_->dataField(feedsTreeView_->currentIndex(), "currentNews").toInt();
    QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("id"));
    QModelIndexList indexList = newsModel_->match(index, Qt::EditRole, newsIdCur);
    if (indexList.count()) {
      int newsRow = indexList.first().row();
      newsView_->setCurrentIndex(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
    }
    rsslisting_->slotUpdateStatus(feedId_);
  }
}

//! Отметить все новости в ленте прочитанными
void NewsTabWidget::markAllNewsRead()
{
  if (type_ == TAB_WEB) return;
  if (newsModel_->rowCount() == 0) return;

  QSqlQuery q(*db_);
  QString qStr = QString("UPDATE news SET read=1 WHERE feedId='%1' AND read=0").
      arg(feedId_);
  q.exec(qStr);
  qStr = QString("UPDATE news SET new=0 WHERE feedId='%1' AND new=1").
      arg(feedId_);
  q.exec(qStr);

  int currentRow = newsView_->currentIndex().row();

  newsModel_->select();

  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  newsView_->setCurrentIndex(newsModel_->index(currentRow, newsModel_->fieldIndex("title")));
  rsslisting_->slotUpdateStatus(feedId_);
}

//! Пометка выбранных новостей звездочкой (избранные)
void NewsTabWidget::markNewsStar()
{
  if (type_ == TAB_WEB) return;

  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(
        newsModel_->fieldIndex("starred"));

  int cnt = indexes.count();
  if (cnt == 0) return;

  if (cnt == 1) {
    curIndex = indexes.at(0);
    if (curIndex.data(Qt::EditRole).toInt() == 0) {
      slotSetItemStar(curIndex, 1);
    } else {
      slotSetItemStar(curIndex, 0);
    }
  } else {
    bool markStar = false;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      if (curIndex.data(Qt::EditRole).toInt() == 0) {
        markStar = true;
        break;
      }
    }

    db_->transaction();
    for (int i = cnt-1; i >= 0; --i) {
      slotSetItemStar(indexes.at(i), markStar);
    }
    db_->commit();
  }
}

//! Удаление новости
void NewsTabWidget::deleteNews()
{
  if (type_ == TAB_WEB) return;

  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(newsModel_->fieldIndex("deleted"));

  int cnt = indexes.count();
  if (cnt == 0) return;

  if (cnt == 1) {
    curIndex = indexes.at(0);
    slotSetItemRead(curIndex, 1);
    newsModel_->setData(curIndex, 1);
    newsModel_->submitAll();
  } else {
    db_->transaction();
    QSqlQuery q(*db_);
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      int newsId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();
      q.exec(QString("UPDATE news SET new=0, read=2, deleted=1 WHERE id=='%1'").
             arg(newsId));
    }
    db_->commit();

    newsModel_->select();
  }
  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  if (curIndex.row() == newsModel_->rowCount())
    curIndex = newsModel_->index(curIndex.row()-1, newsModel_->fieldIndex("title"));
  else if (curIndex.row() > newsModel_->rowCount())
    curIndex = newsModel_->index(newsModel_->rowCount()-1, newsModel_->fieldIndex("title"));
  else curIndex = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("title"));
  newsView_->setCurrentIndex(curIndex);
  slotNewsViewSelected(curIndex);
  rsslisting_->slotUpdateStatus(feedId_);
}

//! Удаление всех новостей из списка
void NewsTabWidget::deleteAllNewsList()
{
  if (type_ == TAB_WEB) return;

  QModelIndex curIndex;

  db_->transaction();
  QSqlQuery q(*db_);
  for (int i = newsModel_->rowCount()-1; i >= 0; --i) {
    int newsId = newsModel_->index(i, newsModel_->fieldIndex("id")).data().toInt();
    q.exec(QString("UPDATE news SET new=0, read=2, deleted=1 WHERE id=='%1'").
           arg(newsId));
  }
  db_->commit();

  newsModel_->select();

  slotNewsViewSelected(curIndex);
  rsslisting_->slotUpdateStatus(feedId_);
}

//! Восстановление новостей
void NewsTabWidget::restoreNews()
{
  if (type_ == TAB_WEB) return;

  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(newsModel_->fieldIndex("deleted"));

  int cnt = indexes.count();
  if (cnt == 0) return;

  if (cnt == 1) {
    curIndex = indexes.at(0);
    newsModel_->setData(curIndex, 0);
    newsModel_->submitAll();
  } else {
    db_->transaction();
    QSqlQuery q(*db_);
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      int newsId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();
      q.exec(QString("UPDATE news SET deleted=0 WHERE id=='%1'").
             arg(newsId));
    }
    db_->commit();

    newsModel_->select();
  }

  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  if (curIndex.row() == newsModel_->rowCount())
    curIndex = newsModel_->index(curIndex.row()-1, newsModel_->fieldIndex("title"));
  else if (curIndex.row() > newsModel_->rowCount())
    curIndex = newsModel_->index(newsModel_->rowCount()-1, newsModel_->fieldIndex("title"));
  else curIndex = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("title"));
  newsView_->setCurrentIndex(curIndex);
  slotNewsViewSelected(curIndex);
  rsslisting_->slotUpdateStatus(feedId_);
}

//! Сортировка новостей по "star" или по "read"
void NewsTabWidget::slotSort(int column, int order)
{
  QString qStr;
  if (column == newsModel_->fieldIndex("read")) {
    qStr = QString("UPDATE news SET rights=read WHERE feedId=='%1'").arg(feedId_);
  }
  else if (column == newsModel_->fieldIndex("starred")) {
    qStr = QString("UPDATE news SET rights=starred WHERE feedId=='%1'").arg(feedId_);
  }
  QSqlQuery q(*db_);
  q.exec(qStr);
  newsModel_->sort(newsModel_->fieldIndex("rights"), (Qt::SortOrder)order);
}

//! Загрузка/обновление содержимого браузера
void NewsTabWidget::updateWebView(QModelIndex index)
{
  if (!index.isValid()) {
    hideWebContent();
    return;
  }

  webPanel_->show();

  QString titleStr, panelTitleStr;
  titleString_ = newsModel_->record(index.row()).field("title").value().toString();
  titleStr = webPanelTitle_->fontMetrics().elidedText(
        titleString_, Qt::ElideRight, webPanelTitle_->width());
  linkString_ = newsModel_->record(index.row()).field("link_href").value().toString();
  if (linkString_.isEmpty())
    linkString_ = newsModel_->record(index.row()).field("link_alternate").value().toString();
  panelTitleStr = QString("<a href='%1'>%2</a>").arg(linkString_).arg(titleStr);
  webPanelTitle_->setText(panelTitleStr);

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
    QModelIndex currentIndex = feedsTreeView_->currentIndex();
    authorName  = feedsTreeModel_->dataField(currentIndex, "author_name").toString();
    authorEmail = feedsTreeModel_->dataField(currentIndex, "author_email").toString();
    authorUri   = feedsTreeModel_->dataField(currentIndex, "author_uri").toString();

    //    qDebug() << "author_feed:" << authorName << authorEmail << authorUri;
    authorString = authorName;
    if (!authorEmail.isEmpty()) authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
    if (!authorUri.isEmpty())   authorString.append(QString(" <a href='%1'>page</a>").arg(authorUri));
  }

  webPanelAuthor_->setText(authorString);
  webPanelAuthorLabel_->setVisible(!authorString.isEmpty());
  webPanelAuthor_->setVisible(!authorString.isEmpty());
  setWebToolbarVisible(false, false);

  bool showDescriptionNews_ = rsslisting_->showDescriptionNews_;

  QVariant displayNews =
      feedsTreeModel_->dataField(feedsTreeView_->currentIndex(), "displayNews");
  if (!displayNews.toString().isEmpty())
    showDescriptionNews_ = !displayNews.toInt();

  if (!showDescriptionNews_) {
    QString linkString = newsModel_->record(
          index.row()).field("link_href").value().toString();
    if (linkString.isEmpty())
      linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();

    webPanel_->hide();
    setWebToolbarVisible(true, false);

    webView_->load(QUrl(linkString.simplified()));
  } else {
    QString content = newsModel_->record(index.row()).field("content").value().toString();
    if (content.isEmpty()) {
      content = newsModel_->record(index.row()).field("description").value().toString();
      emit signalWebViewSetContent(content);
    } else {
      emit signalWebViewSetContent(content);
    }
  }
}

void NewsTabWidget::slotLinkClicked(QUrl url)
{
  if (url.host().isEmpty()) {
    QModelIndex currentIndex = feedsTreeView_->currentIndex();
    QUrl hostUrl = feedsTreeModel_->dataField(currentIndex, "htmlUrl").toString();

    url.setScheme(hostUrl.scheme());
    url.setHost(hostUrl.host());
  }
  if (!rsslisting_->externalBrowserOn_) {
    if (!webView_->midButtonClick) {
      if (!webControlPanel_->isVisible()) {
        webView_->history()->clear();
        webPanel_->hide();
        setWebToolbarVisible(true, false);
      }
      webView_->load(url);
    } else {
      QWebPage *webPage = rsslisting_->createWebTab();
      qobject_cast<WebView*>(webPage->view())->load(url);
    }
  } else openUrl(url);
  webView_->midButtonClick = false;
}

void NewsTabWidget::slotLinkHovered(const QString &link, const QString &, const QString &)
{
  rsslisting_->statusBar()->showMessage(link, 3000);
}

void NewsTabWidget::slotSetValue(int value)
{
  webViewProgress_->setValue(value);
  QString str = QString(" %1 kB / %2 kB").
      arg(webView_->page()->bytesReceived()/1000).
      arg(webView_->page()->totalBytes()/1000);
  webViewProgressLabel_->setText(str);
}

void NewsTabWidget::slotLoadStarted()
{
  if (type_ == TAB_WEB) {
    newsIconTitle_->setMovie(newsIconMovie_);
    newsIconMovie_->start();
  }

  webViewProgress_->setValue(0);
  webViewProgress_->show();
}

void NewsTabWidget::slotLoadFinished(bool)
{
  if (type_ == TAB_WEB) {
    newsIconMovie_->stop();
    QPixmap iconTab;
    iconTab.load(":/images/webPage");
    newsIconTitle_->setPixmap(iconTab);
  }

  webViewProgress_->hide();
  if ((!webView_->url().isValid() ||
      (webView_->url().toString() == "about:blank")) && (type_ != TAB_WEB)) {
    webView_->settings()->setFontSize(
          QWebSettings::DefaultFontSize, rsslisting_->webFontSize_);
  } else {
    webView_->settings()->setFontSize(
          QWebSettings::DefaultFontSize, webDefaultFontSize_);
    webView_->settings()->setFontSize(
          QWebSettings::DefaultFixedFontSize, webDefaultFixedFontSize_);
  }
}

//! Слот для асинхронного обновления новости
void NewsTabWidget::slotWebViewSetContent(QString content)
{
  webView_->history()->setMaximumItemCount(0);
  webView_->setHtml(content);
  webView_->history()->setMaximumItemCount(100);
}

void NewsTabWidget::slotWebTitleLinkClicked(QString urlStr)
{
  slotLinkClicked(urlStr.simplified());
}

//! Переход на краткое содержание новости
void NewsTabWidget::webHomePage()
{
  if (type_ != TAB_WEB) {
    updateWebView(newsView_->currentIndex());
  } else {
    webView_->history()->goToItem(webView_->history()->itemAt(0));
  }
}

//! Открытие отображаемой страницы во внешнем браузере
void NewsTabWidget::openPageInExternalBrowser()
{
  openUrl(webView_->url());
}

//! Открытие новости в браузере
void NewsTabWidget::openInBrowserNews()
{
  slotNewsViewDoubleClicked(newsView_->currentIndex());
}

//! Открытие новости во внешнем браузере
void NewsTabWidget::openInExternalBrowserNews()
{
  QModelIndex index = newsView_->currentIndex();

  if (!index.isValid()) return;

  QString linkString = newsModel_->record(
        index.row()).field("link_href").value().toString();
  if (linkString.isEmpty())
    linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();

  openUrl(linkString.simplified());
}

//! Установка позиции браузера
void NewsTabWidget::setBrowserPosition()
{
  int idx = newsTabWidgetSplitter_->indexOf(webWidget_);

  switch (rsslisting_->browserPosition_) {
  case TOP_POSITION: case LEFT_POSITION:
    newsTabWidgetSplitter_->insertWidget(0, newsTabWidgetSplitter_->widget(idx));
    break;
  case BOTTOM_POSITION: case RIGHT_POSITION:
    newsTabWidgetSplitter_->insertWidget(1, newsTabWidgetSplitter_->widget(idx));
    break;
  default:
    newsTabWidgetSplitter_->insertWidget(1, newsTabWidgetSplitter_->widget(idx));
  }

  switch (rsslisting_->browserPosition_) {
  case TOP_POSITION: case BOTTOM_POSITION:
    newsTabWidgetSplitter_->setOrientation(Qt::Vertical);
    break;
  case RIGHT_POSITION: case LEFT_POSITION:
    newsTabWidgetSplitter_->setOrientation(Qt::Horizontal);
    break;
  default:
    newsTabWidgetSplitter_->setOrientation(Qt::Vertical);
  }
}

//! Закрытие вкладки по кнопке х
void NewsTabWidget::slotTabClose()
{
  rsslisting_->slotTabCloseRequested(rsslisting_->tabWidget_->indexOf(this));
}

//! Вывод на вкладке названия открытой странички браузера
void NewsTabWidget::webTitleChanged(QString title)
{
  if ((type_ == TAB_WEB) && !title.isEmpty()) {
    QString tabText = title;
    newsTitleLabel_->setToolTip(tabText);
    tabText = newsTextTitle_->fontMetrics().elidedText(
          tabText, Qt::ElideRight, 114);
    newsTextTitle_->setText(tabText);
  }
}

//! Открытие новости в новой вкладке
void NewsTabWidget::openNewsNewTab()
{
  slotNewsMiddleClicked(newsView_->currentIndex());
}

//! Открытие ссылки в новой вкладке
void NewsTabWidget::openLinkInNewTab()
{
  webView_->midButtonClick = true;
  slotLinkClicked(linkUrl_);
}

inline static bool launch(const QUrl &url, const QString &client)
{
  return (QProcess::startDetached(client + QLatin1Char(' ') + url.toString().toUtf8()));
}

//! Открытие ссылки во внешем браузере
bool NewsTabWidget::openUrl(const QUrl &url)
{
  if (!url.isValid())
    return false;

  if (url.scheme() == QLatin1String("mailto"))
      return QDesktopServices::openUrl(url);

  rsslisting_->openingLink_ = true;
  if (rsslisting_->externalBrowserOn_ == 2) {
#if defined(Q_WS_WIN)
    quintptr returnValue = (quintptr)ShellExecute(
          rsslisting_->winId(), 0,
          (wchar_t *)QString::fromUtf8(rsslisting_->externalBrowser_.toUtf8()).utf16(),
          (wchar_t *)QString::fromUtf8(url.toEncoded().constData()).utf16(),
          0, SW_SHOWNORMAL);
    if (returnValue > 32)
      return true;
#elif defined(Q_WS_X11)
    if (launch(url, rsslisting_->externalBrowser_.toUtf8()))
      return true;
#endif
  }
  return QDesktopServices::openUrl(url);
}

void NewsTabWidget::slotFindText(const QString &text)
{
  if (findText_->findGroup_->checkedAction()->objectName() == "findInBrowserAct") {
    webView_->findText("", QWebPage::HighlightAllOccurrences);
    webView_->findText(text, QWebPage::HighlightAllOccurrences);
  } else {
    int newsId = newsModel_->index(
          newsView_->currentIndex().row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

    QString filterStr = rsslisting_->newsFilterStr +
        QString(" AND (title LIKE '\%%1\%' OR author_name LIKE '\%%1\%' OR category LIKE '\%%1\%')").
        arg(text);

    newsModel_->setFilter(filterStr);   

    QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("id"));
    QModelIndexList indexList = newsModel_->match(index, Qt::EditRole, newsId);
    if (indexList.count()) {
      int newsRow = indexList.first().row();
      newsView_->setCurrentIndex(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
    } else {
      currentNewsIdOld = newsId;
      hideWebContent();
    }
  }
}

void NewsTabWidget::slotSelectFind()
{
  if (findText_->findGroup_->checkedAction()->objectName() == "findInNewsAct")
    webView_->findText("", QWebPage::HighlightAllOccurrences);
  else {
    QModelIndex index = newsView_->currentIndex();
    newsModel_->setFilter(rsslisting_->newsFilterStr);
    newsView_->setCurrentIndex(index);
  }
  slotFindText(findText_->text());
}

void NewsTabWidget::hideWebContent()
{
  emit signalWebViewSetContent("");
  webPanel_->hide();
  setWebToolbarVisible(false, false);
}

void NewsTabWidget::showContextWebPage(const QPoint &p)
{
  if (webView_->rightButtonClick) {
    webView_->rightButtonClick = false;
    return;
  }

  webMenu_->clear();
  QMenu *menu_t = webView_->page()->createStandardContextMenu();
  webMenu_->addActions(menu_t->actions());

  const QWebHitTestResult &hitTest = webView_->page()->mainFrame()->hitTestContent(p);
  if (!hitTest.linkUrl().isEmpty() && hitTest.linkUrl().scheme() != "javascript") {
    linkUrl_ = hitTest.linkUrl();
    if (!rsslisting_->externalBrowserOn_) {
      webMenu_->addSeparator();
      webMenu_->addAction(urlExternalBrowserAct_);
    }
  } else if (menu_t->actions().indexOf(webView_->pageAction(QWebPage::Reload)) >= 0) {
    webMenu_->addSeparator();
    webMenu_->addAction(rsslisting_->autoLoadImagesToggle_);
    webMenu_->addSeparator();
    webMenu_->addAction(rsslisting_->printAct_);
    webMenu_->addAction(rsslisting_->printPreviewAct_);
  }

  webMenu_->popup(webView_->mapToGlobal(p));
}

//! Открытие ссылки во внешнем браузере
void NewsTabWidget::openUrlInExternalBrowser()
{
  if (linkUrl_.host().isEmpty()) {
    QModelIndex currentIndex = feedsTreeView_->currentIndex();
    QUrl hostUrl = feedsTreeModel_->dataField(currentIndex, "htmlUrl").toString();

    linkUrl_.setScheme(hostUrl.scheme());
    linkUrl_.setHost(hostUrl.host());
  }
  openUrl(linkUrl_);
}

void NewsTabWidget::setVisibleAction(bool show)
{
  newsContextMenu_->actions().at(0)->setVisible(show);
  newsContextMenu_->actions().at(1)->setVisible(show);
}

void NewsTabWidget::setWebToolbarVisible(bool show, bool checked)
{
  if (!checked) webToolbarShow_ = show;
  webControlPanel_->setVisible(webToolbarShow_ &
                               rsslisting_->browserToolbarToggle_->isChecked());
}
