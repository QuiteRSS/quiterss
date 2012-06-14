#ifndef FEEDPROPERTIESDIALOG_H
#define FEEDPROPERTIESDIALOG_H

#include <QtGui>
#include "lineedit.h"

//! Настройки ленты
typedef struct {

  //! Основные настройки
  struct general{
    QString text; //!< Имя
    QString title; //!< Имя
    QString url; //!< URL
    QString homepage; //!< Домашняя страница
    quint32 updateInterval; //!< Интервал обновления
    quint32 intervalParameter; //!< Единицы измерения интервала
    bool updateOnStartup; //!< Обновлять при запуске приложения
    int displayOnStartup; //!< Отображать при запуске приложения
  } general;

  //! Настройки чтения
  struct reading{
    bool markSelectedAsRead; //!< Помечать выбранное как "Прочитано"
    quint32 markSelectedTime; //!< Время до отметки"Прочитано"
    bool markReadWhileReading; //!< Mark news as read while reading in newspaper layout
    bool markDisplayedAsReadWhenSwitch; //!< Помечать отображаемые новости как прочитанные при переключении ленты
    bool markDisplayedAsReadWhenClose; //!< Помечать отображаемые новости как прочитанные при закрытии таба
    bool markDisplayedAsReadOnMin; //!< Помечать отображаемые новости как прочитанные при минимизации
  } reading ;

  //! Настройки отображения
  struct display {
    quint16 layoutType; //!< Раскладка для отображения
    quint16 filterType; //!< Фильтр
    quint16 groupType; //!< Способ группировки
    bool displayContent; //!< Показывать содержимое новости
    bool displayEmbeddedImages; //!< Показывать встроенные изображения
    bool loadMoviesAndOtherContent; //!< Загружать видео и другое содержимое
    bool openLink; //!< Открывать ссылку новости
  } display;

  //! Настройки колонок
  struct column {
    QStringList columns; //!< Список колонок
    QString sortBy; //!< Колонка для сортировки
    QString sortType; //!< Тип сортировки
  } column;

  //! Настройки очистки
  struct cleanup {
    bool enableMaxNews; //!< Разрешить максимальное количество новостей
    quint32 maxNewsToKeep; //!< Максимальное количество новостей для хранения
    bool enableAgeNews; //!< Разрешить время хранения новостей
    quint32 ageOfNewsToKeep; //!< Время хранения новостей (дней)
    bool deleteReadNews; //!< Удалять прочитанное
    bool neverDeleteUnread; //!< Никогда не удалять прочитанное
    bool neverDeleteLabeled; //!< Никогда не удалять отмеченное
  } cleanup;

  //! Статус ленты
  struct status {
    QString feedStatus; //!< Статус ленты
    QString description; //!< Описание
    QDateTime createdTime; //!< Время создания
    QDateTime lastDisplayed; //!< Последний просмотр
    quint64 newsCount; //!< Количесвто новостей
  } status;

} FEED_PROPERTIES;

//! Виджет настроек ленты
class FeedPropertiesDialog : public QDialog
{
  Q_OBJECT
public:
  explicit FeedPropertiesDialog(QWidget *parent = 0);

  FEED_PROPERTIES getFeedProperties(); //!< Получить свойства ленты из диалога
  void setFeedProperties(FEED_PROPERTIES properties); //!< Передать свойства ленты в диалог

private:
  QDialogButtonBox *buttonBox;
  QTabWidget *tabWidget;

  // Вкладка "Общие"
  QWidget *tabGeneral; //!< Виджет вкладки
  LineEdit *editURL; //!< строка ссылки на ленту
  LineEdit *editTitle; //!< Заголовок ленты
  QLabel *labelHomepage; //!< Ссылка на домашнюю страницу
  QCheckBox *displayOnStartup;

  QWidget *CreateGeneralTab(); //!< Создание вкладки "Общие"

  FEED_PROPERTIES feedProperties;

protected:
  virtual void showEvent(QShowEvent *event);

private slots:
  void slotLoadTitle();

signals:
  void signalLoadTitle(const QUrl &url, const QUrl &feedUrl);
  void startGetUrlTimer();

};

#endif // FEEDPROPERTIESDIALOG_H
