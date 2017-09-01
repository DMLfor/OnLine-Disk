#ifndef HELLO_H
#define HELLO_H
#include <QMainWindow>
#include "mainwindow.h"
#include "uplordwin.h"

class Client
{
public:
    Client(){ };
    ~Client();
    void show();
private:
   void on_UpDown_signIn_clicked();
   uplordWin uiUpDown;
   MainWindow uiSignUp;
};

#endif // HELLO_H
