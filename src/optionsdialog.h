#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QtGui>
#include <QtSql>
#include <QNetworkProxy>
#include "lineedit.h"

#define STATIC_ICON_TRAY        0
#define CHANGE_ICON_TRAY        1
#define NEW_COUNT_ICON_TRAY     2
#define UNREAD_COUNT_ICON_TRAY  3

class OptionsDialog : public QDialog
{
  Q_OBJECT
public:
  explicit OptionsDialog(QWidget *parent, QSqlDatabase *db);
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
  QCheckBox *storeDBMemory_;

  // systemTray
  QGroupBox *showTrayIconBox_;
  QCheckBox *startingTray_;
  QCheckBox *minimizingTray_;
  QCheckBox *closingTray_;
  QCheckBox *singleClickTray_;
  QCheckBox *clearStatusNew_;
  QCheckBox *emptyWorking_;

  // browser
  QRadioButton *embeddedBrowserOn_;
  QRadioButton *standartBrowserOn_;
  QRadioButton *externalBrowserOn_;
  LineEdit *editExternalBrowser_;
  QPushButton *selectionExternalBrowser_;
  QCheckBox *javaScriptEnable_;
  QCheckBox *pluginsEnable_;
  QCheckBox *openLinkInBackground_;
  QCheckBox *openLinkInBackgroundEmbedded_;

  // feeds
  QCheckBox *updateFeedsStartUp_;
  QCheckBox *updateFeeds_;
  QSpinBox *updateFeedsTime_;
  QComboBox *intervalTime_;

  QRadioButton *positionLastNews_;
  QRadioButton *positionFirstNews_;
  QRadioButton *positionUnreadNews_;
  QCheckBox *openNewsWebViewOn_;
  QRadioButton *nottoOpenNews_;

  QGroupBox *markNewsReadOn_;
  QRadioButton *markCurNewsRead_;
  QRadioButton *markPrevNewsRead_;
  QSpinBox *markNewsReadTime_;
  QCheckBox *markReadSwitchingFeed_;
  QCheckBox *markReadClosingTab_;
  QCheckBox *markReadMinimize_;

  QCheckBox *showDescriptionNews_;

  QComboBox *formatDateTime_;

  QCheckBox *dayCleanUpOn_;
  QSpinBox *maxDayCleanUp_;
  QCheckBox *newsCleanUpOn_;
  QSpinBox *maxNewsCleanUp_;
  QCheckBox *readCleanUp_;
  QCheckBox *neverUnreadCleanUp_;
  QCheckBox *neverStarCleanUp_;

  // labels
  QTreeWidget *labelsTree_;

  // nitifier
  QCheckBox *soundNewNews_;
  QLineEdit *editSoundNotifer_;
  QPushButton *selectionSoundNotifer_;

  QGroupBox *showNotifyOn_;
  QSpinBox *countShowNewsNotify_;
  QSpinBox *timeShowNewsNotify_;
  QSpinBox *widthTitleNewsNotify_;

  QCheckBox *onlySelectedFeeds_;
  QTreeWidget *feedsTreeNotify_;
  bool itemNotChecked_;

  // fonts
  QTreeWidget *fontTree;

  // shortcut
  void loadActionShortcut(QList<QAction *> actions, QStringList *list);
  void saveActionShortcut(QList<QAction *> actions, QActionGroup *labelGroup);

protected:
  bool eventFilter(QObject *obj, QEvent *event);

private slots:
  void slotCategoriesItemClicked(QTreeWidgetItem* item, int);
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
  void selectionBrowser();
  void selectionSoundNotifer();
  void feedsTreeNotifyItemChanged(QTreeWidgetItem* item,int column);
  void newLabel();
  void editLabel();
  void deleteLabel();
  void moveUpLabel();
  void moveDownLabel();
  void slotCurrentLabelChanged(QTreeWidgetItem *current, QTreeWidgetItem *);

signals:
  void signalCategoriesTreeKeyUpDownPressed();
  void signalShortcutTreeUpDownPressed();

private:
  QSqlDatabase *db_;

  QLabel *contentLabel_;
  QStackedWidget *contentStack_;
  QTreeWidget *categoriesTree;

  //stack widgets
  QFrame *traySystemWidget_;
  QFrame *networkConnectionsWidget_;
  QFrame *browserWidget_;
  QTabWidget *feedsWidget_;
  QWidget *labelsWidget_;
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

  // Labels
  void createLabelsWidget();
  void applyLabels();
  void addIdLabelList(QString idLabel);
  QStringList idLabels_;
  QPushButton *newLabelButton_;
  QPushButton *editLabelButton_;
  QPushButton *deleteLabelButton_;
  QPushButton *moveUpLabelButton_;
  QPushButton *moveDownLabelButton_;

  // language
  void createLanguageWidget();
  QTreeWidget *languageFileList_;

  // internal variables for options
  QNetworkProxy networkProxy_;

  // shortcut
  QStringList *listDefaultShortcut_;
  QTreeWidget *shortcutTree_;
  LineEdit *editShortcut_;
  QGroupBox *editShortcutBox;

};

#endif // OPTIONSDIALOG_H
