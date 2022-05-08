#include <QApplication>

#include <showlib/CommonUsing.h>

#include "Configuration.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    Configuration::singleton();
    std::string filename;

    if (argc > 1 && argv[1][0] != '-') {
        filename = argv[1];
        for (int index = 1; index + 1 < argc; ++index) {
            argv[index] = argv[index + 1];
        }
        --argc;
    }

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    if ( !filename.empty() ) {
        w.load(filename);
    }

    return a.exec();
}
