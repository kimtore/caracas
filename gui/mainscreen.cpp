#include "mainscreen.hpp"

MainScreen::MainScreen()
{
    diagnostic_screen = new DiagnosticScreen;
    music_screen = new QWidget;
    map_screen = new MapScreen;

    icon_path = ICON_PATH;
    map_icon = new QIcon(icon_path + "marble.png");
    //map_icon.rotate(270);

    this->setTabPosition(QTabWidget::East);
    this->setIconSize(QSize(80, 80));

    this->addTab(diagnostic_screen, "Diagnostics");
    this->addTab(music_screen, "Music");
    this->addTab(map_screen, *map_icon, "");
}

