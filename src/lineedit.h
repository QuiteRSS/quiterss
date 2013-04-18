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

protected:
  void resizeEvent(QResizeEvent *);
  void focusInEvent(QFocusEvent *event);
  void focusOutEvent(QFocusEvent *event);

private slots:
  void updateClearButton(const QString &text);
  void slotClear();

private:
  QToolButton *clearButton;

signals:
  void signalClear();
};

#endif // LIENEDIT_H

