#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWebPage>

class WebPage : public QWebPage
{
  Q_OBJECT
public:
  explicit WebPage(QWidget *parent = 0);

  bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);

protected slots:
  QWebPage *createWindow(WebWindowType type);

};

#endif // WEBPAGE_H
