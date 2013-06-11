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
#ifndef ADDFEEDDIALOG_H
#define ADDFEEDDIALOG_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QtSql>

#include "lineedit.h"
#include "parsethread.h"
#include "updatethread.h"

class AddFeedWizard : public QWizard
{
  Q_OBJECT
public:
  explicit AddFeedWizard(QWidget *parent, int curFolderId);
  ~AddFeedWizard();

  LineEdit *nameFeedEdit_;
  LineEdit *urlFeedEdit_;
  QString htmlUrlString_;
  QString feedUrlString_;
  int feedId_;
  int feedParentId_;
  int newCount_;

public slots:
  void getUrlDone(const int &result, const QString &feedUrlStr,
                  const QByteArray &data, const QDateTime &dtReply);
  void slotUpdateFeed(const QString &feedUrl, const bool &, int newCount);

signals:
  void xmlReadyParse(const QByteArray &data, const QString &feedUrlStr,
                     const QDateTime &dtReply);
  void signalRequestUrl(const QString &urlString, const QDateTime &date, const QString &userInfo);

protected:
  virtual bool validateCurrentPage();
  virtual void done(int result);
  void changeEvent(QEvent *event);

private slots:
  void urlFeedEditChanged(const QString&);
  void nameFeedEditChanged(const QString&);
  void backButtonClicked();
  void nextButtonClicked();
  void finishButtonClicked();
  void slotCurrentIdChanged(int);
  void titleFeedAsNameStateChanged(int);
  void slotProgressBarUpdate();
  void newFolder();
  void slotAuthentication(QNetworkReply *reply, QAuthenticator *auth);
  void slotFeedCountsUpdate(FeedCountStruct counts);

private:
  void addFeed();
  void deleteFeed();
  void showProgressBar();
  void finish();

  UpdateThread *persistentUpdateThread_;
  ParseThread *persistentParseThread_;
  QWizardPage *createUrlFeedPage();
  QWizardPage *createNameFeedPage();
  QCheckBox *titleFeedAsName_;
  QGroupBox *authentication_;
  LineEdit *user_;
  LineEdit *pass_;
  QLabel *textWarning;
  QWidget *warningWidget_;
  QProgressBar *progressBar_;
  bool selectedPage;
  bool finishOn;
  QTreeWidget *foldersTree_;
  int curFolderId_;

};

#endif // ADDFEEDDIALOG_H
