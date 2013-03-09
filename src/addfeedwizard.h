#ifndef ADDFEEDDIALOG_H
#define ADDFEEDDIALOG_H

#include <QtGui>
#include <QtSql>
#include "lineedit.h"
#include "parsethread.h"
#include "updatethread.h"

class AddFeedWizard : public QWizard
{
  Q_OBJECT
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

public:
  explicit AddFeedWizard(QWidget *parent, QString dataDirPath, int curFolderId = 0);
  ~AddFeedWizard();

  LineEdit *nameFeedEdit_;
  LineEdit *urlFeedEdit_;
  QString htmlUrlString_;
  QString feedUrlString_;
  int feedId_;
  int feedParentId_;
  int newCount_;

protected:
  virtual bool validateCurrentPage();
  virtual void done(int result);
  void changeEvent(QEvent *event);

signals:
  void xmlReadyParse(const QByteArray &data, const QString &feedUrlStr,
                     const QDateTime &dtReply);
  void signalRequestUrl(const QString &urlString, const QDateTime &date, const QString &userInfo);

public slots:
  void getUrlDone(const int &result, const QString &feedUrlStr,
                  const QByteArray &data, const QDateTime &dtReply);
  void slotUpdateFeed(int feedId, const bool &, int newCount);

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

};

#endif // ADDFEEDDIALOG_H
