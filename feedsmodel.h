#ifndef FEEDSMODEL_H
#define FEEDSMODEL_H

#include <QSqlTableModel>
#include <QtSql>

class FeedsModel : public QSqlTableModel
{
    Q_OBJECT
public:
    FeedsModel(QObject *parent, QSqlDatabase *db);
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
private:
  QSqlDatabase *db_;

};

#endif  // FEEDSMODEL_H
