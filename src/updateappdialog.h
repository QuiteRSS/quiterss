#ifndef UPDATEAPPDIALOG_H
#define UPDATEAPPDIALOG_H

#include <QtGui>
#include <QNetworkReply>

class UpdateAppDialog : public QDialog
{
  Q_OBJECT
public:
  explicit UpdateAppDialog(QWidget *parent = 0);

protected:

private slots:
  void finishUpdateApp();

private:
  QNetworkAccessManager manager_;
  QNetworkReply *reply_;

  QLabel *infoLabel;
signals:

};

#endif // UPDATEAPPDIALOG_H
