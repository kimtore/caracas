#include "mainscreen.hpp"


MainScreen::MainScreen()
{
    diagnostic_screen = new DiagnosticScreen;
    music_screen = new MusicScreen;
    map_screen = new MapScreen;

    icon_path = ICON_PATH;

    music_icon = new QIcon(icon_path + "music.png");
    map_icon = new QIcon(icon_path + "marble.png");
    diagnostic_icon = new QIcon(icon_path + "diagnostic.png");

    this->setObjectName("main_menu");
    this->setTabPosition(QTabWidget::East);
    this->setIconSize(QSize(80, 80));

    this->addTab(music_screen, *music_icon, "");
    this->addTab(map_screen, *map_icon, "");
    this->addTab(diagnostic_screen, *diagnostic_icon, "");
}
