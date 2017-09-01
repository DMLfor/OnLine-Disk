#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QAction>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <iostream>
#include <QtGui>
#include <QtWidgets>
#include <QMetaType>

MainWindow::MainWindow(EventLoop *loopMaster, const InetAddress &serverAddr,
                       EventLoop *loopChunk, QWidget *parent) :
    QMainWindow(parent),
    uiUpDown(loopMaster, serverAddr, loopChunk),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->signInP->setScaledContents(true);
    ui->signInP->setPixmap(QPixmap("/home/dblank/图片/timg.jpg"));

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_signIn_clicked()
{
    //this->close();
    this->hide();
    WorkerThread *thread = new WorkerThread(&uiUpDown.client_);

    thread->start();
    uiUpDown.thread_ = thread;
    QObject::connect(&uiUpDown.client_, &Client::FileList,
                         &uiUpDown, &UpDown::receiveFileList);
    QObject::connect(&uiUpDown.client_, &Client::FileTask,
                         &uiUpDown, &UpDown::setProgressBar);
    QObject::connect(thread, &WorkerThread::FileTask,
                         &uiUpDown, &UpDown::setProgressBar); 
    QObject::connect(&uiUpDown.client_, &Client::UpdateBar,
                         &uiUpDown, &UpDown::updateloading);
    QObject::connect(&uiUpDown.client_, &Client::TaskOver,
                         &uiUpDown, &UpDown::updateTaskOver);
    uiUpDown.updateFileList();
    uiUpDown.show();
    uiUpDown.update();
}

void MainWindow::clientConnect()
{
    uiUpDown.client_.connect();
}
