//
// Created by dblank on 17-8-4.
//

#include "master.h"

MasterServer::MasterServer(EventLoop *loop, const InetAddress &listenAddr, int idleSeconds) :
        idleSeconds_(idleSeconds),
        server_(loop, listenAddr, "MasterServer"),
        codec_(boost::bind(&MasterServer::onStringMessage, this, _1, _2, _3, _4))
{
    server_.setConnectionCallback(
            boost::bind(&MasterServer::onConnection, this, _1));
    server_.setMessageCallback(
            boost::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
    loop->runEvery(1.0, boost::bind(&MasterServer::onTimer, this));

}

void MasterServer::start()
{
    server_.start();
}

void MasterServer::onConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected())
    {
        Node node;
        node.lastReceiveTime = Timestamp::now();
        connectionList_.push_back(conn);
        node.position = --connectionList_.end();
        conn->setContext(node);
    }
    else
    {
        assert(!conn->getContext().empty());
        const Node &node = boost::any_cast<const Node &>(conn->getContext());
        connectionList_.erase(node.position);
    }
}

void MasterServer::onStringMessage(const TcpConnectionPtr &conn, const string &message, Timestamp time, uint16_t type)
{
    Json::Value sendMessage, recvMessage;
    json_.getJson(message, recvMessage);
    if (recvMessage["type"].asString() != "heart-client" && recvMessage["type"].asString() != "heart-chunk")
        LOG_INFO << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort()
                 << " recv " << message;
    string msgType = recvMessage["type"].asString();

    if (msgType == "request-upload")
    {
        string msgBody = recvMessage["file-md5"].asString();
        string sendMsg;
        string fileName = "", filePath = recvMessage["file-name"].asString();
        for(int i = filePath.size()-1; i>=0 && filePath[i] != '/'; i--)
            fileName += filePath[i];
        std::reverse(fileName.begin(), fileName.end());

        if (fileMap_.count(recvMessage["file-md5"].asString()) != 0)
        {
            md5Map_[fileName] = recvMessage["file-md5"].asString();
            sendMessage["type"] = "reply-upload";
            sendMessage["body"] = "fast-pass";
            sendMessage["file-name"] = fileName;
            json_.resolveJson(sendMessage, sendMsg);
            codec_.send(boost::get_pointer(conn), sendMsg, type);
        }
        else
        {
            sendMessage = recvMessage;
            sendMessage["type"] = "reply-upload";
            sendMessage["body"] = "shit-pass";
            Json::Value ipAddr;
            for (auto &it : chunkSet_)
            {
                ipAddr["ip"] = it.first;
                ipAddr["port"] = it.second;
                sendMessage["chunk"].append(ipAddr);
            }
            json_.resolveJson(sendMessage, sendMsg);

            codec_.send(boost::get_pointer(conn), sendMsg, type);

        }
    }
    if (msgType == "trans-over")
    {
        string fileName = "", filePath = recvMessage["file-name"].asString();
        for(int i = filePath.size()-1; i>=0 && filePath[i] != '/'; i--)
            fileName += filePath[i];
        std::reverse(fileName.begin(), fileName.end());
        recvMessage["file-name"] = fileName;
        md5Map_[fileName] = recvMessage["file-md5"].asString();
        string fileMd5 = recvMessage["file-md5"].asString();
        fileMap_[fileMd5].push_back(recvMessage);
    }
    if(msgType == "request-list")
    {
        Json::Value fileList;
        fileList["type"] = "reply-list";
        for(auto &it :md5Map_)
        {
            fileList["file"].append(it.first);
        }
        string sendMsg;
        json_.resolveJson(fileList, sendMsg);
        codec_.send(conn.get(), sendMsg, commTransType);
    }
    if (msgType == "request-download")
    {
        if (md5Map_.count(recvMessage["file-name"].asString()) != 0)
        {
            long long fileSize = 0;
            std::vector<Json::Value> &tmp = fileMap_[md5Map_[recvMessage["file-name"].asString()]];
            sendMessage["type"] = "reply-download";
            sendMessage["body"] = "ok";
            for (int i = 0; i < tmp.size(); i++)
            {
                sendMessage["block"].append(tmp[i]), fileSize += tmp[i]["block-size"].asInt64();
                sendMessage["block"][i]["file-name"] = recvMessage["file-name"].asString();
            }
            sendMessage["file-size"] = fileSize;
            sendMessage["file-name"] = recvMessage["file-name"].asString();
            string sendMsg;
            json_.resolveJson(sendMessage, sendMsg);
            LOG_ERROR << sendMsg;
            codec_.send(conn.get(), sendMsg, commTransType);
        }
        else
        {

            sendMessage["type"] = "reply-download";
            sendMessage["body"] = "no-file";
            string sendMsg;LOG_ERROR << sendMsg;
            json_.resolveJson(sendMessage, sendMsg);
            codec_.send(conn.get(), sendMsg, commTransType);
        }
    }

    if (msgType == "heart-chunk")
    {
        IpPair ipAddr = std::make_pair(recvMessage["ip"].asString(), recvMessage["port"].asInt());
        if (chunkSet_.find(ipAddr) == chunkSet_.end())
        {
            chunkSet_.insert(ipAddr);
        }
    }

    assert(!conn->getContext().empty());
    Node *node = boost::any_cast<Node>(conn->getMutableContext());
    node->lastReceiveTime = time;
    connectionList_.splice(connectionList_.end(), connectionList_, node->position);
    assert(node->position == --connectionList_.end());

}

void MasterServer::onTimer()
{
    Timestamp now = Timestamp::now();
    for (WeakConnectionList::iterator it = connectionList_.begin(); it != connectionList_.end();)
    {
        TcpConnectionPtr conn = it->lock();
        if (conn)
        {
            Node *n = boost::any_cast<Node>(conn->getMutableContext());
            double age = timeDifference(now, n->lastReceiveTime);
            if (age > idleSeconds_)
            {
                IpPair peerAddr = std::make_pair(conn->peerAddress().toIp(), conn->peerAddress().toPort());
                if (conn->connected())
                {
                    if (chunkSet_.find(peerAddr) != chunkSet_.end())
                    {
                        chunkSet_.erase(peerAddr);
                    }
                    conn->shutdown();
                    LOG_INFO << "shutting down " << conn->name() << " by time out";
                    conn->forceClose();
                }
            }
            else if (age < 0)
            {
                LOG_WARN << " Time jump";
                n->lastReceiveTime = now;
            }
            else
                break;

            ++it;
        }
        else
        {
            LOG_WARN << " Expired ";
            it = connectionList_.erase(it);
        }
    }
}



