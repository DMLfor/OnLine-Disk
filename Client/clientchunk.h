#ifndef CLIENTCHUNK_H
#define CLIENTCHUNK_H

#include "codec.h"
#include "md5.h"

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <jsoncpp/json/json.h>
#include <sys/stat.h>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <iostream>
#include <cstdio>
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QObject>

class ClientChunk :public QObject
{
    Q_OBJECT
public:
    ClientChunk(EventLoop *loop, const InetAddress &serverAddr, const Json::Value task);

    void connect();

    void disconnect();

    void sendLoadingUpdate(const QString fileName);

    bool done_;

private:


    void onConnection(const TcpConnectionPtr &conn);

    void onStringMessage(const TcpConnectionPtr&,
                         const string &messgae, Timestamp, int16_t type);

    void onWriteCompelte(const TcpConnectionPtr &conn);

    void sendBlock(const TcpConnectionPtr &conn, int seq);

    void recvBlock(const TcpConnectionPtr &conn, int seq);

    TcpClient client_;
    MutexLock mutex_;
    LengthHeaderCodec codec_;
    TcpConnectionPtr connection_;
    Json::FastWriter writer_;
    Json::Reader reader_;
    Json::Value task_;

signals:
    void LoadingUpdate(const QString fileName);
};
#endif // CLIENTCHUNK_H
