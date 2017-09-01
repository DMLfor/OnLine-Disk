#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "updown.h"
#include "client.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(EventLoop *loopMaster, const InetAddress &serverAddr,
                        EventLoop *loopChunkQWidget, QWidget *parent = 0);
    ~MainWindow();

    void clientConnect();

private slots:


    void on_signIn_clicked();

private:
    Ui::MainWindow *ui;
    UpDown uiUpDown;
};

#endif // MAINWINDOW_H
