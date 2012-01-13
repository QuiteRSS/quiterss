#include "delegatewithoutfocus.h"

DelegateWithoutFocus::DelegateWithoutFocus(QObject *a_parent)
    :QStyledItemDelegate(a_parent)
{
}

void DelegateWithoutFocus::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QStyleOptionViewItemV4 opt = option;
    opt.state &= ~QStyle::State_HasFocus;
    QStyledItemDelegate::paint(painter,opt,index);
}
