#include "chunk.h"


int main(int argc, char *argv[])
{
    LOG_INFO << " pid = " << getpid();
    int idleSeconds = 10.0;
    if(argc > 1)
    {

        uint16_t  port = static_cast<uint16_t>(atoi(argv[2]));
        EventLoop loop;
        InetAddress listenAddr("192.168.234.133",port), serverAddr(argv[1] , 2333);
        Chunk chunk(&loop, listenAddr, serverAddr, idleSeconds);
        chunk.start();
        chunk.connect();
        loop.loop();
    }
    else
    {
        printf("Usage: %s host ip port\n", argv[0]);
    }
    return 0;
}