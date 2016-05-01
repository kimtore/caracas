#include <QApplication>

#include "mainscreen.hpp"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    MainScreen *main_screen = new MainScreen;
    
    main_screen->show();

    app.exec();
}
