#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebView>

class WebView : public QWebView
{
  Q_OBJECT
public:
  explicit WebView(QWidget *parent = 0);

  bool midButtonClick;
  bool rightButtonClick;

protected:
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseReleaseEvent(QMouseEvent*);
  virtual void wheelEvent(QWheelEvent*);

private:
  int posX1;

};

#endif // WEBVIEW_H
