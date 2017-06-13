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
#ifndef CLEANUPWIZARD_H
#define CLEANUPWIZARD_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QThread>
#include <QtSql>

class CleanUpThread;

class CleanUpThread : public QThread
{
  Q_OBJECT
public:
  explicit CleanUpThread(QObject *parent);
  ~CleanUpThread();

  int maxDayCleanUp_;
  int maxNewsCleanUp_;
  bool dayCleanUpOn_;
  bool newsCleanUpOn_;
  bool readCleanUp_;
  bool neverUnreadCleanUp_;
  bool neverStarCleanUp_;
  bool neverLabelCleanUp_;
  bool cleanUpDeleted_;
  bool fullCleanUp_;
  QStringList feedsIdList_;
  QList<int> foldersIdList_;
  int countDeleted;

protected:
  virtual void run();

};

class CleanUpWizard : public QWizard
{
  Q_OBJECT
public:
  explicit CleanUpWizard(QWidget *parent);
  ~CleanUpWizard();

public slots:
  void finishCleanUp();

protected:
  virtual void closeEvent(QCloseEvent*);
  virtual bool validateCurrentPage();

private slots:
  void currentIdChanged(int);
  void finishButtonClicked();
  void feedItemChanged(QTreeWidgetItem *item, int column);
  void setCheckStateItem(QTreeWidgetItem *item, Qt::CheckState state);

private:
  QWizardPage *createChooseFeedsPage();
  QWizardPage *createCleanUpOptionsPage();

  bool selectedPage_;
  bool itemNotChecked_;
  QTreeWidget *feedsTree_;
  QCheckBox *dayCleanUpOn_;
  QSpinBox *maxDayCleanUp_;
  QCheckBox *newsCleanUpOn_;
  QSpinBox *maxNewsCleanUp_;
  QCheckBox *readCleanUp_;
  QCheckBox *neverUnreadCleanUp_;
  QCheckBox *neverStarCleanUp_;
  QCheckBox *neverLabelCleanUp_;
  QCheckBox *cleanUpDeleted_;
  QCheckBox *fullCleanUp_;
  QProgressBar *progressBar_;
  CleanUpThread *cleanUpThread_;

};

#endif // CLEANUPWIZARD_H
