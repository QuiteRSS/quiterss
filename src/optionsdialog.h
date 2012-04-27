#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QtGui>
#include <QNetworkProxy>
#include "lineedit.h"

class OptionsDialog : public QDialog
{
  Q_OBJECT
public:
  explicit OptionsDialog(QWidget *parent = 0);
  QNetworkProxy proxy();
  void setProxy(const QNetworkProxy proxy);
  QString language();
  void setLanguage(QString langFileName);
  int currentIndex();
  void setCurrentItem(int index);
  void setBehaviorIconTray(int behavior);
  int behaviorIconTray();
  void setOpeningFeed(int action);
  int getOpeningFeed();

  // general
  QCheckBox *showSplashScreen_;
  QCheckBox *reopenFeedStartup_;

  // systemTray
  QGroupBox *showTrayIconBox_;
  QCheckBox *startingTray_;
  QCheckBox *minimizingTray_;
  QCheckBox *closingTray_;
  QCheckBox *singleClickTray_;
  QCheckBox *clearStatusNew_;
  QCheckBox *emptyWorking_;

  // browser
  QGroupBox *embeddedBrowserOn_;
  QCheckBox *javaScriptEnable_;
  QCheckBox *pluginsEnable_;

  // feeds
  QCheckBox *updateFeedsStartUp_;
  QCheckBox *updateFeeds_;
  QSpinBox *updateFeedsTime_;
  QComboBox *intervalTime_;

  QRadioButton *reopenLastNews_;
  QRadioButton *openFirstNews_;
  QRadioButton *nottoOpenNews_;

  QCheckBox *markNewsReadOn_;
  QSpinBox *markNewsReadTime_;

  QCheckBox *showDescriptionNews_;

  QCheckBox *dayCleanUpOn_;
  QSpinBox *maxDayCleanUp_;
  QCheckBox *newsCleanUpOn_;
  QSpinBox *maxNewsCleanUp_;
  QCheckBox *readCleanUp_;
  QCheckBox *neverUnreadCleanUp_;
  QCheckBox *neverStarCleanUp_;

  // nitifier
  QCheckBox *soundNewNews_;

  // fonts
  QTreeWidget *fontTree;

  // shortcut
  void loadActionShortcut(QList<QAction *> actions, QStringList *list);
  void saveActionShortcut(QList<QAction *> actions);

protected:
  bool eventFilter(QObject *obj, QEvent *event);

private slots:
  void slotCategoriesItemClicked(QTreeWidgetItem* item, int column);
  void slotCategoriesTreeKeyUpDownPressed();
  void manualProxyToggle(bool checked);
  void updateProxy();
  void applyProxy();
  void acceptSlot();
  void slotFontChange();
  void slotFontReset();
  void intervalTimeChang(QString str);
  void shortcutTreeClicked(QTreeWidgetItem* item, int);
  void slotShortcutTreeUpDownPressed();
  void slotClearShortcut();
  void slotResetShortcut();

signals:
  void signalCategoriesTreeKeyUpDownPressed();
  void signalShortcutTreeUpDownPressed();

private:
  QLabel *contentLabel_;
  QStackedWidget *contentStack_;
  QTreeWidget *categoriesTree;

  //stack widgets
  QFrame *traySystemWidget_;
  QFrame *networkConnectionsWidget_;
  QFrame *browserWidget_;
  QTabWidget *feedsWidget_;
  QFrame *notifierWidget_;
  QWidget *languageWidget_;
  QWidget *fontsWidget_;
  QWidget *shortcutWidget_;

  QDialogButtonBox *buttonBox_;

  // systemTray
  QRadioButton *staticIconTray_;
  QRadioButton *changeIconTray_;
  QRadioButton *newCountTray_;
  QRadioButton *unreadCountTray_;

  // network connection
  QRadioButton *systemProxyButton_;
  QRadioButton *directConnectionButton_;
  QRadioButton *manualProxyButton_;
  QWidget *manualWidget_;
  LineEdit *editHost_;
  LineEdit *editPort_;
  LineEdit *editUser_;
  LineEdit *editPassword_;

  // language
  QListWidget *languageFileList_;

  // internal variables for options
  QNetworkProxy networkProxy_;

  // shortcut
  QTreeWidget *shortcutTree_;
  LineEdit *editShortcut_;
  QStringList *listDefaultShortcut_;
  QGroupBox *editShortcutBox;

};

#endif // OPTIONSDIALOG_H
