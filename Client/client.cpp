//
// Created by dblank on 17-8-4.
//

#include "client.h"

Client::Client(EventLoop *loopMaster, const InetAddress &serverAddr, EventLoop *loopChunk) :
        loopChunk_(loopChunk),
        client_(loopMaster, serverAddr, "Client"),
        codec_(boost::bind(&Client::onStringMessage, this, _1, _2, _3, _4))
{
    client_.setConnectionCallback(
            boost::bind(&Client::onConnection, this, _1));
    client_.setMessageCallback(
            boost::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
    loopMaster->runEvery(static_cast<double>(idleSeconds_), boost::bind(&Client::onTimer, this));
    client_.enableRetry();
}


void Client::connect()
{
    client_.connect();
}

void Client::disconnect()
{
    client_.disconnect();
}

void Client::write(const string &message, int16_t type)
{
    MutexLockGuard lock(mutex_);
    if (connection_)
    {
        codec_.send(connection_.get(), message, type);
    }
}

void Client::onConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO << " ClientThread : " << CurrentThread::tid();
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");

    string sendStr = "{\"type\":\"request-list\"}";
    codec_.send(conn.get(), sendStr, commTransType);
    MutexLockGuard lock(mutex_);
    if (conn->connected())
    {
        connection_ = conn;
    }
    else
    {
        connection_.reset();
    }
}

void Client::onStringMessage(const TcpConnectionPtr &conn, const string &message, Timestamp, int16_t type)
{

    LOG_INFO << conn->localAddress().toIpPort() << " from "
             << conn->peerAddress().toIpPort() << " recv " << message.size() << " bytes is " << message;


    Json::Value recvMsg, sendMsg, taskMsg;

    if (!reader_.parse(message, recvMsg, false))
    {
        LOG_INFO << " not a json";
    }

    if (recvMsg["type"] == "reply-upload")
    {
        if (recvMsg["body"] == "shit-pass")
        {


            Json::Value task = getBlockList(recvMsg);

            int nowBlockSeq = 0, chunkNums = recvMsg["chunk"].size(), blockNums = task["block"].size();
            int everyChunkTaskNums = blockNums / chunkNums;
           // LOG_INFO <<" chunkNUms : " << chunkNums << " blockNums : " << blockNums;
            MutexLockGuard lock(mutex_);

            if(blockNums < chunkNums)
                everyChunkTaskNums = 1;
            for (int i = 0; i < chunkNums; i++)
            {
                InetAddress chunkAddr(recvMsg["chunk"][i]["ip"].asString(),
                                      static_cast<uint16_t>(recvMsg["chunk"][i]["port"].asInt()));
                Json::Value chunkTask;
                if (i == chunkNums - 1)
                {
                    for (; nowBlockSeq < blockNums; nowBlockSeq++)
                        chunkTask["block"].append((task["block"][nowBlockSeq]));
                } else
                for (int j = 0; j < everyChunkTaskNums && nowBlockSeq < blockNums; j++)
                {
                    chunkTask["block"].append(task["block"][nowBlockSeq]);
                    nowBlockSeq++;
                }
                if(chunkTask["block"].size() != 0)
                {
                    chunkTask["file-name"] = task["file-name"].asString();
                    chunkTask["file-size"] = task["file-size"].asInt64();
                    chunkTask["type"] = "upload-block";
                    chunkTask["file-md5"] = task["file-md5"].asString();
                    chunkClient_ = new ClientChunk(loopChunk_, chunkAddr, chunkTask);
                    QObject::connect(chunkClient_, &ClientChunk::LoadingUpdate,
                                     this, &Client::sendUpdateBar);
                    chunkClientList_.push_back(chunkClient_);
                    chunkClient_->connect();
                }
                if(nowBlockSeq > blockNums)
                    break;
            }
        }
        else if (recvMsg["body"] == "fast-pass")
        {
            sendTaskOver(recvMsg["file-name"].asString().c_str());
        }
    }

    if (recvMsg["type"] == "reply-download")
    {
        if (recvMsg["body"] == "ok")
        {

            taskMsg["type"] = "download";
            taskMsg["file-name"] = recvMsg["file-name"].asString();
            taskMsg["file-size"] = recvMsg["file-size"].asInt64();

            sendFileTask(taskMsg);

            Json::Value task = recvMsg;
            task["type"] = "download-block";
            std::map<std::pair<string, int>, Json::Value> chunkTaksMap;
            int blockNums = task["block"].size();
            LOG_INFO << task["block"].size() ;
            for (int i = 0; i < blockNums; i++)
            {
                std::pair<string, int> chunkPair = std::make_pair(task["block"][i]["ip"].asString(),
                                                                  task["block"][i]["port"].asInt());
                if (chunkTaksMap.count(chunkPair) == 0)
                {
                    Json::Value chunkTask;
                    chunkTask["file-name"] = task["file-name"].asString();
                    chunkTask["file-size"] = task["file-size"].asInt64();
                    chunkTask["block"].append(task["block"][i]);
                    chunkTask["type"] = "download-block";
                    chunkTaksMap[chunkPair] = chunkTask;
                }
                else
                {
                    chunkTaksMap[chunkPair]["block"].append(task["block"][i]);
                }
            }

            FILE *fp = NULL;
            string fileName  = "file/";
            fileName += task["file-name"].asString();
            LOG_INFO << fileName;
            fp = fopen(fileName.c_str(), "w");
            long long fileSize = task["file-size"].asInt64();
            fseek(fp, long(fileSize - 1), SEEK_SET);
            fwrite("S", 1, 1, fp);
            fclose(fp);
            MutexLockGuard lock(mutex_);
            for (auto &it : chunkTaksMap)
            {
                it.second["file-name"] = fileName;
                InetAddress chunkAddr(it.first.first,
                                      static_cast<uint16_t >(it.first.second));
                chunkClient_ = new ClientChunk(loopChunk_, chunkAddr, it.second);
                QObject::connect(chunkClient_, &ClientChunk::LoadingUpdate,
                                 this, &Client::sendUpdateBar);
                chunkClientList_.push_back(chunkClient_);
                chunkClient_->connect();
            }
        }
        else if (recvMsg["body"] == "no-file")
        {
            QMessageBox msgBox;
            msgBox.setText("no such file");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
        }
    }

    if(recvMsg["type"] == "reply-list")
    {
        Json::Value fileList = recvMsg;
        sendFileList(fileList);
    }
}

void Client::onTimer()
{
    string message = "{\"type\":\"heart-client\"}";
    codec_.send(boost::get_pointer(connection_), message, commTransType);
   /* for(std::vector<ClientChunk *>::iterator  it= chunkClientList_.begin(); it != chunkClientList_.end();)
    {
        if((*it)->done_)
        {

           // (*it)->disconnect();
            delete *(it);
            it = chunkClientList_.erase(it);
        }
        else it++;
    }*/
}

Json::Value Client::getBlockList(const Json::Value file)
{
    Json::Value root, one;
    long long offset = 0, blockSize;
    long long fileSize = file["file-size"].asInt64();
    string fileName = file["file-name"].asString();
    long long blockNums = (fileSize / S64M) + (fileSize % S64M != 0);
    string md5;
    root["file-name"] = fileName;
    root["file-md5"] = file["file-md5"].asString();
    root["type"] = "upload-block";
    for (int i = 0; i < blockNums; i++)
    {
        md5 = md5file64m(fileName.c_str(), offset);

        if (fileSize - offset >= S64M)
            blockSize = S64M;
        else
            blockSize = fileSize - offset;

        one["block-md5"] = md5;
        one["block-offset"] = offset;
        one["block-size"] = blockSize;
        string oneStr = writer_.write(one);
        offset += S64M;
        root["block"].append(one);
        string taskStr = writer_.write(root);
    }
    return root;
}


//*****************************************************************************

ClientChunk::ClientChunk(EventLoop *loop, const InetAddress &serverAddr, const Json::Value task) :
        task_(task),
        client_(loop, serverAddr, "ClientChunk"),
        codec_(boost::bind(&ClientChunk::onStringMessage, this, _1, _2, _3, _4))
{
    client_.setConnectionCallback(
            boost::bind(&ClientChunk::onConnection, this, _1));
    client_.setMessageCallback(
            boost::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
    client_.setWriteCompleteCallback(
            boost::bind(&ClientChunk::onWriteCompelte, this, _1));
    client_.enableRetry();
    done_ = false;
}


void ClientChunk::connect()
{
    client_.connect();
}


void ClientChunk::disconnect()
{
    client_.disconnect();
}

void ClientChunk::onConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");

    LOG_INFO << " chunk thread : " << CurrentThread::tid();
    CtxPtr ctx(new CtxType);
    ctx->type = commTransType;
    conn->setContext(ctx);

    if (conn->connected())
    {
        connection_ = conn;
        if (task_["type"] == "upload-block")
            sendBlock(conn, 0);
        else if (task_["type"] == "download-block")
            recvBlock(conn, 0);
    }
    else
    {
        connection_.reset();
    }
}

void ClientChunk::onStringMessage(const TcpConnectionPtr &conn, const string &message, Timestamp, int16_t type)
{

    if (type == fileTransType)
    {
        // LOG_INFO << conn->localAddress().toIpPort() << " from "
        //        << conn->peerAddress().toIpPort() << " recv " << message.size() << " bytes";

        const CtxPtr &ctx = boost::any_cast<const CtxPtr &>(conn->getContext());
        sendLoadingUpdate(QString(ctx->block["file-name"].asString().c_str()));
        if (ctx->type == recvType)
        {
            fwrite(message.c_str(), message.size(), 1, ctx->file.get());
        }
    }
    else
    {
      //  LOG_INFO << conn->localAddress().toIpPort() << " from "
        //         << conn->peerAddress().toIpPort() << " recv " << message.size() << " bytes is " << message;


        Json::Value msg;
        if (!reader_.parse(message, msg, false))
        {
            LOG_ERROR << "not a json";
        }
        if (msg["type"] == "reply-upload")
        {
            if (msg["body"] == "shit-pass")
            {
                Json::Value block = task_["block"][0];
                block["type"] = "upload-block";
                string sendMsg = writer_.write(task_["block"][0]);
            }
        }

        if (msg["type"] == "reply-download")
        {
            if (msg["body"] == "trans-over")
            {
                const CtxPtr &ctx = boost::any_cast<const CtxPtr &>(conn->getContext());
              //  LOG_INFO << " ctx->seq :  " << ctx->seq << " block nums : " << task_["block"].size();
                if (ctx->seq + 1 < task_["block"].size())
                {
                    recvBlock(conn, ctx->seq + 1);
                }
                else
                {
                    done_ = true;
                    disconnect();
                    deleteLater();
                }
            }
        }
    }
}

void ClientChunk::sendBlock(const TcpConnectionPtr &conn, int seq)
{
    char buf[BUFSIZE];
    Json::Value fileMsg = task_["block"][seq];
    fileMsg["type"] = "upload-block";
    fileMsg["file-name"] = task_["file-name"].asString();
    fileMsg["file-md5"] = task_["file-md5"].asString();
    string sendMsg = writer_.write(fileMsg);
    codec_.send(conn.get(), sendMsg, commTransType);
   // LOG_INFO << " file name : " << fileMsg["file-name"].asString() << " " << seq;
    long long offset = fileMsg["block-offset"].asInt64();
    FILE *fp = fopen(fileMsg["file-name"].asCString(), "rb");
    fseek(fp, offset, SEEK_SET);
    FilePtr file(fp, ::fclose);
    CtxPtr ctx(new CtxType);
    ctx->file = file;
    ctx->size = fileMsg["block-size"].asInt64();
    ctx->seq = seq;
    ctx->type = sendType;
    ctx->writed = 0;
    ctx->offset = fileMsg["block-offset"].asInt64();
    ctx->block = fileMsg;
    size_t nread = ::fread(buf, 1, sizeof(buf), fp);
    string bufString(buf, nread);
    sendLoadingUpdate(QString(ctx->block["file-name"].asString().c_str()));
    codec_.send(conn.get(), bufString, fileTransType);
    ctx->writed += nread;
    conn->setContext(ctx);

}


void ClientChunk::recvBlock(const TcpConnectionPtr &conn, int seq)
{

    Json::Value fileMsg = task_["block"][seq];
    string path  = "file/";
    path += fileMsg["file-name"].asString();
    FILE *fp = fopen(path.c_str(), "r+");
   // LOG_INFO << path ;
    fileMsg["type"] = "request-download";
    path = "block/";
    fileMsg["file-name"] = path + fileMsg["block-md5"].asString();
    long long offset = fileMsg["block-offset"].asInt64();
    fseek(fp, offset, SEEK_SET);
    FilePtr file(fp, ::fclose);
    CtxPtr ctx(new CtxType);
    ctx->file = file;
    ctx->size = fileMsg["block-size"].asInt64();
    ctx->seq = seq;
    ctx->type = recvType;
    ctx->writed = 0;
    ctx->offset = fileMsg["block-offset"].asInt64();
    ctx->block = fileMsg;

    string sendStr = writer_.write(fileMsg);

    codec_.send(conn.get(), sendStr, commTransType);
    ctx->block["file-name"] = task_["block"][seq]["file-name"].asString();
    conn->setContext(ctx);
}

void ClientChunk::onWriteCompelte(const TcpConnectionPtr &conn)
{
    const CtxPtr &ctx = boost::any_cast<const CtxPtr &>(conn->getContext());
    char buf[BUFSIZE];

    if (ctx->type == sendType)
    {
        size_t nread = ::fread(buf, 1, sizeof(buf), ctx->file.get());
        ctx->writed += nread;
        if (nread > 0 && ctx->writed <= ctx->size)
        {
            string bufString(buf, nread);
            codec_.send(conn.get(), bufString, fileTransType);
            sendLoadingUpdate(QString(ctx->block["file-name"].asString().c_str()));
            //  LOG_INFO << conn->localAddress().toIpPort() << " -> " << conn->peerAddress().toIpPort()
            //           << " send " << bufString.size() << "bytes";
        }
        else
        {
            ctx->block["type"] = "trans-over";
            string overMsg = writer_.write(ctx->block);
            ctx->file = nullptr;
            ctx->type = commTransType;
            codec_.send(conn.get(), overMsg, commTransType);
            if (task_["block"].size() > ctx->seq + 1)
                sendBlock(conn, ctx->seq + 1);
            else
            {
                done_ = true;
                LOG_INFO << conn->localAddress().toIpPort() << conn->peerAddress().toIpPort() << " send file -  done";
            }

        }
    }
    else
    {
        if(done_ == true)
        {
            disconnect();
            //disconnect();
            deleteLater();
        }
    }
}
