#ifndef FEEDSMODEL_H
#define FEEDSMODEL_H

#include <QSqlTableModel>

class FeedsModel : public QSqlTableModel
{
    Q_OBJECT
public:
    FeedsModel(QObject *parent);
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
};

#endif  // FEEDSMODEL_H
