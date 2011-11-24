/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

/*
rsslisting.cpp

Provides a widget for displaying news items from RDF news sources.
RDF is an XML-based format for storing items of information (see
http://www.w3.org/RDF/ for details).

The widget itself provides a simple user interface for specifying
the URL of a news source, and controlling the downloading of news.

The widget downloads and parses the XML asynchronously, feeding the
data to an XML reader in pieces. This allows the user to interrupt
its operation, and also allows very large data sources to be read.
*/


#include <QtDebug>
#include <QtCore>

#include "rsslisting.h"


/*
    Constructs an RSSListing widget with a simple user interface, and sets
    up the XML reader to use a custom handler class.

    The user interface consists of a line edit, a push button, and a
    list view widget. The line edit is used for entering the URLs of news
    sources; the push button starts the process of reading the
    news.
*/

const QString kCreateFeedTableQuery(
    "create table feed_%1("
        "id integer primary key, "
        "guid varchar, "
        "description varchar, "
        "title varchar, "
        "date varcher, "
        "published varchar, "
        "modified varchar, "
        "received varchar, "
        "author varchar, "
        "category varchar, "
        "label varchar, "
        "status integer default 0, "
        "sticky integer default 0, "
        "attachment varchar, "
        "feed varchar, "
        "location varchar, "
        "link varchar"
    ")");

RSSListing::RSSListing(QWidget *parent)
    : QMainWindow(parent), currentReply_(0)
{
    feedEdit_ = new QLineEdit();

    db_ = QSqlDatabase::addDatabase("QSQLITE");
    db_.setDatabaseName("data.db");
    if (QFile("data.db").exists()) {
      db_.open();
    } else {  // Инициализация базы
      db_.open();
      db_.exec("create table feeds(id integer primary key, link varchar)");
      db_.exec("insert into feeds(link) values ('http://labs.qt.nokia.com/blogs/feed')");
      db_.exec("insert into feeds(link) values ('http://www.prog.org.ru/index.php?type=rss;action=.xml')");
      db_.exec("create table info(id integer primary key, name varchar, value varchar)");
      db_.exec("insert into info(name, value) values ('version', '1.0')");
      db_.exec(kCreateFeedTableQuery.arg(1));
      db_.exec(kCreateFeedTableQuery.arg(2));
    }

    model_ = new QSqlTableModel();
    model_->setTable("feeds");
    model_->select();

    feedsTreeView_ = new QTreeView();
    feedsTreeView_->setModel(model_);
    feedsTreeView_->header()->setResizeMode(QHeaderView::ResizeToContents);
    feedsTreeView_->setUniformRowHeights(true);
    feedsTreeView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(feedsTreeView_, SIGNAL(clicked(QModelIndex)),
            this, SLOT(slotFeedsTreeClicked(QModelIndex)));
    connect(feedsTreeView_, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(slotFeedsTreeDoubleClicked(QModelIndex)));
    connect(this, SIGNAL(signalFeedsTreeKeyUpDownPressed()),
            SLOT(slotFeedsTreeKeyUpDownPressed()), Qt::QueuedConnection);


    feedModel_ = new QSqlTableModel();
    feedView_ = new QTableView();
    feedView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    feedView_->horizontalHeader()->setStretchLastSection(true);
    feedView_->verticalHeader()->setDefaultSectionSize(
        feedView_->verticalHeader()->minimumSectionSize());
    feedView_->verticalHeader()->setVisible(false);
    feedView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(feedView_, SIGNAL(clicked(QModelIndex)),
            this, SLOT(slotFeedViewClicked(QModelIndex)));
    connect(this, SIGNAL(signalFeedKeyUpDownPressed()),
            SLOT(slotFeedKeyUpDownPressed()), Qt::QueuedConnection);

    treeWidget_ = new QTreeWidget();
    connect(treeWidget_, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            this, SLOT(itemActivated(QTreeWidgetItem*)));
    QStringList headerLabels;
    headerLabels << tr("Item") << tr("Title") << tr("Link") << tr("Description")
                 << tr("Comments") << tr("pubDate") << tr("guid");
    treeWidget_->setHeaderLabels(headerLabels);
    treeWidget_->header()->setResizeMode(QHeaderView::ResizeToContents);

    webView_ = new QWebView();

    networkProxy_.setType(QNetworkProxy::HttpProxy);
    networkProxy_.setHostName("10.0.0.172");
    networkProxy_.setPort(3150);
    manager_.setProxy(networkProxy_);
    QNetworkProxy::setApplicationProxy(networkProxy_);


    connect(&manager_, SIGNAL(finished(QNetworkReply*)),
             this, SLOT(finished(QNetworkReply*)));

    QVBoxLayout *treeLayout = new QVBoxLayout();
    treeLayout->setMargin(0);
    treeLayout->addWidget(feedsTreeView_);

    QWidget *treeWidget = new QWidget();
    treeWidget->setLayout(treeLayout);

    QVBoxLayout *webLayout = new QVBoxLayout();
    webLayout->setMargin(1);  // Чтобы было видно границу виджета
    webLayout->addWidget(webView_);

    QWidget *webWidget = new QWidget();
    webWidget->setStyleSheet("border: 1px solid gray");
    webWidget->setLayout(webLayout);

    QSplitter *feedTabSplitter = new QSplitter(Qt::Vertical);
    feedTabSplitter->addWidget(feedView_);
    feedTabSplitter->addWidget(webWidget);

    feedTabWidget_ = new QTabWidget();
    feedTabWidget_->addTab(feedTabSplitter, "");

    QSplitter *contentSplitter = new QSplitter(Qt::Vertical);
    contentSplitter->addWidget(treeWidget_);
    contentSplitter->addWidget(feedTabWidget_);

    QSplitter *feedSplitter = new QSplitter();
    feedSplitter->addWidget(treeWidget);
    feedSplitter->addWidget(contentSplitter);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(4);
    layout->addWidget(feedSplitter);

    QWidget *centralWidget = new QWidget();
    centralWidget->setLayout(layout);

    setCentralWidget(centralWidget);

    setWindowTitle(tr("QtRSS"));

    createActions();
    createMenu();
    createToolBar();

    statusBar()->setVisible(true);

    feedsTreeView_->installEventFilter(this);
    feedView_->installEventFilter(this);

    webView_->load(QUrl("qrc:/html/test1.html"));
    webView_->show();
}

bool RSSListing::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == feedsTreeView_) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
      if ((keyEvent->key() == Qt::Key_Up) ||
          (keyEvent->key() == Qt::Key_Down)) {
        emit signalFeedsTreeKeyUpDownPressed();
      }
      return false;
    } else {
      return false;
    }
  } else if (obj == feedView_) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
      if ((keyEvent->key() == Qt::Key_Up) ||
          (keyEvent->key() == Qt::Key_Down)) {
        emit signalFeedKeyUpDownPressed();
      }
      return false;
    } else {
      return false;
    }
  } else {
    // pass the event on to the parent class
    return QMainWindow::eventFilter(obj, event);
  }
}

void RSSListing::createActions()
{
  addFeedAct_ = new QAction(QIcon(":/images/add.png"), tr("&Add..."), this);
  addFeedAct_->setStatusTip(tr("Add new feed"));
  connect(addFeedAct_, SIGNAL(triggered()), this, SLOT(addFeed()));

  deleteFeedAct_ = new QAction(QIcon(":/images/delete.png"), tr("&Delete..."), this);
  deleteFeedAct_->setStatusTip(tr("Delete selected feed"));
  connect(deleteFeedAct_, SIGNAL(triggered()), this, SLOT(deleteFeed()));
}

void RSSListing::createMenu()
{
  fileMenu_ = menuBar()->addMenu(tr("&File"));

  fileMenu_->addAction(addFeedAct_);
  fileMenu_->addAction(deleteFeedAct_);
  fileMenu_->addSeparator();

  QAction *exitAct_ = new QAction("E&xit", this);
  connect(exitAct_, SIGNAL(triggered()), this, SLOT(close()));
  fileMenu_->addAction(exitAct_);

  menuBar()->addMenu(tr("&Edit"));
  menuBar()->addMenu(tr("&View"));
  feedMenu_ = menuBar()->addMenu(tr("Fee&ds"));
  menuBar()->addMenu(tr("&News"));
  menuBar()->addMenu(tr("&Tools"));
  menuBar()->addMenu(tr("&Help"));
}

void RSSListing::createToolBar()
{
  toolBar_ = addToolBar(tr("General"));
  toolBar_->addWidget(feedEdit_);
  toolBar_->addAction(addFeedAct_);
  toolBar_->addAction(deleteFeedAct_);
  toolBar_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
}

/*
    Starts the network request and connects the needed signals
*/
void RSSListing::get(const QUrl &url)
{
    QNetworkRequest request(url);
    if (currentReply_) {
        currentReply_->disconnect(this);
        currentReply_->deleteLater();
    }
    currentReply_ = manager_.get(request);
    connect(currentReply_, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(currentReply_, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
    connect(currentReply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
}

void RSSListing::addFeed()
{
  QSqlQuery q(db_);
  QString qStr = "insert into feeds(link) values ('" + feedEdit_->text() + "')";
  q.exec(qStr);
  q.exec(kCreateFeedTableQuery.arg(q.lastInsertId().toString()));
  model_->select();
  feedEdit_->clear();
}

void RSSListing::deleteFeed()
{
  QSqlQuery q(db_);
  QString str = QString("delete from feeds where link='%1'").
      arg(model_->record(feedsTreeView_->currentIndex().row()).field("link").value().toString());
  q.exec(str);
  q.exec(QString("drop table feed_%1").
      arg(model_->record(feedsTreeView_->currentIndex().row()).field("id").value().toString()));
  model_->select();
}

void RSSListing::metaDataChanged()
{
    QUrl redirectionTarget = currentReply_->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectionTarget.isValid()) {
        get(redirectionTarget);
    }
}

/*
    Reads data received from the RDF source.

    We read all the available data, and pass it to the XML
    stream reader. Then we call the XML parsing function.
*/

void RSSListing::readyRead()
{
    int statusCode = currentReply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode >= 200 && statusCode < 300) {
        QByteArray data = currentReply_->readAll();
        xml.addData(data);
        parseXml();
    }
}

/*
    Finishes processing an HTTP request.

    The default behavior is to keep the text edit read only.

    If an error has occurred, the user interface is made available
    to the user for further input, allowing a new fetch to be
    started.

    If the HTTP get request has finished, we make the
    user interface available to the user for further input.
*/

void RSSListing::finished(QNetworkReply *reply)
{
    Q_UNUSED(reply);
    feedEdit_->setReadOnly(false);
    addFeedAct_->setEnabled(true);
    deleteFeedAct_->setEnabled(true);
    feedsTreeView_->setEnabled(true);
}


/*
    Parses the XML data and creates treeWidget items accordingly.
*/
void RSSListing::parseXml()
{
    static int count = 0;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "item")
                linkString = xml.attributes().value("rss:about").toString();
            currentTag = xml.name().toString();
            qDebug() << count << ": " << xml.namespaceUri().toString() << ": " << currentTag;
        } else if (xml.isEndElement()) {
            if (xml.name() == "item") {

                QTreeWidgetItem *item = new QTreeWidgetItem;
                item->setText(0, itemString);
                item->setText(1, titleString);
                item->setText(2, linkString);
                item->setText(3, descriptionString);
                item->setText(4, commentsString);
                item->setText(5, pubDateString);
                item->setText(6, guidString);
                treeWidget_->addTopLevelItem(item);

                QSqlQuery q(db_);
                QString qStr = QString("select * from feed_%1 where guid == '%2'").
                    arg(model_->index(feedsTreeView_->currentIndex().row(), 0).data().toString()).
                    arg(guidString);
                q.exec(qStr);
                if (!q.next()) {
                  qStr = QString("insert into feed_%1(description, guid, title) values(?, ?, ?)").
                      arg(model_->index(feedsTreeView_->currentIndex().row(), 0).data().toString());
                  q.prepare(qStr);
                  q.addBindValue(descriptionString);
                  q.addBindValue(guidString);
                  q.addBindValue(titleString);
                  q.exec();
                  qDebug() << q.lastError().number() << ": " << q.lastError().text();
                }

                itemString.clear();
                titleString.clear();
                linkString.clear();
                descriptionString.clear();
                commentsString.clear();
                pubDateString.clear();
                guidString.clear();
                ++count;
            }
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            if (currentTag == "item")
              itemString += xml.text().toString();
            else if (currentTag == "title")
              titleString += xml.text().toString();
            else if (currentTag == "link")
              linkString += xml.text().toString();
            else if (currentTag == "description")
              descriptionString += xml.text().toString();
            else if (currentTag == "comments")
              commentsString += xml.text().toString();
            else if (currentTag == "pubDate")
              pubDateString += xml.text().toString();
            else if (currentTag == "guid")
              guidString += xml.text().toString();
        }
    }
    if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        qWarning() << "XML ERROR:" << xml.lineNumber() << ": " << xml.errorString();
    }
    slotFeedsTreeClicked(model_->index(feedsTreeView_->currentIndex().row(), 0));
}

/*
    Open the link in the browser
*/
void RSSListing::itemActivated(QTreeWidgetItem * item)
{
//    QDesktopServices::openUrl(QUrl(item->text(1)));
    webView_->setHtml(item->text(3));
}

void RSSListing::error(QNetworkReply::NetworkError)
{
    qWarning("error retrieving RSS feed");
    currentReply_->disconnect(this);
    currentReply_->deleteLater();
    currentReply_ = 0;
}

void RSSListing::slotFeedsTreeClicked(QModelIndex index)
{
  feedView_->setModel(0);
  feedModel_->setTable(QString("feed_%1").arg(model_->index(index.row(), 0).data().toString()));
  feedModel_->select();
  feedView_->setModel(feedModel_);
  feedView_->setColumnHidden(feedModel_->fieldIndex("id"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("guid"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("description"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("date"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("published"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("modified"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("received"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("author"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("category"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("label"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("status"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("sticky"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("attachment"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("feed"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("location"), true);
  feedView_->setColumnHidden(feedModel_->fieldIndex("link"), true);
  feedTabWidget_->setTabText(0, model_->index(index.row(), 1).data().toString());
}

void RSSListing::slotFeedsTreeDoubleClicked(QModelIndex index)
{
  feedEdit_->setReadOnly(true);
  addFeedAct_->setEnabled(false);
  deleteFeedAct_->setEnabled(false);
  feedsTreeView_->setEnabled(false);
  treeWidget_->clear();

  xml.clear();

  QUrl url(model_->record(index.row()).field("link").value().toString());
  get(url);
}

void RSSListing::slotFeedViewClicked(QModelIndex index)
{
  webView_->setHtml(
      feedModel_->record(index.row()).field("description").value().toString());
}

void RSSListing::slotFeedsTreeKeyUpDownPressed()
{
  slotFeedsTreeClicked(feedsTreeView_->currentIndex());
}

void RSSListing::slotFeedKeyUpDownPressed()
{
  slotFeedViewClicked(feedView_->currentIndex());
}
