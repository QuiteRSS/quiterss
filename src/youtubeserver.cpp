/*************************************************************
 * QuiteRSS - Youtube Ephemeral HTTP Server
 * Serves sequential_youtube_player.html with SSE client tracking
 *************************************************************/

#include "youtubeserver.h"
#include <QTcpSocket>
#include <QTcpServer>
#include <QTimer>
#include <QDateTime>

YoutubeServer::YoutubeServer(QObject *parent)
    : QObject(parent)
    , server_(nullptr)
    , port_(0)
{
    idleTimer_ = new QTimer(this);
    connect(idleTimer_, SIGNAL(timeout()), this, SLOT(checkIdleTimeout()));
}

YoutubeServer::~YoutubeServer()
{
    stop();
}

bool YoutubeServer::start(quint16 port)
{
    if (server_)
        return true;

    server_ = new QTcpServer(this);
    connect(server_, SIGNAL(newConnection()), this, SLOT(handleNewConnection()));

    if (!server_->listen(QHostAddress::LocalHost, port)) {
        delete server_;
        server_ = nullptr;
        return false;
    }

    port_ = server_->serverPort();
    idleTimer_->start(60000); // Check every minute

    return true;
}

void YoutubeServer::stop()
{
    idleTimer_->stop();

    // Close all client connections
    QHashIterator<QTcpSocket*, QDateTime> it(clients_);
    while (it.hasNext()) {
        it.next();
        QTcpSocket *socket = it.key();
        socket->close();
        socket->deleteLater();
    }
    clients_.clear();
    sseClients_.clear();

    if (server_) {
        server_->close();
        delete server_;
        server_ = nullptr;
    }

    port_ = 0;
}

bool YoutubeServer::isRunning() const
{
    return server_ && server_->isListening();
}

QString YoutubeServer::serverUrl() const
{
    if (!isRunning())
        return QString();
    return QString("http://127.0.0.1:%1").arg(port_);
}

void YoutubeServer::setHtmlContent(const QString &html)
{
    htmlContent_ = html;
}

void YoutubeServer::handleNewConnection()
{
    while (server_->hasPendingConnections()) {
        QTcpSocket *socket = server_->nextPendingConnection();
        connect(socket, SIGNAL(disconnected()), this, SLOT(handleClientDisconnected()));
        connect(socket, SIGNAL(readyRead()), this, SLOT(handleClientReadyRead()));
        clients_.insert(socket, QDateTime::currentDateTime());
    }
}

void YoutubeServer::handleClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    clients_.remove(socket);
    sseClients_.remove(socket);
    socket->deleteLater();
}

void YoutubeServer::handleClientReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !clients_.contains(socket))
        return;

    // Update last activity time
    clients_.insert(socket, QDateTime::currentDateTime());

    // Read HTTP request
    QByteArray request = socket->readAll();
    QString requestStr = QString::fromUtf8(request);

    // Check if this is an SSE request
    if (requestStr.contains("Accept: text/event-stream") ||
        requestStr.contains("accept: text/event-stream")) {
        // SSE endpoint
        sseClients_.insert(socket, true);
        sendSseHeaders(socket);
        return;
    }

    // Serve the HTML page
    if (requestStr.startsWith("GET") || requestStr.contains("GET /")) {
        QByteArray htmlBytes = htmlContent_.toUtf8();
        sendHttpResponse(socket, 200, "text/html; charset=utf-8", htmlBytes);
        return;
    }

    // Default: 404
    sendHttpResponse(socket, 404, "text/plain", QByteArray("Not Found"));
}

void YoutubeServer::checkIdleTimeout()
{
    QDateTime now = QDateTime::currentDateTime();

    // Check for idle clients
    QMutableHashIterator<QTcpSocket*, QDateTime> it(clients_);
    while (it.hasNext()) {
        it.next();
        QTcpSocket *socket = it.key();
        QDateTime lastActivity = it.value();

        if (lastActivity.msecsTo(now) > IDLE_TIMEOUT_MS) {
            // Close idle connection
            socket->close();
            // Note: socket will be cleaned up in handleClientDisconnected
        }
    }

    // Send keepalive to SSE clients
    QHashIterator<QTcpSocket*, bool> sseIt(sseClients_);
    while (sseIt.hasNext()) {
        sseIt.next();
        QTcpSocket *socket = sseIt.key();
        if (socket->state() == QAbstractSocket::ConnectedState) {
            // Send SSE comment (keepalive)
            socket->write(":keepalive\n\n");
            socket->flush();
        }
    }

    // If no clients at all and running, we could auto-stop here
    // But the requirement says to stay alive until QuiteRSS closes or idle timeout
    // So we keep running
}

void YoutubeServer::sendHttpResponse(QTcpSocket *socket, int statusCode, const QString &contentType, const QByteArray &body)
{
    QString statusText = (statusCode == 200) ? "OK" : "Not Found";
    QByteArray response = QString("HTTP/1.1 %1 %2\r\n"
                                  "Content-Type: %3\r\n"
                                  "Content-Length: %4\r\n"
                                  "Connection: close\r\n"
                                  "\r\n")
                               .arg(statusCode)
                               .arg(statusText)
                               .arg(contentType)
                               .arg(body.size())
                               .toUtf8();
    response.append(body);

    socket->write(response);
    socket->flush();
    socket->waitForBytesWritten(5000);
    socket->close();
}

void YoutubeServer::sendSseHeaders(QTcpSocket *socket)
{
    QByteArray headers = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/event-stream\r\n"
                         "Cache-Control: no-cache\r\n"
                         "Connection: keep-alive\r\n"
                         "\r\n";
    socket->write(headers);
    socket->flush();
    // Don't close - keep connection open for SSE
}
