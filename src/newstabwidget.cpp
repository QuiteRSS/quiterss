/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "newstabwidget.h"
#include "rsslisting.h"

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

NewsTabWidget::NewsTabWidget(QWidget *parent, TabType type, int feedId, int feedParId)
  : QWidget(parent)
  , type_(type)
  , feedId_(feedId)
  , feedParId_(feedParId)
  , currentNewsIdOld(-1)
  , autoLoadImages_(true)
{
  rssl_ = qobject_cast<RSSListing*>(parent);
  db_ = QSqlDatabase::database();
  feedsTreeView_ = rssl_->feedsTreeView_;
  feedsTreeModel_ = rssl_->feedsTreeModel_;
  feedsProxyModel_ = rssl_->feedsProxyModel_;

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

  if (type_ != TabTypeDownloads) {
    if (type_ != TabTypeWeb) {
      createNewsList();
    } else {
      autoLoadImages_ = rssl_->autoLoadImages_;
    }
    createWebWidget();

    if (type_ != TabTypeWeb) {
      newsTabWidgetSplitter_ = new QSplitter(this);
      newsTabWidgetSplitter_->setObjectName("newsTabWidgetSplitter");

      if ((rssl_->browserPosition_ == TOP_POSITION) ||
          (rssl_->browserPosition_ == LEFT_POSITION)) {
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
    layout->addWidget(rssl_->downloadManager_);
  else if (type_ != TabTypeWeb)
    layout->addWidget(newsTabWidgetSplitter_);
  else
    layout->addWidget(webWidget_);
  setLayout(layout);

  if (type_ < TabTypeWeb) {
    newsTabWidgetSplitter_->setHandleWidth(1);

    if ((rssl_->browserPosition_ == RIGHT_POSITION) ||
        (rssl_->browserPosition_ == LEFT_POSITION)) {
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
          rssl_, SLOT(setTextTitle(QString,NewsTabWidget*)));
}

NewsTabWidget::~NewsTabWidget()
{
  if (type_ == TabTypeDownloads) {
    rssl_->downloadManager_->hide();
    rssl_->downloadManager_->setParent(rssl_);
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
  newsToolBar_->setIconSize(QSize(18, 18));

  QString actionListStr = "markNewsRead,markAllNewsRead,Separator,markStarAct,"
      "newsLabelAction,shareMenuAct,openInExternalBrowserAct,Separator,"
      "nextUnreadNewsAct,prevUnreadNewsAct,Separator,"
      "newsFilter,Separator,deleteNewsAct";
  QString str = rssl_->settings_->value("Settings/newsToolBar", actionListStr).toString();

  foreach (QString actionStr, str.split(",", QString::SkipEmptyParts)) {
    if (actionStr == "Separator") {
      newsToolBar_->addSeparator();
    } else {
      QListIterator<QAction *> iter(rssl_->actions());
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
  newsToolBar_->addAction(rssl_->restoreNewsAct_);

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
  if (!rssl_->newsToolbarToggle_->isChecked())
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
  htmlFile.setFileName(":/html/description");
  htmlFile.open(QFile::ReadOnly);
  htmlString_ = QString::fromUtf8(htmlFile.readAll());
  htmlFile.close();

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

  connect(rssl_->newsToolbarToggle_, SIGNAL(toggled(bool)),
          newsPanelWidget_, SLOT(setVisible(bool)));
}

/** @brief Call context menu of selected news in news list
 *----------------------------------------------------------------------------*/
void NewsTabWidget::showContextMenuNews(const QPoint &pos)
{
  if (!newsView_->currentIndex().isValid()) return;

  QMenu menu;
  menu.addAction(rssl_->restoreNewsAct_);
  menu.addSeparator();
  menu.addAction(rssl_->openInBrowserAct_);
  menu.addAction(rssl_->openInExternalBrowserAct_);
  menu.addAction(rssl_->openNewsNewTabAct_);
  menu.addSeparator();
  menu.addAction(rssl_->markNewsRead_);
  menu.addAction(rssl_->markAllNewsRead_);
  menu.addSeparator();
  menu.addAction(rssl_->markStarAct_);
  menu.addAction(rssl_->newsLabelMenuAction_);
  menu.addAction(rssl_->shareMenuAct_);
  menu.addAction(rssl_->copyLinkAct_);
  menu.addSeparator();
  menu.addAction(rssl_->updateFeedAct_);
  menu.addSeparator();
  menu.addAction(rssl_->deleteNewsAct_);
  menu.addAction(rssl_->deleteAllNewsAct_);

  menu.exec(newsView_->viewport()->mapToGlobal(pos));
}

/** @brief Create web-widget and control panel
 *----------------------------------------------------------------------------*/
void NewsTabWidget::createWebWidget()
{
  webView_ = new WebView(this, rssl_->networkManager_);

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

  locationBar_ = new LineEdit(this);

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
                SLOT(setHtmlWebView(QString,QUrl)), Qt::QueuedConnection);
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

  connect(webView_->page()->networkAccessManager(),
          SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          rssl_, SLOT(slotAuthentication(QNetworkReply*,QAuthenticator*)));

  connect(rssl_->browserToolbarToggle_, SIGNAL(triggered()),
          this, SLOT(setWebToolbarVisible()));

  connect(locationBar_, SIGNAL(returnPressed()),this, SLOT(slotUrlEnter()));
  connect(webView_, SIGNAL(urlChanged(QUrl)),
          this, SLOT(slotUrlChanged(QUrl)), Qt::QueuedConnection);
}

/** @brief Read settings from ini-file
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setSettings(bool newTab)
{
  if (type_ == TabTypeDownloads) return;

  if (newTab) {
    if (type_ < TabTypeWeb) {
      newsTabWidgetSplitter_->restoreState(
            rssl_->settings_->value("NewsTabSplitterState").toByteArray());

      newsView_->setFont(
            QFont(rssl_->newsListFontFamily_, rssl_->newsListFontSize_));
      newsModel_->formatDate_ = rssl_->formatDate_;
      newsModel_->formatTime_ = rssl_->formatTime_;
      newsModel_->simplifiedDateTime_ = rssl_->simplifiedDateTime_;

      newsModel_->textColor_ = rssl_->newsListTextColor_;
      newsView_->setStyleSheet(QString("#newsView_ {background: %1;}").arg(rssl_->newsListBackgroundColor_));
      newsModel_->newNewsTextColor_ = rssl_->newNewsTextColor_;
      newsModel_->unreadNewsTextColor_ = rssl_->unreadNewsTextColor_;
      newsModel_->focusedNewsTextColor_ = rssl_->focusedNewsTextColor_;
      newsModel_->focusedNewsBGColor_ = rssl_->focusedNewsBGColor_;

      QFile cssFile;
      cssFile.setFileName(rssl_->appDataDirPath_+ "/style/news.css");
      if (!cssFile.open(QFile::ReadOnly)) {
        cssFile.setFileName(":/style/newsStyle");
        cssFile.open(QFile::ReadOnly);
      }
      cssString_ = QString::fromUtf8(cssFile.readAll()).
          arg(rssl_->newsTextFontFamily_).
          arg(rssl_->newsTextFontSize_).
          arg(rssl_->newsTitleFontFamily_).
          arg(rssl_->newsTitleFontSize_).
          arg(0).
          arg(qApp->palette().color(QPalette::Dark).name()). // color separator
          arg(rssl_->newsBackgroundColor_). // news background
          arg(rssl_->newsTitleBackgroundColor_). // title background
          arg(rssl_->linkColor_). // link color
          arg(rssl_->titleColor_). // title color
          arg(rssl_->dateColor_). // date color
          arg(rssl_->authorColor_). // author color
          arg(rssl_->newsTextColor_); // text color
      cssFile.close();
    }

    webView_->setZoomFactor(qreal(rssl_->defaultZoomPages_)/100.0);

    if (rssl_->externalBrowserOn_ <= 0) {
      webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    } else {
      webView_->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    }
  }

  QModelIndex feedIndex = feedsTreeModel_->getIndexById(feedId_);

  if (type_ == TabTypeFeed) {
    autoLoadImages_ = feedsTreeModel_->dataField(feedIndex, "displayEmbeddedImages").toInt();
  }
  webView_->settings()->setAttribute(
        QWebSettings::AutoLoadImages, autoLoadImages_);
  rssl_->autoLoadImages_ = !autoLoadImages_;
  rssl_->setAutoLoadImages(false);

  if (type_ < TabTypeWeb) {
    newsView_->setAlternatingRowColors(rssl_->alternatingRowColorsNews_);
    if (!newTab)
      newsModel_->setFilter("feedId=-1");
    newsHeader_->setColumns(rssl_, feedIndex);
    rssl_->slotUpdateStatus(feedId_, false);
    rssl_->newsFilter_->setEnabled(type_ == TabTypeFeed);
    separatorRAct_->setVisible(type_ == TabTypeDel);
    rssl_->restoreNewsAct_->setVisible(type_ == TabTypeDel);
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

    if (type_ != TabTypeWeb) {
      findText_->retranslateStrings();
      newsHeader_->retranslateStrings();
    }
  }

  closeButton_->setToolTip(tr("Close Tab"));
}

/** @brief Process mouse click in news list
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsViewClicked(QModelIndex index)
{
  if (QApplication::keyboardModifiers() == Qt::NoModifier) {
    slotNewsViewSelected(index);
  }
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotNewsViewSelected(QModelIndex index, bool clicked)
{
  QElapsedTimer timer;
  timer.start();
  qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

  int newsId = newsModel_->index(index.row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

  if (rssl_->markNewsReadOn_ && rssl_->markPrevNewsRead_ &&
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
    if (rssl_->markNewsReadOn_ && rssl_->markCurNewsRead_) {
      if (rssl_->markNewsReadTime_ == 0) {
        slotSetItemRead(newsView_->currentIndex(), 1);
      } else {
        markNewsReadTimer_->start(rssl_->markNewsReadTime_*1000);
      }
    }

    if (type_ == TabTypeFeed) {
      // Write current news to feed
      QString qStr = QString("UPDATE feeds SET currentNews='%1' WHERE id=='%2'").
          arg(newsId).arg(feedId_);
      rssl_->sqlQueryExec(qStr);

      qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

      QModelIndex feedIndex = feedsTreeModel_->getIndexById(feedId_);
      feedsTreeModel_->setData(feedsTreeModel_->indexSibling(feedIndex, "currentNews"), newsId);
    } else if (type_ == TabTypeLabel) {
      QString qStr = QString("UPDATE labels SET currentNews='%1' WHERE id=='%2'").
          arg(newsId).
          arg(rssl_->categoriesTree_->currentItem()->text(2).toInt());
      rssl_->sqlQueryExec(qStr);

      rssl_->categoriesTree_->currentItem()->setText(3, QString::number(newsId));
    }

    qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();

    updateWebView(index);

    qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();
  }
  currentNewsIdOld = newsId;
  qDebug() << __FUNCTION__ << __LINE__ << timer.elapsed();
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotNewsViewDoubleClicked(QModelIndex index)
{
  if (!index.isValid()) return;

  QString linkString = newsModel_->record(
        index.row()).field("link_href").value().toString();
  if (linkString.isEmpty())
    linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();

  QUrl url = QUrl::fromEncoded(linkString.simplified().toUtf8());
  slotLinkClicked(url);
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotNewsMiddleClicked(QModelIndex index)
{
  if (!index.isValid()) return;

  if (rssl_->markNewsReadOn_ && rssl_->markCurNewsRead_)
    slotSetItemRead(index, 1);

  QString linkString = newsModel_->record(
        index.row()).field("link_href").value().toString();
  if (linkString.isEmpty())
    linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();

  if (QApplication::keyboardModifiers() == Qt::NoModifier) {
    webView_->buttonClick_ = MIDDLE_BUTTON;
  } else if (QApplication::keyboardModifiers() == Qt::AltModifier) {
    webView_->buttonClick_ = LEFT_BUTTON_ALT;
  } else {
    webView_->buttonClick_ = MIDDLE_BUTTON_MOD;
  }

  QUrl url = QUrl::fromEncoded(linkString.simplified().toUtf8());
  slotLinkClicked(url);
}

/** @brief Process pressing UP-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsUpPressed()
{
  if (type_ >= TabTypeWeb) return;
  newsView_->clearSelection();

  if (!newsView_->currentIndex().isValid()) {
    if (newsModel_->rowCount() > 0) {
      newsView_->setCurrentIndex(newsModel_->index(0, newsModel_->fieldIndex("title")));
      slotNewsViewSelected(newsModel_->index(0, newsModel_->fieldIndex("title")));
    }
    return;
  }

  int row = newsView_->currentIndex().row();
  if (row == 0) {
    newsView_->setCurrentIndex(newsView_->currentIndex());
    return;
  }
  else row--;

  int value = newsView_->verticalScrollBar()->value();
  int pageStep = newsView_->verticalScrollBar()->pageStep();
  if (row < (value + pageStep/2))
    newsView_->verticalScrollBar()->setValue(row - pageStep/2);

  newsView_->setCurrentIndex(newsModel_->index(row, newsModel_->fieldIndex("title")));
  slotNewsViewSelected(newsModel_->index(row, newsModel_->fieldIndex("title")));
}

/** @brief Process pressing DOWN-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsDownPressed()
{
  if (type_ >= TabTypeWeb) return;
  newsView_->clearSelection();

  if (!newsView_->currentIndex().isValid()) {
    if (newsModel_->rowCount() > 0) {
      newsView_->setCurrentIndex(newsModel_->index(0, newsModel_->fieldIndex("title")));
      slotNewsViewSelected(newsModel_->index(0, newsModel_->fieldIndex("title")));
    }
    return;
  }

  int row = newsView_->currentIndex().row();
  if ((row+1) == newsModel_->rowCount()) {
    newsView_->setCurrentIndex(newsView_->currentIndex());
    return;
  }
  else row++;

  int value = newsView_->verticalScrollBar()->value();
  int pageStep = newsView_->verticalScrollBar()->pageStep();
  if (row > (value + pageStep/2))
    newsView_->verticalScrollBar()->setValue(row - pageStep/2);

  newsView_->setCurrentIndex(newsModel_->index(row, newsModel_->fieldIndex("title")));
  slotNewsViewSelected(newsModel_->index(row, newsModel_->fieldIndex("title")));
}

/** @brief Process pressing HOME-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsHomePressed()
{
  int row = 0;
  newsView_->setCurrentIndex(newsModel_->index(row, newsModel_->fieldIndex("title")));
  slotNewsViewSelected(newsModel_->index(row, newsModel_->fieldIndex("title")));
}

/** @brief Process pressing END-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsEndPressed()
{
  int row = newsModel_->rowCount() - 1;
  newsView_->setCurrentIndex(newsModel_->index(row, newsModel_->fieldIndex("title")));
  slotNewsViewSelected(newsModel_->index(row, newsModel_->fieldIndex("title")));
}

/** @brief Process pressing PageUp-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsPageUpPressed()
{
  if (!newsView_->currentIndex().isValid()) {
    if (newsModel_->rowCount() > 0) {
      newsView_->setCurrentIndex(newsModel_->index(0, newsModel_->fieldIndex("title")));
      slotNewsViewSelected(newsModel_->index(0, newsModel_->fieldIndex("title")));
    }
    return;
  }

  int row = newsView_->currentIndex().row() - newsView_->verticalScrollBar()->pageStep();
  if (row < 0) row = 0;
  newsView_->setCurrentIndex(newsModel_->index(row, newsModel_->fieldIndex("title")));
  slotNewsViewSelected(newsModel_->index(row, newsModel_->fieldIndex("title")));
}

/** @brief Process pressing PageDown-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsPageDownPressed()
{
  if (!newsView_->currentIndex().isValid()) {
    if (newsModel_->rowCount() > 0) {
      newsView_->setCurrentIndex(newsModel_->index(0, newsModel_->fieldIndex("title")));
      slotNewsViewSelected(newsModel_->index(0, newsModel_->fieldIndex("title")));
    }
    return;
  }

  int row = newsView_->currentIndex().row() + newsView_->verticalScrollBar()->pageStep();
  if ((row+1) > newsModel_->rowCount()) row = newsModel_->rowCount()-1;
  newsView_->setCurrentIndex(newsModel_->index(row, newsModel_->fieldIndex("title")));
  slotNewsViewSelected(newsModel_->index(row, newsModel_->fieldIndex("title")));
}

/** @brief Mark news Read
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotSetItemRead(QModelIndex index, int read)
{
  markNewsReadTimer_->stop();
  if (!index.isValid() || (newsModel_->rowCount() == 0)) return;

  bool changed = false;
  int newsId = newsModel_->index(index.row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

  if (read == 1) {
    if (newsModel_->index(index.row(), newsModel_->fieldIndex("new")).data(Qt::EditRole).toInt() == 1) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("new")),
            0);
      rssl_->sqlQueryExec(QString("UPDATE news SET new=0 WHERE id=='%1'").arg(newsId));
    }
    if (newsModel_->index(index.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() == 0) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
            1);
      rssl_->sqlQueryExec(QString("UPDATE news SET read=1 WHERE id=='%1'").arg(newsId));
      changed = true;
    }
  } else {
    if (newsModel_->index(index.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() != 0) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
            0);
      rssl_->sqlQueryExec(QString("UPDATE news SET read=0 WHERE id=='%1'").arg(newsId));
      changed = true;
    }
  }

  if (changed) {
    newsView_->viewport()->update();
    int feedId = newsModel_->index(index.row(), newsModel_->fieldIndex("feedId")).
        data(Qt::EditRole).toInt();
    rssl_->slotUpdateStatus(feedId);
    rssl_->recountCategoryCounts();
  }
}

/** @brief Mark news Star
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotSetItemStar(QModelIndex index, int starred)
{
  if (!index.isValid()) return;

  newsModel_->setData(index, starred);

  int newsId = newsModel_->index(index.row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();
  rssl_->sqlQueryExec(QString("UPDATE news SET starred='%1' WHERE id=='%2'").
                            arg(starred).arg(newsId));
  rssl_->recountCategoryCounts();
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
    if (newsModel_->index(curIndex.row(), newsModel_->fieldIndex("read")).data(Qt::EditRole).toInt() == 0) {
      slotSetItemRead(curIndex, 1);
    } else {
      slotSetItemRead(curIndex, 0);
    }
  } else {
    QStringList feedIdList;

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
      newsModel_->setData(
                  newsModel_->index(curIndex.row(), newsModel_->fieldIndex("new")),
                  0);
      newsModel_->setData(
                  newsModel_->index(curIndex.row(), newsModel_->fieldIndex("read")),
                  markRead);

      int newsId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();
      q.exec(QString("UPDATE news SET new=0, read='%1' WHERE id=='%2'").
             arg(markRead).arg(newsId));
      QString feedId = newsModel_->index(i, newsModel_->fieldIndex("feedId")).data(Qt::EditRole).toString();
      if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
    }
    db_.commit();

    foreach (QString feedId, feedIdList) {
      rssl_->slotUpdateStatus(feedId.toInt());
    }
    rssl_->recountCategoryCounts();
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
    int newsId = newsModel_->index(i, newsModel_->fieldIndex("id")).data().toInt();
    q.exec(QString("UPDATE news SET read=1 WHERE id=='%1' AND read=0").arg(newsId));
    q.exec(QString("UPDATE news SET new=0 WHERE id=='%1' AND new=1").arg(newsId));

    QString feedId = newsModel_->index(i, newsModel_->fieldIndex("feedId")).data(Qt::EditRole).toString();
    if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
  }
  db_.commit();

  int currentRow = newsView_->currentIndex().row();

  newsModel_->select();

  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  newsView_->setCurrentIndex(newsModel_->index(currentRow, newsModel_->fieldIndex("title")));

  foreach (QString feedId, feedIdList) {
    rssl_->slotUpdateStatus(feedId.toInt());
  }
  rssl_->recountCategoryCounts();
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
      newsModel_->setData(indexes.at(i), markStar);

      int newsId = newsModel_->index(i, newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();
      QSqlQuery q;
      q.exec(QString("UPDATE news SET starred='%1' WHERE id=='%2'").
             arg(markStar).arg(newsId));
    }
    db_.commit();

    rssl_->recountCategoryCounts();
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
      if (newsModel_->index(curIndex.row(), newsModel_->fieldIndex("starred")).data(Qt::EditRole).toInt() &&
          rssl_->notDeleteStarred_)
        return;
      QString labelStr = newsModel_->index(curIndex.row(),newsModel_->fieldIndex("label")).
          data(Qt::EditRole).toString();
      if (!(labelStr.isEmpty() || (labelStr == ",")) && rssl_->notDeleteLabeled_)
        return;

      slotSetItemRead(curIndex, 1);

      newsModel_->setData(curIndex, 1);
      newsModel_->setData(newsModel_->index(curIndex.row(), newsModel_->fieldIndex("deleteDate")),
                          QDateTime::currentDateTime().toString(Qt::ISODate));

      QString feedId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("feedId")).data(Qt::EditRole).toString();
      if (!feedIdList.contains(feedId)) feedIdList.append(feedId);

      newsModel_->submitAll();
    } else {
      db_.transaction();
      QSqlQuery q;
      for (int i = cnt-1; i >= 0; --i) {
        curIndex = indexes.at(i);
        if (newsModel_->index(curIndex.row(), newsModel_->fieldIndex("starred")).data(Qt::EditRole).toInt() &&
            rssl_->notDeleteStarred_)
          continue;
        QString labelStr = newsModel_->index(curIndex.row(),newsModel_->fieldIndex("label")).
            data(Qt::EditRole).toString();
        if (!(labelStr.isEmpty() || (labelStr == ",")) && rssl_->notDeleteLabeled_)
          continue;

        int newsId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();
        q.exec(QString("UPDATE news SET new=0, read=2, deleted=1, deleteDate='%1' WHERE id=='%2'").
               arg(QDateTime::currentDateTime().toString(Qt::ISODate)).
               arg(newsId));

        QString feedId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("feedId")).data(Qt::EditRole).toString();
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

      int newsId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();
      q.exec(QString("UPDATE news SET description='', content='', received='', "
                     "author_name='', author_uri='', author_email='', "
                     "category='', new='', read='', starred='', label='', "
                     "deleteDate='', feedParentId='', deleted=2 WHERE id=='%1'").
             arg(newsId));

      QString feedId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("feedId")).data(Qt::EditRole).toString();
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
  else curIndex = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("title"));
  newsView_->setCurrentIndex(curIndex);
  slotNewsViewSelected(curIndex);

  foreach (QString feedId, feedIdList) {
    rssl_->slotUpdateStatus(feedId.toInt());
  }
  rssl_->recountCategoryCounts();
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
    int newsId = newsModel_->index(i, newsModel_->fieldIndex("id")).data().toInt();

    if (type_ != TabTypeDel) {
      if (newsModel_->index(i, newsModel_->fieldIndex("starred")).data(Qt::EditRole).toInt() &&
          rssl_->notDeleteStarred_)
        continue;
      QString labelStr = newsModel_->index(i, newsModel_->fieldIndex("label")).data(Qt::EditRole).toString();
      if (!(labelStr.isEmpty() || (labelStr == ",")) && rssl_->notDeleteLabeled_)
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

    QString feedId = newsModel_->index(i, newsModel_->fieldIndex("feedId")).data(Qt::EditRole).toString();
    if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
  }
  db_.commit();

  newsModel_->select();

  slotNewsViewSelected(QModelIndex());

  foreach (QString feedId, feedIdList) {
    rssl_->slotUpdateStatus(feedId.toInt());
  }
  rssl_->recountCategoryCounts();
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

    QString feedId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("feedId")).
        data(Qt::EditRole).toString();
    if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
  } else {
    db_.transaction();
    QSqlQuery q;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      int newsId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("id")).data().toInt();
      q.exec(QString("UPDATE news SET deleted=0, deleteDate='' WHERE id=='%1'").
             arg(newsId));

      QString feedId = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("feedId")).
          data(Qt::EditRole).toString();
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
  else curIndex = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("title"));
  newsView_->setCurrentIndex(curIndex);
  slotNewsViewSelected(curIndex);
  rssl_->slotUpdateStatus(feedId_);
  rssl_->recountCategoryCounts();

  foreach (QString feedId, feedIdList) {
    rssl_->slotUpdateStatus(feedId.toInt());
  }
  rssl_->recountCategoryCounts();
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

    QModelIndex index = indexes.at(i);
    QString linkString = newsModel_->record(index.row()).field("link_href").value().toString();
    if (linkString.isEmpty())
      linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();
    copyStr.append(linkString.simplified());
  }

  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setText(copyStr);
}

/** @brief Sort news by Star or Read column
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotSort(int column, int order)
{
  QString strId;
  if (feedsTreeModel_->isFolder(feedsTreeModel_->getIndexById(feedId_))) {
    strId = QString("(%1)").arg(rssl_->getIdFeedsString(feedId_));
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
  QSqlQuery q;
  q.exec(qStr);
  newsModel_->sort(newsModel_->fieldIndex("rights"), (Qt::SortOrder)order);
}

/** @brief Load/Update browser contents
 *----------------------------------------------------------------------------*/
void NewsTabWidget::updateWebView(QModelIndex index)
{
  if (!index.isValid()) {
    hideWebContent();
    return;
  }

  QString linkString = newsModel_->record(index.row()).field("link_href").value().toString();
  if (linkString.isEmpty())
    linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();
  linkString = linkString.simplified();
  QUrl newsUrl = QUrl::fromEncoded(linkString.toUtf8());

  bool showDescriptionNews_ = rssl_->showDescriptionNews_;

  QModelIndex currentIndex = feedsProxyModel_->mapToSource(feedsTreeView_->currentIndex());
  QVariant displayNews = feedsTreeModel_->dataField(currentIndex, "displayNews");
  if (!displayNews.toString().isEmpty())
    showDescriptionNews_ = !displayNews.toInt();

  if (!showDescriptionNews_) {
    setWebToolbarVisible(true, false);

    webView_->history()->setMaximumItemCount(0);
    webView_->load(newsUrl);
    webView_->history()->setMaximumItemCount(100);
  } else {
    setWebToolbarVisible(false, false);

    QString titleString = newsModel_->record(index.row()).field("title").value().toString();

    QDateTime dtLocal;
    QString dateString = newsModel_->record(index.row()).field("published").value().toString();
    if (!dateString.isNull()) {
      QDateTime dtLocalTime = QDateTime::currentDateTime();
      QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
      int nTimeShift = dtLocalTime.secsTo(dtUTC);

      QDateTime dt = QDateTime::fromString(dateString, Qt::ISODate);
      dtLocal = dt.addSecs(nTimeShift);
    } else {
      dtLocal = QDateTime::fromString(
            newsModel_->record(index.row()).field("received").value().toString(),
            Qt::ISODate);
    }
    if (QDateTime::currentDateTime().date() <= dtLocal.date())
      dateString = dtLocal.toString(rssl_->formatTime_);
    else
      dateString = dtLocal.toString(rssl_->formatDate_ + " " + rssl_->formatTime_);

    // Create author panel from news author
    QString authorString;
    QString authorName = newsModel_->record(index.row()).field("author_name").value().toString();
    QString authorEmail = newsModel_->record(index.row()).field("author_email").value().toString();
    QString authorUri = newsModel_->record(index.row()).field("author_uri").value().toString();
    //  qDebug() << "author_news:" << authorName << authorEmail << authorUri;
    authorString = authorName;
    if (!authorEmail.isEmpty())
      authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
    if (!authorUri.isEmpty())
      authorString.append(QString(" <a href='%1'>page</a>"). arg(authorUri));

    // If news author is absent, create author panel from feed author
    // @note(arhohryakov:2012.01.03) Author is got from current feed, because
    //   news is belong to it
    if (authorString.isEmpty()) {
      authorName  = feedsTreeModel_->dataField(currentIndex, "author_name").toString();
      authorEmail = feedsTreeModel_->dataField(currentIndex, "author_email").toString();
      authorUri   = feedsTreeModel_->dataField(currentIndex, "author_uri").toString();

      //    qDebug() << "author_feed:" << authorName << authorEmail << authorUri;
      authorString = authorName;
      if (!authorEmail.isEmpty())
        authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
      if (!authorUri.isEmpty())
        authorString.append(QString(" <a href='%1'>page</a>").arg(authorUri));
    }

    QString commentsStr;
    QString commentsUrl = QUrl::fromPercentEncoding(newsModel_->record(index.row()).field("comments").value().toByteArray());
    if (!commentsUrl.isEmpty()) {
      commentsStr = QString("<a href=\"%1\"> %2</a>").arg(commentsUrl).arg(tr("Comments"));
    }

    QString category = newsModel_->record(index.row()).field("category").value().toString();

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

    QString content = newsModel_->record(index.row()).field("content").value().toString();
    QString description = newsModel_->record(index.row()).field("description").value().toString();
    if (content.isEmpty() || (description.length() > content.length())) {
      content = description;
    }

    QString enclosureStr;
    QString enclosureUrl = newsModel_->record(index.row()).field("enclosure_url").value().toString();
    if (!enclosureUrl.isEmpty()) {
      QString type = newsModel_->record(index.row()).field("enclosure_type").value().toString();
      if (type.contains("image")) {
        QString imgUrl = newsModel_->record(index.row()).
            field("enclosure_url").value().toString();
        if (!content.contains(imgUrl)) {
          enclosureStr = QString("<IMG SRC=\"%1\" class=\"enclosureImg\"><p>").
              arg(imgUrl);
        }
      } else {
        if (type.contains("audio")) type = tr("audio");
        else if (type.contains("video")) type = tr("video");
        else type = tr("media");

        enclosureStr = QString("<a href=\"%1\" class=\"enclosure\"> %2 %3 </a><p>").
            arg(QUrl::fromPercentEncoding(newsModel_->record(index.row()).field("enclosure_url").value().toByteArray())).
            arg(tr("Link to")).arg(type);
      }
    }

    content = enclosureStr + content;

    if (!linkString.isEmpty()) {
        titleString = QString("<a href='%1' class='unread'>%2</a>").
            arg(QUrl::fromPercentEncoding(linkString.toUtf8())).arg(titleString);
    }

    QString feedId = newsModel_->index(index.row(),newsModel_->fieldIndex("feedId")).
        data(Qt::EditRole).toString();

    QString languageString;
    QSqlQuery q;
    q.exec(QString("SELECT language FROM feeds WHERE id=='%1'").arg(feedId));
    if (q.next()) languageString = q.value(0).toString().toLower();
    bool ltr = true;
    if (languageString == "ar") ltr = false;

    QString cssStr = cssString_.
        arg(ltr ? "left" : "right"). // text-align
        arg(ltr ? "ltr" : "rtl"); // direction

    QString htmlStr = htmlString_.
        arg(cssStr).
        arg(titleString).
        arg(dateString).
        arg(authorString).
        arg(content);

    QUrl url;
    url.setScheme(newsUrl.scheme());
    url.setHost(newsUrl.host());
    emit signalSetHtmlWebView(htmlStr, url);
  }
}

/** @brief Asynchorous update web view
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setHtmlWebView(const QString &html, const QUrl &baseUrl)
{
  webView_->history()->setMaximumItemCount(0);
  webView_->setHtml(html, baseUrl);
  webView_->history()->setMaximumItemCount(100);
}

void NewsTabWidget::hideWebContent()
{
  emit signalSetHtmlWebView();
  setWebToolbarVisible(false, false);
}

void NewsTabWidget::slotLinkClicked(QUrl url)
{
  if (url.scheme() == QLatin1String("mailto")) {
    QDesktopServices::openUrl(url);
    return;
  }

  if (url.host().isEmpty()) {
    QModelIndex currentIndex = feedsProxyModel_->mapToSource(feedsTreeView_->currentIndex());
    QUrl hostUrl = feedsTreeModel_->dataField(currentIndex, "htmlUrl").toString();

    url.setScheme(hostUrl.scheme());
    url.setHost(hostUrl.host());
  }
  if ((rssl_->externalBrowserOn_ <= 0) &&
      (webView_->buttonClick_ != LEFT_BUTTON_ALT)) {
    if (webView_->buttonClick_ == LEFT_BUTTON) {
      if (!webControlPanel_->isVisible())
        setWebToolbarVisible(true, false);
      webView_->load(url);
    } else {
      if ((webView_->buttonClick_ == MIDDLE_BUTTON) ||
          (webView_->buttonClick_ == LEFT_BUTTON_CTRL)) {
        rssl_->openNewsTab_ = NEW_TAB_BACKGROUND;
      } else {
        rssl_->openNewsTab_ = NEW_TAB_FOREGROUND;
      }
      if (!rssl_->openLinkInBackgroundEmbedded_) {
        if (rssl_->openNewsTab_ == NEW_TAB_BACKGROUND)
          rssl_->openNewsTab_ = NEW_TAB_FOREGROUND;
        else
          rssl_->openNewsTab_ = NEW_TAB_BACKGROUND;
      }

      rssl_->createWebTab(url);
    }
  } else {
    openUrl(url);
  }
  webView_->buttonClick_ = 0;
}
//----------------------------------------------------------------------------
void NewsTabWidget::slotLinkHovered(const QString &link, const QString &, const QString &)
{
  rssl_->statusBar()->showMessage(link.simplified(), 3000);
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
    updateWebView(newsView_->currentIndex());
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

  int externalBrowserOn_ = rssl_->externalBrowserOn_;
  rssl_->externalBrowserOn_ = 0;
  slotNewsViewDoubleClicked(newsView_->currentIndex());
  rssl_->externalBrowserOn_ = externalBrowserOn_;
}

/** @brief Open news in external browser
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openInExternalBrowserNews()
{
  if (type_ == TabTypeDownloads) return;

  if (type_ != TabTypeWeb) {
    QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

    int cnt = indexes.count();
    if (cnt == 0) return;

    for (int i = cnt-1; i >= 0; --i) {
      QModelIndex index = indexes.at(i);
      QString linkString = newsModel_->record(
            index.row()).field("link_href").value().toString();
      if (linkString.isEmpty())
        linkString = newsModel_->record(index.row()).field("link_alternate").value().toString();
      QUrl url = QUrl::fromEncoded(linkString.simplified().toUtf8());
      openUrl(url);
    }
  } else {
    openUrl(webView_->url());
  }
}

/** @brief Set browser position
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setBrowserPosition()
{
  if (type_ == TabTypeDownloads) return;

  int idx = newsTabWidgetSplitter_->indexOf(webWidget_);

  switch (rssl_->browserPosition_) {
  case TOP_POSITION: case LEFT_POSITION:
    newsTabWidgetSplitter_->insertWidget(0, newsTabWidgetSplitter_->widget(idx));
    break;
  default:
    newsTabWidgetSplitter_->insertWidget(1, newsTabWidgetSplitter_->widget(idx));
  }

  switch (rssl_->browserPosition_) {
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
  rssl_->slotCloseTab(rssl_->stackedWidget_->indexOf(this));
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
    if (rssl_->markNewsReadOn_ && rssl_->markCurNewsRead_)
      slotSetItemRead(index, 1);

    QString linkString = newsModel_->record(row).field("link_href").value().toString();
    if (linkString.isEmpty())
      linkString = newsModel_->record(row).field("link_alternate").value().toString();

    QUrl url = QUrl::fromEncoded(linkString.simplified().toUtf8());
    if (url.host().isEmpty()) {
      int feedId = newsModel_->dataField(row, "feedId").toInt();
      QUrl hostUrl;
      QSqlQuery q;
      q.exec(QString("SELECT htmlUrl FROM feeds WHERE id=='%1'").arg(feedId));
      if (q.next())
        hostUrl.setUrl(q.value(0).toString());
      url.setScheme(hostUrl.scheme());
      url.setHost(hostUrl.host());
    }

    rssl_->createWebTab(url);
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
  int externalBrowserOn_ = rssl_->externalBrowserOn_;
  rssl_->externalBrowserOn_ = 0;

  if (QApplication::keyboardModifiers() == Qt::NoModifier) {
    webView_->buttonClick_ = MIDDLE_BUTTON;
  } else {
    webView_->buttonClick_ = MIDDLE_BUTTON_MOD;
  }

  slotLinkClicked(linkUrl_);
  rssl_->externalBrowserOn_ = externalBrowserOn_;
}

inline static bool launch(const QUrl &url, const QString &client)
{
  return (QProcess::startDetached(client + QLatin1Char(' ') + url.toString().toUtf8()));
}

/** @brief Open link in browser
 *----------------------------------------------------------------------------*/
bool NewsTabWidget::openUrl(const QUrl &url)
{
  if (!url.isValid())
    return false;

  if (url.scheme() == QLatin1String("mailto"))
      return QDesktopServices::openUrl(url);

  rssl_->openingLink_ = true;
  if ((rssl_->externalBrowserOn_ == 2) || (rssl_->externalBrowserOn_ == -1)) {
#if defined(Q_OS_WIN)
    quintptr returnValue = (quintptr)ShellExecute(
          0, 0,
          (wchar_t *)QString::fromUtf8(rssl_->externalBrowser_.toUtf8()).utf16(),
          (wchar_t *)QString::fromUtf8(url.toEncoded().constData()).utf16(),
          0, SW_SHOWNORMAL);
    if (returnValue > 32)
      return true;
#elif defined(HAVE_X11)
    if (launch(url, rssl_->externalBrowser_.toUtf8()))
      return true;
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
    int newsId = newsModel_->index(
          newsView_->currentIndex().row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

    QString filterStr;
    switch (type_) {
      case TabTypeUnread:
      case TabTypeStar:
      case TabTypeDel:
      case TabTypeLabel:
        filterStr = categoryFilterStr_;
        break;
      default:
        filterStr = rssl_->newsFilterStr;
    }

    if (!text.isEmpty()) {
      if (objectName == "findTitleAct") {
        filterStr.append(
              QString(" AND title LIKE '%%1%'").arg(text));
      } else if (objectName == "findAuthorAct") {
        filterStr.append(
              QString(" AND author_name LIKE '%%1%'").arg(text));
      } else if (objectName == "findCategoryAct") {
        filterStr.append(
              QString(" AND category LIKE '%%1%'").arg(text));
      } else if (objectName == "findContentAct") {
        filterStr.append(
              QString(" AND (content LIKE '%%1%' OR description LIKE '%%1%')").
              arg(text));
      } else {
        filterStr.append(
              QString(" AND (title LIKE '%%1%' OR author_name LIKE '%%1%' "
                      "OR category LIKE '%%1%' OR content LIKE '%%1%' "
                      "OR description LIKE '%%1%')").
              arg(text));
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
  if (findText_->findGroup_->checkedAction()->objectName() == "findInNewsAct")
    webView_->findText("", QWebPage::HighlightAllOccurrences);
  else {
    int newsId = newsModel_->index(
          newsView_->currentIndex().row(), newsModel_->fieldIndex("id")).data(Qt::EditRole).toInt();

    QString filterStr;
    switch (type_) {
      case TabTypeUnread:
      case TabTypeStar:
      case TabTypeDel:
      case TabTypeLabel:
        filterStr = categoryFilterStr_;
        break;
      default:
        filterStr = rssl_->newsFilterStr;
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
//----------------------------------------------------------------------------
void NewsTabWidget::showContextWebPage(const QPoint &p)
{
  QMenu menu;
  QMenu *pageMenu = webView_->page()->createStandardContextMenu();
  if (pageMenu) {
    menu.addActions(pageMenu->actions());

    const QWebHitTestResult &hitTest = webView_->page()->mainFrame()->hitTestContent(p);
    if (!hitTest.linkUrl().isEmpty() && hitTest.linkUrl().scheme() != "javascript") {
      linkUrl_ = hitTest.linkUrl();
      if (rssl_->externalBrowserOn_ <= 0) {
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
      menu.addAction(rssl_->autoLoadImagesToggle_);
      menu.addSeparator();
      menu.addAction(rssl_->printAct_);
      menu.addAction(rssl_->printPreviewAct_);
      menu.addSeparator();
      menu.addAction(rssl_->savePageAsAct_);
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

  if (linkUrl_.host().isEmpty()) {
    QModelIndex currentIndex = feedsProxyModel_->mapToSource(feedsTreeView_->currentIndex());
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
                               rssl_->browserToolbarToggle_->isChecked());
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

  rssl_->recountCategoryCounts();
}

void NewsTabWidget::slotNewslLabelClicked(QModelIndex index)
{
  if (!newsView_->selectionModel()->isSelected(index)) {
    newsView_->selectionModel()->clearSelection();
    newsView_->selectionModel()->select(
          index, QItemSelectionModel::Select|QItemSelectionModel::Rows);
  }
  rssl_->newsLabelMenu_->popup(
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
void NewsTabWidget::setTextTab(const QString &text, int width)
{
  QString textTab = newsTextTitle_->fontMetrics().elidedText(
        text, Qt::ElideRight, width);
  newsTextTitle_->setText(textTab);
  newsTitleLabel_->setToolTip(text);

  emit signalSetTextTab(text, this);
}

/** @brief Share news
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotShareNews(QAction *action)
{
  if (type_ >= TabTypeWeb) return;

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
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("subject", title);
      urlQuery.addQueryItem("body", linkString.simplified());
      url.setQuery(urlQuery);
#else
      url.addQueryItem("subject", title);
      url.addQueryItem("body", linkString.simplified());
#endif
      openUrl(url);
    }
    else {
      if (action->objectName() == "evernoteShareAct") {
        url.setUrl("https://www.evernote.com/clip.action");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString.simplified());
        urlQuery.addQueryItem("title", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString.simplified());
        url.addQueryItem("title", title);
#endif
      }
      else if (action->objectName() == "gplusShareAct") {
        url.setUrl("https://plus.google.com/share");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString.simplified());
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString.simplified());
#endif
      }
      else if (action->objectName() == "facebookShareAct") {
        url.setUrl("https://www.facebook.com/sharer.php");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("u", linkString.simplified());
        urlQuery.addQueryItem("t", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("u", linkString.simplified());
        url.addQueryItem("t", title);
#endif
      }
      else if (action->objectName() == "livejournalShareAct") {
        url.setUrl("http://www.livejournal.com/update.bml");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("event", linkString.simplified());
        urlQuery.addQueryItem("subject", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("event", linkString.simplified());
        url.addQueryItem("subject", title);
#endif
      }
      else if (action->objectName() == "pocketShareAct") {
        url.setUrl("https://getpocket.com/save");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString.simplified());
        urlQuery.addQueryItem("title", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString.simplified());
        url.addQueryItem("title", title);
#endif
      }
      else if (action->objectName() == "twitterShareAct") {
        url.setUrl("https://twitter.com/share");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString.simplified());
        urlQuery.addQueryItem("text", title);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString.simplified());
        url.addQueryItem("text", title);
#endif
      }
      else if (action->objectName() == "vkShareAct") {
        url.setUrl("https://vk.com/share.php");
#ifdef HAVE_QT5
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("url", linkString.simplified());
        urlQuery.addQueryItem("title", title);
        urlQuery.addQueryItem("description", "");
        urlQuery.addQueryItem("image", "");
        url.setQuery(urlQuery);
#else
        url.addQueryItem("url", linkString.simplified());
        url.addQueryItem("title", title);
        url.addQueryItem("description", "");
        url.addQueryItem("image", "");
#endif
      }

      if (rssl_->externalBrowserOn_ <= 0) {
        rssl_->openNewsTab_ = NEW_TAB_FOREGROUND;
        rssl_->createWebTab(url);
      } else openUrl(url);
    }
  }
}
//-----------------------------------------------------------------------------
int NewsTabWidget::getUnreadCount(QString countString)
{
  if (countString.isEmpty()) return 0;

  countString.remove(QRegExp("[()]"));
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
