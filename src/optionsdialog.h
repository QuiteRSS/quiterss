#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QtGui>
#include <QNetworkProxy>

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

  // systemTray
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

  QCheckBox *markNewsReadOn_;
  QSpinBox *markNewsReadTime_;

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
  void shortcutTreeClicked(QTreeWidgetItem* item, int column);
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
  QWidget *generalWidget_;
  QWidget *networkConnectionsWidget_;
  QWidget *browserWidget_;
  QTabWidget *feedsWidget_;
  QWidget *notifierWidget_;
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
  QLineEdit *editHost_;
  QLineEdit *editPort_;
  QLineEdit *editUser_;
  QLineEdit *editPassword_;

  // language
  QListWidget *languageFileList_;

  // internal variables for options
  QNetworkProxy networkProxy_;

  // shortcut
  QTreeWidget *shortcutTree_;
  QLineEdit *editShortcut_;
  QStringList *listDefaultShortcut_;
  QGroupBox *editShortcutBox;

};

#endif // OPTIONSDIALOG_H
