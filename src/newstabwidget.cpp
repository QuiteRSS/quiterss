/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "newstabwidget.h"

#include "mainapplication.h"
#include "adblockicon.h"
#include "settings.h"
#include "webpage.h"

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif
#include <qzregexp.h>

NewsTabWidget::NewsTabWidget(QWidget *parent, TabType type, int feedId, int feedParId)
  : QWidget(parent)
  , type_(type)
  , feedId_(feedId)
  , feedParId_(feedParId)
  , currentNewsIdOld(-1)
  , autoLoadImages_(true)
{
  mainWindow_ = mainApp->mainWindow();
  db_ = QSqlDatabase::database();
  feedsView_ = mainWindow_->feedsView_;
  feedsModel_ = mainWindow_->feedsModel_;
  feedsProxyModel_ = mainWindow_->feedsProxyModel_;

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
  newsTitleLabel_->setLayout(newsTitleLayout);
  newsTitleLabel_->setVisible(false);

  Settings settings;
  bool showCloseButtonTab = settings.value("Settings/showCloseButtonTab", true).toBool();
  if (!showCloseButtonTab) {
    closeButton_->hide();
    newsTitleLabel_->setFixedWidth(MAX_TAB_WIDTH-15);
  } else {
    newsTitleLabel_->setFixedWidth(MAX_TAB_WIDTH);
  }

  if (type_ != TabTypeDownloads) {
    if (type_ != TabTypeWeb) {
      createNewsList();
    } else {
      autoLoadImages_ = mainWindow_->autoLoadImages_;
    }
    createWebWidget();

    if (type_ != TabTypeWeb) {
      newsTabWidgetSplitter_ = new QSplitter(this);
      newsTabWidgetSplitter_->setObjectName("newsTabWidgetSplitter");

      if ((mainWindow_->browserPosition_ == TOP_POSITION) ||
          (mainWindow_->browserPosition_ == LEFT_POSITION)) {
        newsTabWidgetSplitter_->addWidget(webWidget_);
        newsTabWidgetSplitter_->addWidget(newsWidget_);
      } else {
        newsTabWidgetSplitter_->addWidget(newsWidget_);
        newsTabWidgetSplitter_->addWidget(webWidget_);
      }
    }
  }

  QVBoxLayout *layout = new QVBoxLayout();
  layout->setMargin(0);
  layout->setSpacing(0);
  if (type_ == TabTypeDownloads)
    layout->addWidget(mainApp->downloadManager());
  else if (type_ != TabTypeWeb)
    layout->addWidget(newsTabWidgetSplitter_);
  else
    layout->addWidget(webWidget_);
  setLayout(layout);

  if (type_ < TabTypeWeb) {
    newsTabWidgetSplitter_->setHandleWidth(1);

    if ((mainWindow_->browserPosition_ == RIGHT_POSITION) ||
        (mainWindow_->browserPosition_ == LEFT_POSITION)) {
      newsTabWidgetSplitter_->setOrientation(Qt::Horizontal);
      newsTabWidgetSplitter_->setStyleSheet(
            QString("QSplitter::handle {background: qlineargradient("
                    "x1: 0, y1: 0, x2: 0, y2: 1,"
                    "stop: 0 %1, stop: 0.07 %2);}").
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
          mainWindow_, SLOT(setTextTitle(QString,NewsTabWidget*)));
}

NewsTabWidget::~NewsTabWidget()
{
  if (type_ == TabTypeDownloads) {
    mainApp->downloadManager()->hide();
    mainApp->downloadManager()->setParent(mainWindow_);
  }
}

void NewsTabWidget::disconnectObjects()
{
  disconnect(this);
  if (type_ != TabTypeDownloads) {
    webView_->disconnect(this);
    webView_->disconnectObjects();
    qobject_cast<WebPage*>(webView_->page())->disconnectObjects();
  }
}

/** @brief Create news list with all related panels
 *----------------------------------------------------------------------------*/
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

  newsHeader_->init();

  newsToolBar_ = new QToolBar(this);
  newsToolBar_->setObjectName("newsToolBar");
  newsToolBar_->setStyleSheet("QToolBar { border: none; padding: 0px; }");

  Settings settings;
  QString actionListStr = "markNewsRead,markAllNewsRead,Separator,markStarAct,"
                          "newsLabelAction,shareMenuAct,openInExternalBrowserAct,Separator,"
                          "nextUnreadNewsAct,prevUnreadNewsAct,Separator,"
                          "newsFilter,Separator,deleteNewsAct";
  QString str = settings.value("Settings/newsToolBar", actionListStr).toString();

  foreach (QString actionStr, str.split(",", QString::SkipEmptyParts)) {
    if (actionStr == "Separator") {
      newsToolBar_->addSeparator();
    } else {
      QListIterator<QAction *> iter(mainWindow_->actions());
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
  newsToolBar_->addAction(mainWindow_->restoreNewsAct_);

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
  if (!mainWindow_->newsToolbarToggle_->isChecked())
    newsPanelWidget_->hide();

  QVBoxLayout *newsLayout = new QVBoxLayout();
  newsLayout->setMargin(0);
  newsLayout->setSpacing(0);
  newsLayout->addWidget(newsPanelWidget_);
  newsLayout->addWidget(newsView_);

  newsWidget_ = new QWidget(this);
  newsWidget_->setLayout(newsLayout);

  markNewsReadTimer_ = new QTimer(this);

  QFile htmlFile;
  htmlFile.setFileName(":/html/newspaper_head");
  htmlFile.open(QFile::ReadOnly);
  newspaperHeadHtml_ = QString::fromUtf8(htmlFile.readAll());
  htmlFile.close();
  htmlFile.setFileName(":/html/newspaper_description");
  htmlFile.open(QFile::ReadOnly);
  newspaperHtml_ = QString::fromUtf8(htmlFile.readAll());
  htmlFile.close();
  htmlFile.setFileName(":/html/newspaper_description_rtl");
  htmlFile.open(QFile::ReadOnly);
  newspaperHtmlRtl_ = QString::fromUtf8(htmlFile.readAll());
  htmlFile.close();
  htmlFile.setFileName(":/html/description");
  htmlFile.open(QFile::ReadOnly);
  htmlString_ = QString::fromUtf8(htmlFile.readAll());
  htmlFile.close();
  htmlFile.setFileName(":/html/description_rtl");
  htmlFile.open(QFile::ReadOnly);
  htmlRtlString_ = QString::fromUtf8(htmlFile.readAll());
  htmlFile.close();

  connect(newsView_, SIGNAL(pressed(QModelIndex)),
          this, SLOT(slotNewsViewClicked(QModelIndex)));
  connect(newsView_, SIGNAL(pressKeyUp(QModelIndex)),
          this, SLOT(slotNewsUpPressed(QModelIndex)));
  connect(newsView_, SIGNAL(pressKeyDown(QModelIndex)),
          this, SLOT(slotNewsDownPressed(QModelIndex)));
  connect(newsView_, SIGNAL(pressKeyHome(QModelIndex)),
          this, SLOT(slotNewsHomePressed(QModelIndex)));
  connect(newsView_, SIGNAL(pressKeyEnd(QModelIndex)),
          this, SLOT(slotNewsEndPressed(QModelIndex)));
  connect(newsView_, SIGNAL(pressKeyPageUp(QModelIndex)),
          this, SLOT(slotNewsPageUpPressed(QModelIndex)));
  connect(newsView_, SIGNAL(pressKeyPageDown(QModelIndex)),
          this, SLOT(slotNewsPageDownPressed(QModelIndex)));
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
          this, SLOT(slotMarkReadTimeout()));
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

  connect(mainWindow_->newsToolbarToggle_, SIGNAL(toggled(bool)),
          newsPanelWidget_, SLOT(setVisible(bool)));
}

/** @brief Call context menu of selected news in news list
 *----------------------------------------------------------------------------*/
void NewsTabWidget::showContextMenuNews(const QPoint &pos)
{
  if (!newsView_->currentIndex().isValid()) return;

  QMenu menu;
  menu.addAction(mainWindow_->restoreNewsAct_);
  menu.addSeparator();
  menu.addAction(mainWindow_->openInBrowserAct_);
  menu.addAction(mainWindow_->openInExternalBrowserAct_);
  menu.addAction(mainWindow_->openNewsNewTabAct_);
  menu.addSeparator();
  menu.addAction(mainWindow_->markNewsRead_);
  menu.addAction(mainWindow_->markAllNewsRead_);
  menu.addSeparator();
  menu.addAction(mainWindow_->markStarAct_);
  menu.addAction(mainWindow_->newsLabelMenuAction_);
  menu.addAction(mainWindow_->shareMenuAct_);
  menu.addAction(mainWindow_->copyLinkAct_);
  menu.addSeparator();
  menu.addAction(mainWindow_->updateFeedAct_);
  menu.addSeparator();
  menu.addAction(mainWindow_->deleteNewsAct_);
  menu.addAction(mainWindow_->deleteAllNewsAct_);

  menu.exec(newsView_->viewport()->mapToGlobal(pos));
}

/** @brief Create web-widget and control panel
 *----------------------------------------------------------------------------*/
void NewsTabWidget::createWebWidget()
{
  webView_ = new WebView(this);

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
  webToolBar_ = new QToolBar(this);
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

  webToolBar_->addAction(mainApp->mainWindow()->shareMenuAct_);

  webExternalBrowserAct_ = new QAction(this);
  webExternalBrowserAct_->setIcon(QIcon(":/images/openBrowser"));
  webToolBar_->addAction(webExternalBrowserAct_);

  locationBar_ = new LocationBar(webView_, this);

  QHBoxLayout *webControlPanelLayout = new QHBoxLayout();
  webControlPanelLayout->setMargin(2);
  webControlPanelLayout->setSpacing(2);
  webControlPanelLayout->addWidget(webToolBar_);
  webControlPanelLayout->addWidget(locationBar_, 1);

  webControlPanel_ = new QWidget(this);
  webControlPanel_->setObjectName("webControlPanel_");
  webControlPanel_->setStyleSheet(
        QString("#webControlPanel_ {border-bottom: 1px solid %1;}").
        arg(qApp->palette().color(QPalette::Dark).name()));
  webControlPanel_->setLayout(webControlPanelLayout);

  if (type_ != TabTypeWeb)
    setWebToolbarVisible(false, false);
  else
    setWebToolbarVisible(true, false);

  //! Create web layout
  QVBoxLayout *webLayout = new QVBoxLayout();
  webLayout->setMargin(0);
  webLayout->setSpacing(0);
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

  connect(webHomePageAct_, SIGNAL(triggered()),
          this, SLOT(webHomePage()));
  connect(webExternalBrowserAct_, SIGNAL(triggered()),
          this, SLOT(openPageInExternalBrowser()));
  connect(urlExternalBrowserAct_, SIGNAL(triggered()),
          this, SLOT(openUrlInExternalBrowser()));
  connect(this, SIGNAL(signalSetHtmlWebView(QString,QUrl)),
          SLOT(slotSetHtmlWebView(QString,QUrl)), Qt::QueuedConnection);
  connect(webView_, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
  connect(webView_, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished(bool)));
  connect(webView_, SIGNAL(linkClicked(QUrl)), this, SLOT(slotLinkClicked(QUrl)));
  connect(webView_->page(), SIGNAL(linkHovered(QString,QString,QString)),
          this, SLOT(slotLinkHovered(QString,QString,QString)));
  connect(webView_, SIGNAL(loadProgress(int)), this, SLOT(slotSetValue(int)), Qt::QueuedConnection);

  connect(webView_, SIGNAL(titleChanged(QString)),
          this, SLOT(webTitleChanged(QString)));
  connect(webView_->page()->action(QWebPage::OpenLink), SIGNAL(triggered()),
          this, SLOT(openLink()));
  connect(webView_->page()->action(QWebPage::OpenLinkInNewWindow), SIGNAL(triggered()),
          this, SLOT(openLinkInNewTab()));

  connect(webView_, SIGNAL(showContextMenu(QPoint)),
          this, SLOT(showContextWebPage(QPoint)), Qt::QueuedConnection);
  connect(webView_, SIGNAL(signalGoHome()),
          this, SLOT(webHomePage()));

  connect(mainWindow_->autoLoadImagesToggle_, SIGNAL(triggered()),
          this, SLOT(setAutoLoadImages()));
  connect(mainWindow_->browserToolbarToggle_, SIGNAL(triggered()),
          this, SLOT(setWebToolbarVisible()));

  connect(locationBar_, SIGNAL(returnPressed()),this, SLOT(slotUrlEnter()));
  connect(webView_, SIGNAL(rssChanged(bool)), locationBar_, SLOT(showRssIcon(bool)));
  connect(webView_, SIGNAL(urlChanged(QUrl)),
          this, SLOT(slotUrlChanged(QUrl)), Qt::QueuedConnection);
}

/** @brief Read settings from ini-file
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setSettings(bool init, bool newTab)
{
  Settings settings;

  if (type_ == TabTypeDownloads) return;

  if (newTab) {
    if (type_ < TabTypeWeb) {
      newsTabWidgetSplitter_->restoreState(settings.value("NewsTabSplitterState").toByteArray());
      QString iconStr = settings.value("Settings/newsToolBarIconSize", "toolBarIconSmall_").toString();
      mainWindow_->setToolBarIconSize(newsToolBar_, iconStr);

      newsView_->setFont(
            QFont(mainWindow_->newsListFontFamily_, mainWindow_->newsListFontSize_));
      newsModel_->formatDate_ = mainWindow_->formatDate_;
      newsModel_->formatTime_ = mainWindow_->formatTime_;
      newsModel_->simplifiedDateTime_ = mainWindow_->simplifiedDateTime_;

      newsModel_->textColor_ = mainWindow_->newsListTextColor_;
      newsView_->setStyleSheet(QString("#newsView_ {background: %1;}").arg(mainWindow_->newsListBackgroundColor_));
      newsModel_->newNewsTextColor_ = mainWindow_->newNewsTextColor_;
      newsModel_->unreadNewsTextColor_ = mainWindow_->unreadNewsTextColor_;
      newsModel_->focusedNewsTextColor_ = mainWindow_->focusedNewsTextColor_;
      newsModel_->focusedNewsBGColor_ = mainWindow_->focusedNewsBGColor_;

      QString styleSheetNews = settings.value("Settings/styleSheetNews",
                                              mainApp->styleSheetNewsDefaultFile()).toString();
      QFile file(styleSheetNews);
      if (!file.open(QFile::ReadOnly)) {
        file.setFileName(":/style/newsStyle");
        file.open(QFile::ReadOnly);
      }
      cssString_ = QString::fromUtf8(file.readAll()).
          arg(mainWindow_->newsTextFontFamily_).
          arg(mainWindow_->newsTextFontSize_).
          arg(mainWindow_->newsTitleFontFamily_).
          arg(mainWindow_->newsTitleFontSize_).
          arg(0).
          arg(qApp->palette().color(QPalette::Dark).name()). // color separator
          arg(mainWindow_->newsBackgroundColor_). // news background
          arg(mainWindow_->newsTitleBackgroundColor_). // title background
          arg(mainWindow_->linkColor_). // link color
          arg(mainWindow_->titleColor_). // title color
          arg(mainWindow_->dateColor_). // date color
          arg(mainWindow_->authorColor_). // author color
          arg(mainWindow_->newsTextColor_); // text color
      file.close();

      file.setFileName(":/html/audioplayer");
      file.open(QFile::ReadOnly);
      audioPlayerHtml_ = QString::fromUtf8(file.readAll());
      file.close();

      file.setFileName(":/html/videoplayer");
      file.open(QFile::ReadOnly);
      videoPlayerHtml_ = QString::fromUtf8(file.readAll());
      file.close();
    }

    if (mainWindow_->externalBrowserOn_ <= 0) {
      webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    } else {
      webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    }

    webView_->page()->action(QWebPage::Back)->setShortcut(mainWindow_->backWebPageAct_->shortcut());
    webView_->page()->action(QWebPage::Forward)->setShortcut(mainWindow_->forwardWebPageAct_->shortcut());
    webView_->page()->action(QWebPage::Reload)->setShortcut(mainWindow_->reloadWebPageAct_->shortcut());

    QWebSettings::setObjectCacheCapacities(0, 0, 0);
  }

  QModelIndex feedIndex = feedsModel_->indexById(feedId_);

  if (init) {
    QWebSettings::clearMemoryCaches();

    if (type_ == TabTypeFeed) {
      int displayEmbeddedImages = feedsModel_->dataField(feedIndex, "displayEmbeddedImages").toInt();
      if (displayEmbeddedImages == 2) {
        autoLoadImages_ = true;
      } else if (displayEmbeddedImages == 1) {
        autoLoadImages_ = mainWindow_->autoLoadImages_;
      } else {
        autoLoadImages_ = false;
      }
    } else {
      autoLoadImages_ = mainWindow_->autoLoadImages_;
    }
    webView_->settings()->setAttribute(QWebSettings::AutoLoadImages, autoLoadImages_);

    webView_->setZoomFactor(qreal(mainWindow_->defaultZoomPages_)/100.0);
  }
  setAutoLoadImages(false);

  if (type_ == TabTypeFeed) {
    int javaScriptEnable = feedsModel_->dataField(feedIndex, "javaScriptEnable").toInt();
    if (javaScriptEnable == 2) {
      webView_->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    } else if (javaScriptEnable == 1) {
      webView_->settings()->setAttribute(QWebSettings::JavascriptEnabled, mainWindow_->javaScriptEnable_);
    } else if (javaScriptEnable == 0) {
      webView_->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
    }

    int layoutDirection = feedsModel_->dataField(feedIndex, "layoutDirection").toInt();
    if (!layoutDirection) {
      newsView_->setLayoutDirection(Qt::LeftToRight);
    } else {
      newsView_->setLayoutDirection(Qt::RightToLeft);
    }
  } else {
    webView_->settings()->setAttribute(QWebSettings::JavascriptEnabled, mainWindow_->javaScriptEnable_);
  }

  if (type_ < TabTypeWeb) {
    newsView_->setAlternatingRowColors(mainWindow_->alternatingRowColorsNews_);

    QPalette palette = newsView_->palette();
    palette.setColor(QPalette::AlternateBase, mainWindow_->alternatingRowColors_);
    newsView_->setPalette(palette);

    if (!newTab)
      newsModel_->setFilter("feedId=-1");
    newsHeader_->setColumns(feedIndex);
    mainWindow_->slotUpdateStatus(feedId_, false);
    mainWindow_->newsFilter_->setEnabled(type_ == TabTypeFeed);
    separatorRAct_->setVisible(type_ == TabTypeDel);
    mainWindow_->restoreNewsAct_->setVisible(type_ == TabTypeDel);

    switch (mainWindow_->newsLayout_) {
    case 1:
      newsWidget_->setVisible(false);
      break;
    default:
      newsWidget_->setVisible(true);
    }
  }
}

/** @brief Reload translation
 *----------------------------------------------------------------------------*/
void NewsTabWidget::retranslateStrings() {
  if (type_ != TabTypeDownloads) {
    webViewProgress_->setFormat(tr("Loading... (%p%)"));

    webHomePageAct_->setText(tr("Home"));
    webExternalBrowserAct_->setText(tr("Open Page in External Browser"));
    urlExternalBrowserAct_->setText(tr("Open Link in External Browser"));

    if (type_ != TabTypeWeb) {
      findText_->retranslateStrings();
      newsHeader_->retranslateStrings();
    }

    if (mainWindow_->currentNewsTab == this) {
      if (autoLoadImages_) {
        mainWindow_->autoLoadImagesToggle_->setText(tr("Load Images"));
        mainWindow_->autoLoadImagesToggle_->setToolTip(tr("Auto Load Images to News View"));
      } else {
        mainWindow_->autoLoadImagesToggle_->setText(tr("No Load Images"));
        mainWindow_->autoLoadImagesToggle_->setToolTip(tr("No Load Images to News View"));
      }
    }
  }

  closeButton_->setToolTip(tr("Close Tab"));
}

void NewsTabWidget::setAutoLoadImages(bool apply)
{
  if (type_ == NewsTabWidget::TabTypeDownloads) return;
  if (mainWindow_->currentNewsTab != this) return;

  if (apply)
    autoLoadImages_ = !autoLoadImages_;

  if (autoLoadImages_) {
    mainWindow_->autoLoadImagesToggle_->setText(tr("Load Images"));
    mainWindow_->autoLoadImagesToggle_->setToolTip(tr("Auto Load Images in News View"));
    mainWindow_->autoLoadImagesToggle_->setIcon(QIcon(":/images/imagesOn"));
  } else {
    mainWindow_->autoLoadImagesToggle_->setText(tr("Don't Load Images"));
    mainWindow_->autoLoadImagesToggle_->setToolTip(tr("Don't Load Images in News View"));
    mainWindow_->autoLoadImagesToggle_->setIcon(QIcon(":/images/imagesOff"));
  }

  if (apply) {
    webView_->settings()->setAttribute(QWebSettings::AutoLoadImages, autoLoadImages_);
    if (autoLoadImages_) {
      if ((webView_->title() == "news_descriptions") &&
          (type_ == NewsTabWidget::TabTypeFeed)) {
        switch (mainWindow_->newsLayout_) {
        case 1:
          loadNewspaper();
          break;
        default:
          updateWebView(newsView_->currentIndex());
        }
      } else {
        webView_->reload();
      }
    }
  }
}

/** @brief Process mouse click in news list
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsViewClicked(QModelIndex index)
{
  slotNewsViewSelected(index);
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotNewsViewSelected(QModelIndex index, bool clicked)
{
  if (mainWindow_->newsLayout_ == 1) return;

  int newsId = newsModel_->dataField(index.row(), "id").toInt();
  if (mainWindow_->markNewsReadOn_ && mainWindow_->markPrevNewsRead_ &&
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
    return;
  }

  if (!((newsId == currentNewsIdOld) &&
        newsModel_->dataField(index.row(), "read").toInt() >= 1) ||
      clicked) {
    markNewsReadTimer_->stop();
    if (mainWindow_->markNewsReadOn_ && mainWindow_->markCurNewsRead_) {
      if (mainWindow_->markNewsReadTime_ == 0) {
        slotSetItemRead(index, 1);
      } else {
        markNewsReadTimer_->start(mainWindow_->markNewsReadTime_*1000);
      }
    }

    if (type_ == TabTypeFeed) {
      // Write current news to feed
      QString qStr = QString("UPDATE feeds SET currentNews='%1' WHERE id=='%2'").
          arg(newsId).arg(feedId_);
      mainApp->sqlQueryExec(qStr);

      QModelIndex feedIndex = feedsModel_->indexById(feedId_);
      feedsModel_->setData(feedsModel_->indexSibling(feedIndex, "currentNews"), newsId);
    } else if (type_ == TabTypeLabel) {
      QString qStr = QString("UPDATE labels SET currentNews='%1' WHERE id=='%2'").
          arg(newsId).
          arg(mainWindow_->categoriesTree_->currentItem()->text(2).toInt());
      mainApp->sqlQueryExec(qStr);

      mainWindow_->categoriesTree_->currentItem()->setText(3, QString::number(newsId));
    }

    updateWebView(index);
    mainWindow_->statusBar()->showMessage(linkNewsString_, 3000);
  }
  currentNewsIdOld = newsId;
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotNewsViewDoubleClicked(QModelIndex index)
{
  if (!index.isValid()) return;

  QUrl url = QUrl::fromEncoded(getLinkNews(index.row()).toUtf8());
  slotLinkClicked(url);
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotNewsMiddleClicked(QModelIndex index)
{
  if (!index.isValid()) return;

  if (mainWindow_->markNewsReadOn_ && mainWindow_->markCurNewsRead_)
    slotSetItemRead(index, 1);

  if (QApplication::keyboardModifiers() == Qt::NoModifier) {
    webView_->buttonClick_ = MIDDLE_BUTTON;
  } else if (QApplication::keyboardModifiers() == Qt::AltModifier) {
    webView_->buttonClick_ = LEFT_BUTTON_ALT;
  } else {
    webView_->buttonClick_ = MIDDLE_BUTTON_MOD;
  }

  QUrl url = QUrl::fromEncoded(getLinkNews(index.row()).toUtf8());
  slotLinkClicked(url);
}

/** @brief Process pressing UP-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsUpPressed(QModelIndex index)
{
  if (type_ >= TabTypeWeb) return;

  int row;
  if (!index.isValid()) {
    if (!newsView_->currentIndex().isValid())
      row = 0;
    else
      row = newsView_->currentIndex().row() - 1;
    if (row < 0)
      return;
    index = newsModel_->index(row, newsModel_->fieldIndex("title"));
    newsView_->setCurrentIndex(index);
  } else {
    row = index.row();
  }

  int value = newsView_->verticalScrollBar()->value();
  int pageStep = newsView_->verticalScrollBar()->pageStep();
  if (row < (value + pageStep/2))
    newsView_->verticalScrollBar()->setValue(row - pageStep/2);

  slotNewsViewSelected(index);
}

/** @brief Process pressing DOWN-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsDownPressed(QModelIndex index)
{
  if (type_ >= TabTypeWeb) return;

  int row;
  if (!index.isValid()) {
    if (!newsView_->currentIndex().isValid())
      row = 0;
    else
      row = newsView_->currentIndex().row() + 1;
    if (row >= newsModel_->rowCount())
      return;
    index = newsModel_->index(row, newsModel_->fieldIndex("title"));
    newsView_->setCurrentIndex(index);
  } else {
    row = index.row();
  }

  int value = newsView_->verticalScrollBar()->value();
  int pageStep = newsView_->verticalScrollBar()->pageStep();
  if (row > (value + pageStep/2))
    newsView_->verticalScrollBar()->setValue(row - pageStep/2);
  slotNewsViewSelected(index);
}

/** @brief Process pressing HOME-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsHomePressed(QModelIndex index)
{
  slotNewsViewSelected(index);
}

/** @brief Process pressing END-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsEndPressed(QModelIndex index)
{
  slotNewsViewSelected(index);
}

/** @brief Process pressing PageUp-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsPageUpPressed(QModelIndex index)
{
  int row;
  if (!index.isValid()) {
    if (!newsView_->currentIndex().isValid())
      row = 0;
    else
      row = newsView_->currentIndex().row() - newsView_->verticalScrollBar()->pageStep();
    if (row < 0)
      row = 0;
    index = newsModel_->index(row, newsModel_->fieldIndex("title"));
    newsView_->setCurrentIndex(index);
  }

  slotNewsViewSelected(index);
}

/** @brief Process pressing PageDown-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsPageDownPressed(QModelIndex index)
{
  int row;
  if (!index.isValid()) {
    if (!newsView_->currentIndex().isValid())
      row = 0;
    else
      row = newsView_->currentIndex().row() + newsView_->verticalScrollBar()->pageStep();
    if (row >= newsModel_->rowCount())
      row = newsModel_->rowCount()-1;
    index = newsModel_->index(row, newsModel_->fieldIndex("title"));
    newsView_->setCurrentIndex(index);
  }

  slotNewsViewSelected(index);
}

/** @brief Mark news Read
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotSetItemRead(QModelIndex index, int read)
{
  markNewsReadTimer_->stop();
  if (!index.isValid() || (newsModel_->rowCount() == 0)) return;

  bool changed = false;
  int newsId = newsModel_->dataField(index.row(), "id").toInt();

  if (read == 1) {
    if (newsModel_->dataField(index.row(), "new").toInt() == 1) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("new")),
            0);
      mainApp->sqlQueryExec(QString("UPDATE news SET new=0 WHERE id=='%1'").arg(newsId));
    }
    if (newsModel_->dataField(index.row(), "read").toInt() == 0) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
            1);
      mainApp->sqlQueryExec(QString("UPDATE news SET read=1 WHERE id=='%1'").arg(newsId));
      changed = true;
    }
  } else {
    if (newsModel_->dataField(index.row(), "read").toInt() != 0) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
            0);
      mainApp->sqlQueryExec(QString("UPDATE news SET read=0 WHERE id=='%1'").arg(newsId));
      changed = true;
    }
  }

  if (changed) {
    newsView_->viewport()->update();
    int feedId = newsModel_->dataField(index.row(), "feedId").toInt();
    mainWindow_->slotUpdateStatus(feedId);
    mainWindow_->recountCategoryCounts();
  }
}

/** @brief Mark news Star
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotSetItemStar(QModelIndex index, int starred)
{
  if (!index.isValid()) return;

  newsModel_->setData(index, starred);

  int newsId = newsModel_->dataField(index.row(), "id").toInt();
  mainApp->sqlQueryExec(QString("UPDATE news SET starred='%1' WHERE id=='%2'").
                        arg(starred).arg(newsId));
  mainWindow_->recountCategoryCounts();
}

void NewsTabWidget::slotMarkReadTimeout()
{
  slotSetItemRead(newsView_->currentIndex(), 1);
}

/** @brief Mark selected news Read
 *----------------------------------------------------------------------------*/
void NewsTabWidget::markNewsRead()
{
  if (type_ >= TabTypeWeb) return;
  markNewsReadTimer_->stop();

  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  int cnt = indexes.count();
  if (cnt == 0) return;

  if (cnt == 1) {
    curIndex = indexes.at(0);
    if (newsModel_->dataField(curIndex.row(), "read").toInt() == 0) {
      slotSetItemRead(curIndex, 1);
    } else {
      slotSetItemRead(curIndex, 0);
    }
  } else {
    QStringList feedIdList;

    bool markRead = false;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      if (newsModel_->dataField(curIndex.row(), "read").toInt() == 0) {
        markRead = true;
        break;
      }
    }

    db_.transaction();
    QSqlQuery q;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      newsModel_->setData(
            newsModel_->index(curIndex.row(), newsModel_->fieldIndex("new")),
            0);
      newsModel_->setData(
            newsModel_->index(curIndex.row(), newsModel_->fieldIndex("read")),
            markRead);

      int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
      q.exec(QString("UPDATE news SET new=0, read='%1' WHERE id=='%2'").
             arg(markRead).arg(newsId));
      QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
      if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
    }
    db_.commit();

    foreach (QString feedId, feedIdList) {
      mainWindow_->slotUpdateStatus(feedId.toInt());
    }
    mainWindow_->recountCategoryCounts();
    newsView_->viewport()->update();
  }
}

/** @brief Mark all news of the feed Read
 *----------------------------------------------------------------------------*/
void NewsTabWidget::markAllNewsRead()
{
  if (type_ >= TabTypeWeb) return;
  markNewsReadTimer_->stop();

  int cnt = newsModel_->rowCount();
  if (cnt == 0) return;

  QStringList feedIdList;

  db_.transaction();
  QSqlQuery q;
  for (int i = cnt-1; i >= 0; --i) {
    int newsId = newsModel_->dataField(i, "id").toInt();
    q.exec(QString("UPDATE news SET read=1 WHERE id=='%1' AND read=0").arg(newsId));
    q.exec(QString("UPDATE news SET new=0 WHERE id=='%1' AND new=1").arg(newsId));

    QString feedId = newsModel_->dataField(i, "feedId").toString();
    if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
  }
  db_.commit();

  int currentRow = newsView_->currentIndex().row();

  newsModel_->select();

  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  loadNewspaper(RefreshWithPos);

  newsView_->setCurrentIndex(newsModel_->index(currentRow, newsModel_->fieldIndex("title")));

  foreach (QString feedId, feedIdList) {
    mainWindow_->slotUpdateStatus(feedId.toInt());
  }
  mainWindow_->recountCategoryCounts();
}

/** @brief Mark selected news Starred
 *----------------------------------------------------------------------------*/
void NewsTabWidget::markNewsStar()
{
  if (type_ >= TabTypeWeb) return;

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
      curIndex = indexes.at(i);
      newsModel_->setData(curIndex, markStar);

      int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
      QSqlQuery q;
      q.exec(QString("UPDATE news SET starred='%1' WHERE id=='%2'").
             arg(markStar).arg(newsId));
    }
    db_.commit();

    mainWindow_->recountCategoryCounts();
  }
}

/** @brief Delete selected news
 *----------------------------------------------------------------------------*/
void NewsTabWidget::deleteNews()
{
  if (type_ >= TabTypeWeb) return;

  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(newsModel_->fieldIndex("deleted"));

  int cnt = indexes.count();
  if (cnt == 0) return;

  QStringList feedIdList;

  if (type_ != TabTypeDel) {
    if (cnt == 1) {
      curIndex = indexes.at(0);
      if (newsModel_->dataField(curIndex.row(), "starred").toInt() &&
          mainWindow_->notDeleteStarred_)
        return;
      QString labelStr = newsModel_->dataField(curIndex.row(), "label").toString();
      if (!(labelStr.isEmpty() || (labelStr == ",")) && mainWindow_->notDeleteLabeled_)
        return;

      slotSetItemRead(curIndex, 1);

      newsModel_->setData(curIndex, 1);
      newsModel_->setData(newsModel_->index(curIndex.row(), newsModel_->fieldIndex("deleteDate")),
                          QDateTime::currentDateTime().toString(Qt::ISODate));

      QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
      if (!feedIdList.contains(feedId)) feedIdList.append(feedId);

      newsModel_->submitAll();
    } else {
      db_.transaction();
      QSqlQuery q;
      for (int i = cnt-1; i >= 0; --i) {
        curIndex = indexes.at(i);
        if (newsModel_->dataField(curIndex.row(), "starred").toInt() &&
            mainWindow_->notDeleteStarred_)
          continue;
        QString labelStr = newsModel_->dataField(curIndex.row(), "label").toString();
        if (!(labelStr.isEmpty() || (labelStr == ",")) && mainWindow_->notDeleteLabeled_)
          continue;

        int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
        q.exec(QString("UPDATE news SET new=0, read=2, deleted=1, deleteDate='%1' WHERE id=='%2'").
               arg(QDateTime::currentDateTime().toString(Qt::ISODate)).
               arg(newsId));

        QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
        if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
      }
      db_.commit();

      newsModel_->select();
    }
  }
  else {
    db_.transaction();
    QSqlQuery q;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);

      int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
      q.exec(QString("UPDATE news SET description='', content='', received='', "
                     "author_name='', author_uri='', author_email='', "
                     "category='', new='', read='', starred='', label='', "
                     "deleteDate='', feedParentId='', deleted=2 WHERE id=='%1'").
             arg(newsId));

      QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
      if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
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
  else
    curIndex = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("title"));
  newsView_->setCurrentIndex(curIndex);
  slotNewsViewSelected(curIndex);

  foreach (QString feedId, feedIdList) {
    mainWindow_->slotUpdateStatus(feedId.toInt());
  }
  mainWindow_->recountCategoryCounts();
}

/** @brief Delete all news of the feed
 *----------------------------------------------------------------------------*/
void NewsTabWidget::deleteAllNewsList()
{
  if (type_ >= TabTypeWeb) return;

  int cnt = newsModel_->rowCount();
  if (cnt == 0) return;

  QStringList feedIdList;

  db_.transaction();
  QSqlQuery q;
  for (int i = cnt-1; i >= 0; --i) {
    int newsId = newsModel_->dataField(i, "id").toInt();

    if (type_ != TabTypeDel) {
      if (newsModel_->dataField(i, "starred").toInt() &&
          mainWindow_->notDeleteStarred_)
        continue;
      QString labelStr = newsModel_->dataField(i, "label").toString();
      if (!(labelStr.isEmpty() || (labelStr == ",")) && mainWindow_->notDeleteLabeled_)
        continue;

      q.exec(QString("UPDATE news SET new=0, read=2, deleted=1, deleteDate='%1' WHERE id=='%2'").
             arg(QDateTime::currentDateTime().toString(Qt::ISODate)).
             arg(newsId));
    }
    else {
      q.exec(QString("UPDATE news SET description='', content='', received='', "
                     "author_name='', author_uri='', author_email='', "
                     "category='', new='', read='', starred='', label='', "
                     "deleteDate='', feedParentId='', deleted=2 WHERE id=='%1'").
             arg(newsId));
    }

    QString feedId = newsModel_->dataField(i, "feedId").toString();
    if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
  }
  db_.commit();

  newsModel_->select();

  slotNewsViewSelected(QModelIndex());

  foreach (QString feedId, feedIdList) {
    mainWindow_->slotUpdateStatus(feedId.toInt());
  }
  mainWindow_->recountCategoryCounts();
}

/** @brief Restore deleted news
 *----------------------------------------------------------------------------*/
void NewsTabWidget::restoreNews()
{
  if (type_ >= TabTypeWeb) return;

  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(newsModel_->fieldIndex("deleted"));

  int cnt = indexes.count();
  if (cnt == 0) return;

  QStringList feedIdList;

  if (cnt == 1) {
    curIndex = indexes.at(0);
    newsModel_->setData(curIndex, 0);
    newsModel_->setData(newsModel_->index(curIndex.row(), newsModel_->fieldIndex("deleteDate")), "");
    newsModel_->submitAll();

    QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
    if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
  } else {
    db_.transaction();
    QSqlQuery q;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
      q.exec(QString("UPDATE news SET deleted=0, deleteDate='' WHERE id=='%1'").
             arg(newsId));

      QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
      if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
    }
    db_.commit();

    newsModel_->select();
  }

  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  loadNewspaper(RefreshWithPos);

  if (curIndex.row() == newsModel_->rowCount())
    curIndex = newsModel_->index(curIndex.row()-1, newsModel_->fieldIndex("title"));
  else if (curIndex.row() > newsModel_->rowCount())
    curIndex = newsModel_->index(newsModel_->rowCount()-1, newsModel_->fieldIndex("title"));
  else
    curIndex = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("title"));
  newsView_->setCurrentIndex(curIndex);
  slotNewsViewSelected(curIndex);
  mainWindow_->slotUpdateStatus(feedId_);
  mainWindow_->recountCategoryCounts();

  foreach (QString feedId, feedIdList) {
    mainWindow_->slotUpdateStatus(feedId.toInt());
  }
  mainWindow_->recountCategoryCounts();
}

/** @brief Copy news link
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotCopyLinkNews()
{
  if (type_ >= TabTypeWeb) return;

  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  int cnt = indexes.count();
  if (cnt == 0) return;

  QString copyStr;
  for (int i = cnt-1; i >= 0; --i) {
    if (!copyStr.isEmpty()) copyStr.append("\n");
    copyStr.append(getLinkNews(indexes.at(i).row()));
  }

  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setText(copyStr);
}

/** @brief Sort news by Star or Read column
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotSort(int column, int/* order*/)
{
  QString strId;
  if (feedsModel_->isFolder(feedsModel_->indexById(feedId_))) {
    strId = QString("(%1)").arg(mainWindow_->getIdFeedsString(feedId_));
  } else {
    strId = QString("feedId='%1'").arg(feedId_);
  }

  QString qStr;
  if (column == newsModel_->fieldIndex("read")) {
    qStr = QString("UPDATE news SET rights=read WHERE %1").arg(strId);
  }
  else if (column == newsModel_->fieldIndex("starred")) {
    qStr = QString("UPDATE news SET rights=starred WHERE %1").arg(strId);
  }
  else if (column == newsModel_->fieldIndex("rights")) {
    qStr = QString("UPDATE news SET rights = (SELECT text from feeds where id = news.feedId) WHERE %1").arg(strId);
  }

  QSqlQuery q;
  q.exec(qStr);
}

/** @brief Load/Update browser contents
 *----------------------------------------------------------------------------*/
void NewsTabWidget::updateWebView(QModelIndex index)
{
  if (!index.isValid()) {
    hideWebContent();
    return;
  }

  QString newsId = newsModel_->dataField(index.row(), "id").toString();
  linkNewsString_ = getLinkNews(index.row());
  QString linkString = linkNewsString_;
  QUrl newsUrl = QUrl::fromEncoded(linkString.toUtf8());

  bool showDescriptionNews_ = mainWindow_->showDescriptionNews_;
  QModelIndex currentIndex = feedsProxyModel_->mapToSource(feedsView_->currentIndex());
  QVariant displayNews = feedsModel_->dataField(currentIndex, "displayNews");
  QString feedId = newsModel_->dataField(index.row(), "feedId").toString();
  QModelIndex feedIndex = feedsModel_->indexById(feedId.toInt());

  if (!displayNews.toString().isEmpty())
    showDescriptionNews_ = !displayNews.toInt();

  if (!showDescriptionNews_) {
    if (mainWindow_->externalBrowserOn_ <= 0) {
      locationBar_->setText(newsUrl.toString());
      setWebToolbarVisible(true, false);

      webView_->history()->setMaximumItemCount(0);
      webView_->load(newsUrl);
      webView_->history()->setMaximumItemCount(100);
    } else {
      openUrl(newsUrl);
    }
  } else {
    setWebToolbarVisible(false, false);

    QString htmlStr;
    QString content = newsModel_->dataField(index.row(), "content").toString();
    if (!content.contains(QzRegExp("<html(.*)</html>", Qt::CaseInsensitive))) {
      QString description = newsModel_->dataField(index.row(), "description").toString();
      if (content.isEmpty() || (description.length() > content.length())) {
        content = description;
      }

      QString titleString = newsModel_->dataField(index.row(), "title").toString();
      if (!linkString.isEmpty()) {
        titleString = QString("<a href='%1' class='unread'>%2</a>").
            arg(linkString, titleString);
      }

      QDateTime dtLocal;
      QString dateString = newsModel_->dataField(index.row(), "published").toString();
      if (!dateString.isNull()) {
        QDateTime dtLocalTime = QDateTime::currentDateTime();
        QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
        int nTimeShift = dtLocalTime.secsTo(dtUTC);

        QDateTime dt = QDateTime::fromString(dateString, Qt::ISODate);
        dtLocal = dt.addSecs(nTimeShift);
      } else {
        dtLocal = QDateTime::fromString(
              newsModel_->dataField(index.row(), "received").toString(),
              Qt::ISODate);
      }
      if (QDateTime::currentDateTime().date() <= dtLocal.date())
        dateString = dtLocal.toString(mainWindow_->formatTime_);
      else
        dateString = dtLocal.toString(mainWindow_->formatDate_ + " " + mainWindow_->formatTime_);

      // Create author panel from news author
      QString authorString;
      QString authorName = newsModel_->dataField(index.row(), "author_name").toString();
      QString authorEmail = newsModel_->dataField(index.row(), "author_email").toString();
      QString authorUri = newsModel_->dataField(index.row(), "author_uri").toString();

      QzRegExp reg("(^\\S+@\\S+\\.\\S+)", Qt::CaseInsensitive);
      int pos = reg.indexIn(authorName);
      if (pos > -1) {
        authorName.replace(reg.cap(1), QString(" <a href='mailto:%1'>%1</a>").arg(reg.cap(1)));
      }

      authorString = authorName;

      if (!authorEmail.isEmpty())
        authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
      if (!authorUri.isEmpty())
        authorString.append(QString(" <a href='%1'>page</a>"). arg(authorUri));

      // If news author is absent, create author panel from feed author
      // @note(arhohryakov:2012.01.03) Author is got from current feed, because
      //   news is belong to it
      if (authorString.isEmpty()) {
        authorName  = feedsModel_->dataField(feedIndex, "author_name").toString();
        authorEmail = feedsModel_->dataField(feedIndex, "author_email").toString();
        authorUri   = feedsModel_->dataField(feedIndex, "author_uri").toString();

        authorString = authorName;

        if (!authorEmail.isEmpty())
          authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
        if (!authorUri.isEmpty())
          authorString.append(QString(" <a href='%1'>page</a>").arg(authorUri));
      }

      QString commentsStr;
      QString commentsUrl = newsModel_->dataField(index.row(), "comments").toString();

      if (!commentsUrl.isEmpty())
      {
        commentsStr = QString("<a href=\"%1\"> %2</a>").arg(commentsUrl, tr("Comments"));
      }

      QString category = newsModel_->dataField(index.row(), "category").toString();

      if (!authorString.isEmpty())
      {
        authorString = QString(tr("Author: %1")).arg(authorString);

        if (!commentsStr.isEmpty())
        {
          authorString.append(QString(" | %1").arg(commentsStr));
        }
        if (!category.isEmpty())
        {
          authorString.append(QString(" | %1").arg(category));
        }
      }
      else
      {
        if (!commentsStr.isEmpty())
        {
          authorString.append(commentsStr);
        }

        if (!category.isEmpty())
        {
          if (!commentsStr.isEmpty())
          {
            authorString.append(QString(" | %1").arg(category));
          }
          else
          {
            authorString.append(category);
          }
        }
      }

      QString labelsString = getHtmlLabels(index.row());

      authorString.append(QString("<table class=\"labels\" id=\"labels%1\"><tr>%2</tr></table>").
                          arg(newsId).arg(labelsString));

      QString enclosureStr;
      QString enclosureUrl = newsModel_->dataField(index.row(), "enclosure_url").toString();

      if (!enclosureUrl.isEmpty())
      {
        QString type = newsModel_->dataField(index.row(), "enclosure_type").toString();

        if (type.contains("image"))
        {
          if (!content.contains(enclosureUrl) && autoLoadImages_)
          {
            enclosureStr = QString("<IMG SRC=\"%1\" class=\"enclosureImg\"><p>").arg(enclosureUrl);
          }
        }
        else
        {
          if (type.contains("audio"))
          {
            type = tr("audio");
            enclosureStr = audioPlayerHtml_.arg(enclosureUrl);
            enclosureStr.append("<p>");
          }
          else if (type.contains("video"))
          {
            type = tr("video");
            enclosureStr = videoPlayerHtml_.arg(enclosureUrl);
            enclosureStr.append("<p>");
          }
          else
          {
            type = tr("media");
          }

          enclosureStr.append(QString("<a href=\"%1\" class=\"enclosure\"> %2 %3 </a><p>").
                              arg(enclosureUrl, tr("Link to"), type));
        }
      }

      content = enclosureStr + content;

      bool ltr = !feedsModel_->dataField(feedIndex, "layoutDirection").toInt();
      QString cssStr = cssString_.
          arg(ltr ? "left" : "right").  // text-align
          arg(ltr ? "ltr" : "rtl").    // direction
          arg(ltr ? "right" : "left");  // "Date" text-align

      if (!autoLoadImages_) {
        QzRegExp reg("<img[^>]+>", Qt::CaseInsensitive);
        content = content.remove(reg);
      }

      if (ltr)
        htmlStr = htmlString_.arg(cssStr, titleString, dateString, authorString, content);
      else
        htmlStr = htmlRtlString_.arg(cssStr, titleString, dateString, authorString, content);
    } else {
      if (!autoLoadImages_) {
        content = content.remove(QzRegExp("<img[^>]+>", Qt::CaseInsensitive));
      }

      htmlStr = content;
    }

    htmlStr = htmlStr.replace("src=\"//", "src=\"http://");

    QUrl url;
    url.setScheme(newsUrl.scheme());
    url.setHost(newsUrl.host());
    if (url.host().indexOf('.') == -1) {
      QUrl hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();
      url.setHost(hostUrl.host());
    }

    emit signalSetHtmlWebView(htmlStr, url);
  }
}

void NewsTabWidget::loadNewspaper(int refresh)
{
  if (mainWindow_->newsLayout_ != 1) return;
  setWebToolbarVisible(false, false);
  webView_->setUpdatesEnabled(false);

  int sortOrder = newsHeader_->sortIndicatorOrder();
  int scrollBarValue = 0;
  int height = 0;
  if (refresh != RefreshAll) {
    scrollBarValue = webView_->page()->mainFrame()->scrollBarValue(Qt::Vertical);
    height = webView_->page()->mainFrame()->contentsSize().height();
  }
  webView_->settings()->setAttribute(QWebSettings::AutoLoadImages, true);

  QString htmlStr;
  QUrl hostUrl;
  bool ltr = true;

  if (type_ == TabTypeFeed) {
    QModelIndex feedIndex = feedsProxyModel_->mapToSource(feedsView_->currentIndex());
    hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();
    ltr = !feedsModel_->dataField(feedIndex, "layoutDirection").toInt();
  }

  if ((refresh == RefreshAll) || (refresh == RefreshWithPos)) {
    QString cssStr = cssString_.
        arg(ltr ? "left" : "right"). // text-align
        arg(ltr ? "ltr" : "rtl"). // direction
        arg(ltr ? "right" : "left"); // "Date" text-align
    htmlStr = newspaperHeadHtml_.arg(cssStr);

    webView_->setHtml(htmlStr, hostUrl);
  }

  int idx = -1;
  if ((refresh == RefreshInsert) && (sortOrder == Qt::DescendingOrder))
    idx = newsModel_->rowCount();
  while (1) {
    if ((refresh == RefreshInsert) && (sortOrder == Qt::DescendingOrder)) {
      idx--;
      if (idx < 0)
        break;
    } else {
      idx++;
      if (idx >= newsModel_->rowCount())
        break;
    }

    QModelIndex index = newsModel_->index(idx, newsModel_->fieldIndex("id"));
    QString newsId = newsModel_->dataField(index.row(), "id").toString();

    if (refresh == RefreshInsert) {
      QWebElement document = webView_->page()->mainFrame()->documentElement();
      QWebElement element = document.findFirst(QString("div[id=newsItem%1]").arg(newsId));
      if (!element.isNull()) {
        continue;
      }
    }

    linkNewsString_ = getLinkNews(index.row());
    QString linkString = linkNewsString_;

    QString content = newsModel_->dataField(index.row(), "content").toString();
    if (!content.contains(QzRegExp("<html(.*)</html>", Qt::CaseInsensitive))) {
      QString description = newsModel_->dataField(index.row(), "description").toString();
      if (content.isEmpty() || (description.length() > content.length())) {
        content = description;
      }

      //      QTextDocumentFragment textDocument = QTextDocumentFragment::fromHtml(content);
      //      content = textDocument.toPlainText();
      //      content = webView_->fontMetrics().elidedText(
      //            content, Qt::ElideRight, 1500);

      QString feedId = newsModel_->dataField(index.row(), "feedId").toString();
      QModelIndex feedIndex = feedsModel_->indexById(feedId.toInt());

      QString iconStr = "qrc:/images/bulletRead";
      QString titleStyle = "read";
      if (newsModel_->dataField(index.row(), "new").toInt() == 1) {
        iconStr = "qrc:/images/bulletNew";
        titleStyle = "unread";
      } else if (newsModel_->dataField(index.row(), "read").toInt() == 0) {
        iconStr = "qrc:/images/bulletUnread";
        titleStyle = "unread";
      }
      QString readImg = QString("<a href=\"quiterss://read.action.ui?#%1\" title='%3'>"
                                "<img class='quiterss-img' id=\"readAction%1\" src=\"%2\"/></a>").
          arg(newsId).arg(iconStr).arg(tr("Mark Read/Unread"));

      QString feedImg;
      QByteArray byteArray = feedsModel_->dataField(feedIndex, "image").toByteArray();
      if (!byteArray.isEmpty())
        feedImg = QString("<img class='quiterss-img' src=\"data:image/png;base64,") % byteArray % "\"/>";
      else
        feedImg = QString("<img class='quiterss-img' src=\"qrc:/images/feed\"/>");

      QString titleString = newsModel_->dataField(index.row(), "title").toString();
      if (!linkString.isEmpty()) {
        titleString = QString("<a href='%1' class='%2' id='title%3'>%4</a>").
            arg(linkString, titleStyle, newsId, titleString);
      }

      QDateTime dtLocal;
      QString dateString = newsModel_->dataField(index.row(), "published").toString();
      if (!dateString.isNull()) {
        QDateTime dtLocalTime = QDateTime::currentDateTime();
        QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
        int nTimeShift = dtLocalTime.secsTo(dtUTC);

        QDateTime dt = QDateTime::fromString(dateString, Qt::ISODate);
        dtLocal = dt.addSecs(nTimeShift);
      } else {
        dtLocal = QDateTime::fromString(
              newsModel_->dataField(index.row(), "received").toString(),
              Qt::ISODate);
      }
      if (QDateTime::currentDateTime().date() <= dtLocal.date())
        dateString = dtLocal.toString(mainWindow_->formatTime_);
      else
        dateString = dtLocal.toString(mainWindow_->formatDate_ + " " + mainWindow_->formatTime_);

      // Create author panel from news author
      QString authorString;
      QString authorName = newsModel_->dataField(index.row(), "author_name").toString();
      QString authorEmail = newsModel_->dataField(index.row(), "author_email").toString();
      QString authorUri = newsModel_->dataField(index.row(), "author_uri").toString();

      QzRegExp reg("(^\\S+@\\S+\\.\\S+)", Qt::CaseInsensitive);
      int pos = reg.indexIn(authorName);
      if (pos > -1) {
        authorName.replace(reg.cap(1), QString(" <a href='mailto:%1'>%1</a>").arg(reg.cap(1)));
      }
      authorString = authorName;

      if (!authorEmail.isEmpty())
        authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
      if (!authorUri.isEmpty())
        authorString.append(QString(" <a href='%1'>page</a>"). arg(authorUri));

      // If news author is absent, create author panel from feed author
      // @note(arhohryakov:2012.01.03) Author is got from current feed, because
      //   news is belong to it
      if (authorString.isEmpty()) {
        authorName  = feedsModel_->dataField(feedIndex, "author_name").toString();
        authorEmail = feedsModel_->dataField(feedIndex, "author_email").toString();
        authorUri   = feedsModel_->dataField(feedIndex, "author_uri").toString();

        authorString = authorName;
        if (!authorEmail.isEmpty())
          authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
        if (!authorUri.isEmpty())
          authorString.append(QString(" <a href='%1'>page</a>").arg(authorUri));
      }

      QString commentsStr;
      QString commentsUrl = newsModel_->dataField(index.row(), "comments").toString();
      if (!commentsUrl.isEmpty()) {
        commentsStr = QString("<a href=\"%1\"> %2</a>").arg(commentsUrl, tr("Comments"));
      }

      QString category = newsModel_->dataField(index.row(), "category").toString();

      if (!authorString.isEmpty()) {
        authorString = QString(tr("Author: %1")).arg(authorString);
        if (!commentsStr.isEmpty())
          authorString.append(QString(" | %1").arg(commentsStr));
        if (!category.isEmpty())
          authorString.append(QString(" | %1").arg(category));
      } else {
        if (!commentsStr.isEmpty())
          authorString.append(commentsStr);
        if (!category.isEmpty()) {
          if (!commentsStr.isEmpty())
            authorString.append(QString(" | %1").arg(category));
          else
            authorString.append(category);
        }
      }

      QString labelsString = getHtmlLabels(index.row());
      authorString.append(QString("<table class=\"labels\" id=\"labels%1\"><tr>%2</tr></table>").
                          arg(newsId).arg(labelsString));

      QString enclosureStr;
      QString enclosureUrl = newsModel_->dataField(index.row(), "enclosure_url").toString();
      if (!enclosureUrl.isEmpty()) {
        QString type = newsModel_->dataField(index.row(), "enclosure_type").toString();
        if (type.contains("image")) {
          if (!content.contains(enclosureUrl) && autoLoadImages_) {
            enclosureStr = QString("<IMG SRC=\"%1\" class=\"enclosureImg\"><p>").
                arg(enclosureUrl);
          }
        } else {
          if (type.contains("audio")) {
            type = tr("audio");
            enclosureStr = audioPlayerHtml_.arg(enclosureUrl);
            enclosureStr.append("<p>");
          }
          else if (type.contains("video")) {
            type = tr("video");
            enclosureStr = videoPlayerHtml_.arg(enclosureUrl);
            enclosureStr.append("<p>");
          }
          else type = tr("media");

          enclosureStr.append(QString("<a href=\"%1\" class=\"enclosure\"> %2 %3 </a><p>").
                              arg(enclosureUrl, tr("Link to"), type));
        }
      }

      content = enclosureStr + content;

      if (!autoLoadImages_) {
        QzRegExp reg("<img[^>]+>", Qt::CaseInsensitive);
        content = content.remove(reg);
      }

      iconStr = "qrc:/images/starOff";
      if (newsModel_->dataField(index.row(), "starred").toInt() == 1) {
        iconStr = "qrc:/images/starOn";
      }
      QString starAction = QString("<div class=\"star-action\">"
                                   "<a href=\"quiterss://star.action.ui?#%1\" title='%3'>"
                                   "<img class='quiterss-img' id=\"starAction%1\" src=\"%2\"/></a></div>").
          arg(newsId).arg(iconStr).arg(tr("Mark News Star"));
      QString labelsMenu = QString("<div class=\"labels-menu\">"
                                   "<a href=\"quiterss://labels.menu.ui?#%1\" title='%2'>"
                                   "<img class='quiterss-img' id=\"labelsMenu%1\" src=\"qrc:/images/label_5\"/></a></div>").
          arg(newsId).arg(tr("Label"));
      QString shareMenu = QString("<div class=\"share-menu\">"
                                  "<a href=\"quiterss://share.menu.ui?#%1\" title='%2'>"
                                  "<img class='quiterss-img' id=\"shareMenu%1\" src=\"qrc:/images/images/share.png\"/></a></div>").
          arg(newsId).arg(tr("Share"));
      QString openBrowserAction = QString("<div class=\"open-browser\">"
                                          "<a href=\"quiterss://open.browser.ui?#%1\" title='%2'>"
                                          "<img class='quiterss-img' id=\"openBrowser%1\" src=\"qrc:/images/openBrowser\"'/></a></div>").
          arg(newsId).arg(tr("Open News in External Browser"));
      QString deleteAction = QString("<div class=\"delete-action\">"
                                     "<a href=\"quiterss://delete.action.ui?#%1\" title='%2'>"
                                     "<img class='quiterss-img' id=\"deleteAction%1\" src=\"qrc:/images/delete\"/></a></div>").
          arg(newsId).arg(tr("Delete"));
      QString actionNews = starAction % labelsMenu % shareMenu % openBrowserAction %
          deleteAction;

      QString border = "1";
      if (idx + 1 == newsModel_->rowCount())
        border = "0";
      if (ltr) {
        htmlStr = newspaperHtml_.arg(newsId, border, readImg, feedImg, titleString,
                                     dateString, authorString, content, actionNews);
      } else {
        htmlStr = newspaperHtmlRtl_.arg(newsId, border, readImg, feedImg, titleString,
                                        dateString, authorString, content, actionNews);
      }
    } else {
      if (!autoLoadImages_) {
        content = content.remove(QzRegExp("<img[^>]+>", Qt::CaseInsensitive));
      }
      htmlStr = content;
    }

    htmlStr = htmlStr.replace("src=\"//", "src=\"http://");

    QWebElement document = webView_->page()->mainFrame()->documentElement();
    QWebElement element = document.findFirst("body");
    if ((refresh == RefreshInsert) && (sortOrder == Qt::DescendingOrder))
      element.prependInside(htmlStr);
    else
      element.appendInside(htmlStr);
    qApp->processEvents();
  }

  webView_->settings()->setAttribute(QWebSettings::AutoLoadImages, autoLoadImages_);
  if ((refresh == RefreshInsert) && (sortOrder == Qt::DescendingOrder))
    scrollBarValue += webView_->page()->mainFrame()->contentsSize().height() - height;
  if (refresh != RefreshAll)
    webView_->page()->mainFrame()->setScrollBarValue(Qt::Vertical, scrollBarValue);

  webView_->setUpdatesEnabled(true);
}

/** @brief Asynchorous update web view
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotSetHtmlWebView(const QString &html, const QUrl &baseUrl)
{
  webView_->history()->setMaximumItemCount(0);
  webView_->setHtml(html, baseUrl);
  webView_->history()->setMaximumItemCount(100);
}

void NewsTabWidget::hideWebContent()
{
  if (mainWindow_->newsLayout_ == 1) return;

  emit signalSetHtmlWebView();
  setWebToolbarVisible(false, false);
}

void NewsTabWidget::slotLinkClicked(QUrl url)
{
  if (url.scheme() == QLatin1String("quiterss")) {
    actionNewspaper(url);
    return;
  }

  if (url.scheme() == QLatin1String("mailto")) {
    QDesktopServices::openUrl(url);
    return;
  }

  if (type_ != TabTypeWeb) {
    if ((url.host().isEmpty() || (QUrl(url).host().indexOf('.') == -1)) && newsView_->currentIndex().isValid()) {
      int row = newsView_->currentIndex().row();
      int feedId = newsModel_->dataField(row, "feedId").toInt();
      QModelIndex feedIndex = feedsModel_->indexById(feedId);
      QUrl hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();

      url.setScheme(hostUrl.scheme());
      url.setHost(hostUrl.host());
    }
  }

  if ((mainWindow_->externalBrowserOn_ <= 0) &&
      (webView_->buttonClick_ != LEFT_BUTTON_ALT)) {
    if (webView_->buttonClick_ == LEFT_BUTTON) {
      if (!webControlPanel_->isVisible()) {
        locationBar_->setText(url.toString());
        setWebToolbarVisible(true, false);
      }
      webView_->load(url);
    } else {
      if ((webView_->buttonClick_ == MIDDLE_BUTTON) ||
          (webView_->buttonClick_ == LEFT_BUTTON_CTRL)) {
        mainWindow_->openNewsTab_ = NEW_TAB_BACKGROUND;
      } else {
        mainWindow_->openNewsTab_ = NEW_TAB_FOREGROUND;
      }
      if (!mainWindow_->openLinkInBackgroundEmbedded_) {
        if (mainWindow_->openNewsTab_ == NEW_TAB_BACKGROUND)
          mainWindow_->openNewsTab_ = NEW_TAB_FOREGROUND;
        else
          mainWindow_->openNewsTab_ = NEW_TAB_BACKGROUND;
      }

      mainWindow_->createWebTab(url);
    }
  } else {
    openUrl(url);
  }

  webView_->buttonClick_ = 0;
}
//----------------------------------------------------------------------------
void NewsTabWidget::slotLinkHovered(const QString &link, const QString &, const QString &)
{
  if (QUrl(link).scheme() == QLatin1String("quiterss")) return;

  mainWindow_->statusBar()->showMessage(link.simplified(), 3000);
}
//----------------------------------------------------------------------------
void NewsTabWidget::slotSetValue(int value)
{
  emit loadProgress(value);
  QString str = QString(" %1 kB / %2 kB").
      arg(webView_->page()->bytesReceived()/1000).
      arg(webView_->page()->totalBytes()/1000);
  webViewProgressLabel_->setText(str);
}
//----------------------------------------------------------------------------
void NewsTabWidget::slotLoadStarted()
{
  if (type_ == TabTypeWeb) {
    newsIconTitle_->setMovie(newsIconMovie_);
    newsIconMovie_->start();
  }

  webViewProgress_->setValue(0);
  webViewProgress_->show();
}
//----------------------------------------------------------------------------
void NewsTabWidget::slotLoadFinished(bool)
{
  if (type_ == TabTypeWeb) {
    newsIconMovie_->stop();
    QPixmap iconTab;
    iconTab.load(":/images/webPage");
    newsIconTitle_->setPixmap(iconTab);
  }

  webViewProgress_->hide();
}

void NewsTabWidget::slotUrlEnter()
{
  webView_->setFocus();

  if (!locationBar_->text().startsWith("http://") &&
      !locationBar_->text().startsWith("https://")) {
    locationBar_->setText("http://" + locationBar_->text());
  }
  locationBar_->setCursorPosition(0);

  webView_->load(QUrl(locationBar_->text()));
}

void NewsTabWidget::slotUrlChanged(const QUrl &url)
{
  locationBar_->setText(url.toString());
  locationBar_->setCursorPosition(0);
}

/** @brief Go to short news content
 *----------------------------------------------------------------------------*/
void NewsTabWidget::webHomePage()
{
  if (type_ != TabTypeWeb) {
    switch (mainWindow_->newsLayout_) {
    case 1:
      loadNewspaper();
      break;
    default:
      updateWebView(newsView_->currentIndex());
    }
  } else {
    webView_->history()->goToItem(webView_->history()->itemAt(0));
  }
}

/** @brief Open current web page in external browser
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openPageInExternalBrowser()
{
  openUrl(webView_->url());
}

/** @brief Open news in browser
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openInBrowserNews()
{
  if (type_ >= TabTypeWeb) return;

  int externalBrowserOn_ = mainWindow_->externalBrowserOn_;
  mainWindow_->externalBrowserOn_ = 0;
  slotNewsViewDoubleClicked(newsView_->currentIndex());
  mainWindow_->externalBrowserOn_ = externalBrowserOn_;
}

/** @brief Open news in external browser
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openInExternalBrowserNews()
{
  if (type_ == TabTypeDownloads) return;

  if (type_ != TabTypeWeb) {
    QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);
    QStringList feedIdList;

    int cnt = indexes.count();
    if (cnt == 0) return;

    for (int i = cnt-1; i >= 0; --i) {
      QSqlQuery q;
      QModelIndex curIndex = indexes.at(i);
      if (newsModel_->dataField(curIndex.row(), "read").toInt() == 0) {
        newsModel_->setData(
              newsModel_->index(curIndex.row(), newsModel_->fieldIndex("new")),
              0);
        newsModel_->setData(
              newsModel_->index(curIndex.row(), newsModel_->fieldIndex("read")),
              1);

        int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
        q.exec(QString("UPDATE news SET new=0, read=1 WHERE id=='%2'").arg(newsId));
        QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
        if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
      }

      QUrl url = QUrl::fromEncoded(getLinkNews(indexes.at(i).row()).toUtf8());
      if (url.host().isEmpty() || (QUrl(url).host().indexOf('.') == -1)) {
        QString feedId = newsModel_->dataField(indexes.at(i).row(), "feedId").toString();
        QModelIndex feedIndex = feedsModel_->indexById(feedId.toInt());
        QUrl hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();

        url.setScheme(hostUrl.scheme());
        url.setHost(hostUrl.host());
      }

      openUrl(url);
    }

    if (!feedIdList.isEmpty()) {
      foreach (QString feedId, feedIdList) {
        mainWindow_->slotUpdateStatus(feedId.toInt());
      }
      mainWindow_->recountCategoryCounts();
      newsView_->viewport()->update();
    }
  } else {
    openUrl(webView_->url());
  }
}

void NewsTabWidget::setNewsLayout()
{
  if (type_ >= TabTypeWeb) return;

  switch (mainWindow_->newsLayout_) {
  case 1:
    newsWidget_->setVisible(false);
    loadNewspaper();
    break;
  default:
    newsWidget_->setVisible(true);
    updateWebView(newsView_->currentIndex());
  }
}

/** @brief Set browser position
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setBrowserPosition()
{
  if (type_ >= TabTypeWeb) return;

  int idx = newsTabWidgetSplitter_->indexOf(webWidget_);

  switch (mainWindow_->browserPosition_) {
  case TOP_POSITION: case LEFT_POSITION:
    newsTabWidgetSplitter_->insertWidget(0, newsTabWidgetSplitter_->widget(idx));
    break;
  default:
    newsTabWidgetSplitter_->insertWidget(1, newsTabWidgetSplitter_->widget(idx));
  }

  switch (mainWindow_->browserPosition_) {
  case RIGHT_POSITION: case LEFT_POSITION:
    newsTabWidgetSplitter_->setOrientation(Qt::Horizontal);
    newsTabWidgetSplitter_->setStyleSheet(
          QString("QSplitter::handle {background: qlineargradient("
                  "x1: 0, y1: 0, x2: 0, y2: 1,"
                  "stop: 0 %1, stop: 0.07 %2);}").
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

/** @brief Close tab while press X-button
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotTabClose()
{
  mainWindow_->slotCloseTab(mainWindow_->stackedWidget_->indexOf(this));
}

/** @brief Display browser open page title on tab
 *----------------------------------------------------------------------------*/
void NewsTabWidget::webTitleChanged(QString title)
{
  if ((type_ == TabTypeWeb) && !title.isEmpty()) {
    setTextTab(title);
  }
}

/** @brief Open news in new tab
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openNewsNewTab()
{
  if (type_ >= TabTypeWeb) return;

  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  int cnt = indexes.count();
  if (cnt == 0) return;

  for (int i = cnt-1; i >= 0; --i) {
    QModelIndex index = indexes.at(i);
    int row = index.row();
    if (mainWindow_->markNewsReadOn_ && mainWindow_->markCurNewsRead_)
      slotSetItemRead(index, 1);

    QUrl url = QUrl::fromEncoded(getLinkNews(row).toUtf8());
    if (url.host().isEmpty() || (QUrl(url).host().indexOf('.') == -1)) {
      int feedId = newsModel_->dataField(row, "feedId").toInt();
      QModelIndex feedIndex = feedsModel_->indexById(feedId);
      QUrl hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();

      url.setScheme(hostUrl.scheme());
      url.setHost(hostUrl.host());
    }

    mainWindow_->createWebTab(url);
  }
}

/** @brief Open link
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openLink()
{
  slotLinkClicked(linkUrl_);
}

/** @brief Open link in new tab
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openLinkInNewTab()
{
  int externalBrowserOn_ = mainWindow_->externalBrowserOn_;
  mainWindow_->externalBrowserOn_ = 0;

  if (QApplication::keyboardModifiers() == Qt::NoModifier) {
    webView_->buttonClick_ = MIDDLE_BUTTON;
  } else {
    webView_->buttonClick_ = MIDDLE_BUTTON_MOD;
  }

  slotLinkClicked(linkUrl_);
  mainWindow_->externalBrowserOn_ = externalBrowserOn_;
}

/** @brief Open link in browser
 *----------------------------------------------------------------------------*/
bool NewsTabWidget::openUrl(const QUrl &url)
{
  if (!url.isValid())
    return false;

  if (url.scheme() == QLatin1String("mailto"))
    return QDesktopServices::openUrl(url);

  mainWindow_->isOpeningLink_ = true;
  if ((mainWindow_->externalBrowserOn_ == 2) || (mainWindow_->externalBrowserOn_ == -1)) {
#if defined(Q_OS_WIN)
    quintptr returnValue = (quintptr)ShellExecute(
          0, 0,
          (wchar_t *)QString::fromUtf8(mainWindow_->externalBrowser_.toUtf8()).utf16(),
          (wchar_t *)QString::fromUtf8(url.toEncoded().constData()).utf16(),
          0, SW_SHOWNORMAL);
    if (returnValue > 32)
      return true;
#elif defined(Q_OS_MAC)
    return (QProcess::startDetached("open", QStringList() << "-a" <<
                                    QString::fromUtf8(mainWindow_->externalBrowser_.toUtf8()) <<
                                    QString::fromUtf8(url.toEncoded().constData())));
#else
    return (QProcess::startDetached(QString::fromUtf8(mainWindow_->externalBrowser_.toUtf8()) + QLatin1Char(' ') +
                                    QString::fromUtf8(url.toEncoded().constData())));
#endif
  }
  return QDesktopServices::openUrl(url);
}
//----------------------------------------------------------------------------
void NewsTabWidget::slotFindText(const QString &text)
{
  QString objectName = findText_->findGroup_->checkedAction()->objectName();
  if (objectName == "findInBrowserAct") {
    webView_->findText("", QWebPage::HighlightAllOccurrences);
    webView_->findText(text, QWebPage::HighlightAllOccurrences);
  } else {
    int newsId = newsModel_->dataField(newsView_->currentIndex().row(), "id").toInt();

    QString filterStr;
    switch (type_) {
    case TabTypeUnread:
    case TabTypeStar:
    case TabTypeDel:
    case TabTypeLabel:
      filterStr = categoryFilterStr_;
      break;
    default:
      filterStr = mainWindow_->newsFilterStr;
    }

    if (!text.isEmpty()) {
      QString findText = text;
      findText = findText.replace("'", "''").toUpper();
      if (objectName == "findTitleAct") {
        filterStr.append(
              QString(" AND UPPER(title) LIKE '%%1%'").arg(findText));
      } else if (objectName == "findAuthorAct") {
        filterStr.append(
              QString(" AND UPPER(author_name) LIKE '%%1%'").arg(findText));
      } else if (objectName == "findCategoryAct") {
        filterStr.append(
              QString(" AND UPPER(category) LIKE '%%1%'").arg(findText));
      } else if (objectName == "findContentAct") {
        filterStr.append(
              QString(" AND (UPPER(content) LIKE '%%1%' OR UPPER(description) LIKE '%%1%')").
              arg(findText));
      } else if (objectName == "findLinkAct") {
        filterStr.append(
              QString(" AND link_href LIKE '%%1%'").
              arg(findText));
      } else {
        filterStr.append(
              QString(" AND (UPPER(title) LIKE '%%1%' OR UPPER(author_name) LIKE '%%1%' "
                      "OR UPPER(category) LIKE '%%1%' OR UPPER(content) LIKE '%%1%' "
                      "OR UPPER(description) LIKE '%%1%')").
              arg(findText));
      }
    }

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
//----------------------------------------------------------------------------
void NewsTabWidget::slotSelectFind()
{
  webView_->findText("", QWebPage::HighlightAllOccurrences);
  slotFindText(findText_->text());
}
//----------------------------------------------------------------------------
void NewsTabWidget::showContextWebPage(const QPoint &p)
{
  QMenu menu;
  QMenu *pageMenu = webView_->page()->createStandardContextMenu();
  if (pageMenu) {
    menu.addActions(pageMenu->actions());

    webView_->page()->action(QWebPage::OpenLink)->setText(tr("Open Link"));
    webView_->page()->action(QWebPage::OpenLinkInNewWindow)->setText(tr("Open in New Tab"));
    webView_->page()->action(QWebPage::DownloadLinkToDisk)->setText(tr("Save Link..."));
    webView_->page()->action(QWebPage::DownloadImageToDisk)->setText(tr("Save Image..."));
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

    const QWebHitTestResult &hitTest = webView_->page()->mainFrame()->hitTestContent(p);
    if (!hitTest.linkUrl().isEmpty() && hitTest.linkUrl().scheme() != "javascript") {
      linkUrl_ = hitTest.linkUrl();
      if (mainWindow_->externalBrowserOn_ <= 0) {
        menu.addSeparator();
        menu.addAction(urlExternalBrowserAct_);
      }
    } else if (pageMenu->actions().indexOf(webView_->pageAction(QWebPage::Reload)) >= 0) {
      if (webView_->title() == "news_descriptions") {
        webView_->pageAction(QWebPage::Reload)->setVisible(false);
      } else {
        webView_->pageAction(QWebPage::Reload)->setVisible(true);
        menu.addSeparator();
      }
      menu.addAction(mainWindow_->autoLoadImagesToggle_);
      menu.addSeparator();
      menu.addAction(mainWindow_->printAct_);
      menu.addAction(mainWindow_->printPreviewAct_);
      menu.addSeparator();
      menu.addAction(mainWindow_->savePageAsAct_);
    } else if (hitTest.isContentEditable()) {
      for (int i = 0; i < menu.actions().count(); i++) {
        if ((i <= 1) && (menu.actions().at(i)->text() == "Direction")) {
          menu.actions().at(i)->setVisible(false);
          break;
        }
      }
      menu.insertSeparator(menu.actions().at(0));
      menu.insertAction(menu.actions().at(0), webView_->pageAction(QWebPage::Redo));
      menu.insertAction(menu.actions().at(0), webView_->pageAction(QWebPage::Undo));
    }

    if (!hitTest.isContentEditable() && !hitTest.isContentSelected()) {
      menu.addSeparator();
      menu.addAction(mainWindow_->adBlockIcon()->menuAction());
    }

    menu.exec(webView_->mapToGlobal(p));
  }
}

/** @brief Open link in external browser
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openUrlInExternalBrowser()
{
  if (linkUrl_.scheme() == QLatin1String("mailto")) {
    QDesktopServices::openUrl(linkUrl_);
    return;
  }

  if (type_ != TabTypeWeb) {
    if (linkUrl_.host().isEmpty() && newsView_->currentIndex().isValid()) {
      int row = newsView_->currentIndex().row();
      int feedId = newsModel_->dataField(row, "feedId").toInt();
      QModelIndex feedIndex = feedsModel_->indexById(feedId);
      QUrl hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();

      linkUrl_.setScheme(hostUrl.scheme());
      linkUrl_.setHost(hostUrl.host());
    }
  }
  openUrl(linkUrl_);
}

void NewsTabWidget::setWebToolbarVisible(bool show, bool checked)
{
  if (!checked) webToolbarShow_ = show;
  webControlPanel_->setVisible(webToolbarShow_ &
                               mainWindow_->browserToolbarToggle_->isChecked());

}

/** @brief Set label for selected news
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setLabelNews(int labelId)
{
  if (type_ >= TabTypeWeb) return;

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

    int newsId = newsModel_->dataField(index.row(), "id").toInt();

    if ((newsId == currentNewsIdOld) &&
        (webView_->title() == "news_descriptions")) {
      QWebFrame *frame = webView_->page()->mainFrame();
      QWebElement document = frame->documentElement();
      QWebElement element = document.findFirst(QString("table[id=labels%1]").arg(newsId));
      if (!element.isNull()) {
        webView_->settings()->setAttribute(QWebSettings::AutoLoadImages,true);
        element.removeAllChildren();
        QString labelsString = getHtmlLabels(index.row());
        element.appendInside(labelsString);
        webView_->settings()->setAttribute(QWebSettings::AutoLoadImages, autoLoadImages_);
      }
    }

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

      int newsId = newsModel_->dataField(index.row(), "id").toInt();

      if ((newsId == currentNewsIdOld) &&
          (webView_->title() == "news_descriptions")) {
        QWebFrame *frame = webView_->page()->mainFrame();
        QWebElement document = frame->documentElement();
        QWebElement element = document.findFirst(QString("table[id=labels%1]").arg(newsId));
        if (!element.isNull()) {
          element.removeAllChildren();
          QString labelsString = getHtmlLabels(index.row());
          element.appendInside(labelsString);
        }
      }

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
  mainWindow_->recountCategoryCounts();
}

void NewsTabWidget::slotNewslLabelClicked(QModelIndex index)
{
  if (!newsView_->selectionModel()->isSelected(index)) {
    newsView_->selectionModel()->clearSelection();
    newsView_->selectionModel()->select(
          index, QItemSelectionModel::Select|QItemSelectionModel::Rows);
  }
  mainWindow_->newsLabelMenu_->popup(
        newsView_->viewport()->mapToGlobal(newsView_->visualRect(index).bottomLeft()));
}

void NewsTabWidget::showLabelsMenu()
{
  if (type_ >= TabTypeWeb) return;
  if (!newsView_->currentIndex().isValid()) return;

  for (int i = newsHeader_->count()-1; i >= 0; i--) {
    int lIdx = newsHeader_->logicalIndex(i);
    if (!newsHeader_->isSectionHidden(lIdx)) {
      int row = newsView_->currentIndex().row();
      slotNewslLabelClicked(newsModel_->index(row, lIdx));
      break;
    }
  }
}

void NewsTabWidget::reduceNewsList()
{
  if (type_ >= TabTypeWeb) return;

  QList <int> sizes = newsTabWidgetSplitter_->sizes();
  sizes.insert(0, sizes.takeAt(0) - RESIZESTEP);
  newsTabWidgetSplitter_->setSizes(sizes);
}

void NewsTabWidget::increaseNewsList()
{
  if (type_ >= TabTypeWeb) return;

  QList <int> sizes = newsTabWidgetSplitter_->sizes();
  sizes.insert(0, sizes.takeAt(0) + RESIZESTEP);
  newsTabWidgetSplitter_->setSizes(sizes);
}

/** @brief Search unread news
 * @param next search condition: true - search next, else - previous
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

/** @brief Set tab title
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setTextTab(const QString &text)
{
  int padding = 15;

  if (closeButton_->isHidden())
    padding = 0;

  QString textTab = newsTextTitle_->fontMetrics().elidedText(
        text, Qt::ElideRight, newsTitleLabel_->width() - 16 - 3 - padding);
  newsTextTitle_->setText(textTab);
  newsTitleLabel_->setToolTip(text);

  emit signalSetTextTab(text, this);
}

/** @brief Share news
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotShareNews(QAction *action)
{
  QList<QModelIndex> indexes;
  int cnt = 0;
  if (type_ < TabTypeWeb) {
    indexes = newsView_->selectionModel()->selectedRows(0);
    cnt = indexes.count();
  } else if (type_ == TabTypeWeb) {
    cnt = 1;
  }
  if (cnt == 0) return;

  for (int i = cnt-1; i >= 0; --i) {
    QString title;
    QString linkString;
    QString content;
    if (type_ < TabTypeWeb) {
      title = newsModel_->dataField(indexes.at(i).row(), "title").toString();
      linkString = getLinkNews(indexes.at(i).row());

      content = newsModel_->dataField(indexes.at(i).row(), "content").toString();
      QString description = newsModel_->dataField(indexes.at(i).row(), "description").toString();
      if (content.isEmpty() || (description.length() > content.length())) {
        content = description;
      }
      QTextDocumentFragment textDocument = QTextDocumentFragment::fromHtml(content);
      content = textDocument.toPlainText();
    } else {
      title = webView_->title();
      linkString = webView_->url().toString();
      content = webView_->page()->mainFrame()->toPlainText();
    }
#if defined(Q_OS_WIN) || defined(Q_OS_OS2) || defined(Q_OS_MAC)
    content = content.replace("\n", "%0A");
    content = content.replace("\"", "%22");
#endif

    QUrl url;
    if (action->objectName() == "emailShareAct") {
      url.setUrl("mailto:");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("subject", title);
      urlQuery.addQueryItem("body", linkString);
//#if defined(Q_OS_WIN) || defined(Q_OS_OS2) || defined(Q_OS_MAC)
//      urlQuery.addQueryItem("body", linkString + "%0A%0A" + content);
//#else
//      urlQuery.addQueryItem("body", linkString + "\n\n" + content);
//#endif
      url.setQuery(urlQuery);
#else
      url.addQueryItem("subject", title);
#if defined(Q_OS_WIN) || defined(Q_OS_OS2) || defined(Q_OS_MAC)
      url.addQueryItem("body", linkString + "%0A%0A" + content);
#else
      url.addQueryItem("body", linkString + "\n\n" + content);
#endif
#endif
      openUrl(url);
    } else {
      if (action->objectName() == "evernoteShareAct") {
        url.setUrl("https://www.evernote.com/clip.action");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString);
        urlQuery.addQueryItem("title", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString);
        url.addQueryItem("title", title);
#endif
      } else if (action->objectName() == "gplusShareAct") {
        url.setUrl("https://plus.google.com/share");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString);
#endif
      } else if (action->objectName() == "facebookShareAct") {
        url.setUrl("https://www.facebook.com/sharer.php");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("u", linkString);
        urlQuery.addQueryItem("t", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("u", linkString);
        url.addQueryItem("t", title);
#endif
      } else if (action->objectName() == "livejournalShareAct") {
        url.setUrl("http://www.livejournal.com/update.bml");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("event", linkString);
        urlQuery.addQueryItem("subject", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("event", linkString);
        url.addQueryItem("subject", title);
#endif
      } else if (action->objectName() == "pocketShareAct") {
        url.setUrl("https://getpocket.com/save");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString);
        urlQuery.addQueryItem("title", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString);
        url.addQueryItem("title", title);
#endif
      } else if (action->objectName() == "twitterShareAct") {
        url.setUrl("https://twitter.com/share");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString);
        urlQuery.addQueryItem("text", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString);
        url.addQueryItem("text", title);
#endif
      } else if (action->objectName() == "vkShareAct") {
        url.setUrl("https://vk.com/share.php");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString);
        urlQuery.addQueryItem("title", title);
        urlQuery.addQueryItem("description", "");
        urlQuery.addQueryItem("image", "");
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString);
        url.addQueryItem("title", title);
        url.addQueryItem("description", "");
        url.addQueryItem("image", "");
#endif
      } else if (action->objectName() == "linkedinShareAct") {
        url.setUrl("https://www.linkedin.com/shareArticle?mini=true");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString);
        urlQuery.addQueryItem("title", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString);
        url.addQueryItem("title", title);
#endif
      } else if (action->objectName() == "bloggerShareAct") {
        url.setUrl("https://www.blogger.com/blog_this.pyra?t");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("u", linkString);
        urlQuery.addQueryItem("n", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("u", linkString);
        url.addQueryItem("n", title);
#endif
      } else if (action->objectName() == "printfriendlyShareAct") {
        url.setUrl("https://www.printfriendly.com/print");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString);
#endif
      } else if (action->objectName() == "instapaperShareAct") {
        url.setUrl("https://www.instapaper.com/hello2");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString);
        urlQuery.addQueryItem("title", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString);
        url.addQueryItem("title", title);
#endif
      } else if (action->objectName() == "redditShareAct") {
        url.setUrl("https://reddit.com/submit");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString);
        urlQuery.addQueryItem("title", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString);
        url.addQueryItem("title", title);
#endif
      }

      if (mainWindow_->externalBrowserOn_ <= 0) {
        mainWindow_->openNewsTab_ = NEW_TAB_FOREGROUND;
        mainWindow_->createWebTab(url);
      } else openUrl(url);
    }
  }
}
//-----------------------------------------------------------------------------
int NewsTabWidget::getUnreadCount(QString countString)
{
  if (countString.isEmpty()) return 0;

  countString.remove(QzRegExp("[()]"));
  switch (type_) {
  case TabTypeUnread:
    return countString.toInt();
  case TabTypeStar:
  case TabTypeLabel:
    return countString.section("/", 0, 0).toInt();
  default:
    return 0;
  }
}

QString NewsTabWidget::getLinkNews(int row)
{
  QString linkString = newsModel_->dataField(row, "link_href").toString();
  if (linkString.isEmpty())
    linkString = newsModel_->dataField(row, "link_alternate").toString();
  return linkString.simplified();
}

void NewsTabWidget::savePageAsDescript()
{
  if (type_ >= TabTypeWeb) return;

  QModelIndex curIndex = newsView_->currentIndex();
  if (!curIndex.isValid()) return;

  QString html = webView_->page()->currentFrame()->toHtml().replace("'", "''");
  newsModel_->setData(
        newsModel_->index(curIndex.row(), newsModel_->fieldIndex("content")),
        html);
  int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
  QString qStr = QString("UPDATE news SET content='%1' WHERE id=='%2'").
      arg(html).arg(newsId);
  mainApp->sqlQueryExec(qStr);
}

QString NewsTabWidget::getHtmlLabels(int row)
{
  QStringList strLabelIdList = newsModel_->dataField(row, "label").toString().
      split(",", QString::SkipEmptyParts);
  QString labelsString;
  QList<QTreeWidgetItem *> labelListItems = mainWindow_->categoriesTree_->getLabelListItems();
  foreach (QTreeWidgetItem *item, labelListItems) {
    if (strLabelIdList.contains(item->text(2))) {
      strLabelIdList.removeOne(item->text(2));
      QByteArray byteArray = item->data(0, CategoriesTreeWidget::ImageRole).toByteArray();
      labelsString.append(QString("<td><img class='quiterss-img' src=\"data:image/png;base64,") % byteArray.toBase64() % "\"/></td>");
      labelsString.append("<td>" % item->text(0));
      if (strLabelIdList.count())
        labelsString.append(",");
      labelsString.append("</td>");
    }
  }
  return labelsString;
}

void NewsTabWidget::actionNewspaper(QUrl url)
{
  QString newsId = url.fragment();
  QModelIndex startIndex = newsModel_->index(0, newsModel_->fieldIndex("id"));
  QModelIndexList indexList = newsModel_->match(startIndex, Qt::EditRole, newsId);
  if (!indexList.isEmpty()) {
    QString iconStr;
    if (url.host() == "read.action.ui") {
      QString titleStyle;
      if (newsModel_->dataField(indexList.first().row(), "read").toInt() == 0) {
        slotSetItemRead(indexList.first(), 1);
        iconStr = "qrc:/images/bulletRead";
        titleStyle = "read";
      } else {
        slotSetItemRead(indexList.first(), 0);
        iconStr = "qrc:/images/bulletUnread";
        titleStyle = "unread";
      }
      QWebElement document = webView_->page()->mainFrame()->documentElement();
      QWebElement newsItem = document.findFirst(QString("div[id=newsItem%1]").arg(newsId));
      if (!newsItem.isNull()) {
        webView_->settings()->setAttribute(QWebSettings::AutoLoadImages, true);
        QWebElement element = newsItem.findFirst(QString("img[id=readAction%1]").arg(newsId));
        if (!element.isNull())
          element.setAttribute("src", iconStr);
        element = newsItem.findFirst(QString("a[id=title%1]").arg(newsId));
        if (!element.isNull())
          element.setAttribute("class", titleStyle);
        webView_->settings()->setAttribute(QWebSettings::AutoLoadImages, autoLoadImages_);
      }
    } else if (url.host() == "star.action.ui") {
      int row = indexList.first().row();
      if (newsModel_->dataField(row, "starred").toInt() == 0) {
        slotSetItemStar(newsModel_->index(row, newsModel_->fieldIndex("starred")), 1);
        iconStr = "qrc:/images/starOn";
      } else {
        slotSetItemStar(newsModel_->index(row, newsModel_->fieldIndex("starred")), 0);
        iconStr = "qrc:/images/starOff";
      }
      QWebElement document = webView_->page()->mainFrame()->documentElement();
      QWebElement newsItem = document.findFirst(QString("div[id=newsItem%1]").arg(newsId));
      if (!newsItem.isNull()) {
        QWebElement element = newsItem.findFirst(QString("img[id=starAction%1]").arg(newsId));
        if (!element.isNull()) {
          webView_->settings()->setAttribute(QWebSettings::AutoLoadImages, true);
          element.setAttribute("src", iconStr);
          webView_->settings()->setAttribute(QWebSettings::AutoLoadImages, autoLoadImages_);
        }
      }
    } else if (url.host() == "labels.menu.ui") {
      newsView_->selectionModel()->clearSelection();
      newsView_->selectionModel()->select(
            indexList.first(), QItemSelectionModel::Select|QItemSelectionModel::Rows);
      currentNewsIdOld = newsId.toInt();
      mainWindow_->newsLabelMenu_->popup(QCursor::pos());
    } else if (url.host() == "share.menu.ui") {
      newsView_->selectionModel()->clearSelection();
      newsView_->selectionModel()->select(
            indexList.first(), QItemSelectionModel::Select|QItemSelectionModel::Rows);
      currentNewsIdOld = newsId.toInt();
      mainWindow_->shareMenu_->popup(QCursor::pos());
    } else if (url.host() == "open.browser.ui") {
      QUrl url = QUrl::fromEncoded(getLinkNews(indexList.first().row()).toUtf8());
      if (url.host().isEmpty() || (QUrl(url).host().indexOf('.') == -1)) {
        QString feedId = newsModel_->dataField(indexList.first().row(), "feedId").toString();
        QModelIndex feedIndex = feedsModel_->indexById(feedId.toInt());
        QUrl hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();

        url.setScheme(hostUrl.scheme());
        url.setHost(hostUrl.host());
      }
      openUrl(url);
    } else if (url.host() == "delete.action.ui") {
      newsView_->selectionModel()->clearSelection();
      newsView_->selectionModel()->select(
            indexList.first(), QItemSelectionModel::Select|QItemSelectionModel::Rows);
      deleteNews();
      QWebElement document = webView_->page()->mainFrame()->documentElement();
      QWebElement newsItem = document.findFirst(QString("div[id=newsItem%1]").arg(newsId));
      if (!newsItem.isNull()) {
        newsItem.removeFromDocument();
      }
    }
  }
}
