#ifndef TOOLBUTTON_H
#define TOOLBUTTON_H

#include <QPushButton>
#include <QToolButton>

#ifdef Q_OS_MAC
class ToolButton : public QPushButton
{
  Q_OBJECT
public:
  explicit ToolButton(QWidget *parent = 0);

  void setIconSize(const QSize &size);

  void setAutoRaise(bool enable);
  bool autoRaise() const;

private:
  bool autoRise_;
  QSize buttonFixedSize_;

};
#else
class ToolButton : public QToolButton
{
  Q_OBJECT
public:
  explicit ToolButton(QWidget *parent = 0);

};
#endif

#endif // TOOLBUTTON_H
