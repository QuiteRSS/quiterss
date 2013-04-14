#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWebPage>
#include <QNetworkAccessManager>

class WebPage : public QWebPage
{
  Q_OBJECT
public:
  explicit WebPage(QObject *parent, QNetworkAccessManager *networkManager);

  bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);
  void scheduleAdjustPage();

  bool isLoading_;
  bool adjustingScheduled_;

protected slots:
  QWebPage *createWindow(WebWindowType type);
  void slotLoadStarted();
  void slotLoadFinished();

};

#endif // WEBPAGE_H
