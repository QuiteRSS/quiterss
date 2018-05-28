/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2018 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "delegatewithoutfocus.h"

DelegateWithoutFocus::DelegateWithoutFocus(QObject *parent)
  :QStyledItemDelegate(parent)
{
}

void DelegateWithoutFocus::paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
#if QT_VERSION >= 0x050000
  QStyleOptionViewItem opt = option;
#else  
  QStyleOptionViewItemV4 opt = option;
#endif
  opt.state &= ~QStyle::State_HasFocus;
  QStyledItemDelegate::paint(painter,opt,index);
}
