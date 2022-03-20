#include <QApplication>
#include <QPushButton>
#include "src/Server/qt_server_dialog.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QtServerDialog::QtServerDialog foo;
    foo.show();
    foo.setDefaultInfo("127.0.0.1",kServerPort);
    return QApplication::exec();
}
