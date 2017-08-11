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
#include "cleanupwizard.h"

#include "mainapplication.h"
#include "database.h"
#include "settings.h"

#if defined(Q_OS_WIN)
#include <windows.h>
#include <psapi.h>
#endif

CleanUpThread::CleanUpThread(QObject *parent)
  : QThread(parent)
  , maxDayCleanUp_(0)
  , maxNewsCleanUp_(0)
  , dayCleanUpOn_(false)
  , newsCleanUpOn_(false)
  , readCleanUp_(false)
  , neverUnreadCleanUp_(false)
  , neverStarCleanUp_(false)
  , neverLabelCleanUp_(false)
  , cleanUpDeleted_(false)
  , fullCleanUp_(false)
  , countDeleted(0)
{
}

CleanUpThread::~CleanUpThread()
{
  exit();
  wait();
}

/*virtual*/ void CleanUpThread::run()
{
  QSqlQuery q;
  QString qStr;
  QSqlDatabase db = QSqlDatabase::database();
  db.transaction();

  q.exec("SELECT count(id) FROM news WHERE deleted < 2");
  if (q.first()) countDeleted = q.value(0).toInt();

  foreach (QString feedId, feedsIdList_) {
    int cntT = 0;
    int cntNews = 0;

    qStr = QString("SELECT undeleteCount FROM feeds WHERE id=='%1'").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) cntNews = q.value(0).toInt();

    if (fullCleanUp_)
      q.exec(QString("DELETE FROM news WHERE feedId=='%1' AND deleted >= 2").arg(feedId));

    QString qStr1 = QString("UPDATE news SET description='', content='', received='', "
                            "author_name='', author_uri='', author_email='', "
                            "category='', new='', read='', starred='', label='', "
                            "deleteDate='', feedParentId='', deleted=2");

    qStr = QString("SELECT id, received FROM news WHERE feedId=='%1' AND deleted = 0").arg(feedId);
    if (neverUnreadCleanUp_) qStr.append(" AND read!=0");
    if (neverStarCleanUp_) qStr.append(" AND starred==0");
    if (neverLabelCleanUp_) qStr.append(" AND (label=='' OR label==',' OR label IS NULL)");
    qStr.append(" ORDER BY published");
    q.exec(qStr);
    while (q.next()) {
      int newsId = q.value(0).toInt();

      if (newsCleanUpOn_ && (cntT < (cntNews - maxNewsCleanUp_))) {
        if (fullCleanUp_)
          qStr = QString("DELETE FROM news WHERE id='%1'").arg(newsId);
        else
          qStr = QString("%1 WHERE id=='%2'").arg(qStr1).arg(newsId);
        QSqlQuery qt;
        qt.exec(qStr);
        cntT++;
        continue;
      }

      QDateTime dateTime = QDateTime::fromString(q.value(1).toString(), Qt::ISODate);
      if (dayCleanUpOn_ &&
          (dateTime.daysTo(QDateTime::currentDateTime()) > maxDayCleanUp_)) {
        if (fullCleanUp_)
          qStr = QString("DELETE FROM news WHERE id='%1'").arg(newsId);
        else
          qStr = QString("%1 WHERE id=='%2'").arg(qStr1).arg(newsId);
        QSqlQuery qt;
        qt.exec(qStr);
        cntT++;
        continue;
      }

      if (readCleanUp_) {
        if (fullCleanUp_)
          qStr = QString("DELETE FROM news WHERE id='%1'").arg(newsId);
        else
          qStr = QString("%1 WHERE read!=0 AND id=='%2'").arg(qStr1).arg(newsId);
        QSqlQuery qt;
        qt.exec(qStr);
        cntT++;
      }
    }

    int undeleteCount = 0;
    qStr = QString("SELECT count(id) FROM news WHERE feedId=='%1' AND deleted==0").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) undeleteCount = q.value(0).toInt();

    int unreadCount = 0;
    qStr = QString("SELECT count(read) FROM news WHERE feedId=='%1' AND read==0 AND deleted==0").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) unreadCount = q.value(0).toInt();

    int newCount = 0;
    qStr = QString("SELECT count(new) FROM news WHERE feedId=='%1' AND new==1 AND deleted==0").
        arg(feedId);
    q.exec(qStr);
    if (q.next()) newCount = q.value(0).toInt();

    qStr = QString("UPDATE feeds SET unread='%1', newCount='%2', undeleteCount='%3' WHERE id=='%4'").
        arg(unreadCount).arg(newCount).arg(undeleteCount).arg(feedId);
    q.exec(qStr);
  }

  foreach (int folderIdStart, foldersIdList_) {
    if (folderIdStart < 1) continue;

    int categoryId = folderIdStart;
    // Process all parents
    while (0 < categoryId) {
      int unreadCount = -1;
      int undeleteCount = -1;
      int newCount = -1;

      // Calculate sum of all feeds with same parent
      qStr = QString("SELECT sum(unread), sum(undeleteCount), sum(newCount) "
                     "FROM feeds WHERE parentId=='%1'").arg(categoryId);
      q.exec(qStr);
      if (q.next()) {
        unreadCount   = q.value(0).toInt();
        undeleteCount = q.value(1).toInt();
        newCount = q.value(2).toInt();
      }

      if (unreadCount != -1) {
        qStr = QString("UPDATE feeds SET unread='%1', undeleteCount='%2', newCount='%3' WHERE id=='%4'").
            arg(unreadCount).arg(undeleteCount).arg(newCount).arg(categoryId);
        q.exec(qStr);
      }

      // go to next parent's parent
      qStr = QString("SELECT parentId FROM feeds WHERE id=='%1'").
          arg(categoryId);
      categoryId = 0;
      q.exec(qStr);
      if (q.next()) categoryId = q.value(0).toInt();
    }
  }

  if (cleanUpDeleted_) {
    q.exec("UPDATE news SET description='', content='', received='', "
           "author_name='', author_uri='', author_email='', "
           "category='', new='', read='', starred='', label='', "
           "deleteDate='', feedParentId='', deleted=2 WHERE deleted==1");
  }

  q.exec("SELECT count(id) FROM news WHERE deleted < 2");
  if (q.first()) countDeleted = countDeleted - q.value(0).toInt();

  q.finish();
  db.commit();

  if (!mainApp->storeDBMemory()) {
    db.exec("VACUUM");
  } else {
    Database::sqliteDBMemFile();
    Database::setVacuum();
  }
}

CleanUpWizard::CleanUpWizard(QWidget *parent)
  : QWizard(parent)
  , selectedPage_(true)
  , itemNotChecked_(false)
{
  setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Clean Up"));
  setWizardStyle(QWizard::ModernStyle);
  setOptions(QWizard::NoBackButtonOnStartPage);
  setMinimumWidth(440);
  setMinimumHeight(380);

  addPage(createChooseFeedsPage());
  addPage(createCleanUpOptionsPage());

  Settings settings;
  restoreGeometry(settings.value("CleanUpWizard/geometry").toByteArray());

  cleanUpThread_ = new CleanUpThread(this);

  connect(button(QWizard::FinishButton), SIGNAL(clicked()),
          this, SLOT(finishButtonClicked()));
  connect(this, SIGNAL(currentIdChanged(int)), SLOT(currentIdChanged(int)));
}

CleanUpWizard::~CleanUpWizard()
{
  Settings settings;
  settings.setValue("CleanUpWizard/geometry", saveGeometry());
}

/*virtual*/ void CleanUpWizard::closeEvent(QCloseEvent* event)
{
  if (progressBar_->isVisible())
    event->ignore();
}

QWizardPage *CleanUpWizard::createChooseFeedsPage()
{
  QWizardPage *page = new QWizardPage;
  page->setTitle(tr("Choose Feeds"));

  Settings settings;
  QStringList feedsIdList = settings.value("CleanUpWizard/feedsIdList").toStringList();

  feedsTree_ = new QTreeWidget(this);
  feedsTree_->setObjectName("feedsTreeFR");
  feedsTree_->setColumnCount(3);
  feedsTree_->setColumnHidden(1, true);
  feedsTree_->setColumnHidden(2, true);
  feedsTree_->setHeaderHidden(true);

  QStringList treeItem;
  treeItem << tr("All Feeds") << "0"<< "-1";
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  if (feedsIdList.isEmpty())
    treeWidgetItem->setCheckState(0, Qt::Checked);
  else
    treeWidgetItem->setCheckState(0, Qt::Unchecked);
  feedsTree_->addTopLevelItem(treeWidgetItem);

  QSqlQuery q;
  QQueue<int> parentIds;
  parentIds.enqueue(0);
  while (!parentIds.empty()) {
    int parentId = parentIds.dequeue();
    QString qStr = QString("SELECT text, id, image, xmlUrl FROM feeds WHERE parentId='%1' ORDER BY rowToParent").
        arg(parentId);
    q.exec(qStr);
    while (q.next()) {
      QString feedText = q.value(0).toString();
      QString feedIdStr = q.value(1).toString();
      QByteArray byteArray = q.value(2).toByteArray();
      QString xmlUrl = q.value(3).toString();

      treeItem.clear();
      treeItem << feedText << feedIdStr << (xmlUrl.isEmpty() ? "0" : "1");
      treeWidgetItem = new QTreeWidgetItem(treeItem);

      if (feedsIdList.contains(feedIdStr) || feedsIdList.isEmpty())
        treeWidgetItem->setCheckState(0, Qt::Checked);
      else
        treeWidgetItem->setCheckState(0, Qt::Unchecked);

      QPixmap iconItem;
      if (xmlUrl.isEmpty()) {
        iconItem.load(":/images/folder");
      }
      else {
        MainWindow *mainWindow = mainApp->mainWindow();
        if (byteArray.isNull() || mainWindow->defaultIconFeeds_) {
          iconItem.load(":/images/feed");
        }
        else {
          iconItem.loadFromData(QByteArray::fromBase64(byteArray));
        }
      }
      treeWidgetItem->setIcon(0, iconItem);

      QList<QTreeWidgetItem *> treeItems =
          feedsTree_->findItems(QString::number(parentId),
                                Qt::MatchFixedString | Qt::MatchRecursive,
                                1);
      treeItems.at(0)->addChild(treeWidgetItem);
      if (xmlUrl.isEmpty())
        parentIds.enqueue(feedIdStr.toInt());
    }
  }
  feedsTree_->expandAll();

  connect(feedsTree_, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
          this, SLOT(feedItemChanged(QTreeWidgetItem*,int)));

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(feedsTree_);
  page->setLayout(layout);

  return page;
}

QWizardPage *CleanUpWizard::createCleanUpOptionsPage()
{
  QWizardPage *page = new QWizardPage;
  page->setTitle(tr("Clean Up Options"));

  dayCleanUpOn_ = new QCheckBox(tr("Maximum age of news in days to keep:"));
  maxDayCleanUp_ = new QSpinBox();
  maxDayCleanUp_->setEnabled(false);
  maxDayCleanUp_->setRange(0, 9999);
  connect(dayCleanUpOn_, SIGNAL(toggled(bool)),
          maxDayCleanUp_, SLOT(setEnabled(bool)));

  newsCleanUpOn_ = new QCheckBox(tr("Maximum number of news to keep:"));
  maxNewsCleanUp_ = new QSpinBox();
  maxNewsCleanUp_->setEnabled(false);
  maxNewsCleanUp_->setRange(0, 9999);
  connect(newsCleanUpOn_, SIGNAL(toggled(bool)),
          maxNewsCleanUp_, SLOT(setEnabled(bool)));

  readCleanUp_ = new QCheckBox(tr("Delete read news"));
  neverUnreadCleanUp_ = new QCheckBox(tr("Never delete unread news"));
  neverStarCleanUp_ = new QCheckBox(tr("Never delete starred news"));
  neverLabelCleanUp_ = new QCheckBox(tr("Never delete labeled news"));

  QGridLayout *cleanUpFeedsLayout = new QGridLayout();
  cleanUpFeedsLayout->setColumnStretch(1, 1);
  cleanUpFeedsLayout->addWidget(dayCleanUpOn_, 0, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(maxDayCleanUp_, 0, 1, 1, 1, Qt::AlignLeft);
  cleanUpFeedsLayout->addWidget(newsCleanUpOn_, 1, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(maxNewsCleanUp_, 1, 1, 1, 1, Qt::AlignLeft);
  cleanUpFeedsLayout->addWidget(readCleanUp_, 2, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(neverUnreadCleanUp_, 3, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(neverStarCleanUp_, 4, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(neverLabelCleanUp_, 5, 0, 1, 1);

  cleanUpDeleted_ = new QCheckBox(tr("Clean up 'Deleted'"));
  fullCleanUp_ = new QCheckBox(tr("Purge DB"));
  QVBoxLayout *fullCleanUpDescriptionLayout = new QVBoxLayout;
  fullCleanUpDescriptionLayout->setContentsMargins(15, 0, 0, 0);
  fullCleanUpDescriptionLayout->addWidget(
        new QLabel(tr("Totally remove records that had marked 'deleted' from DB.\n"
                      "Ancient news could reappear")));

  progressBar_ = new QProgressBar(this);
  progressBar_->setObjectName("progressBar_");
  progressBar_->setTextVisible(false);
  progressBar_->setFixedHeight(15);
  progressBar_->setMinimum(0);
  progressBar_->setMaximum(0);
  progressBar_->setVisible(false);

  Settings settings;
  settings.beginGroup("CleanUpWizard");
  maxDayCleanUp_->setValue(settings.value("maxDay", 30).toInt());
  maxNewsCleanUp_->setValue(settings.value("maxNews", 200).toInt());
  dayCleanUpOn_->setChecked(settings.value("maxDayOn", true).toBool());
  newsCleanUpOn_->setChecked(settings.value("maxNewsOn", true).toBool());
  readCleanUp_->setChecked(settings.value("readCleanUp", false).toBool());
  neverUnreadCleanUp_->setChecked(settings.value("neverUnread", true).toBool());
  neverStarCleanUp_->setChecked(settings.value("neverStar", true).toBool());
  neverLabelCleanUp_->setChecked(settings.value("neverLabel", true).toBool());
  cleanUpDeleted_->setChecked(settings.value("cleanUpDeleted", true).toBool());
  fullCleanUp_->setChecked(settings.value("fullCleanUp", false).toBool());
  settings.endGroup();

  QVBoxLayout *layout = new QVBoxLayout(page);
  layout->addLayout(cleanUpFeedsLayout);
  layout->addSpacing(10);
  layout->addWidget(cleanUpDeleted_);
  layout->addSpacing(10);
  layout->addWidget(fullCleanUp_);
  layout->addLayout(fullCleanUpDescriptionLayout);
  layout->addStretch(1);
  layout->addWidget(progressBar_);

  return page;
}

/*virtual*/ bool CleanUpWizard::validateCurrentPage()
{
  if (!selectedPage_) {
    return false;
  }
  return true;
}

void CleanUpWizard::feedItemChanged(QTreeWidgetItem *item, int column)
{
  if ((column != 0) || itemNotChecked_) return;

  itemNotChecked_ = true;
  if (item->checkState(0) == Qt::Unchecked) {
    setCheckStateItem(item, Qt::Unchecked);

    QTreeWidgetItem *parentItem = item->parent();
    while (parentItem) {
      parentItem->setCheckState(0, Qt::Unchecked);
      parentItem = parentItem->parent();
    }
  } else {
    setCheckStateItem(item, Qt::Checked);
  }
  itemNotChecked_ = false;
}

void CleanUpWizard::setCheckStateItem(QTreeWidgetItem *item, Qt::CheckState state)
{
  for (int i = 0; i < item->childCount(); ++i) {
    QTreeWidgetItem *childItem = item->child(i);
    childItem->setCheckState(0, state);
    setCheckStateItem(childItem, state);
  }
}

void CleanUpWizard::currentIdChanged(int idPage)
{
  if (idPage == 1)
    selectedPage_ = false;
  else
    selectedPage_ = true;
}

void CleanUpWizard::finishButtonClicked()
{
  button(QWizard::BackButton)->setEnabled(false);
  button(QWizard::CancelButton)->setEnabled(false);
  button(QWizard::FinishButton)->setEnabled(false);
  page(1)->setEnabled(false);
  progressBar_->show();
  qApp->processEvents();

  feedsTree_->expandAll();

  QStringList feedsIdList;
  QList<int> foldersIdList;
  QTreeWidgetItem *treeItem = feedsTree_->itemBelow(feedsTree_->topLevelItem(0));
  while (treeItem) {
    if (treeItem->checkState(0) == Qt::Checked)
      feedsIdList << treeItem->text(1);
    if (treeItem->text(2).toInt() == 0)
      foldersIdList << treeItem->text(1).toInt();
    treeItem = feedsTree_->itemBelow(treeItem);
  }

  cleanUpThread_->maxDayCleanUp_ = maxDayCleanUp_->value();
  cleanUpThread_->maxNewsCleanUp_ = maxNewsCleanUp_->value();
  cleanUpThread_->dayCleanUpOn_ = dayCleanUpOn_->isChecked();
  cleanUpThread_->newsCleanUpOn_ = newsCleanUpOn_->isChecked();
  cleanUpThread_->readCleanUp_ = readCleanUp_->isChecked();
  cleanUpThread_->neverUnreadCleanUp_ = neverUnreadCleanUp_->isChecked();
  cleanUpThread_->neverStarCleanUp_ = neverStarCleanUp_->isChecked();
  cleanUpThread_->neverLabelCleanUp_ = neverLabelCleanUp_->isChecked();
  cleanUpThread_->cleanUpDeleted_ = cleanUpDeleted_->isChecked();
  cleanUpThread_->fullCleanUp_ = fullCleanUp_->isChecked();
  cleanUpThread_->feedsIdList_ = feedsIdList;
  cleanUpThread_->foldersIdList_ = foldersIdList;

  connect(cleanUpThread_, SIGNAL(finished()),
          this, SLOT(finishCleanUp()));

  cleanUpThread_->start();
}

void CleanUpWizard::finishCleanUp()
{
  int feedId = -1;
  MainWindow *mainWindow = mainApp->mainWindow();

  feedsTree_->expandAll();
  QTreeWidgetItem *treeItem = feedsTree_->itemBelow(feedsTree_->topLevelItem(0));
  while (treeItem) {
    if (treeItem->checkState(0) == Qt::Checked) {
      if (mainWindow->currentNewsTab->feedId_ == treeItem->text(1).toInt())
        feedId = treeItem->text(1).toInt();
    }
    treeItem = feedsTree_->itemBelow(treeItem);
  }
  if (feedId != -1)
    mainWindow->slotUpdateNews(NewsTabWidget::RefreshAll);
  mainWindow->slotUpdateStatus(feedId, false);

  mainWindow->feedsModelReload();
  mainWindow->recountCategoryCounts();

  Settings settings;
  settings.beginGroup("CleanUpWizard");
  settings.setValue("feedsIdList", cleanUpThread_->feedsIdList_);
  settings.setValue("maxDay", maxDayCleanUp_->value());
  settings.setValue("maxNews", maxNewsCleanUp_->value());
  settings.setValue("maxDayOn", dayCleanUpOn_->isChecked());
  settings.setValue("maxNewsOn", newsCleanUpOn_->isChecked());
  settings.setValue("readCleanUp", readCleanUp_->isChecked());
  settings.setValue("neverUnread", neverUnreadCleanUp_->isChecked());
  settings.setValue("neverStar", neverStarCleanUp_->isChecked());
  settings.setValue("neverLabel", neverLabelCleanUp_->isChecked());
  settings.setValue("cleanUpDeleted", cleanUpDeleted_->isChecked());
  settings.setValue("fullCleanUp", fullCleanUp_->isChecked());
  settings.endGroup();

  selectedPage_ = true;

  accept();

  QMessageBox::information(0, tr("Information"),
                           tr("Clean Up wizard deleted %1 news").
                           arg(cleanUpThread_->countDeleted));
}
