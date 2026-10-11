// test_stubserver.h - A local HTTP endpoint for the LLM tests.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - Serves a queue of canned replies to whatever the client
// posts and keeps what it was sent. Shared by test_llmclient and test_llmsession
// so ctest never depends on an endpoint being reachable; a test that silently
// passes when the machine is offline is worse than no test.
// Modelled on qurcuma's test_stubserver.h.
#pragma once

#include <QByteArray>
#include <QQueue>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>

class StubServer : public QTcpServer {
public:
    explicit StubServer(QObject* parent = nullptr)
        : QTcpServer(parent)
    {
        listen(QHostAddress::LocalHost, 0);
    }

    QString baseUrl() const
    {
        return QStringLiteral("http://127.0.0.1:%1/v1").arg(serverPort());
    }

    struct Reply {
        int status = 200;
        QByteArray body = "{}";
        QByteArray contentType = "application/json";
    };

    /// Next request gets this and only this, until another is queued.
    void reply(int status, const QByteArray& body, const QByteArray& contentType = "application/json")
    {
        m_queue.clear();
        m_queue.enqueue({ status, body, contentType });
    }

    /// Queue several: an agent loop takes more than one round.
    void enqueue(const QByteArray& body, int status = 200)
    {
        m_queue.enqueue({ status, body, "application/json" });
    }

    /// Every request body seen so far, oldest first.
    QVector<QByteArray> requests;
    QByteArray requestHeaders;   ///< of the most recent request
    QByteArray requestBody;      ///< of the most recent request

protected:
    void incomingConnection(qintptr descriptor) override
    {
        auto* socket = new QTcpSocket(this);
        socket->setSocketDescriptor(descriptor);
        auto* buffer = new QByteArray;
        connect(socket, &QTcpSocket::disconnected, socket, [socket, buffer] {
            delete buffer;
            socket->deleteLater();
        });
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer] {
            *buffer += socket->readAll();
            const int headerEnd = buffer->indexOf("\r\n\r\n");
            if (headerEnd < 0)
                return;
            const QByteArray headers = buffer->left(headerEnd);
            const int contentLength = lengthOf(headers);
            const QByteArray body = buffer->mid(headerEnd + 4);
            if (body.size() < contentLength)
                return;  // wait for the rest

            requestHeaders = headers;
            requestBody = body.left(contentLength);
            requests.append(requestBody);
            buffer->clear();

            const Reply r = m_queue.isEmpty() ? Reply {} : m_queue.dequeue();
            const QByteArray response = "HTTP/1.1 " + QByteArray::number(r.status) + " X\r\n"
                + "Content-Type: " + r.contentType + "\r\n"
                + "Content-Length: " + QByteArray::number(r.body.size()) + "\r\n"
                + "Connection: close\r\n\r\n" + r.body;
            socket->write(response);
            socket->flush();
            socket->disconnectFromHost();
        });
    }

private:
    static int lengthOf(const QByteArray& headers)
    {
        for (const QByteArray& line : headers.split('\n')) {
            const QByteArray trimmed = line.trimmed();
            if (trimmed.toLower().startsWith("content-length:"))
                return trimmed.mid(trimmed.indexOf(':') + 1).trimmed().toInt();
        }
        return 0;
    }

    QQueue<Reply> m_queue;
};
