/*! \file db_func.cpp *********************************************************
 * Функции для работы с файлом базы
 * \date created 17.04.2012
 ******************************************************************************/

#include <QtCore>
#include <QtSql>

QString kDbName = "feeds.db";

const QString kCreateFeedsTableQuery_v0_1_0(
    "create table feeds("
    "id integer primary key, "
    "text varchar, "             // Текст ленты (сейчас заменяет имя)
    "title varchar, "            // Имя ленты
    "description varchar, "      // Описание ленты
    "xmlUrl varchar, "           // интернет-адрес самой ленты
    "htmlUrl varchar, "          // интернет-адрес сайта, с которого забираем ленту
    "language varchar, "         // язык, на котором написана лента
    "copyrights varchar, "       // права
    "author_name varchar, "      // автор лента: имя
    "author_email varchar, "     //              е-мейл
    "author_uri varchar, "       //              личная страница
    "webMaster varchar, "        // е-мейл адрес ответственного за технические неполядки ленты
    "pubdate varchar, "          // Дата публикации содержимого ленты
    "lastBuildDate varchar, "    // Последняя дата изменения содержимого ленты
    "category varchar, "         // категории содержимого, освещаемые в ленте
    "contributor varchar, "      // участник (через табы)
    "generator varchar, "        // программа, используемая для генерации содержимого
    "docs varchar, "             // ссылка на документ, описывающий стандарт RSS
    "cloud_domain varchar, "     // Веб-сервис, предоставляющий rssCloud интерфейс
    "cloud_port varchar, "       //   .
    "cloud_path varchar, "       //   .
    "cloud_procedure varchar, "  //   .
    "cloud_protocal varchar, "   //   .
    "ttl integer, "              // Время в минутах, в течение которого канал может быть кеширован
    "skipHours varchar, "        // Подсказка аггрегаторам, когда не нужно обновлять ленту (указываются часы)
    "skipDays varchar, "         // Подсказка аггрегаторам, когда не нужно обновлять ленту (указываются дни недели)
    "image blob, "               // gif, jpeg, png рисунок, который может быть ассоциирован с каналом
    "unread integer, "           // количество непрочитанных новостей
    "newCount integer, "         // количество новых новостей
    "currentNews integer, "      // отображаемая новость
    "label varchar"              // выставляется пользователем
    ")");

const QString kCreateFeedsTableQuery(
    "create table feeds("
    "id integer primary key, "
    "text varchar, "             // Текст ленты (сейчас заменяет имя)
    "title varchar, "            // Имя ленты
    "description varchar, "      // Описание ленты
    "xmlUrl varchar, "           // интернет-адрес самой ленты
    "htmlUrl varchar, "          // интернет-адрес сайта, с которого забираем ленту
    "language varchar, "         // язык, на котором написана лента
    "copyrights varchar, "       // права
    "author_name varchar, "      // автор лента: имя
    "author_email varchar, "     //              е-мейл
    "author_uri varchar, "       //              личная страница
    "webMaster varchar, "        // е-мейл адрес ответственного за технические неполядки ленты
    "pubdate varchar, "          // Дата публикации содержимого ленты
    "lastBuildDate varchar, "    // Последняя дата изменения содержимого ленты
    "category varchar, "         // категории содержимого, освещаемые в ленте
    "contributor varchar, "      // участник (через табы)
    "generator varchar, "        // программа, используемая для генерации содержимого
    "docs varchar, "             // ссылка на документ, описывающий стандарт RSS
    "cloud_domain varchar, "     // Веб-сервис, предоставляющий rssCloud интерфейс
    "cloud_port varchar, "       //   .
    "cloud_path varchar, "       //   .
    "cloud_procedure varchar, "  //   .
    "cloud_protocal varchar, "   //   .
    "ttl integer, "              // Время в минутах, в течение которого канал может быть кеширован
    "skipHours varchar, "        // Подсказка аггрегаторам, когда не нужно обновлять ленту (указываются часы)
    "skipDays varchar, "         // Подсказка аггрегаторам, когда не нужно обновлять ленту (указываются дни недели)
    "image blob, "               // gif, jpeg, png рисунок, который может быть ассоциирован с каналом
    "unread integer, "           // количество непрочитанных новостей
    "newCount integer, "         // количество новых новостей
    "currentNews integer, "      // отображаемая новость
    "label varchar, "            // выставляется пользователем
    // -- added in v0.9.0 --
    // --- Categories ---
    "hasChildren int, "  // наличие потомков
    "parentId int, "     // id родителя
    "rowToParent int, "  // номер строки относительно родителя
    // --- RSSowl::General ---
    "updateIntervalEnable int, "    // включение автообновления
    "updateInterval int, "          // интервал обновления
    "updateIntervalType varchar, "  // тип интервала (минуты, часы, ...)
    "updateOnStartup int, "         // обновлять при запуске
    "displayOnStartup int, "        // показывать ленту в отдельном табе при запуске
    // --- RSSowl::Reading ---
    "markReadAfterSecondsEnable int, "    // задействовать таймер "прочитанности"
    "markReadAfterSeconds int, "          // количество секунд, через которое новость считается прочитанной
    "markReadInNewspaper int, "           // помечать прочитанной, если читается через "газету"
    "markDisplayedOnSwitchingFeed int, "  // помечать прочитанной, после переключения на другую ленту
    "markDisplayedOnClosingTab int, "     // помечать прочитанной, после закрытия таба
    "markDisplayedOnMinimize int, "       // помечать прочитанной, после минимизации окна
    // --- RSSowl::Display ---
    "layout text, "      // стиль отображения новостей
    "filter text, "      // фильтр отображаемых новостей
    "groupBy int, "      // номер столца, по которому производится группировка
    "displayNews int, "  // 0 - отображаем Content новости; 1 - вытягиваем содержимое, расположенное по link
    "displayEmbeddedImages int, "   // отображать изображения встроенные в новость
    "loadTypes text, "              // типы загружаемого контента ("images" or "images sounds")
    "openLinkOnEmptyContent int, "  // загружать link, если content пустой
    // --- RSSowl::Columns ---
    "columns text, "  // перечень и порядок колонок, отображаемых в таблице новостей
    "sort text, "     // имя колонки для сортировки
    "sortType int, "  // по возрастанию, по убыванию
    // --- RSSowl::Clean Up ---
    "maximumToKeep int, "           // максимальное число хранимых новостей
    "maximumToKeepEnable int, "     // задействоваь ограничитель
    "maximumAgeOfNews int, "        // максимальное время хранения новости
    "maximumAgoOfNewEnable int, "   // задействоваь ограничитель
    "deleteReadNews int, "          // удалять прочитанные новости
    "neverDeleteUnreadNews int, "   // не удалять непрочитанные новости
    "neverDeleteStarredNews int, "  // не удалять избранные новости
    "neverDeleteLabeledNews int, "  // не удалять новости имеющие метку
    // --- RSSowl::Status ---
    "status text, "                 // результат последнего обновления
    "created text, "                // время создания ленты
    "updated text, "                // время последнего обновления ленты (update)
    "lastDisplayed text "           // время последнего просмотра ленты
    ")");

const QString kCreateNewsTableQuery_v0_1_0(
    "create table feed_%1("
    "id integer primary key, "
    "feed integer, "                       // идентификатор ленты из таблицы feeds
    "guid varchar, "                       // уникальный номер
    "guidislink varchar default 'true', "  // флаг того, что уникальный номер является ссылкой на новость
    "description varchar, "                // краткое содержание
    "content varchar, "                    // полное содержание (atom)
    "title varchar, "                      // заголовок
    "published varchar, "                  // дата публикащии
    "modified varchar, "                   // дата модификации
    "received varchar, "                   // дата приёма новости (выставляется при приёме)
    "author_name varchar, "                // имя автора
    "author_uri varchar, "                 // страничка автора (atom)
    "author_email varchar, "               // почта автора (atom)
    "category varchar, "                   // категория, может содержать несколько категорий (например через знак табуляции)
    "label varchar, "                      // метка (выставляется пользователем)
    "new integer default 1, "              // Флаг "новая". Устанавливается при приёме, снимается при закрытии программы
    "read integer default 0, "             // Флаг "прочитанная". Устанавливается после выбора новости
    "sticky integer default 0, "           // Флаг "отличная". Устанавливается пользователем
    "deleted integer default 0, "          // Флаг "удалённая". Новость помечается удалённой, но физически из базы не удаляется,
                                           //   чтобы при обновлении новостей она не появлялась вновь.
                                           //   Физическое удаление новость будет производится при общей чистке базы
    "attachment varchar, "                 // ссылка на прикрепленные файлы (ссылки могут быть разделены табами)
    "comments varchar, "                   // интернел-ссылка на страницу, содержащую комментарии(ответы) к новости
    "enclosure_length, "                   // медиа-объект, ассоциированный с новостью:
    "enclosure_type, "                     //   длина, тип,
    "enclosure_url, "                      //   адрес.
    "source varchar, "                     // источник, если это перепубликация  (atom: <link via>)
    "link_href varchar, "                  // интернет-ссылка на новость (atom: <link self>)
    "link_enclosure varchar, "             // интернет-ссылка на потенциально большой объём информации,
                                           //   который нереально передать в новости (atom)
    "link_related varchar, "               // интернет-ссылка на сопутствующие данный для новости  (atom)
    "link_alternate varchar, "             // интернет-ссылка на альтернативное представление новости
    "contributor varchar, "                // участник (через табы)
    "rights varchar "                      // права
    ")");

const QString kCreateNewsTableQuery(
    "create table feed_%1("
    "id integer primary key, "
    "feed integer, "                       // идентификатор ленты из таблицы feeds
    "guid varchar, "                       // уникальный номер
    "guidislink varchar default 'true', "  // флаг того, что уникальный номер является ссылкой на новость
    "description varchar, "                // краткое содержание
    "content varchar, "                    // полное содержание (atom)
    "title varchar, "                      // заголовок
    "published varchar, "                  // дата публикащии
    "modified varchar, "                   // дата модификации
    "received varchar, "                   // дата приёма новости (выставляется при приёме)
    "author_name varchar, "                // имя автора
    "author_uri varchar, "                 // страничка автора (atom)
    "author_email varchar, "               // почта автора (atom)
    "category varchar, "                   // категория, может содержать несколько категорий (например через знак табуляции)
    "label varchar, "                      // метка (выставляется пользователем)
    "new integer default 1, "              // Флаг "новая". Устанавливается при приёме, снимается при закрытии программы
    "read integer default 0, "             // Флаг "прочитанная". Устанавливается после выбора новости
    "sticky integer default 0, "           // Флаг "отличная". Устанавливается пользователем
    "deleted integer default 0, "          // Флаг "удалённая". Новость помечается удалённой, но физически из базы не удаляется,
                                           //   чтобы при обновлении новостей она не появлялась вновь.
                                           //   Физическое удаление новость будет производится при общей чистке базы
    "attachment varchar, "                 // ссылка на прикрепленные файлы (ссылки могут быть разделены табами)
    "comments varchar, "                   // интернел-ссылка на страницу, содержащую комментарии(ответы) к новости
    "enclosure_length, "                   // медиа-объект, ассоциированный с новостью:
    "enclosure_type, "                     //   длина, тип,
    "enclosure_url, "                      //   адрес.
    "source varchar, "                     // источник, если это перепубликация  (atom: <link via>)
    "link_href varchar, "                  // интернет-ссылка на новость (atom: <link self>)
    "link_enclosure varchar, "             // интернет-ссылка на потенциально большой объём информации,
                                           //   который нереально передать в новости (atom)
    "link_related varchar, "               // интернет-ссылка на сопутствующие данный для новости  (atom)
    "link_alternate varchar, "             // интернет-ссылка на альтернативное представление новости
    "contributor varchar, "                // участник (через табы)
    "rights varchar "                      // права
    ")");

const QString kCreateFiltersTable(
    "create table filters("
    "id integer primary key, "
    "name varchar, "            // имя фильтра
    "type int, "                // тип фильтра (И, ИЛИ, для всех лент)
    "feeds varchar "            // перечень лент, которые пользуются фильтром
    ")");

const QString kCreateFilterConditionsTable(
    "create table filters("
    "id integer primary key, "
    "idFilter int, "            // идентификатор фильтра
    "field varchar, "           // поле, по которому производится фильтрация
    "condition varchar, "       // условие, применяемое к полю
    "content varchar "          // содержимое поля, используемое условием
    ")");

const QString kCreateFilterActionsTable(
    "create table filters("
    "id integer primary key, "
    "idFilter int, "            // идентификатор фильтра
    "action varchar, "          // действие, выполняемое с новостью, удовлетворяющей фильтру
    "params varchar "           // параметры действия
    ")");

//-----------------------------------------------------------------------------
void initDB(const QString dbFileName)
{
  QString dbVersionString("");
  if (!QFile(dbFileName).exists()) {  // Инициализация базы
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "dbFileName_");
    db.setDatabaseName(dbFileName);
    db.open();
    db.transaction();
    db.exec(kCreateFeedsTableQuery);
    db.exec("create table info(id integer primary key, name varchar, value varchar)");
    db.exec("insert into info(name, value) values ('version', '1.0')");
    db.commit();
    db.close();
  } else {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "dbFileName_");
    db.setDatabaseName(dbFileName);
    db.open();
//    db.transaction();
    QSqlQuery q(db);
    q.exec("select value from info where name='version'");
    if (q.next())
      dbVersionString = q.value(0).toString();
    if (!dbVersionString.isEmpty()) {
      // Версия базы 1.0 (На самом деле 0.1.0)
      if (dbVersionString == "1.0") {

        //! Обновляем таблицу лент
        q.exec(QString(kCreateFeedsTableQuery)
            .replace("table feeds", "temp table feeds_temp"));
        qDebug() << q.lastQuery() << q.lastError().text();
        // переписываем только используемые ранее поля
        q.exec("INSERT "
            "INTO feeds_temp("
               "id, text, title, description, xmlUrl, htmlUrl, "
               "author_name, author_email, author_uri, pubdate, lastBuildDate, "
               "image, unread, newCount, currentNews"
            ") SELECT "
            "id, text, title, description, xmlUrl, htmlUrl, "
            "author_name, author_email, author_uri, pubdate, lastBuildDate, "
            "image, unread, newCount, currentNews "
            "FROM feeds");
        qDebug() << q.lastQuery() << q.lastError().text();
        q.exec("drop table feeds");
        qDebug() << q.lastQuery() << q.lastError().text();

        q.exec(kCreateFeedsTableQuery);
        qDebug() << q.lastQuery() << q.lastError().text();
        q.exec("insert into feeds select * from feeds_temp");
        qDebug() << q.lastQuery() << q.lastError().text();
        q.exec("drop table feeds_temp");
        qDebug() << q.lastQuery() << q.lastError().text();

        //! Обновляем таблицу новостей
        //! \TODO: Реализовать сабж

        dbVersionString = "0.9.0";
        qDebug() << "DB converted to version =" << dbVersionString;
      } else {
        qDebug() << "dbVersion =" << dbVersionString;
      }
    }
//    db.commit();
    db.close();
  }
  QSqlDatabase::removeDatabase("dbFileName_");
}
