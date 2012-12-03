#include <QApplication>
#include <QDateTime>
#include <QDebug>

#include "feedstreemodel.h"

//FeedsTreeModel::FeedsTreeModel(const QString& tableName,
//    const QStringList& captions, const QStringList& fieldNames,
//    int rootParentId, const QString& decoratedField, QObject* parent) :
//    QyurSqlTreeModel(
//        tableName, captions, fieldNames, rootParentId, decoratedField, parent)
FeedsTreeModel::FeedsTreeModel(const QString& tableName,
                               const QStringList& captions, const QStringList& fieldNames,
                               int rootParentId, const QString& decoratedField, QObject* parent) :
    QSortFilterProxyModel(parent)
{
  filterIdsList_ = QList<int>();
  filterUnread_  = false;
  filterNew_     = false;
  filterLabel_   = QString();
  filterFind_    = QString();

  sourceModel_ = new QyurSqlTreeModel(
        tableName, captions, fieldNames, rootParentId, decoratedField, parent);
  setSourceModel(sourceModel_);
}

QVariant FeedsTreeModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::FontRole) {
    QFont font = font_;
    if (proxyColumnByOriginal("text") == index.column()) {
//      if ((0 < index.sibling(index.row(), proxyColumnByOriginal("unread")).data(Qt::EditRole).toInt()) &&
      if ((0 < dataField(index, "unread").toInt()) &&
          (proxyColumnByOriginal("unread") != index.column()))
        font.setBold(true);
    }
    return font;
  } else if (role == Qt::DisplayRole){
    if (proxyColumnByOriginal("unread") == index.column()) {
//      int unread = index.sibling(index.row(), proxyColumnByOriginal("unread")).data(Qt::EditRole).toInt();
      int unread = dataField(index, "unread").toInt();
      if (0 == unread) {
        return QVariant();
      } else {
        QString qStr = QString("(%1)").arg(unread);
        return qStr;
      }
    } else if (proxyColumnByOriginal("undeleteCount") == index.column()) {
      QString qStr = QString("(%1)").
//          arg(index.sibling(index.row(), proxyColumnByOriginal("undeleteCount")).data(Qt::EditRole).toInt());
          arg(dataField(index, "undeleteCount").toInt());
      return qStr;
    } else if (proxyColumnByOriginal("updated") == index.column()) {
      QDateTime dtLocal;
//      QString strDate = index.sibling(index.row(), proxyColumnByOriginal("updated")).data(Qt::EditRole).toString();
      QString strDate = dataField(index, "updated").toString();

      if (!strDate.isNull()) {
        QDateTime dtLocalTime = QDateTime::currentDateTime();
        QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
        int nTimeShift = dtLocalTime.secsTo(dtUTC);

        QDateTime dt = QDateTime::fromString(strDate, Qt::ISODate);
        dtLocal = dt.addSecs(nTimeShift);

        QString strResult;
        if (QDateTime::currentDateTime().date() == dtLocal.date())
          strResult = dtLocal.toString("hh:mm");
        else
          strResult = dtLocal.toString(formatDateTime_.left(formatDateTime_.length()-6));
        return strResult;
      } else {
        return QVariant();
      }
    }
  } else if (role == Qt::TextColorRole) {
    QBrush brush;
    brush = qApp->palette().brush(QPalette::WindowText);
    if (proxyColumnByOriginal("unread") == index.column()) {
      brush = qApp->palette().brush(QPalette::Link);
    } else if (proxyColumnByOriginal("text") == index.column()) {
//      if (index.sibling(index.row(), proxyColumnByOriginal("newCount")).data(Qt::EditRole).toInt() > 0) {
      if (0 < dataField(index, "newCount").toInt()) {
        brush = qApp->palette().brush(QPalette::Link);
      }
    }
    return brush;
  } else if (role == Qt::DecorationRole) {
    if (proxyColumnByOriginal("text") == index.column()) {
//      QByteArray byteArray = index.sibling(index.row(), proxyColumnByOriginal("image")).data(Qt::EditRole).toByteArray();
      QByteArray byteArray = dataField(index, "image").toByteArray();
      if (!byteArray.isNull()) {
        QPixmap icon;
        if (icon.loadFromData(QByteArray::fromBase64(byteArray))) {
          return icon;
        }
      }
      if (isFolder(index))
        return QPixmap(":/images/folder");
      else
        return QPixmap(":/images/feed");
    }
  } else if (role == Qt::TextAlignmentRole) {
    if (proxyColumnByOriginal("id") == index.column()) {
      int flag = Qt::AlignRight|Qt::AlignVCenter;
      return flag;
    }
  }

  return sourceModel()->data(mapToSource(index), role);
}

/**
 * @brief Получение данных из поля, находящегося в той же строке что и index
 * @param index индекс, рядом с которым запрашиваем поле
 * @param fieldName название поля
 * @return значение поля
 *----------------------------------------------------------------------------*/
QVariant FeedsTreeModel::dataField(const QModelIndex &index, const QString &fieldName) const
{
  QModelIndex indexSource = mapToSource(index);
  return indexSource.sibling(
      indexSource.row(), proxyColumnByOriginal(fieldName))
      .data(Qt::EditRole);
}

/*virtual*/ bool	FeedsTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  return sourceModel()->setData(mapToSource(index), value, role);
}

/**
 * @brief Запись данных напрямую в исходную модель
 * @param feedId идентификатор ленты
 * @param feedParId идентификатор родителя ленты
 * @param fieldName изменяемое поле
 * @param value записываемое значение
 * @param role роль записываемого значения
 * @return результат записи
 *----------------------------------------------------------------------------*/
bool FeedsTreeModel::setSourceData(int feedId, int feedParId, const QString &fieldName, const QVariant &value, int role)
{
  QModelIndex indexSource = sourceModel_->getIndexById(feedId, feedParId);
  return sourceModel_->setData(
      indexSource.sibling(indexSource.row(), proxyColumnByOriginal(fieldName)),
      value, role);
}

Qt::ItemFlags FeedsTreeModel::flags(const QModelIndex &index) const
{
  Qt::ItemFlags defaultFlags = sourceModel()->flags(mapToSource(index));

  if (index.isValid())
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
  else
    return Qt::ItemIsDropEnabled | defaultFlags;
}

Qt::DropActions FeedsTreeModel::supportedDropActions() const
{
  return Qt::MoveAction;
}

/**
 * @brief Проверка, что узел является категорией, а не лентой
 *
 *  Если поле xmlUrl пустое, то узел считается категорией
 * @param index Проверяемый узел
 * @return Признал ленты
 * @retval true Узел - категория
 * @retval false Узел - лента
 *----------------------------------------------------------------------------*/
bool FeedsTreeModel::isFolder(const QModelIndex &index) const
{
  return dataField(index, "xmlUrl").toString().isEmpty();
}

/**
 * @brief Проверка, что узел является категорией, а не лентой
 *
 *  Если поле xmlUrl пустое, то узел считается категорией
 * @param feedId идентификатор ленты
 * @param feedParId идентификатор родителя ленты
 * @return Признал ленты
 * @retval true Узел - категория
 * @retval false Узел - лента
 *----------------------------------------------------------------------------*/
bool FeedsTreeModel::isSourceFolder(int feedId, int feedParId) const
{
  QModelIndex indexSource = sourceModel_->getIndexById(feedId, feedParId);
  return sourceModel_->data(
      indexSource.sibling(indexSource.row(), proxyColumnByOriginal("xmlUrl")))
      .toString().isEmpty();
}

/**
 * @brief Получение идентификатора ленты
 * @param index Индекс ленты
 * @return Идентификатор ленты
 *----------------------------------------------------------------------------*/
int FeedsTreeModel::getIdByIndex(const QModelIndex &index) const
{
  return sourceModel_->getIdByIndex(mapToSource(index));
}

/**
 * @brief Получение идентификатора ленты индекса исходной модели
 * @param indexSource Индекс ленты исходной модели
 * @return Идентификатор ленты
 *----------------------------------------------------------------------------*/
int FeedsTreeModel::getIdBySourceIndex(const QModelIndex &indexSource) const
{
  return sourceModel_->getIdByIndex(indexSource);
}

/**
 * @brief Получение идентификатора родителя ленты
 * @param index Индекс ленты
 * @return Идентификатор родителя
 *----------------------------------------------------------------------------*/
int FeedsTreeModel::getParidByIndex(const QModelIndex &index) const
{
  return sourceModel_->getParidByIndex(mapToSource(index));
}

/**
 * @brief Получение идентификатора родителя ленты индекса исходной модели
 * @param index Индекс ленты исходной модели
 * @return Идентификатор родителя
 *----------------------------------------------------------------------------*/
int FeedsTreeModel::getParidBySourceIndex(const QModelIndex &index) const
{
  return sourceModel_->getParidByIndex(index);
}

/**
 * @brief Получение индекса модели по идентификатору ленты и идентификатору родителя ленты
 * @param id Идентификатор ленты
 * @param parentId Идентификатор родителя ленты
 * @return Найденный индекс
 *----------------------------------------------------------------------------*/
QModelIndex FeedsTreeModel::getIndexById(int id, int parentId) const
{
  return mapFromSource(sourceModel_->getIndexById(id, parentId));
}

/**
 * @brief Получение индекса исходной модели по идентификатору ленты и идентификатору родителя ленты
 * @param id Идентификатор ленты
 * @param parentId Идентификатор родителя ленты
 * @return Найденный индекс
 *----------------------------------------------------------------------------*/
QModelIndex FeedsTreeModel::getSourceIndexById(int id, int parentId) const
{
  return sourceModel_->getIndexById(id, parentId);
}

/**
 * @brief Получение номера колонки исходной модели по номеру прокси модели
 * @param field Название поля прокси модели
 * @return Номер колонки исходной модели
 *----------------------------------------------------------------------------*/
int FeedsTreeModel::proxyColumnByOriginal(const QString &field) const
{
  return sourceModel_->proxyColumnByOriginal(field);
}

/**
 * @brief Установка фильтра отображения исходной модели
 * @param filter Строка, содержащая фильтр
 *----------------------------------------------------------------------------*/
//void FeedsTreeModel::setFilter(const QString &filter)
//{
//  sourceModel_->setFilter(filter);
//}

/**
 * @brief Установка параметров фильтра отображения исходной модели
 * @param feedIdsList Список идентификаторов лент, которые должны отображаться
 * @param isUnread    Отображать ленты, имеющие непрочтеные сообщения
 * @param isNewCount  Отображать ленты, имеющие новые сообщения
 * @param labelString Лента будет отображена, если значение переменной
 *  содержится в поле label
 * @param findString  Строка из "поля поиска"
 *----------------------------------------------------------------------------*/
void FeedsTreeModel::setFilter(const QList<int> &feedIdsList,
    bool isUnread, bool isNew,
    const QString &labelString, const QString &findString)
{
  filterIdsList_ = feedIdsList;
  filterUnread_  = isUnread;
  filterNew_     = isNew;
  filterLabel_   = labelString;
  filterFind_    = findString;
  qDebug() << "FeedsTreeModel::setFilter()"
      << filterIdsList_ << filterUnread_ << filterNew_ << filterLabel_ << filterFind_;
  invalidateFilter();
}

/**
 * @brief Фильтрация строк исходной модели
 * @param sourceRow     Номер строки
 * @param sourceParent  Индеск родителя
 * @retval true   Если ряд необходимо отображать
 * @retval false  Если ряд необходимо скрыть
 *----------------------------------------------------------------------------*/
bool FeedsTreeModel::filterAcceptsRow(int sourceRow,
                                      const QModelIndex &sourceParent) const
{
  QModelIndex indexId = sourceModel()->index(sourceRow, proxyColumnByOriginal("id"), sourceParent);
  int feedId = sourceModel()->data(indexId).toInt();
  QModelIndex indexUnread = sourceModel()->index(sourceRow, proxyColumnByOriginal("unread"), sourceParent);
  int feedUnread = sourceModel()->data(indexUnread).toInt();
  QModelIndex indexNew = sourceModel()->index(sourceRow, proxyColumnByOriginal("newCount"), sourceParent);
  int feedNew = sourceModel()->data(indexNew).toInt();

  bool rowIncludedId = true;
  if (!filterIdsList_.isEmpty()) rowIncludedId = filterIdsList_.contains(feedId);

  bool rowIncludedUnread = true;
  if (filterUnread_) rowIncludedUnread = (0 < feedUnread);

  bool rowIncludedNew = true;
  if (filterNew_) rowIncludedNew = (0 < feedNew);

  return rowIncludedId || (rowIncludedUnread && rowIncludedNew);
}

/**
 * @brief Обновление исходной модели
 *----------------------------------------------------------------------------*/
void FeedsTreeModel::refresh()
{
  sourceModel_->refresh();
}
