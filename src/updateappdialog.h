#ifndef UPDATEAPPDIALOG_H
#define UPDATEAPPDIALOG_H

#include <QtGui>
#include <QNetworkReply>

class UpdateAppDialog : public QDialog
{
  Q_OBJECT
private:
  QSettings *settings_;

  QNetworkAccessManager manager_;
  QNetworkReply *reply_;
  QNetworkReply *historyReply_;

  QLabel *infoLabel;
  QTextBrowser *history_;
  QString lang_;

public:
  explicit UpdateAppDialog(const QString &lang, QSettings *settings, QWidget *parent);

private slots:
  void closeDialog();
  void finishUpdateApp();
  void slotFinishHistoryReply();

};

#endif // UPDATEAPPDIALOG_H
