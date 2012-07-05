#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <QtGui>
#include <QtSql>

class NewsItem : public QWidget
{
  Q_OBJECT
public:
  NewsItem(int idFeed, int idNews, int width, QWidget * parent = 0) :
    QWidget(parent), feedId_(idFeed), newsId_(idNews)
  {
    read = false;

    setCursor(Qt::PointingHandCursor);

    iconNews = new QLabel(this);
    QPixmap icon(":/images/feed");
    iconNews->setPixmap(icon);
    iconNews->setFixedSize(icon.size());
    titleNews = new QLabel(this);


    readButton = new QToolButton(this);
    readButton->setIcon(QIcon(":/images/bulletUnread"));
    readButton->setToolTip(tr("Mark Read/Unread"));
    readButton->setAutoRaise(true);
    readButton->hide();

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setMargin(0);
    buttonsLayout->setSpacing(5);
    buttonsLayout->addWidget(iconNews);
    buttonsLayout->addWidget(titleNews, 1);
    buttonsLayout->addWidget(readButton);

    setLayout(buttonsLayout);
    installEventFilter(this);

    titleNews->setFixedWidth(width);

    connect(readButton, SIGNAL(clicked()),
            this, SLOT(markRead()));
  }
  int feedId_;
  int newsId_;
  QLabel *iconNews;
  QLabel *titleNews;
  QToolButton *readButton;
  bool read;

protected:
  bool eventFilter(QObject *obj, QEvent *event)
  {
    if(event->type() == QEvent::MouseButtonPress) {
      emit signalTitleClicked(feedId_, newsId_);
      return true;
    } else {
      return QObject::eventFilter(obj, event);
    }
  }

private slots:
  void markRead()
  {
    read = !read;
    if (read)
      readButton->setIcon(QIcon(":/images/bulletRead"));
    else
      readButton->setIcon(QIcon(":/images/bulletUnread"));
    emit signalMarkRead(newsId_);
  }

signals:
  void signalMarkRead(int);
  void signalTitleClicked(int, int);

};

class NotificationWidget : public QWidget
{
  Q_OBJECT
public:
  NotificationWidget(QSqlDatabase *db, QList<int> idFeedList,
                     QList<int> cntNewNewsList,
                     int countShowNews, int timeShowNews, int widthTitleNews,
                     QWidget * parent = 0);

protected:
  virtual void showEvent(QShowEvent*);
  bool eventFilter(QObject *obj, QEvent *event);
  /*virtual*/ void enterEvent(QEvent*);
  /*virtual*/ void leaveEvent(QEvent*);

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
  int widthTitleNews_;

private slots:
  void nextPage();
  void previousPage();
  void markRead(int id);


signals:
  void signalShow();
  void signalDelete();
  void signalOpenNews(int feedId, int newsId);

};

#endif // NOTIFICATIONS_H
