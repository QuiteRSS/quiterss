/* =============================================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2013 QuiteRSS Team <quiterssteam@gmail.com>
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* =========================================================================== */
#include "locationbar.h"

#include <QEvent>

LocationBar::LocationBar(QWidget *parent)
  : QLineEdit(parent)
  , focus_(false)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setStyleSheet("QLineEdit {margin-bottom: 1px; padding: 0px 3px 0px 3px;}");
}

void LocationBar::mouseReleaseEvent(QMouseEvent *event)
{
  if (focus_) {
    selectAll();
    focus_ = false;
  }
  QLineEdit::mouseReleaseEvent(event);
}

void LocationBar::focusInEvent(QFocusEvent *event)
{
  focus_ = true;
  QLineEdit::focusInEvent(event);
}
