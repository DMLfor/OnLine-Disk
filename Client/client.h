#ifndef CLIENT_H
#define CLIENT_H
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
#define idleSeconds_ 1.0

using namespace muduo;
using namespace muduo::net;


class ClientChunk;

class Client : public QObject
{
    Q_OBJECT
public:
    Client(EventLoop *loopMaster, const InetAddress &serverAddr, EventLoop *loopChunk);

    void connect();

    void disconnect();

    void write(const string &message, int16_t type);

private:


    void onConnection(const TcpConnectionPtr &conn);

    void onStringMessage(const TcpConnectionPtr&,
                        const string &messgae, Timestamp, int16_t type);

    Json::Value getBlockList(const Json::Value file);

    void onTimer();

    Json::FastWriter writer_;
    Json::Reader reader_;
    TcpClient client_;
    ClientChunk *chunkClient_;
    std::vector<ClientChunk *>chunkClientList_;
    LengthHeaderCodec codec_;
    MutexLock mutex_;
    TcpConnectionPtr connection_;
    EventLoop *loopChunk_;

    void sendFileList(const Json::Value &fileList)
    {
        emit FileList(fileList);
    }

    void sendFileTask(const Json::Value &fileTask)
    {
        emit FileTask(fileTask);
    }

    void sendUpdateBar(const QString &fileName)
    {
        emit UpdateBar(fileName);
    }
    void sendTaskOver(const QString &fileName)
    {
        emit TaskOver(fileName);
    }

signals:
    void FileList(const Json::Value &fileList);

    void FileTask(const Json::Value &task);

    void UpdateBar(const QString &fileName);

    void TaskOver(const QString &fileName);

};

class ClientChunk :public QObject
{
    Q_OBJECT
public:
    ClientChunk(EventLoop *loop, const InetAddress &serverAddr, const Json::Value task);

    void connect();

    void disconnect();

    void sendLoadingUpdate(const QString fileName)
    {
        emit LoadingUpdate(fileName);
    }

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
#endif // CLIENT_H
