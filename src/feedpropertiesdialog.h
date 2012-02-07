#ifndef FEEDPROPERTIESDIALOG_H
#define FEEDPROPERTIESDIALOG_H

#include <QtGui>

class FeedPropertiesDialog : public QDialog
{
  Q_OBJECT
public:
  explicit FeedPropertiesDialog(QWidget *parent = 0);
  QLineEdit *titleEdit_;
  QLineEdit *urlEdit_;
  QLabel *homepageLabel_;
  QPushButton *loadTitleButton_;
  QString titleString_;
  QString feedUrl_;
  QString homePage_;

private:
  QDialogButtonBox *buttonBox_;
  QTabWidget *tabWidget_;

private slots:
  void slotLoadTitle();

signals:
  void signalLoadTitle(const QUrl &url);
  void startGetUrlTimer();

};

#endif // FEEDPROPERTIESDIALOG_H
