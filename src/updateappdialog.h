#ifndef UPDATEAPPDIALOG_H
#define UPDATEAPPDIALOG_H

#include <QNetworkReply>
#include <QWebPage>
#include <QWebFrame>
#include "dialog.h"

class UpdateAppDialog : public Dialog
{
  Q_OBJECT
private:
  QString lang_;
  QSettings *settings_;
  bool showDialog_;

  QWebPage *page_;
  QNetworkAccessManager *networkManager_;
  QNetworkReply *reply_;
  QNetworkReply *historyReply_;

  QLabel *infoLabel;
  QTextBrowser *history_;

  QPushButton *installButton_;
  QCheckBox *remindAboutVersion_;

public:
  explicit UpdateAppDialog(const QString &lang, QSettings *settings,
                           QWidget *parent, bool show = true);
  ~UpdateAppDialog();

private slots:
  void closeDialog();
  void finishUpdatesChecking();
  void slotFinishHistoryReply();
  void updaterRun();
  void renderStatistics();

signals:
  void signalNewVersion(QString newVersion);

};

#endif // UPDATEAPPDIALOG_H
