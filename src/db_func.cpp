/*! \file db_func.cpp *********************************************************
 * Функции для работы с файлом базы
 * \date created 17.04.2012
 ******************************************************************************/

#include <QtCore>
#include <QtSql>

QString kDbName    = "feeds.db";
QString kDbVersion = "0.10.0";

const QString kCreateFeedsTableQuery_v0_1_0(
    "CREATE TABLE feeds("
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

const QString kCreateFeedsTableQuery_v0_9_0(
    "CREATE TABLE feeds("
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
    "label varchar, "            // метка. выставляется пользователем
    // -- added in v0.9.0 --
    "undeleteCount integer, "         // количество всех новостей (не помеченныъ удалёнными)
    "tags varchar, "             // теги. выставляются пользователем
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

const QString kCreateFeedsTableQuery(
    "CREATE TABLE feeds("
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
    "label varchar, "            // метка. выставляется пользователем
    // -- added in v0.9.0 --
    "undeleteCount integer, "    // количество всех новостей (не помеченныъ удалёнными)
    "tags varchar, "             // теги. выставляются пользователем
    // --- Categories ---
    "hasChildren integer default 0, "  // наличие потомков. По умолчанию - нет
    "parentId integer default 0, "     // id родителя. По умолчанию - корень дерева
    "rowToParent integer, "            // номер строки относительно родителя
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
    "displayEmbeddedImages integer default 1, "  // отображать изображения встроенные в новость
    "loadTypes text, "                           // типы загружаемого контента ("images" or "images sounds" - только изображения или изображения и звуки)
    "openLinkOnEmptyContent int, "               // загружать link, если content пустой
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
    "lastDisplayed text, "           // время последнего просмотра ленты
    // -- added in v0.10.0 --`
    "f_Expanded integer default 1, "  // флаг раскрытости категории дерева
    "flags text"                    // другие флаги перечисляемые текстом (могут быть, например "focused", "hidden")
    // -- changed in v0.10.0 -- исправлено в тексте выше
    // "displayEmbeddedImages integer default 1, "  // отображать изображения встроенные в новость
    // "hasChildren integer default 0, "  // наличие потомков. По умолчанию - нет
    // "parentId integer default 0, "     // id родителя. По умолчанию - корень дерева
    ")");

const QString kCreateNewsTableQuery_v0_1_0(
    "CREATE TABLE feed_%1("
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
    "CREATE TABLE news("
    "id integer primary key, "
    "feedId integer, "                     // идентификатор ленты из таблицы feeds
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
    "starred integer default 0, "          // Флаг "отличная". Устанавливается пользователем
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

const QString kCreateFiltersTable_v0_9_0(
    "CREATE TABLE filters("
    "id integer primary key, "
    "name varchar, "            // имя фильтра
    "type int, "                // тип фильтра (И, ИЛИ, для всех лент)
    "feeds varchar "            // перечень лент, которые пользуются фильтром
    ")");

const QString kCreateFiltersTable(
    "CREATE TABLE filters("
    "id integer primary key, "
    "name varchar, "              // имя фильтра
    "type integer, "              // тип фильтра (И, ИЛИ, для всех лент)
    "feeds varchar, "             // перечень лент, которые пользуются фильтром
    "enable integer default 1, "  // 1 - фильтр используется, 0 - не используется
    "num integer "                // номер по порядку. Может использоваться для сортировки фильтров)
    ")");

const QString kCreateFilterConditionsTable(
    "CREATE TABLE filterConditions("
    "id integer primary key, "
    "idFilter int, "            // идентификатор фильтра
    "field varchar, "           // поле, по которому производится фильтрация
    "condition varchar, "       // условие, применяемое к полю
    "content varchar "          // содержимое поля, используемое условием
    ")");

const QString kCreateFilterActionsTable(
    "CREATE TABLE filterActions("
    "id integer primary key, "
    "idFilter int, "            // идентификатор фильтра
    "action varchar, "          // действие, выполняемое с новостью, удовлетворяющей фильтру
    "params varchar "           // параметры действия
    ")");

const QString kCreateLabelsTable(
    "CREATE TABLE labels("
    "id integer primary key, "
    "name varchar, "            // имя метки
    "image blob, "              // картинка метки
    "color_text varchar, "      // цвет текста новости в списке
    "color_bg varchar, "        // цвет фона новости в списке
    "num integer "              // номер по порядку, для сортировки
    ")");

void initLabelsTable(QSqlDatabase *db)
{
  QSqlQuery q(*db);
  q.exec(kCreateLabelsTable);
  QStringList strNameLabels;
  strNameLabels << "Important" << "Work" << "Personal"
                << "To Do" << "Later" << "Amusingly";
  for (int i = 0; i < 6; i++) {
    q.prepare("INSERT INTO labels(name, image) "
              "VALUES (:name, :image)");
    q.bindValue(":name", strNameLabels.at(i));

    QFile file(QString(":/images/label_%1").arg(i+1));
    file.open(QFile::ReadOnly);
    q.bindValue(":image", file.readAll());
    file.close();

    q.exec();

    int labelId = q.lastInsertId().toInt();
    q.exec(QString("UPDATE labels SET num='%1' WHERE id=='%1'").arg(labelId));
  }
}

//-----------------------------------------------------------------------------
QString initDB(const QString dbFileName)
{
  if (!QFile(dbFileName).exists()) {  // Инициализация базы
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "dbFileName_");
    db.setDatabaseName(dbFileName);
    db.open();
    db.transaction();
    db.exec(kCreateFeedsTableQuery);
    db.exec(kCreateNewsTableQuery);
    // Создаём индекс по полю feedId
    db.exec("CREATE INDEX feedId ON news(feedId)");

    // Создаём дополнительную таблицу лент на всякий случай
    db.exec("CREATE TABLE feeds_ex(id integer primary key, "
        "feedId integer, "  // идентификатор ленты
        "name varchar, "    // имя параметра
        "value varchar "    // значение параметра
        ")");
    // Создаём дополнительную таблицу новостей на всякий случай
    db.exec("CREATE TABLE news_ex(id integer primary key, "
        "feedId integer, "  // идентификатор ленты
        "newsId integer, "  // идентификатор новости
        "name varchar, "    // имя параметра
        "value varchar "    // значение параметра
        ")");
    // Создаём таблицы фильтров
    db.exec(kCreateFiltersTable);
    db.exec(kCreateFilterConditionsTable);
    db.exec(kCreateFilterActionsTable);
    // Создаём дополнительную таблицу фильтров на всякий случай
    db.exec("CREATE TABLE filters_ex(id integer primary key, "
        "idFilter integer, "  // идентификатор фильтра
        "name text, "         // имя параметра
        "value text"          // значение параметра
        ")");
    // Создаём таблицу меток
    initLabelsTable(&db);
    //
    db.exec("CREATE TABLE info(id integer primary key, name varchar, value varchar)");
    QSqlQuery q(db);
    q.prepare("INSERT INTO info(name, value) VALUES ('version', :version)");
    q.bindValue(":version", kDbVersion);
    q.exec();
    db.commit();
    db.close();
  } else {
    QString dbVersionString;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "dbFileName_");
    db.setDatabaseName(dbFileName);
    db.open();
    db.transaction();
    QSqlQuery q(db);
    q.exec("SELECT value FROM info WHERE name='version'");
    if (q.next())
      dbVersionString = q.value(0).toString();
    if (!dbVersionString.isEmpty()) {
      // Версия базы 1.0 (На самом деле 0.1.0)
      if (dbVersionString == "1.0") {
        //-- Преобразуем таблицу feeds
        // Создаём временную таблицу версии 0.10.0
        q.exec(QString(kCreateFeedsTableQuery)
            .replace("TABLE feeds", "TEMP TABLE feeds_temp"));
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
        // Хороним старую таблицу
        q.exec("DROP TABLE feeds");

        // Копируем временную таблицу на место старой
        q.exec(kCreateFeedsTableQuery);
        q.exec("INSERT INTO feeds SELECT * FROM feeds_temp");
        q.exec("DROP TABLE feeds_temp");

        // Модифицируем поле rowToParent таблицы feeds, чтобы все элементы
        // лежали в корне дерева
        q.exec("SELECT id FROM feeds");
        int rowToParent = 0;
        while (q.next()) {
          int id = q.value(0).toInt();
          QSqlQuery q2(db);
          q2.prepare("UPDATE feeds SET rowToParent=:rowToParent WHERE id=:id");
          q2.bindValue(":rowToParent", rowToParent);
          q2.bindValue(":id", id);
          q2.exec();
          rowToParent++;
        }

        //-- Преобразуем таблицы feed_# в общую таблицу news
        // Создаём общую таблицу новостей
        q.exec(kCreateNewsTableQuery);

        // Переписываем все новости в общую таблицу с добавлением идентификатора ленты
        //! \NOTE: Удалить в одном цикле не получается, т.к. при удалении
        //! таблицы получается больше одного активного запроса, чего делать нельзя
        //! Поэтому идентификаторы запоминаются в список
        q.exec("SELECT id FROM feeds");
        QList<int> idList;
        while (q.next()) {
          int feedId = q.value(0).toInt();
          idList << feedId;
          QSqlQuery q2(db);
          q2.exec(QString("SELECT "
              "guid, guidislink, description, content, title, published, received, "  // 0..6
              "author_name, author_uri, author_email, category, "                     // 7..10
              "new, read, sticky, deleted, "                                          // 11..14
              "link_href "                                                            // 15
              "FROM feed_%1").arg(feedId));
          while (q2.next()) {
            QSqlQuery q3(db);
            q3.prepare(QString("INSERT INTO news("
                  "feedId, "
                  "guid, guidislink, description, content, title, published, received, "
                  "author_name, author_uri, author_email, category, "
                  "new, read, starred, deleted, "
                  "link_href "
                  ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
            q3.addBindValue(feedId);
            q3.addBindValue(q2.value(0));
            q3.addBindValue(q2.value(1));
            q3.addBindValue(q2.value(2));
            q3.addBindValue(q2.value(3));
            q3.addBindValue(q2.value(4));
            q3.addBindValue(q2.value(5));
            q3.addBindValue(q2.value(6));
            q3.addBindValue(q2.value(7));
            q3.addBindValue(q2.value(8));
            q3.addBindValue(q2.value(9));
            q3.addBindValue(q2.value(10));
            q3.addBindValue(q2.value(11));
            q3.addBindValue(q2.value(12));
            q3.addBindValue(q2.value(13));
            q3.addBindValue(q2.value(14));
            q3.addBindValue(q2.value(15));
            q3.exec();
          }
        }

        // Удаляем старые таблицы новостей
        foreach (int id, idList)
          q.exec(QString("DROP TABLE feed_%1").arg(id));

        // Создаём индекс по полю feedId (для более скоростного поиска)
        q.exec("CREATE INDEX feedId ON news(feedId)");

        // Создаём дополнительную таблицу лент на всякий случай
        q.exec("CREATE TABLE feeds_ex(id integer primary key, "
            "feedId integer, "  // идентификатор ленты
            "name varchar, "    // имя параметра
            "value varchar "    // значение параметра
            ")");
        // Создаём дополнительную таблицу новостей на всякий случай
        q.exec("CREATE TABLE news_ex(id integer primary key, "
            "feedId integer, "  // идентификатор ленты
            "newsId integer, "  // идентификатор новости
            "name varchar, "    // имя параметра
            "value varchar "    // значение параметра
            ")");

        // Создаём таблицы фильтров
        q.exec(kCreateFiltersTable);
        q.exec(kCreateFilterConditionsTable);
        q.exec(kCreateFilterActionsTable);
        // Создаём дополнительную таблицу фильтров на всякий случай
        q.exec("CREATE TABLE filters_ex(id integer primary key, "
            "idFilter integer, "  // идентификатор фильтра
            "name text, "         // имя параметра
            "value text"          // значение параметра
            ")");

        // Обновляем таблицу с информацией
        q.prepare("UPDATE info SET value=:version WHERE name='version'");
        q.bindValue(":version", kDbVersion);
        q.exec();

        qDebug() << "DB converted to version =" << kDbVersion;
      }
      // Версия базы 0.9.0
      else if (dbVersionString == "0.9.0") {
        //-- Преобразуем таблицу feeds
        // Создаём временную таблицу версии 0.10.0
        q.exec(QString(kCreateFeedsTableQuery)
            .replace("TABLE feeds", "TEMP TABLE feeds_temp"));
        // переписываем только используемые ранее поля
        q.exec("INSERT "
            "INTO feeds_temp("
               "id, text, title, description, xmlUrl, htmlUrl, "
               "author_name, author_email, author_uri, pubdate, lastBuildDate, "
               "image, unread, newCount, currentNews, undeleteCount, updated, "
               "displayOnStartup, displayEmbeddedImages "
            ") SELECT "
            "id, text, title, description, xmlUrl, htmlUrl, "
            "author_name, author_email, author_uri, pubdate, lastBuildDate, "
            "image, unread, newCount, currentNews, undeleteCount, updated, "
            "displayOnStartup, displayEmbeddedImages "
            "FROM feeds");
        // Хороним старую таблицу
        q.exec("DROP TABLE feeds");

        // Копируем временную таблицу на место старой
        q.exec(kCreateFeedsTableQuery);
        q.exec("INSERT INTO feeds SELECT * FROM feeds_temp");
        q.exec("DROP TABLE feeds_temp");

        // Модифицируем поле rowToParent таблицы feeds, чтобы все элементы
        // лежали в корне дерева
        q.exec("SELECT id FROM feeds");
        int rowToParent = 0;
        while (q.next()) {
          int id = q.value(0).toInt();
          QSqlQuery q2(db);
          q2.prepare("UPDATE feeds SET rowToParent=:rowToParent WHERE id=:id");
          q2.bindValue(":rowToParent", rowToParent);
          q2.bindValue(":id", id);
          q2.exec();
          rowToParent++;
        }

        //-- Таблицу news не трогаем
        //-- Перестраиваем индекс
        q.exec("DROP INDEX feedId");
        q.exec("CREATE INDEX feedId ON news(feedId)");
        // Дополнительные таблицы лент и новостей уже созданы
        // Заменяем старую пустую таблицу фильтров на новую с новыми полями
        q.exec("DROP TABLE filters");
        q.exec(kCreateFiltersTable);
        // Таблицы условий и действий фильтров не трогаем

        // Создаём дополнительную таблицу фильтров на всякий случай
        q.exec("CREATE TABLE filters_ex(id integer primary key, "
            "idFilter integer, "  // идентификатор фильтра
            "name text, "         // имя параметра
            "value text"          // значение параметра
            ")");

        // Обновляем таблицу с информацией
        q.prepare("UPDATE info SET value=:version WHERE name='version'");
        q.bindValue(":version", kDbVersion);
        q.exec();

        qDebug() << "DB converted to version =" << kDbVersion;
      }
      else {
        qDebug() << "dbVersion =" << dbVersionString;
      }
    }
    // Создаём таблицу меток
    bool createTable = false;
    q.exec("SELECT count(name) FROM sqlite_master WHERE name='labels'");
    if (q.next()) {
      if (q.value(0).toInt()) createTable = true;
    }
    if (!createTable) {
      initLabelsTable(&db);
    }
    //
    db.commit();
    db.exec("VACUUM");
    db.close();
  }
  QSqlDatabase::removeDatabase("dbFileName_");
  return kDbVersion;
}
