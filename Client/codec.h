//
// Created by dblank on 17-8-4.
//

#ifndef CODEC_CODEC_H
#define CODEC_CODEC_H

#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Endian.h>
#include <muduo/net/TcpConnection.h>
#include <iostream>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <jsoncpp/json/json.h>

using namespace muduo;
using namespace muduo::net;

#define fileTransType   1
#define commTransType   0
#define sendType        2
#define recvType        3
#define BUFSIZE 1024*1024
#define S64M (1024*1024*64)
typedef boost::shared_ptr<FILE> FilePtr;
struct CtxType
{
    int seq;
    FilePtr file;
    long long offset;
    int type;
    long long writed;
    long long size;
    Json::Value block;
};
typedef boost::shared_ptr<CtxType> CtxPtr;

class LengthHeaderCodec : boost::noncopyable
{
public:

    typedef boost::function<void (const TcpConnectionPtr&,
            const string & message,
            Timestamp, int type)> StringMessageCallback;

    explicit  LengthHeaderCodec(const StringMessageCallback &cb) : messageCallback_(cb)
    {

    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime)
    {
        while (buf->readableBytes() >= KHeaderLen_)
        {
            const void *data = buf->peek();
            struct Header header = *static_cast<const struct Header *>(data);
            header.init();
            if (header.len > 1024*1024 || header.len < 0)
            {
                LOG_ERROR << "Invalid length :" << header.len;
                conn->shutdown();
                break;
            }

            if (header.flag != Flag_)
            {
                LOG_ERROR << "Invalid flag" << header.flag;
                if(buf->readableBytes() >= header.len + KHeaderLen_)
                    buf->retrieve(header.len);
            }

            if (buf->readableBytes() >= header.len + KHeaderLen_)
            {
                buf->retrieve(KHeaderLen_);
                string message(buf->peek(), header.len);
                messageCallback_(conn, message, receiveTime, header.type);
                buf->retrieve(header.len);
            }
            else
                break;
        }
    }

    void send(TcpConnection *conn, const string &message, int16_t type)
    {
        Buffer buf;
        buf.append(message.data(), message.size());

        int32_t len = static_cast<int32_t>(message.size());

        buf.prependInt16(type);
        buf.prependInt16(Flag_);
        buf.prependInt32(len);

        conn->send(&buf);
    }

private:
    struct Header
    {
        int32_t len;
        int16_t flag;
        int16_t type;

        void init()
        {
            len = sockets::networkToHost32(len);
            flag = sockets::networkToHost16(flag);
            type = sockets::networkToHost16(type);
        }
    };
    StringMessageCallback messageCallback_;
    const static size_t KHeaderLen_ = sizeof(struct Header);
    const static int16_t Flag_ = 233;

};

#endif //CODEC_CODEC_H
