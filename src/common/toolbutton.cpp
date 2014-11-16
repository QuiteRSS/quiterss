#include "toolbutton.h"

#ifdef Q_OS_MAC
ToolButton::ToolButton(QWidget *parent)
  : QPushButton(parent)
  , autoRise_(false)
  , buttonFixedSize_(18, 18)
{
}

void ToolButton::setIconSize(const QSize &size)
{
  QPushButton::setIconSize(size);
  buttonFixedSize_ = QSize(size.width() + 2, size.height() + 2);
}

void ToolButton::setAutoRaise(bool enable)
{
  autoRise_ = enable;
  setFlat(enable);
  if (enable) {
    setFixedSize(buttonFixedSize_);
  }
}

bool ToolButton::autoRaise() const
{
  return autoRise_;
}
#else
ToolButton::ToolButton(QWidget *parent)
  : QToolButton(parent)
{
}
#endif
