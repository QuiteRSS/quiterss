#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QtSql>
#include <QNetworkProxy>
#include "dialog.h"
#include "lineedit.h"

#define STATIC_ICON_TRAY        0
#define CHANGE_ICON_TRAY        1
#define NEW_COUNT_ICON_TRAY     2
#define UNREAD_COUNT_ICON_TRAY  3

class OptionsDialog : public Dialog
{
  Q_OBJECT
public:
  explicit OptionsDialog(QWidget *parent);
  int currentIndex();
  void setCurrentItem(int index);

  // general
  QCheckBox *showSplashScreen_;
  QCheckBox *reopenFeedStartup_;
  QCheckBox *hideFeedsOpenTab_;
  QCheckBox *storeDBMemory_;

  // systemTray
  void setBehaviorIconTray(int behavior);
  int behaviorIconTray();
  QGroupBox *showTrayIconBox_;
  QCheckBox *startingTray_;
  QCheckBox *minimizingTray_;
  QCheckBox *closingTray_;
  QCheckBox *singleClickTray_;
  QCheckBox *clearStatusNew_;
  QCheckBox *emptyWorking_;

  // network connection
  QNetworkProxy proxy();
  void setProxy(const QNetworkProxy proxy);

  // browser
  QRadioButton *embeddedBrowserOn_;
  QRadioButton *externalBrowserOn_;
  QRadioButton *defaultExternalBrowserOn_;
  QRadioButton *otherExternalBrowserOn_;
  LineEdit *otherExternalBrowserEdit_;
  QPushButton *otherExternalBrowserButton_;
  QCheckBox *javaScriptEnable_;
  QCheckBox *pluginsEnable_;
  QCheckBox *openLinkInBackground_;
  QCheckBox *openLinkInBackgroundEmbedded_;

  // feeds
  void setOpeningFeed(int action);
  int getOpeningFeed();
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

  QComboBox *formatDate_;
  QComboBox *formatTime_;

  QCheckBox *alternatingRowColorsNews_;

  QCheckBox *dayCleanUpOn_;
  QSpinBox *maxDayCleanUp_;
  QCheckBox *newsCleanUpOn_;
  QSpinBox *maxNewsCleanUp_;
  QCheckBox *readCleanUp_;
  QCheckBox *neverUnreadCleanUp_;
  QCheckBox *neverStarCleanUp_;
  QCheckBox *neverLabelCleanUp_;

  // labels
  QStringList idLabels_;
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

  // language
  QString language();
  void setLanguage(QString langFileName);

  // fonts
  QTreeWidget *fontsTree_;

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
  void acceptDialog();
  void closeDialog();
  void slotFontChange();
  void slotFontReset();
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
  void slotDeletePass();
  void slotDeleteAllPass();
  void slotShowPass();

signals:
  void signalCategoriesTreeKeyUpDownPressed();
  void signalShortcutTreeUpDownPressed();

private:
  QSqlDatabase db_;

  QLabel *contentLabel_;
  QTreeWidget *categoriesTree_;
  QStackedWidget *contentStack_;

  //stack widgets
  QFrame *generalWidget_;
  QFrame *traySystemWidget_;
  QFrame *networkConnectionsWidget_;
  QFrame *browserWidget_;
  QTabWidget *feedsWidget_;
  QWidget *labelsWidget_;
  QFrame *notifierWidget_;
  QWidget *passwordsWidget_;
  QWidget *languageWidget_;
  QWidget *fontsWidget_;
  QWidget *shortcutWidget_;

  // general
  void createGeneralWidget();

  // systemTray
  void createTraySystemWidget();
  QRadioButton *staticIconTray_;
  QRadioButton *changeIconTray_;
  QRadioButton *newCountTray_;
  QRadioButton *unreadCountTray_;

  // network connection
  void createNetworkConnectionsWidget();
  QRadioButton *systemProxyButton_;
  QRadioButton *directConnectionButton_;
  QRadioButton *manualProxyButton_;
  QWidget *manualWidget_;
  LineEdit *editHost_;
  LineEdit *editPort_;
  LineEdit *editUser_;
  LineEdit *editPassword_;
  QNetworkProxy networkProxy_;

  // browser
  void createBrowserWidget();

  // feeds
  void createFeedsWidget();

  // notifier
  void createNotifierWidget();
  void loadNotifier();
  void applyNotifier();
  bool loadNotifierOk_;

  // labels
  void createLabelsWidget();
  void loadLabels();
  void applyLabels();
  void addIdLabelList(QString idLabel);
  QPushButton *newLabelButton_;
  QPushButton *editLabelButton_;
  QPushButton *deleteLabelButton_;
  QPushButton *moveUpLabelButton_;
  QPushButton *moveDownLabelButton_;
  bool loadLabelsOk_;

  // passwords
  void createPasswordsWidget();
  void applyPass();
  QTreeWidget *passTree_;

  // language
  void createLanguageWidget();
  QTreeWidget *languageFileList_;

  // fonts
  void createFontsWidget();

  // shortcut
  void createShortcutWidget();
  QStringList *listDefaultShortcut_;
  QTreeWidget *shortcutTree_;
  LineEdit *editShortcut_;
  QGroupBox *editShortcutBox;

};

#endif // OPTIONSDIALOG_H
