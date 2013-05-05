/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#ifndef FEEDPROPERTIESDIALOG_H
#define FEEDPROPERTIESDIALOG_H

#include "dialog.h"
#include "lineedit.h"

//! Feed properties structure
typedef struct {
  //! General properties
  struct general{
    QString text; //!< Имя
    QString title; //!< Имя
    QString url; //!< URL
    QString homepage; //!< Домашняя страница
    bool updateEnable; //!< Включение автообновления
    int updateInterval; //!< Интервал обновления
    int intervalType; //!< Тип итервала обновления
    bool updateOnStartup; //!< Обновлять при запуске приложения
    int displayOnStartup; //!< Отображать при запуске приложения
    bool starred; //!< Избранная лента
    bool duplicateNewsMode; //!< Автоматический удалять дубликаты новостей
  } general;

  //! Autthentication properties
  struct authentication{
    bool on;        //!< Включение
    QString user;   //!< Пользователь
    QString pass;   //!< Пароль
  } authentication;

  //! Reading properties
  struct reading{
    bool markSelectedAsRead; //!< Помечать выбранное как "Прочитано"
    quint32 markSelectedTime; //!< Время до отметки "Прочитано"
    bool markReadWhileReading; //!< Mark news as read while reading in newspaper layout
    bool markDisplayedAsReadWhenSwitch; //!< Помечать отображаемые новости как прочитанные при переключении ленты
    bool markDisplayedAsReadWhenClose; //!< Помечать отображаемые новости как прочитанные при закрытии таба
    bool markDisplayedAsReadOnMin; //!< Помечать отображаемые новости как прочитанные при минимизации
  } reading ;

  //! Display properties
  struct display {
    quint16 layoutType; //!< Раскладка для отображения
    quint16 filterType; //!< Фильтр
    quint16 groupType; //!< Способ группировки
    int displayNews; //!< Показывать содержимое новости
    int displayEmbeddedImages; //!< Показывать встроенные изображения
    bool loadMoviesAndOtherContent; //!< Загружать видео и другое содержимое
    bool openLink; //!< Открывать ссылку новости
  } display;

  //! Columns properties
  struct column {
    QList<int> columns; //!< Список индексов отображаемых колонок
    int sortBy; //!< Колонка для сортировки
    int sortType; //!< Тип сортировки
    QList<int> indexList; //!< Список индексов всех колонок
    QStringList nameList; //!< Список названий колонок
  } column;

  //! Cleaup properties
  struct cleanup {
    bool enableMaxNews; //!< Разрешить максимальное количество новостей
    quint32 maxNewsToKeep; //!< Максимальное количество новостей для хранения
    bool enableAgeNews; //!< Разрешить время хранения новостей
    quint32 ageOfNewsToKeep; //!< Время хранения новостей (дней)
    bool deleteReadNews; //!< Удалять прочитанное
    bool neverDeleteUnread; //!< Никогда не удалять прочитанное
    bool neverDeleteLabeled; //!< Никогда не удалять отмеченное
  } cleanup;

  //! Status properties
  struct status {
    QString feedStatus; //!< Статус ленты
    QString description; //!< Описание
    QDateTime createdTime; //!< Время создания
    QDateTime lastDisplayed; //!< Последний просмотр
    QDateTime lastUpdate; //!< Последние обновление
    int undeleteCount; //!< Количество всех новостей
    int newCount; //!< Количество новых новостей
    int unreadCount; //!< Количество непрочитанных новостей
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

signals:
  void signalLoadTitle(const QString &urlString, const QString &feedUrl);

protected:
  virtual void showEvent(QShowEvent *event);

private slots:
  void slotLoadTitle();
  void slotCurrentColumnChanged(QTreeWidgetItem *current, QTreeWidgetItem *);
  void showMenuAddButton();
  void addColumn(QAction *action);
  void removeColumn();
  void moveUpColumn();
  void moveDownColumn();

private:
  QTabWidget *tabWidget;

  // Tab "General"
  LineEdit *editURL; //!< Feed URL
  LineEdit *editTitle; //!< Feed title
  QLabel *labelHomepage; //!< Link to feed's homepage
  QCheckBox *updateEnable_;
  QSpinBox *updateInterval_;
  QComboBox *updateIntervalType_;
  QCheckBox *displayOnStartup;
  QCheckBox *showDescriptionNews_;
  QCheckBox *starredOn_;
  QCheckBox *loadImagesOn;
  QCheckBox *duplicateNewsMode_;

  QWidget *CreateGeneralTab();

  // Tab "Columns"
  QTreeWidget *columnsTree_;
  QComboBox *sortByColumnBox_;
  QComboBox *sortOrderBox_;

  QMenu *addButtonMenu_;

  QPushButton *addButton_;
  QPushButton *removeButton_;
  QPushButton *moveUpButton_;
  QPushButton *moveDownButton_;
  QWidget *CreateColumnsTab();

  // Tab "Authentication"
  QGroupBox *authentication_;
  LineEdit *user_;
  LineEdit *pass_;
  QWidget *CreateAuthenticationTab();

  // Tab "Status"
  QTextEdit *descriptionText_;
  QLabel *createdFeed_;
  QLabel *lastUpdateFeed_;
  QLabel *newsCount_;
  QWidget *CreateStatusTab();

  FEED_PROPERTIES feedProperties;

  bool isFeed_;

};

#endif // FEEDPROPERTIESDIALOG_H
