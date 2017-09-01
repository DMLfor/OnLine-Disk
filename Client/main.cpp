#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QMetaType>
#include "mainwindow.h"
#include "client.h"

int main(int argc, char *argv[])
{
    LOG_INFO << " pid = " << getpid();
    if(argc > 1)
    {
        LOG_INFO << " main thread : " << CurrentThread::tid();
        qRegisterMetaType<Json::Value>("Json::Value");
        QApplication app(argc, argv);
        EventLoopThread loopThreadMaster, loopThreadChunk;
        uint16_t port = 2333;
        InetAddress serverAddr(argv[1], port);
        MainWindow win(loopThreadMaster.startLoop(), serverAddr, loopThreadChunk.startLoop());
        win.clientConnect();
        win.show();
        return app.exec();
    }
    else
        LOG_INFO << " please input master ip";
}
