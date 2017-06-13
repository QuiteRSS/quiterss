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
#ifndef FINDFEED_H
#define FINDFEED_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif

class FindFeed : public QLineEdit
{
  Q_OBJECT
public:
  FindFeed(QWidget *parent = 0);
  void retranslateStrings();

  QActionGroup *findGroup_;

signals:
  void signalClear();
  void signalSelectFind();

protected:
  void resizeEvent(QResizeEvent *);
  void focusInEvent(QFocusEvent *event);
  void focusOutEvent(QFocusEvent *event);

private slots:
  void updateClearButton(const QString &text);
  void slotClear();
  void slotMenuFind();
  void slotSelectFind(QAction *act);

private:
  QMenu *findMenu_;
  QAction *findNameAct_;
  QAction *findLinkAct_;

  QLabel *findLabel_;
  QToolButton *findButton_;
  QToolButton *clearButton_;

};

#endif // FINDFEED_H
