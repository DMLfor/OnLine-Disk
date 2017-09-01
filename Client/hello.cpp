#include "hello.h"
#include <QAction>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <iostream>

Client::~Client()
{

}

void Client::show()
{
    uiSignUp.show();
}
void Client::on_UpDown_signIn_clicked()
{
    uiSignUp.close();
    uiSignUp.hide();
    uiUpDown.show();
  //  uiUpDown.exec();
}
