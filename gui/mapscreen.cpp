#include <QVBoxLayout>

#include "mapscreen.hpp"

using namespace Marble;


MapScreen::MapScreen()
{
    layout = new QVBoxLayout(this);

    map_widget = new MarbleWidget();
    map_widget->setMapThemeId("earth/openstreetmap/openstreetmap.dgml");

    layout->addWidget(map_widget);
}
