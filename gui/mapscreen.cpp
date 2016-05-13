#include "mapscreen.hpp"

using namespace Marble;


#define MSEC_KPH_FACTOR 3.6

#define ZOOM_SPEED_DELOREAN 2634
#define ZOOM_SPEED_HIGH     2773
#define ZOOM_SPEED_MEDIUM   2911
#define ZOOM_SPEED_LOW      3050

#define THRESHOLD_SPEED_DELOREAN 100.0
#define THRESHOLD_SPEED_HIGH     70.0
#define THRESHOLD_SPEED_MEDIUM   40.0


MapScreen::MapScreen()
{
    const PositionProviderPlugin * pp;

    setup_directions();

    layout = new QVBoxLayout(this);
    main_layout = new QHBoxLayout();
    info_layout = new QHBoxLayout();
    info_layout->setAlignment(Qt::AlignBottom);
    button_layout = new QVBoxLayout();
    button_layout->setObjectName("map_buttons");
    button_layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    map_widget = new MarbleWidget();
    map_widget->setMapThemeId("earth/openstreetmap/openstreetmap.dgml");
    map_widget->setProjection(Marble::Mercator);

    /* Hide stuff */
    foreach (AbstractFloatItem * item, map_widget->floatItems()) {
        item->setVisible(false);
    }
    map_widget->setShowCrosshairs(false);
    map_widget->setShowGrid(false);
    map_widget->setShowOtherPlaces(false);
    map_widget->setShowRelief(false);

    /* Don't animate anything */
    map_widget->inputHandler()->setInertialEarthRotationEnabled(false);
    map_widget->setAnimationsEnabled(false);

    /* Plugin manager */
    gpsd_provider_plugin = NULL;
    foreach(pp, map_widget->model()->pluginManager()->positionProviderPlugins()) {
        if (pp->nameId() == "Gpsd") {
            gpsd_provider_plugin = pp->newInstance();
        }
    }

    if (!gpsd_provider_plugin) {
        qDebug() << "Unable to find GPSd among position provider plugins, aborting.";
        abort();
    }

    map_widget->model()->positionTracking()->setPositionProviderPlugin(gpsd_provider_plugin);

    /* Speed widget */
    speed_widget = new QLabel();
    speed_widget->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
    info_layout->addWidget(speed_widget);

    /* Direction widget */
    direction_widget = new QLabel();
    direction_widget->setAlignment(Qt::AlignBottom);
    info_layout->addWidget(direction_widget);

    /* Latitude widget */
    lat_widget = new QLabel();
    lat_widget->setAlignment(Qt::AlignBottom | Qt::AlignRight);
    info_layout->addWidget(lat_widget);

    /* Longitude widget */
    lon_widget = new QLabel();
    lon_widget->setAlignment(Qt::AlignBottom | Qt::AlignRight);
    info_layout->addWidget(lon_widget);

    auto_homing_button.setText("Track");
    auto_homing_button.setCheckable(true);
    auto_homing_button.setChecked(true);
    button_layout->addWidget(&auto_homing_button);
    zoom_in_widget.setText("+");
    button_layout->addWidget(&zoom_in_widget);
    zoom_out_widget.setText("-");
    button_layout->addWidget(&zoom_out_widget);
    search_button_widget.setText("Search");
    button_layout->addWidget(&search_button_widget);

    /* Set up main layout */
    main_layout->addLayout(button_layout);
    main_layout->addWidget(map_widget);
    main_layout->setStretch(1, 100);
    layout->addLayout(main_layout);
    layout->addLayout(info_layout);
    layout->setStretch(0, 100);

    QObject::connect(gpsd_provider_plugin, &PositionProviderPlugin::positionChanged,
                     this, &MapScreen::position_changed);

    QObject::connect(&zoom_in_widget, &QPushButton::clicked,
                     this, &MapScreen::zoom_in);

    QObject::connect(&zoom_out_widget, &QPushButton::clicked,
                     this, &MapScreen::zoom_out);

}

void
MapScreen::zoom_in()
{
    auto_homing_button.setChecked(false);
    map_widget->zoomIn();
}

void
MapScreen::zoom_out()
{
    auto_homing_button.setChecked(false);
    map_widget->zoomOut();
}

void
MapScreen::position_changed(GeoDataCoordinates position)
{
    qreal speed;

    speed = gpsd_provider_plugin->speed();
    speed_widget->setText(
        "Speed: " +
        QString::number(gpsd_provider_plugin->speed() * MSEC_KPH_FACTOR, 'f', 1) +
        " km/h"
    );

    direction_widget->setText("Heading: " + direction_from_heading(gpsd_provider_plugin->direction()));
    lat_widget->setText("Lat: " + QString::number(position.latitude(GeoDataCoordinates::Degree), 'f', 4));
    lon_widget->setText("Lon: " + QString::number(position.longitude(GeoDataCoordinates::Degree), 'f', 4));

    if (!auto_homing_button.isChecked()) {
        return;
    }

    if (speed > THRESHOLD_SPEED_DELOREAN) {
        map_widget->setZoom(ZOOM_SPEED_DELOREAN);
    } else if (speed > THRESHOLD_SPEED_HIGH) {
        map_widget->setZoom(ZOOM_SPEED_HIGH);
    } else if (speed > THRESHOLD_SPEED_MEDIUM) {
        map_widget->setZoom(ZOOM_SPEED_MEDIUM);
    } else {
        map_widget->setZoom(ZOOM_SPEED_LOW);
    }

    map_widget->centerOn(position);
}

void
MapScreen::setup_directions()
{
    directions.push_back("North");
    directions.push_back("North-northeast");
    directions.push_back("Northeast");
    directions.push_back("East-northeast");
    directions.push_back("East");
    directions.push_back("East-southeast");
    directions.push_back("Southeast");
    directions.push_back("South-southeast");
    directions.push_back("South");
    directions.push_back("South-southwest");
    directions.push_back("Southwest");
    directions.push_back("West-southwest");
    directions.push_back("West");
    directions.push_back("West-northwest");
    directions.push_back("Northwest");
    directions.push_back("North-northwest");
}

QString
MapScreen::direction_from_heading(qreal heading)
{
    int index = (int)((heading/22.5)+.5) % 16;
    return directions[index];
}
