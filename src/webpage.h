#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWebPage>
#include <QNetworkAccessManager>

class WebPage : public QWebPage
{
  Q_OBJECT
public:
  explicit WebPage(QWidget *parent = 0);

  bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);

protected slots:
  QWebPage *createWindow(WebWindowType type);

private slots:
  void handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors);

};

#endif // WEBPAGE_H
