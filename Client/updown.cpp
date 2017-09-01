#include "updown.h"
#include "ui_updown.h"
#include <QtGui>
#include <QtWidgets>
#include <QSize>
#include <iostream>
#include <QSound>

std::list<QString> taskList;
QMutex mutex;
QWaitCondition taskListNotEmpty;

UpDown::UpDown(EventLoop *loopMaster, const InetAddress &serverAddr,
               EventLoop *loopChunkQWidget, QWidget *parent) :
    QDialog(parent),
    client_(loopMaster, serverAddr, loopChunkQWidget),
    ui(new Ui::UpDown)
{

    ui->setupUi(this);

}

UpDown::~UpDown()
{
    delete ui;
}

void UpDown::on_openFile_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open File"), "/home/dblank");
    ui->uploadPath->setText(path);
}

void UpDown::on_upload_clicked()
{
    QString uploadPath = ui->uploadPath->text();
    if(uploadPath.size() == 0)
    {
        if(QMessageBox::Ok ==  QMessageBox::warning(this, tr("Warning"), tr("path is NULL!"),
                                                    QMessageBox::Ok, QMessageBox::Ok))
        {
            ;
        }
    }
    else
    {
        int listSize = ui->loadingList->count();
        for(int i = 0; i<listSize; i++)
        {
            QListWidgetItem *nitem = ui->loadingList->item(i);
            QProgressBar *oneQProgressBar =  progressBarToList[nitem];

            if(oneQProgressBar == nullptr)
                continue;

            QString format = oneQProgressBar->format();

            if(format.contains(uploadPath))
            {
                if(QMessageBox::Ok ==  QMessageBox::warning(this, tr("Warning"), tr("The file is loading!"),
                                                            QMessageBox::Ok, QMessageBox::Ok))
                {
                    ;
                }
                return ;
            }
        }
        struct stat st;
        if(stat(uploadPath.toStdString().c_str(), &st)==-1)
        {
            if(QMessageBox::Ok ==  QMessageBox::warning(this, tr("Warning"), tr("No such file!"),
                                                        QMessageBox::Ok, QMessageBox::Ok))
            {
                ;
            }
            return ;
        }

        Json::Value taskMsg;
        taskMsg["type"] = "upload";
        taskMsg["file-name"] = uploadPath.toStdString();
        taskMsg["file-size"] = static_cast<long long>(st.st_size);
        thread_->sendFileTask(taskMsg);

        mutex.lock();
        taskList.push_back(uploadPath);
        taskListNotEmpty.wakeOne();
        mutex.unlock();
    }
}

void UpDown::receiveFileList(const Json::Value fileList)
{
    ui->fileListWidge->clear();
    for(int i = 0; i<fileList["file"].size(); i++)
    {
        new QListWidgetItem(QIcon(":/new/photo/file.png"), fileList["file"][i].asString().c_str(), ui->fileListWidge);
    }
    this->update();
}

void UpDown::on_download_clicked()
{
    QString downloadPath = ui->downloadPath->text();
    if(downloadPath.size() == 0)
    {
        if(QMessageBox::Ok ==  QMessageBox::warning(this, tr("Warning"), tr("path is NULL!"),
                                                    QMessageBox::Ok, QMessageBox::Ok))
        {
            ;
        }
    }
    else
    {
        for(int i = 0; i<ui->downloadedList->count(); i++)
        {
            QListWidgetItem *nitem = ui->downloadedList->item(i);
            if(nitem->text() == downloadPath)
            {
                if(QMessageBox::Ok ==  QMessageBox::warning(this, tr("Warning"), tr("downloaded!"),
                                                            QMessageBox::Ok, QMessageBox::Ok))
                {
                    ;
                }
                return ;
            }
        }

        Json::Value sendMsg;
        Json::FastWriter writer;
        std::string sendStr;
        sendMsg["type"] = "request-download";
        sendMsg["body"] = "please";
        sendMsg["file-name"] = downloadPath.toStdString();
        sendStr = writer.write(sendMsg);
        client_.write(sendStr, commTransType);

    }
}

void WorkerThread::run()
{
    while(1)
    {
        if(taskList.empty())
        {
            mutex.lock();
            taskListNotEmpty.wait(&mutex);
            mutex.unlock();
        }
        if(!taskList.empty())
        {

            mutex.lock();
            QString oneTask = taskList.front();
            taskList.pop_front();
            mutex.unlock();

            std::string md5Str, sendStr;
            Json::Value sendMsg, taskMsg;
            Json::FastWriter writer;

            struct stat st;
            stat(oneTask.toStdString().c_str(), &st);
            long long fileSize = static_cast<long long>(st.st_size);

            md5Str = md5file(oneTask.toStdString().c_str());
            sendMsg["body"] = "please";
            sendMsg["type"] = "request-upload";
            sendMsg["file-name"] = oneTask.toStdString();
            sendMsg["file-md5"] = md5Str;
            sendMsg["file-size"] = fileSize;
            sendMsg["block-nums"] = fileSize / S64M + (fileSize % S64M != 0);
            sendStr = writer.write(sendMsg);

            client_->write(sendStr, commTransType);
        }
    }
}

void UpDown::on_fileListWidge_itemClicked(QListWidgetItem *item)
{
    QString fileName = item->text();
    ui->downloadPath->setText(fileName);
}

void UpDown::setProgressBar(const Json::Value &oneTask)
{
    long long fileSzie = oneTask["file-size"].asInt64();
    std::string fileName = oneTask["file-name"].asString();
    int fileSizeM = std::max(static_cast<int>(fileSzie / (1024*1024)), 1);

    QProgressBar *oneQProgressBar = new QProgressBar(ui->loadingList);

    oneQProgressBar->setRange(0, fileSizeM);
    oneQProgressBar->setValue(0);

    QListWidgetItem *nitem = new QListWidgetItem(ui->loadingList);
    ui->loadingList->addItem(nitem);
    ui->loadingList->setItemWidget(nitem,oneQProgressBar);

    progressBarToList[nitem] = oneQProgressBar;
    progressBarToTime[oneQProgressBar] = Timestamp::now();
    progressBarToLoadM[oneQProgressBar] = 0;
    progressBarToV[oneQProgressBar] = "0.0Mb/S";

    nitem->setSizeHint(QSize(760,45));
    double dProgress = (oneQProgressBar->value() - oneQProgressBar->minimum()) * 100.0
                    / (oneQProgressBar->maximum() - oneQProgressBar->minimum());
    if(oneTask["type"] == "upload")
    {
        oneQProgressBar->setFormat(QString("uploading: %1 %2%").arg(fileName.c_str())
                                   .arg(QString::number(dProgress, 'f', 1)));
    }
    else if(oneTask["type"] == "download")
    {
        oneQProgressBar->setFormat(QString("downloading: %1 %2%").arg(fileName.c_str())
                                   .arg(QString::number(dProgress, 'f', 1)));
    }
    ui->tabWidget->setTabText(2, QString("正在传输(%1)").arg(ui->loadingList->count()));
}

void UpDown::updateloading(const QString &fileName)
{
    int listSize = ui->loadingList->count();
    for(int i = 0; i<listSize; i++)
    {
        QListWidgetItem *nitem = ui->loadingList->item(i);
        QProgressBar *oneQProgressBar =  progressBarToList[nitem];

        if(oneQProgressBar == nullptr)
            continue;

        QString format = oneQProgressBar->format();

        if(format.contains(fileName))
        {
            oneQProgressBar->setValue(oneQProgressBar->value()+1);

            progressBarToLoadM[oneQProgressBar] ++;

            double dProgress = (oneQProgressBar->value() - oneQProgressBar->minimum()) * 100.0
                    / (oneQProgressBar->maximum() - oneQProgressBar->minimum());

            int64_t subTime = Timestamp::now().microSecondsSinceEpoch() -
                    progressBarToTime[oneQProgressBar].microSecondsSinceEpoch();

            double v = 1000000.0*progressBarToLoadM[oneQProgressBar] /subTime;

            QString vString;

            if(subTime > 500000)
            {
                progressBarToTime[oneQProgressBar] = Timestamp::now();
                progressBarToLoadM[oneQProgressBar] = 0;
                if(v > 1)
                {
                    vString = QString(" %1 %2").arg(QString::number(v, 'f', 1)).arg("Mb/S");
                }
                else
                    vString = QString(" %1 %2").arg(QString::number(v*1024, 'f', 1)).arg("Kb/S");
                progressBarToV[oneQProgressBar] = vString;

            }
            if(format.contains("uploading"))
                oneQProgressBar->setFormat(QString("uploading: %1 %2%%3").arg(fileName)
                                           .arg(QString::number(dProgress, 'f', 1)).arg(progressBarToV[oneQProgressBar]));
            else
                oneQProgressBar->setFormat(QString("downloading: %1 %2%%3").arg(fileName)
                                           .arg(QString::number(dProgress, 'f', 1)).arg(progressBarToV[oneQProgressBar]));
            if(oneQProgressBar->value() >= oneQProgressBar->maximum())
            {

                progressBarToV.erase(progressBarToV.find(oneQProgressBar));
                progressBarToTime.erase(progressBarToTime.find(oneQProgressBar));
                progressBarToLoadM.erase(progressBarToLoadM.find(oneQProgressBar));
                progressBarToList.erase(progressBarToList.find(nitem));
                ui->loadingList->removeItemWidget(nitem),delete nitem;
                delete oneQProgressBar;

                ui->tabWidget->setTabText(2, QString("正在传输(%1)").arg(ui->loadingList->count()));
                QSound::play(":/new/photo/sound.wav");
                if(format.contains("uploading"))
                {
                    updateFileList();
                }
                else
                {

                    new QListWidgetItem(QIcon(":/new/photo/file.png"), fileName, ui->downloadedList);
                    ui->tabWidget->setTabText(1, QString("已下载(%1)").arg(ui->downloadedList->count()));
                }
            }
        }
    }
}

void UpDown::on_downloadedList_itemDoubleClicked(QListWidgetItem *item)
{
    std::string url = "file/";
    url += item->text().toStdString();
    QDesktopServices::openUrl(QUrl(url.c_str()));
}

void UpDown::on_fileListWidge_itemDoubleClicked(QListWidgetItem *item)
{
    on_download_clicked();
}

void UpDown::updateTaskOver(const QString &fileName)
{
    int listSize = ui->loadingList->count();
    for(int i = 0; i<listSize; i++)
    {
        QListWidgetItem *nitem = ui->loadingList->item(i);
        QProgressBar *oneQProgressBar =  progressBarToList[nitem];

        if(oneQProgressBar == nullptr)
            continue;
        QString format = oneQProgressBar->format();

        if(format.contains(fileName))
        {
            progressBarToV.erase(progressBarToV.find(oneQProgressBar));
            progressBarToTime.erase(progressBarToTime.find(oneQProgressBar));
            progressBarToLoadM.erase(progressBarToLoadM.find(oneQProgressBar));
            progressBarToList.erase(progressBarToList.find(nitem));
            ui->loadingList->removeItemWidget(nitem);
            delete nitem;
            delete oneQProgressBar;

            ui->tabWidget->setTabText(2, QString("正在传输(%1)").arg(ui->loadingList->count()));

            if(format.contains("uploading"))
            {
                updateFileList();
            }
        }
    }
}
