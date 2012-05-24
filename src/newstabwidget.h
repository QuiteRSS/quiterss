#ifndef NEWSTABWIDGET_H
#define NEWSTABWIDGET_H

#include <QtGui>
#include <QtSql>
#include <QtWebKit>

#include "newsheader.h"
#include "newsmodel.h"
#include "newsview.h"
#include "webview.h"

class NewsTabWidget : public QWidget
{
  Q_OBJECT
private:
  void createNewsList();
  void createWebWidget();
  void readSettings();

  QSettings *settings_;
  QWidget *newsWidget_;

public:
  explicit NewsTabWidget(QSettings *settings, int feedId, QWidget *parent = 0);
  void retranslateStrings();

  int feedId_;

  NewsModel *newsModel_;
  NewsView *newsView_;
  NewsHeader *newsHeader_;
  QToolBar *newsToolBar_;

  QLabel *webPanelTitle_;
  QLabel *webPanelTitleLabel_;
  QLabel *webPanelAuthorLabel_;
  QLabel *webPanelAuthor_;
  QWidget *webPanel_;
  QWidget *webControlPanel_;

  QToolBar *webToolBar_;
  QWidget *webWidget_;
  WebView *webView_;
  QProgressBar *webViewProgress_;
  QLabel *webViewProgressLabel_;

signals:

public slots:

private slots:
//  void slotLinkClicked(QUrl url);
//  void slotSetValue(int value);

};

#endif // NEWSTABWIDGET_H
