#include "newstabwidget.h"
#include "rsslisting.h"

RSSListing *rsslisting_;

NewsTabWidget::NewsTabWidget(int feedId, QWidget *parent)
  : QWidget(parent),
    feedId_(feedId)
{
  rsslisting_ = qobject_cast<RSSListing*>(parent);
  feedsView_ = rsslisting_->feedsView_;
  feedsModel_ = rsslisting_->feedsModel_;

  currentNewsIdOld = -1;
  currentFeedIdOld = -1;

  newsIconTitle_ = new QLabel(this);
  newsTextTitle_ = new QLabel(this);

  QHBoxLayout *newsTitleLayout = new QHBoxLayout();
  newsTitleLayout->setMargin(0);
  newsTitleLayout->setSpacing(3);
  newsTitleLayout->addWidget(newsIconTitle_);
  newsTitleLayout->addWidget(newsTextTitle_, 1);

  newsTitleLabel_ = new QWidget(this);
  newsTitleLabel_->setStyleSheet("min-height: 16px;");
  newsTitleLabel_->setFixedWidth(132);
  newsTitleLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
  newsTitleLabel_->setLayout(newsTitleLayout);
  newsTitleLabel_->setVisible(false);

  closeButton_ = new QToolButton(this);
  closeButton_->setFixedSize(15, 15);
  closeButton_->setCursor(Qt::ArrowCursor);
  closeButton_->setStyleSheet(
        "QToolButton { border: none; padding: 0px;"
        "image: url(:/images/close); }"
        "QToolButton:hover {"
        "image: url(:/images/closeHover); }"
        );
  closeButton_->setVisible(false);
  connect(closeButton_, SIGNAL(clicked()),
          this, SLOT(slotTabClose()));

  if (feedId_ > -1) {
    createNewsList();
    createMenuNews();
  }
  createWebWidget();

  if (feedId_ > -1) {
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
  if (feedId_ > -1)
    layout->addWidget(newsTabWidgetSplitter_, 0);
  else
    layout->addWidget(webWidget_, 0);
  setLayout(layout);


  if (feedId_ > -1) {
    newsTabWidgetSplitter_->restoreGeometry(
          rsslisting_->settings_->value("NewsTabSplitter").toByteArray());
    newsTabWidgetSplitter_->restoreState(
          rsslisting_->settings_->value("NewsTabSplitter").toByteArray());

    if ((rsslisting_->browserPosition_ == RIGHT_POSITION) ||
        (rsslisting_->browserPosition_ == LEFT_POSITION)) {
      newsTabWidgetSplitter_->setOrientation(Qt::Horizontal);
    } else {
      newsTabWidgetSplitter_->setOrientation(Qt::Vertical);
    }
    newsTabWidgetSplitter_->setHandleWidth(1);
    newsTabWidgetSplitter_->setStyleSheet("QSplitter::handle {background: gray;}");
  }
}

//! Создание новостного списка и всех сопутствующих панелей
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

  newsHeader_->init(rsslisting_->settings_);

  newsToolBar_ = new QToolBar(this);
  newsToolBar_->setStyleSheet("QToolBar { border: none; padding: 0px; }");
  newsToolBar_->setIconSize(QSize(18, 18));
  newsToolBar_->addAction(rsslisting_->newsFilter_);

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

  markNewsReadTimer_ = new QTimer(this);

  QAction *openNewsWebViewAct_ = new QAction(this);
  openNewsWebViewAct_->setShortcut(QKeySequence(Qt::Key_Return));
  rsslisting_->addAction(openNewsWebViewAct_);
  connect(openNewsWebViewAct_, SIGNAL(triggered()),
          this, SLOT(slotOpenNewsWebView()));

  connect(newsView_, SIGNAL(pressed(QModelIndex)),
          this, SLOT(slotNewsViewClicked(QModelIndex)));
  connect(newsView_, SIGNAL(pressKeyUp()), this, SLOT(slotNewsUpPressed()));
  connect(newsView_, SIGNAL(pressKeyDown()), this, SLOT(slotNewsDownPressed()));
  connect(newsView_, SIGNAL(pressKeyHome()), this, SLOT(slotNewsHomePressed()));
  connect(newsView_, SIGNAL(pressKeyEnd()), this, SLOT(slotNewsEndPressed()));
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

  connect(rsslisting_->markNewsRead_, SIGNAL(triggered()),
          this, SLOT(markNewsRead()));
  connect(rsslisting_->markAllNewsRead_, SIGNAL(triggered()),
          this, SLOT(markAllNewsRead()));
  connect(rsslisting_->markStarAct_, SIGNAL(triggered()),
          this, SLOT(markNewsStar()));
  connect(rsslisting_->deleteNewsAct_, SIGNAL(triggered()),
          this, SLOT(deleteNews()));

  connect(rsslisting_->newsKeyUpAct_, SIGNAL(triggered()),
          this, SLOT(slotNewsUpPressed()));
  connect(rsslisting_->newsKeyDownAct_, SIGNAL(triggered()),
          this, SLOT(slotNewsDownPressed()));

  connect(rsslisting_->openInBrowserAct_, SIGNAL(triggered()),
          this, SLOT(openInBrowserNews()));
  connect(rsslisting_->openInExternalBrowserAct_, SIGNAL(triggered()),
          this, SLOT(openInExternalBrowserNews()));
  connect(rsslisting_->openNewsNewTabAct_, SIGNAL(triggered()),
          this, SLOT(slotOpenNewsNewTab()));
}

void NewsTabWidget::createMenuNews()
{
  newsContextMenu_ = new QMenu(this);
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
  webToolBar_->setIconSize(QSize(16, 16));

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

  if (feedId_ > -1)
    webControlPanel_->setVisible(false);

  //! Create web panel
  webPanelTitleLabel_ = new QLabel(this);
  webPanelTitleLabel_->setCursor(Qt::PointingHandCursor);
  webPanelAuthorLabel_ = new QLabel(this);

  webPanelAuthor_ = new QLabel(this);
  webPanelAuthor_->setObjectName("webPanelAuthor_");

  webPanelTitle_ = new QLabel(this);
  webPanelTitle_->setObjectName("webPanelTitle_");

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

  connect(webHomePageAct_, SIGNAL(triggered()),
          this, SLOT(webHomePage()));
  connect(webExternalBrowserAct_, SIGNAL(triggered()),
          this, SLOT(openPageInExternalBrowser()));
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
}

/*! \brief Чтение настроек из ini-файла ***************************************/
void NewsTabWidget::setSettings()
{
  if (feedId_ > -1)
    newsView_->setFont(
          QFont(rsslisting_->newsFontFamily_, rsslisting_->newsFontSize_));

  webView_->settings()->setFontFamily(
        QWebSettings::StandardFont, rsslisting_->webFontFamily_);
  webView_->settings()->setFontSize(
        QWebSettings::DefaultFontSize, rsslisting_->webFontSize_);

  if (rsslisting_->embeddedBrowserOn_) {
    webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
  } else {
    webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
  }
  webView_->settings()->setAttribute(
        QWebSettings::JavascriptEnabled, rsslisting_->javaScriptEnable_);
  webView_->settings()->setAttribute(
        QWebSettings::PluginsEnabled, rsslisting_->pluginsEnable_);

  webView_->settings()->setAttribute(
        QWebSettings::AutoLoadImages, rsslisting_->autoLoadImages_);
}

//! Перезагрузка перевода
void NewsTabWidget::retranslateStrings() {
  webViewProgress_->setFormat(tr("Loading... (%p%)"));
  webPanelTitleLabel_->setText(tr("Title:"));
  webPanelAuthorLabel_->setText(tr("Author:"));

  webHomePageAct_->setText(tr("Home"));
  webExternalBrowserAct_->setText(tr("Open in external browser"));

  webView_->page()->action(QWebPage::OpenLink)->setText(tr("Open Link"));
  webView_->page()->action(QWebPage::OpenLinkInNewWindow)->setText(tr("Open in new tab"));
  webView_->page()->action(QWebPage::DownloadLinkToDisk)->setText(tr("Save Link..."));
  webView_->page()->action(QWebPage::CopyLinkToClipboard)->setText(tr("Copy Link"));
  webView_->page()->action(QWebPage::Copy)->setText(tr("Copy"));
  webView_->page()->action(QWebPage::Back)->setText(tr("Go Back"));
  webView_->page()->action(QWebPage::Forward)->setText(tr("Go Forward"));
  webView_->page()->action(QWebPage::Stop)->setText(tr("Stop"));
  webView_->page()->action(QWebPage::Reload)->setText(tr("Reload"));

  if (feedId_ > -1)
    newsHeader_->retranslateStrings();
}

/*! \brief Обработка нажатия в дереве новостей ********************************/
void NewsTabWidget::slotNewsViewClicked(QModelIndex index)
{
  if (QApplication::keyboardModifiers() == Qt::NoModifier) {
    if ((newsView_->selectionModel()->selectedRows(0).count() == 1)) {
      slotNewsViewSelected(index);
    }
  }
}

void NewsTabWidget::slotNewsViewSelected(QModelIndex index, bool clicked)
{
  QElapsedTimer timer;
  timer.start();
  qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

  int indexId;

  indexId = newsModel_->index(index.row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

  if (!index.isValid()) {
    webView_->setHtml("");
    webPanel_->hide();
    webControlPanel_->hide();
    rsslisting_->slotUpdateStatus();  // необходимо, когда выбрана другая лента, но новость в ней не выбрана
    currentNewsIdOld = indexId;
    currentFeedIdOld = feedId_;
    qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed() << "(invalid index)";
    return;
  }

  if (!((indexId == currentNewsIdOld) && (feedId_ == currentFeedIdOld) &&
        newsModel_->index(index.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() >= 1) ||
      clicked) {

    qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

    markNewsReadTimer_->stop();
    if (rsslisting_->markNewsReadOn_) {
      if (rsslisting_->markNewsReadTime_ == 0) {
        slotSetItemRead(newsView_->currentIndex(), 1);
      } else {
        markNewsReadTimer_->start(rsslisting_->markNewsReadTime_*1000);
      }
    }

    QSqlQuery q(rsslisting_->db_);
    QString qStr = QString("UPDATE feeds SET currentNews='%1' WHERE id=='%2'").
        arg(indexId).arg(feedId_);
    q.exec(qStr);

    qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

    QWebSettings::globalSettings()->clearMemoryCaches();
    webView_->page()->history()->clear();

    updateWebView(index);

    qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

  }
  rsslisting_->slotUpdateStatus();

  currentNewsIdOld = indexId;
  currentFeedIdOld = feedId_;
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
  if (!newsView_->currentIndex().isValid()) {
    if (newsModel_->rowCount() > 0) {
      newsView_->setCurrentIndex(newsModel_->index(0, 6));
      slotNewsViewClicked(newsModel_->index(0, 6));
    }
    return;
  }

  int row = newsView_->currentIndex().row();
  if (row == 0) return;
  else row--;
  newsView_->setCurrentIndex(newsModel_->index(row, 6));
  slotNewsViewClicked(newsModel_->index(row, 6));
}

void NewsTabWidget::slotNewsDownPressed()
{
  if (!newsView_->currentIndex().isValid()) {
    if (newsModel_->rowCount() > 0) {
      newsView_->setCurrentIndex(newsModel_->index(0, 6));
      slotNewsViewClicked(newsModel_->index(0, 6));
    }
    return;
  }

  int row = newsView_->currentIndex().row();
  if ((row+1) == newsModel_->rowCount()) return;
  else row++;
  newsView_->setCurrentIndex(newsModel_->index(row, 6));
  slotNewsViewClicked(newsModel_->index(row, 6));
}

/*! \brief Обработка клавиш Home/End в дереве новостей *************************/
void NewsTabWidget::slotNewsHomePressed()
{
  int row = 0;
  newsView_->setCurrentIndex(newsModel_->index(row, 6));
  slotNewsViewClicked(newsModel_->index(row, 6));
}

void NewsTabWidget::slotNewsEndPressed()
{
  int row = newsModel_->rowCount() - 1;
  newsView_->setCurrentIndex(newsModel_->index(row, 6));
  slotNewsViewClicked(newsModel_->index(row, 6));
}

//! Пометка новости прочитанной
void NewsTabWidget::slotSetItemRead(QModelIndex index, int read)
{
  if (!index.isValid()) return;

  if (newsModel_->rowCount() != 0) {
    int topRow = newsView_->verticalScrollBar()->value();
    QModelIndex curIndex = newsView_->currentIndex();
    if (newsModel_->index(index.row(), newsModel_->fieldIndex("new")).data(Qt::EditRole).toInt() == 1) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("new")),
            0);
    }
    if (read == 1) {
      if (newsModel_->index(index.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() == 0) {
        newsModel_->setData(
              newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
              1);
      }
    } else {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
            0);
    }

    while (newsModel_->canFetchMore())
      newsModel_->fetchMore();

    if (newsView_->currentIndex() != curIndex)
      newsView_->setCurrentIndex(curIndex);
    newsView_->verticalScrollBar()->setValue(topRow);
  }

  rsslisting_->slotUpdateStatus();
}

//! Пометка новости звездочкой (избранная)
void NewsTabWidget::slotSetItemStar(QModelIndex index, int starred)
{
  if (!index.isValid()) return;

  int topRow = newsView_->verticalScrollBar()->value();
  QModelIndex curIndex = newsView_->currentIndex();
  newsModel_->setData(
        newsModel_->index(index.row(), newsModel_->fieldIndex("starred")),
        starred);

  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  newsView_->setCurrentIndex(curIndex);
  newsView_->verticalScrollBar()->setValue(topRow);
}

void NewsTabWidget::slotReadTimer()
{
  markNewsReadTimer_->stop();
  slotSetItemRead(newsView_->currentIndex(), 1);
}

//! Отметить выделенные новости прочитанными
void NewsTabWidget::markNewsRead()
{
  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  int feedId = feedsModel_->index(
      feedsView_->currentIndex().row(), feedsModel_->fieldIndex("id")).data().toInt();

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

    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      int newsId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();
      QSqlQuery q(rsslisting_->db_);
      q.exec(QString("UPDATE news SET new=0, read='%1' WHERE feedId='%2' AND id=='%3'").
             arg(markRead).arg(feedId).arg(newsId));
    }

    rsslisting_->setNewsFilter(rsslisting_->newsFilterGroup_->checkedAction(), false);

    while (newsModel_->canFetchMore())
      newsModel_->fetchMore();

    int row = -1;
    for (int i = 0; i < newsModel_->rowCount(); i++) {
      if (newsModel_->index(i, newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt() ==
          feedsModel_->index(feedsView_->currentIndex().row(),
                             feedsModel_->fieldIndex("currentNews")).data().toInt()) {
        row = i;
        break;
      }
    }
    newsView_->setCurrentIndex(newsModel_->index(row, 6));
    rsslisting_->slotUpdateStatus();
  }
}

//! Отметить все новости в ленте прочитанными
void NewsTabWidget::markAllNewsRead(bool openFeed)
{
  if (newsModel_->rowCount() == 0) return;

  int feedId;
  if (feedsView_->selectIndex.isValid())
    feedId = feedsModel_->index(
            feedsView_->selectIndex.row(),
            feedsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();
  else
    feedId = feedId_;

  QSqlQuery q(rsslisting_->db_);
  QString qStr = QString("UPDATE news SET read=1 WHERE feedId='%1' AND read=0").
      arg(feedId);
  q.exec(qStr);
  qStr = QString("UPDATE news SET new=0 WHERE feedId='%1' AND new=1").
      arg(feedId);
  q.exec(qStr);

  if (openFeed) {
    int currentRow = newsView_->currentIndex().row();

    rsslisting_->setNewsFilter(rsslisting_->newsFilterGroup_->checkedAction(), false);

    while (newsModel_->canFetchMore())
      newsModel_->fetchMore();

    int currentNews = -1;
    for (int i = 0; i < feedsModel_->rowCount(); i++) {
      if (feedsModel_->index(i, feedsModel_->fieldIndex("id")).data().toInt() == feedId) {
        currentNews = feedsModel_->index(i, feedsModel_->fieldIndex("currentNews")).data().toInt();
        break;
      }
    }

    int row = -1;
    for (int i = 0; i < newsModel_->rowCount(); i++) {
      if (newsModel_->index(i, newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt() == currentNews) {
        row = i;
        break;
      }
    }
    newsView_->setCurrentIndex(newsModel_->index(row, 6));
    if (currentRow != row)
      updateWebView(newsModel_->index(row, 6));
  }
  rsslisting_->slotUpdateStatus(openFeed);
}

//! Пометка выбранных новостей звездочкой (избранные)
void NewsTabWidget::markNewsStar()
{
  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  int cnt = indexes.count();

  if (cnt == 1) {
    curIndex = indexes.at(0);
    if (newsModel_->index(curIndex.row(), newsModel_->fieldIndex("starred")).data(Qt::EditRole).toInt() == 0) {
      slotSetItemStar(curIndex, 1);
    } else {
      slotSetItemStar(curIndex, 0);
    }
  } else {
    bool markStar = false;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      if (newsModel_->index(curIndex.row(), newsModel_->fieldIndex("starred")).data(Qt::EditRole).toInt() == 0)
        markStar = true;
    }

    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      if (markStar) slotSetItemStar(curIndex, 1);
      else slotSetItemStar(curIndex, 0);
    }
  }
}

//! Удаление новости
void NewsTabWidget::deleteNews()
{
  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  int cnt = indexes.count();
  if (cnt == 0) return;

  int feedId = feedsModel_->index(
      feedsView_->currentIndex().row(), feedsModel_->fieldIndex("id")).data().toInt();

  if (cnt == 1) {
    curIndex = indexes.at(0);
    int row = curIndex.row();
    slotSetItemRead(curIndex, 1);

    newsModel_->setData(
          newsModel_->index(row, newsModel_->fieldIndex("deleted")), 1);
  } else {
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      int newsId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();
      QSqlQuery q(rsslisting_->db_);
      q.exec(QString("UPDATE news SET new=0, read=2, deleted=1 "
                     "WHERE feedId='%1' AND id=='%2'").
          arg(feedId).arg(newsId));
    }
    rsslisting_->setNewsFilter(rsslisting_->newsFilterGroup_->checkedAction(), false);
  }

  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  if (curIndex.row() == newsModel_->rowCount())
    curIndex = newsModel_->index(curIndex.row()-1, 6);
  else curIndex = newsModel_->index(curIndex.row(), 6);
  newsView_->setCurrentIndex(curIndex);
  slotNewsViewSelected(curIndex);
  rsslisting_->slotUpdateStatus();
}

//! Загрузка/обновление содержимого браузера
void NewsTabWidget::updateWebView(QModelIndex index)
{
  if (!index.isValid()) {
    webView_->setHtml("");
    webPanel_->hide();
    webControlPanel_->hide();
    return;
  }

  webPanel_->show();

  QString titleString, linkString, panelTitleString;
  titleString = newsModel_->record(index.row()).field("title").value().toString();
  if (rsslisting_->isVisible())
    titleString = webPanelTitle_->fontMetrics().elidedText(
          titleString, Qt::ElideRight, webPanelTitle_->width());
  linkString = newsModel_->record(index.row()).field("link_href").value().toString();
  if (linkString.isEmpty())
    linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();
  panelTitleString = QString("<a href='%1'>%2</a>").arg(linkString).arg(titleString);
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
  webControlPanel_->setVisible(false);

  if (!rsslisting_->showDescriptionNews_) {
    QString linkString = newsModel_->record(
          index.row()).field("link_href").value().toString();
    if (linkString.isEmpty())
      linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();

    webControlPanel_->setVisible(true);
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
      QUrl hostUrl(feedsModel_->record(feedsView_->currentIndex().row()).
                   field("htmlUrl").value().toUrl());
      url.setScheme(hostUrl.scheme());
      url.setHost(hostUrl.host());
    }
    if (rsslisting_->embeddedBrowserOn_) {
      if (!webView_->midButtonClick) {
        if (!webControlPanel_->isVisible()) {
          webView_->history()->clear();
          webControlPanel_->setVisible(true);
        }
        webView_->load(url);
      } else {
        qobject_cast<WebView*>(rsslisting_->createWebTab()->view())->load(url);
      }
    } else QDesktopServices::openUrl(url);
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
//  if (newsView_->currentIndex().isValid()) {
    webViewProgress_->setValue(0);
    webViewProgress_->show();
//  }
}

void NewsTabWidget::slotLoadFinished(bool ok)
{
  if (!ok)
    rsslisting_->statusBar()->showMessage(tr("Error loading to WebView"), 3000);
  webViewProgress_->hide();
}

//! Слот для асинхронного обновления новости
void NewsTabWidget::slotWebViewSetContent(QString content)
{
  webView_->setHtml(content);
}

void NewsTabWidget::slotWebTitleLinkClicked(QString urlStr)
{
  slotLinkClicked(urlStr.simplified());
}

//! Переход на краткое содержание новости
void NewsTabWidget::webHomePage()
{
  if (feedId_ > -1) {
    updateWebView(newsView_->currentIndex());
    webView_->history()->clear();
  } else {
    webView_->history()->goToItem(webView_->history()->itemAt(0));
  }
}

//! Открытие отображаемой страницы во внешнем браузере
void NewsTabWidget::openPageInExternalBrowser()
{
  QDesktopServices::openUrl(webView_->url());
}

//! Открытие новости клавишей Enter
void NewsTabWidget::slotOpenNewsWebView()
{
  if (!newsView_->hasFocus()) return;
  slotNewsViewClicked(newsView_->currentIndex());
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

  QDesktopServices::openUrl(QUrl(linkString.simplified()));
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
  if (feedId_ == -1) {
    QString tabText = title;
    tabText = newsTextTitle_->fontMetrics().elidedText(
          tabText, Qt::ElideRight, 114);
    newsTextTitle_->setText(tabText);
  }
}

void NewsTabWidget::slotOpenNewsNewTab()
{
  slotNewsMiddleClicked(newsView_->currentIndex());
}
