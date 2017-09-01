#include "master.h"

int main(int argc, char *argv[])
{
    LOG_INFO << " pid  = " << getpid();
    int idleSeconds = 180;
    if(argc > 1)
    {
        uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
        EventLoop loop;
        InetAddress listenAddr(port);
        MasterServer server(&loop, listenAddr, idleSeconds);
        server.start();
        loop.loop();
    }
    else
    {
        printf("Usgae : %s port\n",argv[0]);
    }
    return 0;
}
