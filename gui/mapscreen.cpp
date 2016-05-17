#include "mapscreen.hpp"

using namespace Marble;


#define MSEC_KPH_FACTOR 3.6
#define MID_SPEED_MEASUREMENTS 3


MapScreen::MapScreen()
{
    const PositionProviderPlugin * pp;

    setup_directions();
    mid_speed = 0;

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

    /* Zoom to world */
    map_widget->setZoom(map_widget->minimumZoom());
    map_widget->zoomIn();

    /* Speed widget */
    speed_widget = new QLabel();
    speed_widget->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
    info_layout->addWidget(speed_widget);

    /* Direction widget */
    direction_widget = new QLabel();
    direction_widget->setAlignment(Qt::AlignBottom | Qt::AlignCenter);
    info_layout->addWidget(direction_widget);

    /* Coordinate widget */
    coords_widget = new QLabel();
    coords_widget->setAlignment(Qt::AlignBottom | Qt::AlignRight);
    info_layout->addWidget(coords_widget);

    /* Set up button toolbar */
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

    QObject::connect(&auto_homing_button, &QPushButton::toggled,
                     this, &MapScreen::track_toggled);

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

QString
MapScreen::latlon_to_string(qreal n)
{
    return QString::number(n, 'f', 4);
}

void
MapScreen::register_speed(qreal speed)
{
    static qreal speeds[MID_SPEED_MEASUREMENTS] = {0};
    qreal r = 0;

    for (int i = 0; i < MID_SPEED_MEASUREMENTS - 1; i++) {
        speeds[i] = speeds[i + 1];
        r += speeds[i];
    }

    speeds[MID_SPEED_MEASUREMENTS - 1] = speed;
    r += speed;

    mid_speed = r / MID_SPEED_MEASUREMENTS;
}

void
MapScreen::zoom_for_speed(qreal speed)
{
    int zoom = 1;  /* minimum zoom level is one offset from OSM */

    if (speed >= 20.0) {
        zoom += 2;
    }

    if (speed >= 80.0) {
        zoom += 2;
    }

    map_widget->setZoom(map_widget->maximumZoom());

    while (zoom-- != 0) {
        map_widget->zoomOut();
    }
}

void
MapScreen::center_and_zoom()
{
    map_widget->centerOn(last_position);
    zoom_for_speed(mid_speed);
}

void
MapScreen::track_toggled(bool checked)
{
    if (!checked) {
        return;
    }

    center_and_zoom();
}

void
MapScreen::position_changed(GeoDataCoordinates position)
{
    qreal speed;

    speed = gpsd_provider_plugin->speed() * MSEC_KPH_FACTOR;
    register_speed(speed);

    speed_widget->setText(QString::number(speed, 'f', 1) + " km/h");

    direction_widget->setText(direction_from_heading(gpsd_provider_plugin->direction()));

    coords_widget->setText(
        latlon_to_string(position.latitude(GeoDataCoordinates::Degree)) +
        ", " +
        latlon_to_string(position.longitude(GeoDataCoordinates::Degree))
    );

    last_position = position;

    if (auto_homing_button.isChecked()) {
        center_and_zoom();
    }
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
