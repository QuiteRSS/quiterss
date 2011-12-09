#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QtGui>
#include <QNetworkProxy>

class OptionsDialog : public QDialog
{
  Q_OBJECT
public:
  explicit OptionsDialog(QWidget *parent = 0);
  void setProxy(const QNetworkProxy proxy);

private slots:
  void slotCategoriesItemCLicked(QTreeWidgetItem* item, int column);
  void manualProxyToggle(bool checked);
  void updateProxy();

signals:

private:
  QLabel *contentLabel_;
  QStackedWidget *contentStack_;

  QWidget *networkConnectionsWidget_;
  QWidget *widgetFirst_;
  QWidget *widgetSecond_;

  QDialogButtonBox *buttonBox_;

  QRadioButton *systemProxyButton_;
  QRadioButton *directConnectionButton_;
  QRadioButton *manualProxyButton_;
  QWidget *manualWidget_;


  QNetworkProxy networkProxy_;
};

#endif // OPTIONSDIALOG_H
