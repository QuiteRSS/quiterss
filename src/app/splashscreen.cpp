/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2014 QuiteRSS Team <quiterssteam@gmail.com>
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
* ============================================================ */
#include "splashscreen.h"

SplashScreen::SplashScreen(const QPixmap &pixmap, Qt::WindowFlags flag)
  : QSplashScreen(pixmap, flag)
{
  setFixedSize(pixmap.width(), pixmap.height());
  setContentsMargins(5, 0, 5, 0);
  setEnabled(false);
  showMessage("Prepare loading...", Qt::AlignRight | Qt::AlignTop, Qt::darkGray);
  setAttribute(Qt::WA_DeleteOnClose);

  splashProgress_.setObjectName("splashProgress");
  splashProgress_.setTextVisible(false);
  splashProgress_.setFixedHeight(10);
  splashProgress_.setMaximum(0);
  splashProgress_.hide();

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addStretch(1);
  layout->addWidget(&splashProgress_);
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
  splashProgress_.show();
  splashProgress_.setMaximum(100);
  for (int i = 0; i < 100; ) {
    if (time.elapsed() >= 1) {
      time.start();
      ++i;
      qApp->processEvents();
      splashProgress_.setValue(i);
      showMessage("Loading: " + QString::number(i) + "%",
                           Qt::AlignRight | Qt::AlignTop, Qt::darkGray);
    }
  }
}

void SplashScreen::slotSplashTimeOut()
{
  splashProgress_.setValue(0);
}
