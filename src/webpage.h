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

protected slots:
  QWebPage *createWindow(WebWindowType type);

};

#endif // WEBPAGE_H
