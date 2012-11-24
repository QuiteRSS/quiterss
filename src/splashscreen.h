#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QtGui>

class SplashScreen : public QSplashScreen
{
  Q_OBJECT
private:
  QProgressBar splashProgress;

private slots:
  void slotSplashTimeOut();

public:
  explicit SplashScreen(const QPixmap & pixmap = QPixmap(), Qt::WindowFlags f = 0);

  void loadModules();
  QTimer *splashTimer_;

public slots:


signals:


};

#endif // SPLASHSCREEN_H
