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
#include "cleanupwizard.h"
#include "rsslisting.h"

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
{
}

/*virtual*/ void CleanUpThread::run()
{
  QSqlQuery q;
  QString qStr;
  QSqlDatabase db = QSqlDatabase::database();
  db.transaction();

  foreach (int feedId, feedsIdList_) {
    int cntT = 0;
    int cntNews = 0;

    QSqlQuery q;
    QString qStr;
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

  db.commit();

  q.exec("VACUUM");
  q.finish();

  emit signalFinishCleanUp();
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
  setMinimumWidth(400);
  setMinimumHeight(340);

  addPage(createChooseFeedsPage());
  addPage(createCleanUpOptionsPage());

  connect(this, SIGNAL(signalFinish()),
          SLOT(finishCleanUp()), Qt::QueuedConnection);

  connect(button(QWizard::FinishButton), SIGNAL(clicked()),
          this, SLOT(finishButtonClicked()));
  connect(this, SIGNAL(currentIdChanged(int)),
          SLOT(currentIdChanged(int)));
}

CleanUpWizard::~CleanUpWizard()
{
}

QWizardPage *CleanUpWizard::createChooseFeedsPage()
{
  QWizardPage *page = new QWizardPage;
  page->setTitle(tr("Choose Feeds"));

  RSSListing *rssl_ = qobject_cast<RSSListing*>(parentWidget());

  feedsTree_ = new QTreeWidget(this);
  feedsTree_->setObjectName("feedsTreeFR");
  feedsTree_->setColumnCount(3);
  feedsTree_->setColumnHidden(1, true);
  feedsTree_->setColumnHidden(2, true);
  feedsTree_->setHeaderHidden(true);

  QStringList treeItem;
  treeItem << tr("All Feeds") << "0"<< "-1";
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setCheckState(0, Qt::Checked);
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
      QString feedIdT = q.value(1).toString();
      QByteArray byteArray = q.value(2).toByteArray();
      QString xmlUrl = q.value(3).toString();

      treeItem.clear();
      treeItem << feedText << feedIdT << (xmlUrl.isEmpty() ? "0" : "1");
      treeWidgetItem = new QTreeWidgetItem(treeItem);

      treeWidgetItem->setCheckState(0, Qt::Checked);

      QPixmap iconItem;
      if (xmlUrl.isEmpty()) {
        iconItem.load(":/images/folder");
      }
      else {
        if (byteArray.isNull() || rssl_->defaultIconFeeds_) {
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
        parentIds.enqueue(feedIdT.toInt());
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
  fullCleanUpDescriptionLayout->setContentsMargins(24, 0, 0, 0);
  fullCleanUpDescriptionLayout->addWidget(new QLabel(tr(
        "Totally remove records that had marked 'deleted' from DB.\n"
        "Ancient news could reappear")));

  progressBar_ = new QProgressBar(this);
  progressBar_->setObjectName("progressBar_");
  progressBar_->setTextVisible(false);
  progressBar_->setFixedHeight(15);
  progressBar_->setMinimum(0);
  progressBar_->setMaximum(0);
  progressBar_->setVisible(false);

  maxDayCleanUp_->setValue(0);
  maxNewsCleanUp_->setValue(0);
  dayCleanUpOn_->setChecked(true);
  newsCleanUpOn_->setChecked(true);
  readCleanUp_->setChecked(false);
  neverUnreadCleanUp_->setChecked(true);
  neverStarCleanUp_->setChecked(true);
  neverLabelCleanUp_->setChecked(true);
  cleanUpDeleted_->setChecked(true);
  fullCleanUp_->setChecked(true);

  QVBoxLayout *layout = new QVBoxLayout(page);
  layout->addLayout(cleanUpFeedsLayout);
  layout->addSpacing(10);
  layout->addWidget(cleanUpDeleted_);
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
  for(int i = 0; i < item->childCount(); ++i) {
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

  feedsTree_->expandAll();

  QList<int> feedsIdList;
  QList<int> foldersIdList;
  QTreeWidgetItem *treeItem = feedsTree_->itemBelow(feedsTree_->topLevelItem(0));
  while (treeItem) {
    if (treeItem->checkState(0) == Qt::Checked)
      feedsIdList << treeItem->text(1).toInt();
    if (treeItem->text(2).toInt() == 0)
      foldersIdList << treeItem->text(1).toInt();
    treeItem = feedsTree_->itemBelow(treeItem);
  }

  CleanUpThread *cleanUpThread = new CleanUpThread(this);
  cleanUpThread->maxDayCleanUp_ = maxDayCleanUp_->value();
  cleanUpThread->maxNewsCleanUp_ = maxNewsCleanUp_->value();
  cleanUpThread->dayCleanUpOn_ = dayCleanUpOn_->isChecked();
  cleanUpThread->newsCleanUpOn_ = newsCleanUpOn_->isChecked();
  cleanUpThread->readCleanUp_ = readCleanUp_->isChecked();
  cleanUpThread->neverUnreadCleanUp_ = neverUnreadCleanUp_->isChecked();
  cleanUpThread->neverStarCleanUp_ = neverStarCleanUp_->isChecked();
  cleanUpThread->neverLabelCleanUp_ = neverLabelCleanUp_->isChecked();
  cleanUpThread->cleanUpDeleted_ = cleanUpDeleted_->isChecked();
  cleanUpThread->fullCleanUp_ = fullCleanUp_->isChecked();
  cleanUpThread->feedsIdList_ = feedsIdList;
  cleanUpThread->foldersIdList_ = foldersIdList;

  connect(cleanUpThread, SIGNAL(signalFinishCleanUp()),
          this, SLOT(finishCleanUp()));

  cleanUpThread->start();
}

void CleanUpWizard::finishCleanUp()
{
  RSSListing *rssl = qobject_cast<RSSListing*>(parentWidget());
  int feedId = -1;

  feedsTree_->expandAll();
  QTreeWidgetItem *treeItem = feedsTree_->itemBelow(feedsTree_->topLevelItem(0));
  while (treeItem) {
    if (treeItem->checkState(0) == Qt::Checked) {
      if (rssl->currentNewsTab->feedId_ == treeItem->text(1).toInt())
        feedId = treeItem->text(1).toInt();
    }
    treeItem = feedsTree_->itemBelow(treeItem);
  }
  if (feedId != -1)
    rssl->slotUpdateNews();
  rssl->slotUpdateStatus(feedId, false);

  rssl->feedsModelReload();
  rssl->recountCategoryCounts();

  selectedPage_ = true;
  accept();
}
