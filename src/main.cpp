#include <QApplication>
#include "ui/mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    app.setOrganizationName("SFINGE");
    app.setOrganizationDomain("sfinge.org");
    app.setApplicationName("SFINGE-Qt6");
    app.setApplicationVersion("1.0.0");
    
    SFinGe::MainWindow window;
    window.show();
    
    return app.exec();
}
