#ifndef UPLORDWIN_H
#define UPLORDWIN_H

#include <QMainWindow>

namespace Ui {
class uplordWin;
}

class uplordWin : public QMainWindow
{
    Q_OBJECT

public:
    explicit uplordWin(QWidget *parent = 0);
    ~uplordWin();

private:
    Ui::uplordWin *ui;
};

#endif // UPLORDWIN_H
