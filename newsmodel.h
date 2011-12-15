#ifndef NEWSMODEL_H
#define NEWSMODEL_H

#include <QSqlTableModel>

#define dataFormat "yyyy.MM.dd hh:mm:ss"

class NewsModel : public QSqlTableModel
{
    Q_OBJECT
public:
    NewsModel(QObject *parent);
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
};

#endif // NEWSMODEL_H
