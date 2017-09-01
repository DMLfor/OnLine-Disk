#include "uplordwin.h"
#include "ui_uplordwin.h"

uplordWin::uplordWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::uplordWin)
{
    ui->setupUi(this);
}

uplordWin::~uplordWin()
{
    delete ui;
}
