#ifndef UPDOWN_H
#define UPDOWN_H

#include <QDialog>
#include <QThread>
#include <QMutex>
#include <QListWidget>
#include <jsoncpp/json/json.h>
#include <QProgressBar>
#include <QDebug>
#include <QMap>
#include "client.h"

typedef std::pair<string , int> OneTask;

namespace Ui {
class UpDown;
}
class WorkerThread : public QThread
{
    Q_OBJECT
public:
    WorkerThread(Client *client, QObject *parent = 0)
        : QThread(parent),client_(client)
    {
    }

    void sendFileTask(const Json::Value &task)
    {
        emit FileTask(task);
    }

private:
    Client *client_;
protected:
    void run();
signals:
    void done();
    void FileTask(const Json::Value &task);
};

class UpDown : public QDialog
{
    Q_OBJECT
public:
    explicit UpDown(EventLoop *loopMaster, const InetAddress &serverAddr,
                    EventLoop *loopChunkQWidget, QWidget *parent = 0);
    ~UpDown();

    void receiveFileList(const Json::Value fileList);

    void setProgressBar(const Json::Value & oneTask);

    void updateFileList()
    {
        string sendStr = "{\"type\":\"request-list\"}";
        client_.write(sendStr, commTransType);
    }

    void updateloading(const QString &fileName);

    void updateTaskOver(const QString &fileName);

    Client client_;

    WorkerThread *thread_;

private slots:

    void on_openFile_clicked();

    void on_upload_clicked();

    void on_download_clicked();

    void on_fileListWidge_itemClicked(QListWidgetItem *item);

    void on_downloadedList_itemDoubleClicked(QListWidgetItem *item);

    void on_fileListWidge_itemDoubleClicked(QListWidgetItem *item);

private:

    typedef QMap<QListWidgetItem *, QProgressBar *> QMapBarList;
    typedef QMap<QProgressBar *, Timestamp> QMapTime;
    typedef QMap<QProgressBar *, int> QMapLoadM;
    QMapTime progressBarToTime;
    QMapLoadM progressBarToLoadM;
    QMapBarList progressBarToList;
    QMap<QProgressBar *,QString> progressBarToV;
    Ui::UpDown *ui;
};

#endif // UPDOWN_H
