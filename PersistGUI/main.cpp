#include "MainWindow.h"

#include <QApplication>
#include "Configuration.h"

int main(int argc, char *argv[])
{
    Configuration::singleton();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
