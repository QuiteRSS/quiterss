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

  QSqlDatabase *db_;
  UpdateThread *persistentUpdateThread_;
  ParseThread *persistentParseThread_;
  QByteArray data_;
  QUrl url_;
  QWizardPage *createUrlFeedPage();
  QWizardPage *createNameFeedPage();
  QCheckBox *titleFeedAsName_;
  QLabel *textWarning;
  QWidget *warningWidget_;
  QProgressBar *progressBar_;
  bool selectedPage;
  bool finishOn;

public:
  explicit AddFeedWizard(QWidget *parent, QSqlDatabase *db);
  ~AddFeedWizard();

  LineEdit *nameFeedEdit_;
  LineEdit *urlFeedEdit_;
  QString htmlUrlString_;
  QString feedUrlString_;

protected:
  virtual bool validateCurrentPage();
  virtual void done(int result);
  void changeEvent(QEvent *event);

signals:
  void startGetUrlTimer();
  void xmlReadyParse(const QByteArray &data, const QUrl &url);

public slots:
  void slotUpdateFeed(const QUrl &url, const bool &);

private slots:
  void urlFeedEditChanged(const QString&);
  void nameFeedEditChanged(const QString&);
  void backButtonClicked();
  void nextButtonClicked();
  void finishButtonClicked();
  void slotCurrentIdChanged(int);
  void titleFeedAsNameStateChanged(int);
  void slotProgressBarUpdate();
  void receiveXml(const QByteArray &data, const QUrl &url);
  void getUrlDone(const int &result, const QDateTime &dtReply);

};

#endif // ADDFEEDDIALOG_H
