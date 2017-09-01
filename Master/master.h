//
// Created by dblank on 17-8-3.
//

#ifndef MASTER_MASTER_H
#define MASTER_MASTER_H


#include "codec.h"
#include "jsonHandle.h"

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <jsoncpp/json/json.h>
#include <boost/bind.hpp>
#include <map>
#include <set>
#include <list>
#include <string>
#include <cstdio>
#include <vector>

using namespace muduo;
using namespace muduo::net;
typedef std::pair<string, int> IpPair;

class MasterServer
{
public:
    MasterServer(EventLoop *loop, const InetAddress &listenAddr, int idleSeconds);

    void start();
private:
    void onConnection(const TcpConnectionPtr &conn);

    void onStringMessage(const TcpConnectionPtr &conn, const string &message, Timestamp, uint16_t);

    void onTimer();

    typedef  boost::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
    typedef  std::list<WeakTcpConnectionPtr> WeakConnectionList;

    struct Node : public muduo::copyable
    {
        Timestamp lastReceiveTime;
        WeakConnectionList::iterator position;
    };
    typedef std::map<string, std::vector<Json::Value>> FileMap;
    typedef std::map<string, string> md5Map;
    md5Map md5Map_;
    std::set<IpPair> chunkSet_;
    int idleSeconds_;
    LengthHeaderCodec codec_;
    TcpServer server_;
    FileMap fileMap_;
    WeakConnectionList connectionList_;
    JsonHandle json_;

};

#endif //MASTER_MASTER_H
