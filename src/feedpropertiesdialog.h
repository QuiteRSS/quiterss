/* ============================================================
 * QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
 * Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ============================================================ */
#ifndef FEEDPROPERTIESDIALOG_H
#define FEEDPROPERTIESDIALOG_H

#include "dialog.h"
#include "lineedit.h"
#include "toolbutton.h"

//! Feed properties structure
typedef struct {
  //! General properties
  struct general{
    QString text; //!< Text field from feed-xml
    QString title; //!< Title field from feed-xml
    QString url; //!< URL field from feed-xml
    QString homepage; //!< Homepage field from feed-xml
    QByteArray image;
    bool disableUpdate;
    bool updateEnable; //!< Flag enabling autoupdate
    int updateInterval; //!< Update interval
    int intervalType; //!< Update interval type (sec, min, day)
    bool updateOnStartup; //!< Flag to update feed on startup
    int displayOnStartup; //!< Flag to display feed on startup in sepatare tab
    bool starred; //!< Starred feed (favourite)
    bool duplicateNewsMode; //!< Automatically delete news duplicates
  } general;

  //! Autthentication properties
  struct authentication{
    bool on;        //!< Enabling flag
    QString user;   //!< Username
    QString pass;   //!< User password
  } authentication;

  //! Reading properties
  struct reading{
    bool markSelectedAsRead; //!< Mark focused news as Read
    quint32 markSelectedTime; //!< Mark focused news as Read after elapsed time (sec)
    bool markReadWhileReading; //!< Mark news as Read while reading in newspaper layout
    bool markDisplayedAsReadWhenSwitch; //!< Mark displayed news as Read on switching feed
    bool markDisplayedAsReadWhenClose; //!< Mark displayed news as Read on closing tab
    bool markDisplayedAsReadOnMin; //!< Mark displayed news as Read on minimize
  } reading ;

  //! Display properties
  struct display {
    quint16 layoutType; //!< display layout
    quint16 filterType; //!< Filter mode
    quint16 groupType; //!< Group mode
    int displayNews; //!< Display news mode
    int displayEmbeddedImages; //!< Display embedded images
    bool loadMoviesAndOtherContent; //!< Flag to load media content
    bool openLink; //!< Flag to open news link
    int layoutDirection; //!< LTR or RTL layout
    int javaScriptEnable;
  } display;

  //! Columns properties
  struct column {
    QList<int> columns; //!< Indexes list of columns to display
    int sortBy; //!< Index of column to sort by
    int sortType; //!< Type of sort (ascending|descending - 0|1)
    QList<int> indexList; //!< Indexes list of all columns
    QStringList nameList; //!< Names list of all columns
  } column;

  //! Columns default properties
  struct columnDefault {
    QList<int> columns; //!< List of column to display
    int sortBy; //!< Name of column to sort by
    int sortType; //!< Type of sort (ascending|descending)
  } columnDefault;

  //! Cleaup properties
  struct cleanup {
    bool enableMaxNews; //!< Enable flag for \a maxNewsToKeep
    quint32 maxNewsToKeep; //!< Maximum number of news to keep in DB
    bool enableAgeNews; //!< Enable flag for \a ageOfNewsToKeep
    quint32 ageOfNewsToKeep; //!< Time to keep news (days)
    bool deleteReadNews; //!< Flag to delete already read news
    bool neverDeleteUnread; //!< Flag to never delete unread news
    bool neverDeleteLabeled; //!< Flag to never delete labeled news
  } cleanup;

  //! Status properties
  struct status {
    QString feedStatus; //!< Feed status
    QString description; //!< Feed description
    QDateTime createdTime; //!< Time when feed created
    QDateTime lastDisplayed; //!< Last time that feed displayed
    QDateTime lastUpdate; //!< Time of feed last update
    QDateTime lastBuildDate;
    int undeleteCount; //!< Number of all news
    int newCount; //!< Number of new news
    int unreadCount; //!< Number of unread news
    int feedsCount; //!< Number of feeds
  } status;

} FEED_PROPERTIES;

//! Feed properties dialog
class FeedPropertiesDialog : public Dialog
{
  Q_OBJECT
public:
  explicit FeedPropertiesDialog(bool isFeed, QWidget *parent);

  FEED_PROPERTIES getFeedProperties(); //!< Get feed properties from dialog
  void setFeedProperties(FEED_PROPERTIES properties); //!< Set feed properties into dialog

public slots:
  void slotFaviconUpdate(const QString &feedUrl, const QByteArray &faviconData);

signals:
  void signalLoadIcon(const QString &urlString, const QString &feedUrl);

protected:
  virtual void showEvent(QShowEvent *);

private slots:
  void setDefaultTitle();
  void loadDefaultIcon();
  void selectIcon();
  void slotCurrentColumnChanged(QTreeWidgetItem *current, QTreeWidgetItem *);
  void showMenuAddButton();
  void addColumn(QAction *action);
  void removeColumn();
  void moveUpColumn();
  void moveDownColumn();
  void defaultColumns();

private:
  QTabWidget *tabWidget;

  // Tab "General"
  LineEdit *editURL; //!< Feed URL
  LineEdit *editTitle; //!< Feed title
  QLabel *labelHomepage; //!< Link to feed's homepage
  QToolButton *selectIconButton_;
  QCheckBox *disableUpdate_;
  QCheckBox *updateEnable_;
  QSpinBox *updateInterval_;
  QComboBox *updateIntervalType_;
  QCheckBox *displayOnStartup;
  QCheckBox *starredOn_;
  QCheckBox *duplicateNewsMode_;

  QWidget *createGeneralTab();

  // Tab "Display"
  QCheckBox *showDescriptionNews_;
  QCheckBox *loadImagesOn_;
  QCheckBox *javaScriptEnable_;
  QCheckBox *layoutDirection_;

  QWidget *createDisplayTab();

  // Tab "Columns"
  QTreeWidget *columnsTree_;
  QComboBox *sortByColumnBox_;
  QComboBox *sortOrderBox_;

  QMenu *addButtonMenu_;

  QPushButton *addButton_;
  QPushButton *removeButton_;
  QPushButton *moveUpButton_;
  QPushButton *moveDownButton_;
  QWidget *createColumnsTab();

  // Tab "Authentication"
  QGroupBox *authentication_;
  LineEdit *user_;
  LineEdit *pass_;
  QWidget *createAuthenticationTab();

  // Tab "Status"
  QLabel *statusFeed_;
  QTextEdit *descriptionText_;
  QLabel *createdFeed_;
  QLabel *lastUpdateFeed_;
  QLabel *newsCount_;
  QLabel *feedsCount_;
  QWidget *createStatusTab();

  FEED_PROPERTIES feedProperties;

  bool isFeed_;

};

#endif // FEEDPROPERTIESDIALOG_H
