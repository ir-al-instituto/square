#include "square.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Square w;
    w.show();
    return QApplication::exec();
}
