#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <QtGui>
#include <QtSql>

class NewsItem : public QWidget
{
  Q_OBJECT
public:
  NewsItem(QWidget * parent = 0) : QWidget(parent)
  {
    iconNews = new QLabel(this);
    titleNews = new QLabel(this);

    readButton = new QToolButton(this);
    readButton->setIcon(QIcon(":/images/bulletUnread"));
    readButton->setToolTip(tr("Mark Read/Unread"));
    readButton->setAutoRaise(true);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setMargin(0);
    buttonsLayout->setSpacing(5);
    buttonsLayout->addWidget(iconNews);
    buttonsLayout->addWidget(titleNews, 1);
    buttonsLayout->addWidget(readButton);

    setLayout(buttonsLayout);
    titleNews->setFixedWidth(300);
  }
  QLabel *iconNews;
  QLabel *titleNews;
  QToolButton *readButton;

private slots:

signals:

};

class NotificationWidget : public QWidget
{
  Q_OBJECT
public:
  NotificationWidget(QSqlDatabase *db, QList<int> idFeedList,
                     QList<int> cntNewNewsList, QWidget * parent = 0);

public slots:

private:
  QLabel *iconTitle_;
  QLabel *textTitle_;
  QToolButton *closeButton_;
  QStackedWidget *stackedWidget_;
  QLabel *numPage_;
  QToolButton *leftButton_;
  QToolButton *rightButton_;

  QTimer *showTimer_;

  QSqlDatabase *db_;
  QList<int> idFeedList_;
  QList<int> cntNewNewsList_;
  int countShowNews_;
  int timeShowNews_;

private slots:
  void nextPage();
  void previousPage();

};

#endif // NOTIFICATIONS_H
