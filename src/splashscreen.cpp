#include "splashscreen.h"

SplashScreen::SplashScreen(const QPixmap &pixmap, Qt::WindowFlags f)
  : QSplashScreen(pixmap, f)
{
  setFixedSize(pixmap.width(), pixmap.height());
  setContentsMargins(5, 0, 5, 0);
  setEnabled(false);
  showMessage("Prepare loading...", Qt::AlignRight | Qt::AlignTop, Qt::darkGray);
  setAttribute(Qt::WA_DeleteOnClose);

  splashProgress.setObjectName("splashProgress");
  splashProgress.setTextVisible(false);
  splashProgress.setFixedHeight(10);
  splashProgress.setMaximum(0);
  splashProgress.hide();

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addStretch(1);
  layout->addWidget(&splashProgress);
  setLayout(layout);

  splashTimer_ = new QTimer(this);
  connect(splashTimer_, SIGNAL(timeout()), this, SLOT(slotSplashTimeOut()));
//  splashTimer_->start(10);
}

void SplashScreen::loadModules()
{
  QElapsedTimer time;
  time.start();
  splashTimer_->stop();
  splashProgress.show();
  splashProgress.setMaximum(100);
  for (int i = 0; i < 100; ) {
    if (time.elapsed() >= 1) {
      time.start();
      ++i;
      qApp->processEvents();
      splashProgress.setValue(i);
      showMessage("Loading: " + QString::number(i) + "%",
                           Qt::AlignRight | Qt::AlignTop, Qt::darkGray);
    }
  }
}

void SplashScreen::slotSplashTimeOut()
{
  splashProgress.setValue(0);
}
