#include <QApplication>
#include <QPushButton>
#include "src/Client/qt_client_dialog.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QtClientDialog::QtClientDialog foo;
    foo.setDefaultInfo("default","12345","127.0.0.1",kServerPort);
    foo.show();

    return QApplication::exec();
}
