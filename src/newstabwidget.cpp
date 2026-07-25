/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2021 QuiteRSS Team <quiterssteam@gmail.com>
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
#include "settings.h"

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
    createNewsList();

    newsTabWidgetSplitter_ = new QSplitter(this);
    newsTabWidgetSplitter_->setObjectName("newsTabWidgetSplitter");
    newsTabWidgetSplitter_->addWidget(newsWidget_);
  }

  QVBoxLayout *layout = new QVBoxLayout();
  layout->setMargin(0);
  layout->setSpacing(0);
  if (type_ == TabTypeDownloads)
    layout->addWidget(mainApp->downloadManager());
  else
    layout->addWidget(newsTabWidgetSplitter_);
  setLayout(layout);

  if (type_ != TabTypeDownloads) {
    newsTabWidgetSplitter_->setHandleWidth(1);
    newsTabWidgetSplitter_->setOrientation(Qt::Vertical);
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

  connect(findText_, SIGNAL(signalVisible(bool)),
          mainWindow_, SLOT(findText()));

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

/** @brief Read settings from ini-file
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setSettings(bool init, bool newTab)
{
  Settings settings;

  if (type_ == TabTypeDownloads) return;

  QString style = settings.value("Settings/styleApplication", "defaultStyle_").toString();
  if (style == "darkStyle_")
    newsIconMovie_->setFileName(":/images/loading_dark");
  else
    newsIconMovie_->setFileName(":/images/loading");

  if (newTab) {
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
  }

  QModelIndex feedIndex = feedsModel_->indexById(feedId_);

  if (init) {

    Q_UNUSED(feedIndex)
  }

  if (type_ == TabTypeFeed) {
    int layoutDirection = feedsModel_->dataField(feedIndex, "layoutDirection").toInt();
    if (!layoutDirection) {
      newsView_->setLayoutDirection(Qt::LeftToRight);
    } else {
      newsView_->setLayoutDirection(Qt::RightToLeft);
    }
  }

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
}

/** @brief Reload translation
 *----------------------------------------------------------------------------*/
void NewsTabWidget::retranslateStrings() {
  if (type_ != TabTypeDownloads) {
    findText_->retranslateStrings();
    newsHeader_->retranslateStrings();
  }

  closeButton_->setToolTip(tr("Close Tab"));
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

    mainWindow_->statusBar()->showMessage(getLinkNews(index.row()), 3000);
  }
  currentNewsIdOld = newsId;
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotNewsViewDoubleClicked(QModelIndex index)
{
  if (!index.isValid()) return;

  slotNewsViewSelected(index);
  openInExternalBrowserNews();
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotNewsMiddleClicked(QModelIndex index)
{
  if (!index.isValid()) return;

  if (mainWindow_->markNewsReadOn_ && mainWindow_->markCurNewsRead_)
    slotSetItemRead(index, 1);

  openNewsNewTab();
}

/** @brief Process pressing UP-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsUpPressed(QModelIndex index)
{
  if (type_ == TabTypeDownloads) return;

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
  if (type_ == TabTypeDownloads) return;

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
  if (type_ == TabTypeDownloads) return;
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
  if (type_ == TabTypeDownloads) return;
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
  if (type_ == TabTypeDownloads) return;

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
  if (type_ == TabTypeDownloads) return;

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
  if (type_ == TabTypeDownloads) return;

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
  if (type_ == TabTypeDownloads) return;

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
  if (type_ == TabTypeDownloads) return;

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

/** @brief Open news in browser
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openInBrowserNews()
{
  if (type_ == TabTypeDownloads) return;

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

  if (type_ != TabTypeDownloads) {
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
  }
}

/** @brief Close tab while press X-button
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotTabClose()
{
  mainWindow_->slotCloseTab(mainWindow_->stackedWidget_->indexOf(this));
}

/** @brief Placeholder
 *---------------------------------------------------------------------------*/
void NewsTabWidget::openNewsNewTab()
{
  if (type_ == TabTypeDownloads) return;

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

    openUrl(url);
  }
}

/** @brief Placeholder
 *---------------------------------------------------------------------------*/
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

/** @brief Placeholder
 *---------------------------------------------------------------------------*/
void NewsTabWidget::setLabelNews(int labelId)
{
  if (type_ == TabTypeDownloads) return;

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
  if (type_ == TabTypeDownloads) return;
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
  if (type_ == TabTypeDownloads) return;

  QList <int> sizes = newsTabWidgetSplitter_->sizes();
  sizes.insert(0, sizes.takeAt(0) - RESIZESTEP);
  newsTabWidgetSplitter_->setSizes(sizes);
}

void NewsTabWidget::increaseNewsList()
{
  if (type_ == TabTypeDownloads) return;

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
  bool externalApp = false;

  QList<QModelIndex> indexes;
  int cnt = 0;
  if (type_ != TabTypeDownloads) {
    indexes = newsView_->selectionModel()->selectedRows(0);
    cnt = indexes.count();
  } else if (false) {
    cnt = 1;
  }
  if (cnt == 0) return;

  for (int i = cnt-1; i >= 0; --i) {
    QString title;
    QString linkString;
    QString content;
    if (type_ != TabTypeDownloads) {
      title = newsModel_->dataField(indexes.at(i).row(), "title").toString();
      linkString = getLinkNews(indexes.at(i).row());

      content = newsModel_->dataField(indexes.at(i).row(), "content").toString();
      QString description = newsModel_->dataField(indexes.at(i).row(), "description").toString();
      if (content.isEmpty() || (description.length() > content.length())) {
        content = description;
      }
      QTextDocumentFragment textDocument = QTextDocumentFragment::fromHtml(content);
      content = textDocument.toPlainText();
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
      externalApp = true;
    } else if (action->objectName() == "evernoteShareAct") {
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
    } else if (action->objectName() == "hackerNewsShareAct") {
      url.setUrl("http://news.ycombinator.com/submitlink");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("u", linkString);
      urlQuery.addQueryItem("t", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("u", linkString);
      url.addQueryItem("t", title);
#endif
    } else if (action->objectName() == "telegramShareAct") {
      url.setUrl("tg://msg_url");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("url", linkString);
      urlQuery.addQueryItem("text", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("url", linkString);
      url.addQueryItem("text", title);
#endif
      externalApp = true;
    } else if (action->objectName() == "viberShareAct") {
      url.setUrl("viber://forward");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("text", title + "%20" + linkString);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("text", title + "%20" + linkString);
#endif
      externalApp = true;
    }
    QDesktopServices::openUrl(url);
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

