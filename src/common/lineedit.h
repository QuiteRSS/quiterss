/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2017 QuiteRSS Team <quiterssteam@gmail.com>
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
/****************************************************************************
**
** Copyright (c) 2007 Trolltech ASA <info@trolltech.com>
**
** Use, modification and distribution is allowed without limitation,
** warranty, liability or support of any kind.
**
****************************************************************************/
#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QLineEdit>
#include <QLabel>

class QToolButton;

class LineEdit : public QLineEdit
{
  Q_OBJECT
public:
  LineEdit(QWidget *parent = 0, const QString &text = "");
  QLabel *textLabel_;

signals:
  void signalClear();

protected:
  void resizeEvent(QResizeEvent *);
  void focusInEvent(QFocusEvent *event);
  void focusOutEvent(QFocusEvent *event);

private slots:
  void updateClearButton(const QString &text);
  void slotClear();

private:
  QToolButton *clearButton;

};

#endif // LIENEDIT_H

