//
// Created by dblank on 17-8-6.
//

#include <sys/stat.h>
#include "chunk.h"

using namespace muduo;

using namespace muduo::net;

void mySetContext(const TcpConnectionPtr &conn, int type, FILE *fp)
{
    const CtxPtr &ctx = boost::any_cast<const CtxPtr &>(conn->getContext());
    if(type == commTransType)
    {
        if(ctx->file != nullptr)
            fflush(ctx->file.get());
        ctx->file = nullptr;
        ctx->type = type;
    }
    else
    {
        FilePtr file(fp, ::fclose);
        ctx->type = type;
        ctx->file = file;
       // type == sendType ? ctx->sendFile  = file : ctx->recvFile = file;
    }
}

Chunk::Chunk(EventLoop *loop, const InetAddress &listenAddr, const InetAddress &serverAddr, int idleSeconds) :
    server_(loop, listenAddr, "ChunkServer"),
    client_(loop, serverAddr, "ClientChunk"),
    codecServer_(boost::bind(&Chunk::onServerStringMessage, this, _1, _2, _3, _4)),
    codecClient_(boost::bind(&Chunk::onClientStringMessage, this, _1, _2, _3, _4)),
    idleSeconds_(idleSeconds),
    ip_(listenAddr.toIp()), port_(listenAddr.toPort())
{
    server_.setConnectionCallback(
            boost::bind(&Chunk::onServerConnection, this, _1));
    server_.setMessageCallback(
            boost::bind(&LengthHeaderCodec::onMessage, &codecServer_, _1, _2, _3));
    server_.setWriteCompleteCallback(boost::bind(&Chunk::onWriteCompelte, this, _1));

    client_.setConnectionCallback(
            boost::bind(&Chunk::onClientConnection, this, _1));
    client_.setMessageCallback(
            boost::bind(&LengthHeaderCodec::onMessage, &codecClient_, _1, _2, _3));

    loop->runEvery(static_cast<double>(idleSeconds), boost::bind(&Chunk::onTimer, this));

    client_.enableRetry();

}
void Chunk::onServerConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO << conn->localAddress() .toIpPort() << " <- "
             <<conn->peerAddress().toIpPort() << " is "
             <<(conn->connected() ? "UP" : "DOWN");
    if(conn->connected())
    {
        CtxPtr ctx(new CtxType);
        ctx->type = commTransType;
        conn->setContext(ctx);
    }
}

void Chunk::onServerStringMessage(const TcpConnectionPtr &conn, const string &message, Timestamp, int16_t type)
{
    if(type == commTransType)
    {

        LOG_INFO << conn->localAddress().toIpPort() << " <- "
              << conn->peerAddress().toIpPort() << " send message : " << message;
        Json::Value sendMessage, recvMessage;
        json_.getJson(message, recvMessage);

        string msgType = recvMessage["type"].asString();
        string copy;
        if(msgType == "upload-block")
        {
            Json::Value block;
            json_.getJson(message , block);
            string path = "block/";
            path += block["block-md5"].asCString();
            FILE *fp = fopen(path.c_str(), "w");
            FilePtr file(fp, ::fclose);
            CtxPtr ctx(new CtxType);
            ctx->file = file;
            ctx->type = recvType;
            ctx->offset = block["block-offset"].asInt64();
            conn->setContext(ctx);
        }

        if(msgType == "trans-over")
        {
            LOG_INFO << " over ";
            Json::Value blockOver = recvMessage;
            blockOver["ip"] = ip_;
            blockOver["port"] = port_;
            string msg;
            json_.resolveJson(blockOver, msg);
            codecClient_.send(connection_.get(), msg, commTransType);
            mySetContext(conn, commTransType, nullptr);
        }

        if (msgType == "request-download")
        {
            sendMessage = recvMessage;
            sendFile(conn, sendMessage);
        }

    }
    else
    {
       // LOG_INFO <<conn->peerAddress().toIpPort()<< " -> " << conn->localAddress().toIpPort()
        //            <<"send data " << message.size() << " bytes";
        const CtxPtr &ctx = boost::any_cast<const CtxPtr &>(conn->getContext());
        if(ctx->type == recvType)
        {
            size_t n = fwrite(message.c_str(), message.size(), 1, ctx->file.get());
        }
    }

}

void Chunk::sendFile(const TcpConnectionPtr &conn, Json::Value msgFile)
{
    char buf[BUFSIZE];
    FILE *fp = ::fopen(msgFile["file-name"].asCString(), "r");
    if(fp != nullptr)
    {
        LOG_INFO << "send file " << msgFile["file-name"].asString();
        mySetContext(conn, sendType, fp);
        size_t nread = ::fread(buf, 1, sizeof(buf), fp);
        string bufString(buf, nread);
        codecServer_.send(conn.get(), bufString, fileTransType);
       // LOG_INFO << conn->localAddress().toIpPort() << " -> "<< conn->peerAddress().toIpPort()
        //             << " send " << bufString.size() << " bytes";
    }
    else
    {
        conn->shutdown();
        LOG_INFO <<" download - no such file";
    }
}

void Chunk::onWriteCompelte(const TcpConnectionPtr &conn)
{
    const CtxPtr &ctx = boost::any_cast<const CtxPtr &>(conn->getContext());
    char buf[BUFSIZE];
    if(ctx->type == sendType)
    {
        size_t nread = ::fread(buf, 1, sizeof(buf),ctx->file.get());
        if (nread > 0)
        {
            string bufString(buf, nread);
            codecServer_.send(conn.get(), bufString, fileTransType);
           // LOG_INFO << conn->localAddress().toIpPort() << " -> "<< conn->peerAddress().toIpPort()
            //         << " send " << bufString.size() << " bytes";
        }
        else
        {
            LOG_INFO << conn->localAddress().toIpPort() << " -> "<< conn->peerAddress().toIpPort()
                     << " send file - done";
            string overMsg =  "{\"type\":\"reply-download\", \"body\":\"trans-over\"}";
            mySetContext(conn, commTransType, nullptr);
            codecServer_.send(conn.get(), overMsg, commTransType);
        }
    }
}


void Chunk::start()
{
    server_.start();
}


void Chunk::connect()
{
    client_.connect();
}

void Chunk::disconnect()
{
    client_.disconnect();
}


void Chunk::onClientConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");

    if(conn->connected())
    {
        connection_ = conn;
        onTimer();
    }
    else
    {
        connection_.reset();
    }

}

void Chunk::onClientStringMessage(const TcpConnectionPtr &conn, const string &message, Timestamp, int16_t type)
{
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << "  get message : ";

    if(type == commTransType)
    {
        //TODO
        ;
    }
}

void Chunk::onTimer()
{
    string message;
    Json::Value value;
    value["type"] = "heart-chunk";
    value["ip"] = ip_;
    value["port"] = port_;
    json_.resolveJson(value, message);
    codecClient_.send(boost::get_pointer(connection_), message, commTransType);
}

