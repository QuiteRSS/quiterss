/*! \file db_func.h ***********************************************************/

#ifndef DB_FUNC_H
#define DB_FUNC_H

#include <QtCore>
#include <QtSql>

extern QString kDbName;
extern QString kDbVersion;

/*! Инициализация файла базы. Если базы нет то база создаётся по шаблону.
 * Если база старой версии, то она преобразовывается к актуальной версии
 * \param[in] dbFileName путь файла базы
 ******************************************************************************/
QString initDB(const QString dbFileName);

/** @brief Применение пользовательских фильтров
 * @param db - база данных
 * @param feedId - Id ленты
 * @param filterId - Id конкретного фильтра
 *---------------------------------------------------------------------------*/
void setUserFilter(int feedId, int filterId = -1);

#endif
