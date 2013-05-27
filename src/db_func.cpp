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
/*! \file db_func.cpp *********************************************************
 * Функции для работы с файлом базы
 * \date created 17.04.2012
 ******************************************************************************/
#include <QtCore>
#include <QtSql>

QString kDbName    = "feeds.db";
QString kDbVersion = "0.12.1";

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
    "flags text, "                    // другие флаги перечисляемые текстом (могут быть, например "focused", "hidden")
    "authentication integer default 0, "    // включение авторизации, устанавливается при добавлении ленты
    "duplicateNewsMode integer default 0, " // режим работы с дубликатами новостей
    "typeFeed integer default 0 "           // на будущее

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
    "rights varchar, "                     // права
    "deleteDate varchar, "                 // дата удаления новости
    "feedParentId integer default 0 "      // идентификатор родителя ленты из таблицы feeds
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
    "num integer, "             // номер по порядку, для сортировки
    "currentNews integer "      // отображаемая новость
    ")");

const QString kCreatePasswordsTable(
    "CREATE TABLE passwords("
    "id integer primary key, "
    "server varchar, "          // сервер
    "username varchar, "        // пользователь
    "password varchar "         // пароль
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

/** Создание резервной копии файла базы
 *
 *  Формат резервного файла:
 *  <старое-имя-файла>.backup_<версия-базы>_<время-воздания-резервного-файла>
 * @param dbFileName абсолютный путь и имя файла базы
 * @param dbVersion версия базы
 *----------------------------------------------------------------------------*/
void createDBBackup(const QString &dbFileName, const QString &dbVersion)
{
  QFileInfo fi(dbFileName);

  // Создаём backup директорию в директории файла базы
  QDir backupDir(fi.absoluteDir());
  if (!backupDir.exists("backup"))
    backupDir.mkpath("backup");
  backupDir.cd("backup");

  // Создаём архивную копию файла
  QString dbBackupName(backupDir.absolutePath() + '/' + fi.fileName());
  dbBackupName.append(QString("[%1_%2].old")
          .arg(dbVersion)
          .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")));
  QFile::copy(dbFileName, dbBackupName);
}


//-----------------------------------------------------------------------------
QString initDB(const QString &dbFileName, QSettings *settings)
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
    // Создаём таблицу паролей
    db.exec(kCreatePasswordsTable);
    //
    db.exec("CREATE TABLE info(id integer primary key, name varchar, value varchar)");
    QSqlQuery q(db);
    q.prepare("INSERT INTO info(name, value) VALUES ('version', :version)");
    q.bindValue(":version", kDbVersion);
    q.exec();
    q.exec("INSERT OR REPLACE INTO info(name, value) VALUES ('rowToParentCorrected_0.12.3', 'true')");
    q.finish();
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
      // Версия базы 0.10.0
      else if (dbVersionString == "0.10.0") {
        // Добавление полей для таблицы лент
        q.exec("ALTER TABLE feeds ADD COLUMN authentication integer default 0");
        q.exec("ALTER TABLE feeds ADD COLUMN duplicateNewsMode integer default 0");
        q.exec("ALTER TABLE feeds ADD COLUMN typeFeed integer default 0");

        // Добавление полей для таблицы новостей
        q.exec("ALTER TABLE news ADD COLUMN deleteDate varchar");
        q.exec("ALTER TABLE news ADD COLUMN feedParentId integer default 0");

        // Создаём таблицу меток
        bool createTable = false;
        q.exec("SELECT count(name) FROM sqlite_master WHERE name='labels'");
        if (q.next()) {
          if (q.value(0).toInt() > 0) createTable = true;
        }
        if (!createTable) {
          initLabelsTable(&db);
        } else {
          q.exec("SELECT count(currentNews) FROM labels");
          if (!q.next()) {
            q.exec("ALTER TABLE labels ADD COLUMN currentNews integer");
          }
        }
        // Создаём таблицу паролей
        createTable = false;
        q.exec("SELECT count(name) FROM sqlite_master WHERE name='passwords'");
        if (q.next()) {
          if (q.value(0).toInt() > 0) createTable = true;
        }
        if (!createTable)
          q.exec(kCreatePasswordsTable);

        // Обновляем таблицу с информацией
        q.prepare("UPDATE info SET value=:version WHERE name='version'");
        q.bindValue(":version", kDbVersion);
        q.exec();

        qDebug() << "DB converted to version =" << kDbVersion;
      }
      // Версия базы последняя
      else {

        bool rowToParentCorrected = false;
        q.exec("SELECT value FROM info WHERE name='rowToParentCorrected_0.12.3'");
        if (q.next() && q.value(0).toString()=="true")
          rowToParentCorrected = true;

        if (rowToParentCorrected) {
          qDebug() << "dbVersion =" << dbVersionString;
        }
        else {
          qDebug() << "dbversion =" << dbVersionString << ": rowToParentCorrected_0.12.3 = true";

          createDBBackup(dbFileName, dbVersionString);

          q.exec("INSERT OR REPLACE INTO info(name, value) VALUES ('rowToParentCorrected_0.12.3', 'true')");

          // Начинаем поиск детей с потенциального родителя 0 (с корня)
          bool sortFeeds = settings->value("Settings/sortFeeds", false).toBool();
          QString sortStr("id");
          if (sortFeeds) sortStr = "text";

          QList<int> parentIdsPotential;
          parentIdsPotential << 0;
          while (!parentIdsPotential.empty()) {
            int parentId = parentIdsPotential.takeFirst();

            // Ищем детей родителя parentId
            q.prepare(QString("SELECT id FROM feeds WHERE parentId=? ORDER BY %1").
                      arg(sortStr));
            q.addBindValue(parentId);
            q.exec();

            // Каждому ребенку прописываем его rowToParent
            // ... сохраняем его в списке потенциальных родителей
            int rowToParent = 0;
            while (q.next()) {
              int parentIdNew = q.value(0).toInt();

              QSqlQuery q2(db);
              q2.prepare("UPDATE feeds SET rowToParent=? WHERE id=?");
              q2.addBindValue(rowToParent);
              q2.addBindValue(parentIdNew);
              q2.exec();

              parentIdsPotential << parentIdNew;
              ++rowToParent;
            }
          }
        }   // if (rowToParentCorrected) {
      } // Версия базы последняя
    }
    q.finish();

    db.commit();
    db.close();
  }
  QSqlDatabase::removeDatabase("dbFileName_");
  return kDbVersion;
}

//-----------------------------------------------------------------------------
void setUserFilter(int feedId, int filterId)
{
  QSqlQuery q;
  bool onlyNew = true;

  if (filterId != -1) {
    onlyNew = false;
    q.exec(QString("SELECT enable, type FROM filters WHERE id='%1' AND feeds LIKE '%,%2,%'").
           arg(filterId).arg(feedId));
  } else {
    q.exec(QString("SELECT enable, type, id FROM filters WHERE feeds LIKE '%,%1,%' ORDER BY num").
           arg(feedId));
  }

  while (q.next()) {
    if (q.value(0).toInt() == 0) continue;

    if (onlyNew)
      filterId = q.value(2).toInt();
    int filterType = q.value(1).toInt();

    QString qStr("UPDATE news SET");
    QString qStr1;
    QString qStr2;

    QSqlQuery q1;
    q1.exec(QString("SELECT action, params FROM filterActions "
                    "WHERE idFilter=='%1'").arg(filterId));
    while (q1.next()) {
      switch (q1.value(0).toInt()) {
      case 0: // action -> Mark news as read
        if (!qStr1.isNull()) qStr1.append(",");
        qStr1.append(" new=0, read=2");
        break;
      case 1: // action -> Add star
        if (!qStr1.isNull()) qStr1.append(",");
        qStr1.append(" starred=1");
        break;
      case 2: // action -> Delete
        if (!qStr1.isNull()) qStr1.append(",");
        qStr1.append(" new=0, read=2, deleted=1, ");
        qStr1.append(QString("deleteDate='%1'").
            arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
        break;
      case 3: // action -> Add Label
        qStr2.append(QString("%1,").arg(q1.value(1).toInt()));
        break;
      }
    }

    if (!qStr2.isEmpty()) {
      if (!qStr1.isNull()) qStr1.append(",");
      qStr1.append(QString(" label=',%1'").arg(qStr2));
    }
    qStr.append(qStr1);
    qStr.append(QString(" WHERE feedId='%1' AND deleted=0").arg(feedId));

    if (onlyNew) qStr.append(" AND new=1");

    qStr2.clear();
    switch (filterType) {
    case 1: // Match all conditions
      qStr2.append("AND ");
      break;
    case 2: // Match any condition
      qStr2.append("OR ");
      break;
    }

    if ((filterType == 1) || (filterType == 2)) {
      qStr.append(" AND ( ");
      qStr1.clear();

      q1.exec(QString("SELECT field, condition, content FROM filterConditions "
                      "WHERE idFilter=='%1'").arg(filterId));
      while (q1.next()) {
        if (!qStr1.isNull()) qStr1.append(qStr2);
        switch (q1.value(0).toInt()) {
        case 0: // field -> Title
          switch (q1.value(1).toInt()) {
          case 0: // condition -> contains
            qStr1.append(QString("title LIKE '%%1%' ").arg(q1.value(2).toString()));
            break;
          case 1: // condition -> doesn't contains
            qStr1.append(QString("title NOT LIKE '%%1%' ").arg(q1.value(2).toString()));
            break;
          case 2: // condition -> is
            qStr1.append(QString("title LIKE '%1' ").arg(q1.value(2).toString()));
            break;
          case 3: // condition -> isn't
            qStr1.append(QString("title NOT LIKE '%1' ").arg(q1.value(2).toString()));
            break;
          case 4: // condition -> begins with
            qStr1.append(QString("title LIKE '%1%' ").arg(q1.value(2).toString()));
            break;
          case 5: // condition -> ends with
            qStr1.append(QString("title LIKE '%%1' ").arg(q1.value(2).toString()));
            break;
          }
          break;
        case 1: // field -> Description
          switch (q1.value(1).toInt()) {
          case 0: // condition -> contains
            qStr1.append(QString("description LIKE '%%1%' ").arg(q1.value(2).toString()));
            break;
          case 1: // condition -> doesn't contains
            qStr1.append(QString("description NOT LIKE '%%1%' ").arg(q1.value(2).toString()));
            break;
          }
          break;
        case 2: // field -> Author
          switch (q1.value(1).toInt()) {
          case 0: // condition -> contains
            qStr1.append(QString("author_name LIKE '%%1%' ").arg(q1.value(2).toString()));
            break;
          case 1: // condition -> doesn't contains
            qStr1.append(QString("author_name NOT LIKE '%%1%' ").arg(q1.value(2).toString()));
            break;
          case 2: // condition -> is
            qStr1.append(QString("author_name LIKE '%1' ").arg(q1.value(2).toString()));
            break;
          case 3: // condition -> isn't
            qStr1.append(QString("author_name NOT LIKE '%1' ").arg(q1.value(2).toString()));
            break;
          }
          break;
        case 3: // field -> Category
          switch (q1.value(1).toInt()) {
          case 0: // condition -> is
            qStr1.append(QString("category LIKE '%1' ").arg(q1.value(2).toString()));
            break;
          case 1: // condition -> isn't
            qStr1.append(QString("category NOT LIKE '%1' ").arg(q1.value(2).toString()));
            break;
          case 2: // condition -> begins with
            qStr1.append(QString("category LIKE '%1%' ").arg(q1.value(2).toString()));
            break;
          case 3: // condition -> ends with
            qStr1.append(QString("category LIKE '%%1' ").arg(q1.value(2).toString()));
            break;
          }
          break;
        case 4: // field -> Status
          if (q1.value(1).toInt() == 0) { // Status -> is
            switch (q1.value(2).toInt()) {
            case 0:
              qStr1.append("new==1 ");
              break;
            case 1:
              qStr1.append("read>=1 ");
              break;
            case 2:
              qStr1.append("starred==1 ");
              break;
            }
          } else { // Status -> isn't
            switch (q1.value(2).toInt()) {
            case 0:
              qStr1.append("new==0 ");
              break;
            case 1:
              qStr1.append("read==0 ");
              break;
            case 2:
              qStr1.append("starred==0 ");
              break;
            }
          }
          break;
        }
      }
      qStr.append(qStr1).append(")");
    }
    q1.exec(qStr);
//    qCritical() << qStr;
  }
}
