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
#ifndef FINDTEXT_H
#define FINDTEXT_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif

class FindTextContent : public QLineEdit
{
  Q_OBJECT
public:
  FindTextContent(QWidget *parent = 0);
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
  QAction *findInNewsAct_;
  QAction *findTitleAct_;
  QAction *findAuthorAct_;
  QAction *findCategoryAct_;
  QAction *findContentAct_;
  QAction *findLinkAct_;
  QAction *findInBrowserAct_;

  QLabel *findLabel_;
  QToolButton *findButton_;
  QToolButton *clearButton_;

};

#endif // FINDTEXT_H
