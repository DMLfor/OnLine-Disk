#pragma once

#include <jsoncpp/json/json.h>
#include <cstdio>
#include <string>
#include <muduo/base/Logging.h>
using namespace muduo;
using namespace muduo::net;
class JsonHandle
{
public:
    Json::Reader reader;
    Json::FastWriter writer;
    Json::Value value;

    void resolveJson(const Json::Value &root, string &str)
    {
        str = writer.write(root);
    }

    void getJson(const string &str, Json::Value &root)
    {
        if(!reader.parse(str, root, false))
        {
            LOG_ERROR << " not a json ";
        }
    }
};