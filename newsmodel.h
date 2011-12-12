#ifndef NEWSMODEL_H
#define NEWSMODEL_H

#include <QSqlTableModel>

class NewsModel : public QSqlTableModel
{
    Q_OBJECT
public:
    NewsModel(QObject *parent);
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
};

#endif // NEWSMODEL_H
