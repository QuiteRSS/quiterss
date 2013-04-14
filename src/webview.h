#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebView>

#define LEFT_BUTTON 0
#define MIDDLE_BUTTON 1
#define MIDDLE_BUTTON_MOD 2
#define LEFT_BUTTON_CTRL 3
#define LEFT_BUTTON_SHIFT 4
#define LEFT_BUTTON_ALT 5

class WebView : public QWebView
{
  Q_OBJECT
public:
  explicit WebView(QWidget *parent, QNetworkAccessManager *networkManager);

  int buttonClick_;
  bool rightButtonClick_;

protected:
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseReleaseEvent(QMouseEvent*);
  virtual void wheelEvent(QWheelEvent*);

private:
  int posX1;

};

#endif // WEBVIEW_H
