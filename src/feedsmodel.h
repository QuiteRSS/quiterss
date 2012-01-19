#ifndef FEEDSMODEL_H
#define FEEDSMODEL_H

#include <QSqlTableModel>
#include <QFont>

class FeedsModel : public QSqlTableModel
{
    Q_OBJECT
public:
    FeedsModel(QObject *parent);
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    QFont font_;
};

#endif  // FEEDSMODEL_H
