#ifndef UPDATEAPPDIALOG_H
#define UPDATEAPPDIALOG_H

class UpdateAppDialog : public QDialog
{
  Q_OBJECT
private:
  QString lang_;
  QSettings *settings_;
  bool showDialog_;

  QNetworkAccessManager manager_;
  QNetworkReply *reply_;
  QNetworkReply *historyReply_;

  QLabel *infoLabel;
  QTextBrowser *history_;


public:
  explicit UpdateAppDialog(const QString &lang, QSettings *settings,
                           QWidget *parent, bool show = true);

private slots:
  void closeDialog();
  void finishUpdateApp();
  void slotFinishHistoryReply();

signals:
  void signalNewVersion(bool newVersion);

};

#endif // UPDATEAPPDIALOG_H
