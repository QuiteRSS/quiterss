#ifndef ADDFEEDDIALOG_H
#define ADDFEEDDIALOG_H

#include <QtGui>
#include "lineedit.h"

class AddFeedDialog : public QWizard
{
  Q_OBJECT
private:
  QWizardPage *createUrlFeedPage();
  QWizardPage *createNameFeedPage();

public:
  explicit AddFeedDialog(QWidget *parent = 0);

  LineEdit *nameFeedEdit_;
  LineEdit *urlFeedEdit_;

protected:
  virtual bool validateCurrentPage();

signals:

public slots:

private slots:
  void urlFeedEditChanged(const QString&);
  void nameFeedEditChanged(const QString&);
  void slotCurrentIdChanged(int);

};

#endif // ADDFEEDDIALOG_H
