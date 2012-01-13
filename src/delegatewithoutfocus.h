#ifndef DELEGATEWITHOUTFOCUS_H
#define DELEGATEWITHOUTFOCUS_H

#include <QStyledItemDelegate>

class DelegateWithoutFocus: public QStyledItemDelegate
{
    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
public:
    DelegateWithoutFocus(QObject *a_parent);
};

#endif // DELEGATEWITHOUTFOCUS_H
