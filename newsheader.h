#ifndef NEWSHEADER_H
#define NEWSHEADER_H

#include <QtGui>

class NewsHeader : public QHeaderView
{
  Q_OBJECT
public:
  NewsHeader(Qt::Orientation orientation, QWidget * parent = 0);

private slots:

signals:

private:

};

#endif // NEWSHEADER_H
