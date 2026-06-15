/* ============================================================
 * QuiteRSS - Youtube Ephemeral HTTP Server
 * Serves sequential_youtube_player.html with SSE client tracking
 * ============================================================ */
#ifndef YOUTUBESERVER_H
#define YOUTUBESERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QHash>
#include <QDateTime>

class YoutubeServer : public QObject
{
    Q_OBJECT
public:
    explicit YoutubeServer(QObject *parent = nullptr);
    ~YoutubeServer();

    bool start(quint16 port = 0); // port 0 = auto-select
    void stop();
    bool isRunning() const;
    QString serverUrl() const;
    void setHtmlContent(const QString &html);

private slots:
    void handleNewConnection();
    void handleClientDisconnected();
    void handleClientReadyRead();
    void checkIdleTimeout();

private:
    void sendHttpResponse(QTcpSocket *socket, int statusCode, const QString &contentType, const QByteArray &body);
    void sendSseHeaders(QTcpSocket *socket);
    void closeClientConnection(QTcpSocket *socket);

    QTcpServer *server_;
    QString htmlContent_;
    QHash<QTcpSocket*, QDateTime> clients_; // socket -> last activity time
    QHash<QTcpSocket*, bool> sseClients_;   // socket -> is SSE client
    QTimer *idleTimer_;
    quint16 port_;
    static const int IDLE_TIMEOUT_MS = 5 * 60 * 1000; // 5 minutes
    static const int SSE_KEEPALIVE_INTERVAL_MS = 30000; // 30 seconds
};

#endif // YOUTUBESERVER_H
