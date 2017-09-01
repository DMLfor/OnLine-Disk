//
// Created by dblank on 17-8-6.
//

#ifndef Chunk_Chunk_H
#define Chunk_Chunk_H

#include "codec.h"
#include "jsonHandle.h"
#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>
#include <jsoncpp/json/json.h>
#include <boost/bind.hpp>
#include <map>
#include <string>
#include <cstdio>
#include <iostream>

using namespace muduo;
using namespace muduo::net;

typedef boost::shared_ptr<FILE> FilePtr;

void mySetContext(const TcpConnectionPtr &conn, int type, FILE *fp);

class Chunk : boost::noncopyable
{
public:
    friend void mySetContext(const TcpConnectionPtr &conn, int type, FILE *fp);

    Chunk(EventLoop *loop, const InetAddress &listenAddr, const InetAddress &serverAddr, int idleSeconds);

    void start();

    void connect();

    void disconnect();

private:
    void onServerConnection(const TcpConnectionPtr &conn);

    void onServerStringMessage(const TcpConnectionPtr &conn, const string &message, Timestamp, int16_t type);

    void onWriteCompelte(const TcpConnectionPtr &conn);

    void sendFile(const TcpConnectionPtr &conn, Json::Value msgFile);

    void onClientConnection(const TcpConnectionPtr &conn);

    void onClientStringMessage(const TcpConnectionPtr &conn, const string &message, Timestamp, int16_t type);

    void onTimer();

    int idleSeconds_;
    string ip_;
    uint16_t port_;
    TcpClient client_;
    TcpServer server_;
    TcpConnectionPtr connection_;
    LengthHeaderCodec codecClient_;
    LengthHeaderCodec codecServer_;
    std::map<string, int> fileMap_;

    JsonHandle json_;
};
#endif //Chunk_Chunk_H
