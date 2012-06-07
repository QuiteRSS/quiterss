#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebView>

class WebView : public QWebView
{
  Q_OBJECT
public:
  explicit WebView(QWidget *parent = 0);

  bool midButtonClick;

protected:
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseReleaseEvent(QMouseEvent*);
  virtual void contextMenuEvent(QContextMenuEvent*);

private:
  int posX1;
  bool rightButtonClick;

};

#endif // WEBVIEW_H
