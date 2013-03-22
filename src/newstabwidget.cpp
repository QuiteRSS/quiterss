#include "newstabwidget.h"
#include "rsslisting.h"

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

NewsTabWidget::NewsTabWidget( QWidget *parent, int type, int feedId, int feedParId)
  : QWidget(parent), type_(type),
    feedId_(feedId), feedParId_(feedParId)
{
  rsslisting_ = qobject_cast<RSSListing*>(parent);
  db_ = QSqlDatabase::database();
  feedsTreeView_ = rsslisting_->feedsTreeView_;
  feedsTreeModel_ = rsslisting_->feedsTreeModel_;

  currentNewsIdOld = -1;
  autoLoadImages_ = true;
  pageLoaded_ = false;

  newsIconTitle_ = new QLabel();
  newsIconMovie_ = new QMovie(":/images/loading");
  newsIconTitle_->setMovie(newsIconMovie_);
  newsTextTitle_ = new QLabel();
  newsTextTitle_->setObjectName("newsTextTitle_");

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
  newsTitleLabel_->setMinimumHeight(16);
  newsTitleLabel_->setFixedWidth(148);
  newsTitleLabel_->setLayout(newsTitleLayout);
  newsTitleLabel_->setVisible(false);

  if (type_ != TAB_WEB) {
    createNewsList();
    createMenuNews();
  } else {
    autoLoadImages_ = rsslisting_->autoLoadImages_;
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
    newsTabWidgetSplitter_->setHandleWidth(1);

    if ((rsslisting_->browserPosition_ == RIGHT_POSITION) ||
        (rsslisting_->browserPosition_ == LEFT_POSITION)) {
      newsTabWidgetSplitter_->setOrientation(Qt::Horizontal);
      newsTabWidgetSplitter_->setStyleSheet(
            QString("QSplitter::handle {background: qlineargradient("
                    "x1: 0, y1: 0, x2: 0, y2: 1,"
                    "stop: 0 %1, stop: 0.07 %2);"
                    "margin-top: 1px; margin-bottom: 1px;}").
            arg(newsPanelWidget_->palette().background().color().name()).
            arg(qApp->palette().color(QPalette::Dark).name()));
    } else {
      newsTabWidgetSplitter_->setOrientation(Qt::Vertical);
      newsTabWidgetSplitter_->setStyleSheet(
            QString("QSplitter::handle {background: %1; margin-top: 1px; margin-bottom: 1px;}").
            arg(qApp->palette().color(QPalette::Dark).name()));
    }
  }

  connect(this, SIGNAL(signalSetTextTab(QString,NewsTabWidget*)),
          rsslisting_, SLOT(setTextTitle(QString,NewsTabWidget*)));
}

void NewsTabWidget::resizeEvent(QResizeEvent *)
{
  setTitleWebPanel();
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

  newsHeader_->init(rsslisting_);

  newsToolBar_ = new QToolBar(this);
  newsToolBar_->setObjectName("newsToolBar");
  newsToolBar_->setStyleSheet("QToolBar { border: none; padding: 0px; }");
  newsToolBar_->setIconSize(QSize(18, 18));

  QString actionListStr = "markNewsRead,markAllNewsRead,Separator,markStarAct,"
      "newsLabelAction,shareMenuAct,Separator,nextUnreadNewsAct,prevUnreadNewsAct,"
      "Separator,newsFilter,Separator,deleteNewsAct";
  QString str = rsslisting_->settings_->value("Settings/newsToolBar", actionListStr).toString();

  foreach (QString actionStr, str.split(",", QString::SkipEmptyParts)) {
    if (actionStr == "Separator") {
      newsToolBar_->addSeparator();
    } else {
      QListIterator<QAction *> iter(rsslisting_->actions());
      while (iter.hasNext()) {
        QAction *pAction = iter.next();
        if (!pAction->icon().isNull()) {
          if (pAction->objectName() == actionStr) {
            newsToolBar_->addAction(pAction);
            break;
          }
        }
      }
    }
  }
  separatorRAct_ = newsToolBar_->addSeparator();
  separatorRAct_->setObjectName("separatorRAct");
  newsToolBar_->addAction(rsslisting_->restoreNewsAct_);

  findText_ = new FindTextContent(this);
  findText_->setFixedWidth(200);

  QHBoxLayout *newsPanelLayout = new QHBoxLayout();
  newsPanelLayout->setMargin(2);
  newsPanelLayout->setSpacing(2);
  newsPanelLayout->addWidget(newsToolBar_);
  newsPanelLayout->addStretch(1);
  newsPanelLayout->addWidget(findText_);

  newsPanelWidget_ = new QWidget(this);
  newsPanelWidget_->setObjectName("newsPanelWidget_");
  newsPanelWidget_->setStyleSheet(
        QString("#newsPanelWidget_ {border-bottom: 1px solid %1;}").
        arg(qApp->palette().color(QPalette::Dark).name()));

  newsPanelWidget_->setLayout(newsPanelLayout);
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
  connect(newsView_, SIGNAL(signaNewslLabelClicked(QModelIndex)),
          this, SLOT(slotNewslLabelClicked(QModelIndex)));
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
  newsContextMenu_->addAction(rsslisting_->newsLabelMenuAction_);
  newsContextMenu_->addAction(rsslisting_->shareMenuAct_);
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
  webView_ = new WebView(this, rsslisting_->networkManager_);

  webMenu_ = new QMenu(webView_);

  webViewProgress_ = new QProgressBar(this);
  webViewProgress_->setObjectName("webViewProgress_");
  webViewProgress_->setFixedHeight(15);
  webViewProgress_->setMinimum(0);
  webViewProgress_->setMaximum(100);
  webViewProgress_->setVisible(true);
  connect(this, SIGNAL(loadProgress(int)),
          webViewProgress_, SLOT(setValue(int)), Qt::QueuedConnection);

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

  QHBoxLayout *webControlPanelLayout = new QHBoxLayout();
  webControlPanelLayout->setMargin(2);
  webControlPanelLayout->setSpacing(2);
  webControlPanelLayout->addWidget(webToolBar_);
  webControlPanelLayout->addStretch(1);

  webControlPanel_ = new QWidget(this);
  webControlPanel_->setObjectName("webControlPanel_");
  webControlPanel_->setStyleSheet(
        QString("#webControlPanel_ {border-bottom: 1px solid %1;}").
        arg(qApp->palette().color(QPalette::Dark).name()));
  webControlPanel_->setLayout(webControlPanelLayout);

  if (type_ != TAB_WEB)
    setWebToolbarVisible(false, false);
  else
    setWebToolbarVisible(true, false);

  //! Create web panel
  webPanelAuthor_ = new QLabel(this);
  webPanelAuthor_->setObjectName("webPanelAuthor_");
  webPanelAuthor_->hide();

  webPanelTitle_ = new QLabel(this);
  webPanelTitle_->setObjectName("webPanelTitle_");
  webPanelTitle_->setTextInteractionFlags(Qt::TextBrowserInteraction);

  webPanelDate_ = new QLabel(this);

  QGridLayout *webPanelLayout = new QGridLayout();
  webPanelLayout->setMargin(5);
  webPanelLayout->setSpacing(5);
  webPanelLayout->setColumnStretch(0, 1);
  webPanelLayout->addWidget(webPanelTitle_, 0, 0);
  webPanelLayout->addWidget(webPanelDate_, 0, 1, 1, 1, Qt::AlignRight);
  webPanelLayout->addWidget(webPanelAuthor_, 1, 0);

  webPanel_ = new QWidget(this);
  webPanel_->setObjectName("webPanel_");
  webPanel_->setStyleSheet(
          QString("#webPanel_ {border-bottom: 1px solid %1;}").
          arg(qApp->palette().color(QPalette::Dark).name()));
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

  webView_->page()->action(QWebPage::OpenLink)->disconnect();
  webView_->page()->action(QWebPage::OpenLinkInNewWindow)->disconnect();

  urlExternalBrowserAct_ = new QAction(this);
  urlExternalBrowserAct_->setIcon(QIcon(":/images/openBrowser"));

  webPanelTitle_->installEventFilter(this);

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
  connect(webPanelTitle_, SIGNAL(linkHovered(QString)),
          this, SLOT(slotLinkHovered(QString)));
  connect(this, SIGNAL(signalWebViewSetContent(QString, bool)),
                SLOT(slotWebViewSetContent(QString, bool)), Qt::QueuedConnection);
  connect(webView_, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
  connect(webView_, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished(bool)));
  connect(webView_, SIGNAL(linkClicked(QUrl)), this, SLOT(slotLinkClicked(QUrl)));
  connect(webView_->page(), SIGNAL(linkHovered(QString,QString,QString)),
          this, SLOT(slotLinkHovered(QString,QString,QString)));
  connect(webView_, SIGNAL(loadProgress(int)), this, SLOT(slotSetValue(int)));

  connect(webView_, SIGNAL(titleChanged(QString)),
          this, SLOT(webTitleChanged(QString)));
  connect(webView_->page()->action(QWebPage::OpenLink), SIGNAL(triggered()),
          this, SLOT(openLink()));
  connect(webView_->page()->action(QWebPage::OpenLinkInNewWindow), SIGNAL(triggered()),
          this, SLOT(openLinkInNewTab()));

  connect(webView_, SIGNAL(customContextMenuRequested(QPoint)),
          this, SLOT(showContextWebPage(const QPoint &)));

  connect(webView_->page()->networkAccessManager(),
          SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          rsslisting_, SLOT(slotAuthentication(QNetworkReply*,QAuthenticator*)));

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
      newsModel_->formatDate_ = rsslisting_->formatDate_;
      newsModel_->formatTime_ = rsslisting_->formatTime_;
    }

    QFont font = QFont(rsslisting_->panelNewsFontFamily_, rsslisting_->panelNewsFontSize_);
    font.setBold(true);
    webPanelTitle_->setFont(font);
    webPanelDate_->setFont(
          QFont(rsslisting_->panelNewsFontFamily_, rsslisting_->panelNewsFontSize_));
    webPanelAuthor_->setFont(
          QFont(rsslisting_->panelNewsFontFamily_, rsslisting_->panelNewsFontSize_));

    webView_->settings()->setFontFamily(
          QWebSettings::StandardFont, rsslisting_->webFontFamily_);
    if (webView_->title().isEmpty() && (type_ != TAB_WEB)) {
      webView_->settings()->setFontSize(
            QWebSettings::DefaultFontSize, rsslisting_->webFontSize_);
    }
    webView_->settings()->setFontSize(
          QWebSettings::MinimumFontSize, rsslisting_->browserMinFontSize_);
    webView_->settings()->setFontSize(
          QWebSettings::MinimumLogicalFontSize, rsslisting_->browserMinLogFontSize_);

    if (rsslisting_->externalBrowserOn_ <= 0) {
      webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    } else {
      webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    }
    webView_->settings()->setAttribute(
          QWebSettings::JavascriptEnabled, rsslisting_->javaScriptEnable_);
    webView_->settings()->setAttribute(
          QWebSettings::PluginsEnabled, rsslisting_->pluginsEnable_);
  }

  if (type_ == TAB_FEED) {
    QSqlQuery q;
    q.exec(QString("SELECT displayEmbeddedImages FROM feeds WHERE id=='%1'").
           arg(feedId_));
    if (q.next()) autoLoadImages_ = q.value(0).toInt();

    newsView_->setAlternatingRowColors(rsslisting_->alternatingRowColorsNews_);
  }

  webView_->settings()->setAttribute(
        QWebSettings::AutoLoadImages, autoLoadImages_);

  rsslisting_->autoLoadImages_ = !autoLoadImages_;
  rsslisting_->setAutoLoadImages(false);

  if (type_ != TAB_WEB) {
    rsslisting_->slotUpdateStatus(feedId_, false);

    rsslisting_->newsFilter_->setEnabled(type_ == TAB_FEED);
    rsslisting_->deleteNewsAct_->setEnabled(type_ != TAB_CAT_DEL);
    separatorRAct_->setVisible(type_ == TAB_CAT_DEL);
    rsslisting_->restoreNewsAct_->setVisible(type_ == TAB_CAT_DEL);
  }
}

//! Перезагрузка перевода
void NewsTabWidget::retranslateStrings() {
  webViewProgress_->setFormat(tr("Loading... (%p%)"));

  QString str = webPanelAuthor_->text();
  str = str.right(str.length() - str.indexOf(':') - 2);
  webPanelAuthor_->setText(QString(tr("Author: %1")).arg(str));

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
  webView_->page()->action(QWebPage::CopyImageToClipboard)->setText(tr("Copy Image"));
#if QT_VERSION >= 0x040800
  webView_->page()->action(QWebPage::CopyImageUrlToClipboard)->setText(tr("Copy Image Address"));
#endif

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
    } else if (event->type() == QEvent::Show) {
      setTitleWebPanel();
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

    if (type_ == TAB_FEED) {
      // Запись текущей новости в ленту
      QSqlQuery q;
      QString qStr = QString("UPDATE feeds SET currentNews='%1' WHERE id=='%2'").
          arg(newsId).arg(feedId_);
      q.exec(qStr);

      qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

      QModelIndex feedIndex = feedsTreeModel_->getIndexById(feedId_, feedParId_);
      feedsTreeModel_->setData(
            feedIndex.sibling(feedIndex.row(), feedsTreeModel_->proxyColumnByOriginal("currentNews")),
            newsId);
    } else if (type_ == TAB_CAT_LABEL) {
      QSqlQuery q;
      QString qStr = QString("UPDATE labels SET currentNews='%1' WHERE id=='%2'").
          arg(newsId).
          arg(rsslisting_->newsCategoriesTree_->currentItem()->text(2).toInt());
      q.exec(qStr);

      rsslisting_->newsCategoriesTree_->currentItem()->setText(3, QString::number(newsId));
    }

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

  QUrl url = QUrl::fromEncoded(linkString.simplified().toLocal8Bit());
  slotLinkClicked(url);
}

void NewsTabWidget::slotNewsMiddleClicked(QModelIndex index)
{
  if (!index.isValid()) return;

  QString linkString = newsModel_->record(
        index.row()).field("link_href").value().toString();
  if (linkString.isEmpty())
    linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();

  webView_->midButtonClick = true;
  QUrl url = QUrl::fromEncoded(linkString.simplified().toLocal8Bit());
  slotLinkClicked(url);
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
    newsView_->verticalScrollBar()->setValue(row - pageStep/2);

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
    newsView_->verticalScrollBar()->setValue(row - pageStep/2);

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
  markNewsReadTimer_->stop();

  bool changed = false;
  int newsId = newsModel_->index(index.row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();
  QSqlQuery q;

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
    int feedId = newsModel_->index(index.row(), newsModel_->fieldIndex("feedId")).
        data(Qt::EditRole).toInt();
    rsslisting_->slotUpdateStatus(feedId);
  }
}

//! Пометка новости звездочкой (избранная)
void NewsTabWidget::slotSetItemStar(QModelIndex index, int starred)
{
  if (!index.isValid()) return;

  newsModel_->setData(index, starred);

  int newsId = newsModel_->index(index.row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();
  QSqlQuery q;
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

    db_.transaction();
    QSqlQuery q;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      int newsId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();
      q.exec(QString("UPDATE news SET new=0, read='%1' WHERE id=='%2'").
             arg(markRead).arg(newsId));
    }
    db_.commit();

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
  markNewsReadTimer_->stop();

  QString strId;
  QSqlQuery q;
  q.exec(QString("SELECT xmlUrl FROM feeds WHERE id=='%1'").arg(feedId_));
  if (q.next()) {
    if (q.value(0).toString().isEmpty()) {
      strId = QString("(%1)").arg(rsslisting_->getIdFeedsString(feedId_));
    } else {
      strId = QString("feedId='%1'").arg(feedId_);
    }
  }

  QString qStr = QString("UPDATE news SET read=1 WHERE %1 AND read=0").
      arg(strId);
  q.exec(qStr);
  qStr = QString("UPDATE news SET new=0 WHERE %1 AND new=1").
      arg(strId);
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

    db_.transaction();
    for (int i = cnt-1; i >= 0; --i) {
      slotSetItemStar(indexes.at(i), markStar);
    }
    db_.commit();
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
    newsModel_->setData(newsModel_->index(curIndex.row(), newsModel_->fieldIndex("deleteDate")),
                        QDateTime::currentDateTime().toString(Qt::ISODate));
    newsModel_->submitAll();
  } else {
    db_.transaction();
    QSqlQuery q;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      int newsId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();
      q.exec(QString("UPDATE news SET new=0, read=2, deleted=1, deleteDate='%1' WHERE id=='%2'").
             arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(newsId));
    }
    db_.commit();

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

  db_.transaction();
  QSqlQuery q;
  for (int i = newsModel_->rowCount()-1; i >= 0; --i) {
    int newsId = newsModel_->index(i, newsModel_->fieldIndex("id")).data().toInt();
    q.exec(QString("UPDATE news SET new=0, read=2, deleted=1, deleteDate='%1' WHERE id=='%2'").
           arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(newsId));
  }
  db_.commit();

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
    newsModel_->setData(newsModel_->index(curIndex.row(), newsModel_->fieldIndex("deleteDate")), "");
    newsModel_->submitAll();
  } else {
    db_.transaction();
    QSqlQuery q;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      int newsId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();
      q.exec(QString("UPDATE news SET deleted=0, deleteDate='' WHERE id=='%1'").
             arg(newsId));
    }
    db_.commit();

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
  QString strId;
  QSqlQuery q;
  q.exec(QString("SELECT xmlUrl FROM feeds WHERE id=='%1'").arg(feedId_));
  if (q.next()) {
    if (q.value(0).toString().isEmpty()) {
      strId = QString("(%1)").arg(rsslisting_->getIdFeedsString(feedId_));
    } else {
      strId = QString("feedId='%1'").arg(feedId_);
    }
  }

  QString qStr;
  if (column == newsModel_->fieldIndex("read")) {
    qStr = QString("UPDATE news SET rights=read WHERE %1").arg(strId);
  }
  else if (column == newsModel_->fieldIndex("starred")) {
    qStr = QString("UPDATE news SET rights=starred WHERE %1").arg(strId);
  }
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

  titleString_ = newsModel_->record(index.row()).field("title").value().toString();
  linkString_ = newsModel_->record(index.row()).field("link_href").value().toString();
  if (linkString_.isEmpty())
    linkString_ = newsModel_->record(index.row()).field("link_alternate").value().toString();

  setTitleWebPanel();

  QDateTime dtLocal;
  QString strDate = newsModel_->record(index.row()).field("published").value().toString();

  if (!strDate.isNull()) {
    QDateTime dtLocalTime = QDateTime::currentDateTime();
    QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
    int nTimeShift = dtLocalTime.secsTo(dtUTC);

    QDateTime dt = QDateTime::fromString(strDate, Qt::ISODate);
    dtLocal = dt.addSecs(nTimeShift);
  } else {
    dtLocal = QDateTime::fromString(
          newsModel_->record(index.row()).field("received").value().toString(),
          Qt::ISODate);
  }
  if (QDateTime::currentDateTime().date() == dtLocal.date())
    strDate = dtLocal.toString(rsslisting_->formatTime_);
  else
    strDate = dtLocal.toString(rsslisting_->formatDate_ + " " + rsslisting_->formatTime_);
  webPanelDate_->setText(strDate);

  // Формируем панель автора из автора новости
  QString authorString;
  QString authorName = newsModel_->record(index.row()).field("author_name").value().toString();
  QString authorEmail = newsModel_->record(index.row()).field("author_email").value().toString();
  QString authorUri = newsModel_->record(index.row()).field("author_uri").value().toString();
  //  qDebug() << "author_news:" << authorName << authorEmail << authorUri;
  authorString = authorName;
  if (!authorEmail.isEmpty())
    authorString.append(QString(" <a href='mailto:%1' style=\"text-decoration:none;\">e-mail</a>").
                        arg(authorEmail));
  if (!authorUri.isEmpty())
    authorString.append(QString(" <a href='%1' style=\"text-decoration:none;\">page</a>").
                        arg(authorUri));

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
    if (!authorEmail.isEmpty())
      authorString.append(QString(" <a href='mailto:%1' style=\"text-decoration:none;\">e-mail</a>").
                          arg(authorEmail));
    if (!authorUri.isEmpty())
      authorString.append(QString(" <a href='%1' style=\"text-decoration:none;\">page</a>").
                          arg(authorUri));
  }

  webPanelAuthor_->setText(QString(tr("Author: %1")).arg(authorString));
  webPanelAuthor_->setVisible(!authorString.isEmpty());

  bool showDescriptionNews_ = rsslisting_->showDescriptionNews_;

  QVariant displayNews =
      feedsTreeModel_->dataField(feedsTreeView_->currentIndex(), "displayNews");
  if (!displayNews.toString().isEmpty())
    showDescriptionNews_ = !displayNews.toInt();

  if (!showDescriptionNews_) {
    webPanel_->hide();
    setWebToolbarVisible(true, false);

    QString linkString = newsModel_->record(
          index.row()).field("link_href").value().toString();
    if (linkString.isEmpty())
      linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();

    QUrl url = QUrl::fromEncoded(linkString.simplified().toLocal8Bit());
    webView_->setUrl(url);
  } else {
    setWebToolbarVisible(false, false);
    webPanel_->show();

    QString content = newsModel_->record(index.row()).field("content").value().toString();
    QString description = newsModel_->record(index.row()).field("description").value().toString();
    if (content.isEmpty()) {
      emit signalWebViewSetContent(description);
    } else if (description.length() > content.length()) {
      emit signalWebViewSetContent(description);
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
  if (rsslisting_->externalBrowserOn_ <= 0) {
    if (!webView_->midButtonClick) {
      if (!webControlPanel_->isVisible()) {
        webView_->history()->clear();
        webPanel_->hide();
        setWebToolbarVisible(true, false);
      }
      webView_->setUrl(url);
    } else {
      QWebPage *webPage = rsslisting_->createWebTab();
      qobject_cast<WebView*>(webPage->view())->load(url);
    }
  } else openUrl(url);
  webView_->midButtonClick = false;
}

void NewsTabWidget::slotLinkHovered(const QString &link, const QString &, const QString &)
{
  rsslisting_->statusBar()->showMessage(link.simplified(), 3000);
}

void NewsTabWidget::slotSetValue(int value)
{
  emit loadProgress(value);
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
}

//! Слот для асинхронного обновления новости
void NewsTabWidget::slotWebViewSetContent(QString content, bool hide)
{
  QString htmlStr;
  QUrl baseUrl;

  if (!hide) {
    QModelIndex index = newsView_->currentIndex();
    QString enclosureUrl = newsModel_->record(index.row()).
        field("enclosure_url").value().toString();
    if (!enclosureUrl.isEmpty()) {
      QString type = newsModel_->record(index.row()).
          field("enclosure_type").value().toString();
      if (type.contains("image")) {
        htmlStr = QString("<IMG SRC=\"%1\" style=\"max-width: 100%\"><p>").
            arg(newsModel_->record(index.row()).field("enclosure_url").value().toString());
      } else {
        if (type.contains("audio")) type = tr("audio");
        else if (type.contains("video")) type = tr("video");
        else type = tr("media");

        htmlStr = QString("<a href=\"%1\" style=\"color: #4b4b4b;\"> %2 %3 </a><p>").
            arg(newsModel_->record(index.row()).field("enclosure_url").value().toString()).
            arg(tr("Link to")).arg(type);
      }
    }
    htmlStr.append(content);

    QString baseUrlStr = newsModel_->record(
          index.row()).field("link_href").value().toString();
    if (baseUrlStr.isEmpty())
      baseUrlStr = newsModel_->record(index.row()).field("link_alternate").value().toString();
    baseUrl = QUrl::fromEncoded(baseUrlStr.simplified().toLocal8Bit());
  }

  webView_->history()->setMaximumItemCount(0);
  webView_->setHtml(htmlStr, baseUrl);
  webView_->history()->setMaximumItemCount(100);
}

void NewsTabWidget::slotWebTitleLinkClicked(QString urlStr)
{
  QUrl url = QUrl::fromEncoded(urlStr.simplified().toLocal8Bit());
  slotLinkClicked(url);
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
  if (type_ == TAB_WEB) return;

  int externalBrowserOn_ = rsslisting_->externalBrowserOn_;
  rsslisting_->externalBrowserOn_ = 0;
  slotNewsViewDoubleClicked(newsView_->currentIndex());
  rsslisting_->externalBrowserOn_ = externalBrowserOn_;
}

//! Открытие новости во внешнем браузере
void NewsTabWidget::openInExternalBrowserNews()
{
  if (type_ != TAB_WEB) {
    QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

    int cnt = indexes.count();
    if (cnt == 0) return;

    for (int i = cnt-1; i >= 0; --i) {
      QModelIndex index = indexes.at(i);
      QString linkString = newsModel_->record(
            index.row()).field("link_href").value().toString();
      if (linkString.isEmpty())
        linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();
      QUrl url = QUrl::fromEncoded(linkString.simplified().toLocal8Bit());
      openUrl(url);
    }
  } else {
    openUrl(webView_->url());
  }
}

//! Установка позиции браузера
void NewsTabWidget::setBrowserPosition()
{
  int idx = newsTabWidgetSplitter_->indexOf(webWidget_);

  switch (rsslisting_->browserPosition_) {
  case TOP_POSITION: case LEFT_POSITION:
    newsTabWidgetSplitter_->insertWidget(0, newsTabWidgetSplitter_->widget(idx));
    break;
  default:
    newsTabWidgetSplitter_->insertWidget(1, newsTabWidgetSplitter_->widget(idx));
  }

  switch (rsslisting_->browserPosition_) {
  case RIGHT_POSITION: case LEFT_POSITION:
    newsTabWidgetSplitter_->setOrientation(Qt::Horizontal);
    newsTabWidgetSplitter_->setStyleSheet(
          QString("QSplitter::handle {background: qlineargradient("
                  "x1: 0, y1: 0, x2: 0, y2: 1,"
                  "stop: 0 %1, stop: 0.07 %2);"
                  "margin-top: 1px; margin-bottom: 1px;}").
          arg(newsPanelWidget_->palette().background().color().name()).
          arg(qApp->palette().color(QPalette::Dark).name()));
    break;
  default:
    newsTabWidgetSplitter_->setOrientation(Qt::Vertical);
    newsTabWidgetSplitter_->setStyleSheet(
          QString("QSplitter::handle {background: %1; margin-top: 1px; margin-bottom: 1px;}").
          arg(qApp->palette().color(QPalette::Dark).name()));
  }
}

//! Закрытие вкладки по кнопке х
void NewsTabWidget::slotTabClose()
{
  rsslisting_->slotTabCloseRequested(rsslisting_->stackedWidget_->indexOf(this));
}

//! Вывод на вкладке названия открытой странички браузера
void NewsTabWidget::webTitleChanged(QString title)
{
  if ((type_ == TAB_WEB) && !title.isEmpty()) {
    setTextTab(title);
  }

  if (title.isEmpty() && (type_ != TAB_WEB)) {
    if (pageLoaded_) {
      webView_->settings()->setFontSize(
            QWebSettings::DefaultFontSize, rsslisting_->webFontSize_);
      pageLoaded_ = false;
    }
  } else {
    if (!pageLoaded_) {
      webView_->settings()->setFontSize(
            QWebSettings::DefaultFontSize, webDefaultFontSize_);
      webView_->settings()->setFontSize(
            QWebSettings::DefaultFixedFontSize, webDefaultFixedFontSize_);
      pageLoaded_ = true;
    }
  }
}

//! Открытие новости в новой вкладке
void NewsTabWidget::openNewsNewTab()
{
  if (type_ == TAB_WEB) return;

  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  int cnt = indexes.count();
  if (cnt == 0) return;

  int externalBrowserOn_ = rsslisting_->externalBrowserOn_;
  rsslisting_->externalBrowserOn_ = 0;
  for (int i = cnt-1; i >= 0; --i) {
    slotNewsMiddleClicked(indexes.at(i));
  }
  rsslisting_->externalBrowserOn_ = externalBrowserOn_;
}

//! Открытие ссылки
void NewsTabWidget::openLink()
{
  slotLinkClicked(linkUrl_);
}

//! Открытие ссылки в новой вкладке
void NewsTabWidget::openLinkInNewTab()
{
  int externalBrowserOn_ = rsslisting_->externalBrowserOn_;
  rsslisting_->externalBrowserOn_ = 0;
  webView_->midButtonClick = true;
  slotLinkClicked(linkUrl_);
  rsslisting_->externalBrowserOn_ = externalBrowserOn_;
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
  if ((rsslisting_->externalBrowserOn_ == 2) || (rsslisting_->externalBrowserOn_ == -1)) {
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

    QString filterStr;
    switch (type_) {
      case TAB_CAT_UNREAD: case TAB_CAT_STAR: case TAB_CAT_DEL: case TAB_CAT_LABEL:
      filterStr = categoryFilterStr_;
      break;
    default:
      filterStr = rsslisting_->newsFilterStr;
    }
    filterStr.append(
          QString(" AND (title LIKE '%%1%' OR author_name LIKE '%%1%' OR category LIKE '%%1%')").
          arg(text));
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
    int newsId = newsModel_->index(
          newsView_->currentIndex().row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

    QString filterStr;
    switch (type_) {
    case TAB_CAT_UNREAD: case TAB_CAT_STAR: case TAB_CAT_DEL: case TAB_CAT_LABEL:
      filterStr = categoryFilterStr_;
      break;
    default:
      filterStr = rsslisting_->newsFilterStr;
    }
    newsModel_->setFilter(filterStr);

    QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("id"));
    QModelIndexList indexList = newsModel_->match(index, Qt::EditRole, newsId);
    if (indexList.count()) {
      int newsRow = indexList.first().row();
      newsView_->setCurrentIndex(newsModel_->index(newsRow, newsModel_->fieldIndex("title")));
    } else {
      currentNewsIdOld = newsId;
    }
  }
  slotFindText(findText_->text());
}

void NewsTabWidget::hideWebContent()
{
  emit signalWebViewSetContent("", true);
  webPanel_->hide();
  setWebToolbarVisible(false, false);
}

void NewsTabWidget::showContextWebPage(const QPoint &p)
{
  if (webView_->rightButtonClick) {
    webView_->rightButtonClick = false;
    return;
  }

  QListIterator<QAction *> iter(webMenu_->actions());
  while (iter.hasNext()) {
    QAction *nextAction = iter.next();
    if (nextAction->text().isEmpty()) {
      delete nextAction;
    }
  }
  webMenu_->clear();

  QMenu *menu_t = webView_->page()->createStandardContextMenu();
  webMenu_->addActions(menu_t->actions());

  const QWebHitTestResult &hitTest = webView_->page()->mainFrame()->hitTestContent(p);
  if (!hitTest.linkUrl().isEmpty() && hitTest.linkUrl().scheme() != "javascript") {
    linkUrl_ = hitTest.linkUrl();
    if (rsslisting_->externalBrowserOn_ <= 0) {
      webMenu_->addSeparator();
      webMenu_->addAction(urlExternalBrowserAct_);
    }
  } else if (menu_t->actions().indexOf(webView_->pageAction(QWebPage::Reload)) >= 0) {
    if (webView_->title().isEmpty()) {
      webView_->pageAction(QWebPage::Reload)->setVisible(false);
    } else {
      webView_->pageAction(QWebPage::Reload)->setVisible(true);
      webMenu_->addSeparator();
    }
    webMenu_->addAction(rsslisting_->autoLoadImagesToggle_);
    webMenu_->addSeparator();
    webMenu_->addAction(rsslisting_->printAct_);
    webMenu_->addAction(rsslisting_->printPreviewAct_);
    webMenu_->addSeparator();
    webMenu_->addAction(rsslisting_->savePageAsAct_);
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

void NewsTabWidget::setWebToolbarVisible(bool show, bool checked)
{
  if (!checked) webToolbarShow_ = show;
  webControlPanel_->setVisible(webToolbarShow_ &
                               rsslisting_->browserToolbarToggle_->isChecked());
}

void NewsTabWidget::setTitleWebPanel()
{
  QString titleStr, panelTitleStr;
  titleStr = webPanelTitle_->fontMetrics().elidedText(
        titleString_, Qt::ElideRight, webPanelTitle_->width());
  panelTitleStr = QString("<a href='%1' style=\"text-decoration:none;\">%2</a>").
      arg(linkString_).arg(titleStr);
  webPanelTitle_->setText(panelTitleStr);
  if (titleString_ != titleStr)
    webPanelTitle_->setToolTip(titleString_);
  else
    webPanelTitle_->setToolTip("");
}

/**
 * @brief Установка метки для выбранных новостей
 ******************************************************************************/
void NewsTabWidget::setLabelNews(int labelId)
{
  if (type_ == TAB_WEB) return;

  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(
        newsModel_->fieldIndex("label"));

  int cnt = indexes.count();
  if (cnt == 0) return;

  if (cnt == 1) {
    QModelIndex index = indexes.at(0);
    QString strIdLabels = index.data(Qt::EditRole).toString();
    if (!strIdLabels.contains(QString(",%1,").arg(labelId))) {
      if (strIdLabels.isEmpty()) strIdLabels.append(",");
      strIdLabels.append(QString::number(labelId));
      strIdLabels.append(",");
    } else {
      strIdLabels.replace(QString(",%1,").arg(labelId), ",");
    }
    newsModel_->setData(index, strIdLabels);

    int newsId = newsModel_->index(index.row(), newsModel_->fieldIndex("id")).
        data(Qt::EditRole).toInt();
    QSqlQuery q;
    q.exec(QString("UPDATE news SET label='%1' WHERE id=='%2'").
           arg(strIdLabels).arg(newsId));
    if (newsId != currentNewsIdOld) {
      newsView_->selectionModel()->select(
            index, QItemSelectionModel::Deselect|QItemSelectionModel::Rows);
    }
  } else {
    bool setLabel = false;
    for (int i = cnt-1; i >= 0; --i) {
      QModelIndex index = indexes.at(i);
      QString strIdLabels = index.data(Qt::EditRole).toString();
      if (!strIdLabels.contains(QString(",%1,").arg(labelId))) {
        setLabel = true;
        break;
      }
    }

    db_.transaction();
    for (int i = cnt-1; i >= 0; --i) {
      QModelIndex index = indexes.at(i);
      QString strIdLabels = index.data(Qt::EditRole).toString();
      if (setLabel) {
        if (strIdLabels.contains(QString(",%1,").arg(labelId))) continue;
        if (strIdLabels.isEmpty()) strIdLabels.append(",");
        strIdLabels.append(QString::number(labelId));
        strIdLabels.append(",");
      } else {
        strIdLabels.replace(QString(",%1,").arg(labelId), ",");
      }
      newsModel_->setData(index, strIdLabels);

      int newsId = newsModel_->index(index.row(), newsModel_->fieldIndex("id")).
          data(Qt::EditRole).toInt();
      QSqlQuery q;
      q.exec(QString("UPDATE news SET label='%1' WHERE id=='%2'").
             arg(strIdLabels).arg(newsId));
      if (newsId != currentNewsIdOld) {
        newsView_->selectionModel()->select(
              index, QItemSelectionModel::Deselect|QItemSelectionModel::Rows);
      }
    }
    db_.commit();
  }
  newsView_->viewport()->update();
}

void NewsTabWidget::slotNewslLabelClicked(QModelIndex index)
{
  if (!newsView_->selectionModel()->isSelected(index)) {
    newsView_->selectionModel()->clearSelection();
    newsView_->selectionModel()->select(
          index, QItemSelectionModel::Select|QItemSelectionModel::Rows);
  }
  rsslisting_->newsLabelMenu_->popup(
        newsView_->viewport()->mapToGlobal(newsView_->visualRect(index).bottomLeft()));
}

void NewsTabWidget::reduceNewsList()
{
  if (type_ == TAB_WEB) return;

  QList <int> sizes = newsTabWidgetSplitter_->sizes();
  sizes.insert(0, sizes.takeAt(0) - RESIZESTEP);
  newsTabWidgetSplitter_->setSizes(sizes);
}

void NewsTabWidget::increaseNewsList()
{
  if (type_ == TAB_WEB) return;

  QList <int> sizes = newsTabWidgetSplitter_->sizes();
  sizes.insert(0, sizes.takeAt(0) + RESIZESTEP);
  newsTabWidgetSplitter_->setSizes(sizes);
}

/** @brief Поиск непрочитанной новости
 * @param next Условие поиска: true - ищет следующую, иначе предыдущую
 *----------------------------------------------------------------------------*/
int NewsTabWidget::findUnreadNews(bool next)
{
  int newsRow = -1;

  int newsRowCur = newsView_->currentIndex().row();
  QModelIndex index;
  QModelIndexList indexList;
  if (next) {
    index = newsModel_->index(newsRowCur+1, newsModel_->fieldIndex("read"));
    indexList = newsModel_->match(index, Qt::EditRole, 0);
    if (indexList.isEmpty()) {
      index = newsModel_->index(0, newsModel_->fieldIndex("read"));
      indexList = newsModel_->match(index, Qt::EditRole, 0);
    }
  } else {
    index = newsModel_->index(newsRowCur, newsModel_->fieldIndex("read"));
    indexList = newsModel_->match(index, Qt::EditRole, 0, -1);
  }
  if (!indexList.isEmpty()) newsRow = indexList.last().row();

  return newsRow;
}

/** @brief Установка текста вкладки
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setTextTab(const QString &text, int width)
{
  QString textTab = newsTextTitle_->fontMetrics().elidedText(
        text, Qt::ElideRight, width);
  newsTextTitle_->setText(textTab);
  newsTitleLabel_->setToolTip(text);

  emit signalSetTextTab(text, this);
}

/** @brief Поделиться новостью
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotShareNews(QAction *action)
{
  if (type_ == TAB_WEB) return;

  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  int cnt = indexes.count();
  if (cnt == 0) return;

  for (int i = cnt-1; i >= 0; --i) {
    QModelIndex index = indexes.at(i);
    QString title = newsModel_->record(
          index.row()).field("title").value().toString();
    QString linkString = newsModel_->record(
          index.row()).field("link_href").value().toString();
    if (linkString.isEmpty())
      linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();

    QUrl url;
    if (action->objectName() == "emailShareAct") {
      url.setUrl("mailto:");
      url.addQueryItem("subject", title);
      url.addQueryItem("body", linkString.simplified());
      openUrl(url);
    } else {
      if (action->objectName() == "evernoteShareAct") {
        url.setUrl("http://www.evernote.com/clip.action");
        url.addQueryItem("url", linkString.simplified());
        url.addQueryItem("title", title);
      } else if (action->objectName() == "facebookShareAct") {
        url.setUrl("http://www.facebook.com/sharer.php");
        url.addQueryItem("u", linkString.simplified());
        url.addQueryItem("t", title);
      } else if (action->objectName() == "livejournalShareAct") {
        url.setUrl("http://www.livejournal.com/update.bml");
        url.addQueryItem("event", linkString.simplified());
        url.addQueryItem("subject", title);
      } else if (action->objectName() == "pocketShareAct") {
        url.setUrl("https://getpocket.com/save");
        url.addQueryItem("url", linkString.simplified());
        url.addQueryItem("title", title);
      } else if (action->objectName() == "twitterShareAct") {
        url.setUrl("https://twitter.com/share");
        url.addQueryItem("url", linkString.simplified());
        url.addQueryItem("text", title);
      } else if (action->objectName() == "vkShareAct") {
        url.setUrl("http://vk.com/share.php");
        url.addQueryItem("url", linkString.simplified());
        url.addQueryItem("title", title);
        url.addQueryItem("description", "");
        url.addQueryItem("image", "");
      }

      if (rsslisting_->externalBrowserOn_ <= 0) {
        rsslisting_->openNewsTab_ = 1;
        QWebPage *webPage = rsslisting_->createWebTab();
        qobject_cast<WebView*>(webPage->view())->load(url);
      } else openUrl(url);
    }
  }
}
