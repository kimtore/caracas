#include "mapscreen.hpp"


#define MAX_DISTANCE 1000.0
#define DISTANCE_INCREASE_FACTOR 2.0


MapScreen::MapScreen()
{
    navigation_screen = new NavigationScreen();
    search_screen = new SearchScreen();
    results_screen = new ListScreen();
    search_manager = new SearchRunnerManager(navigation_screen->map_widget->model());

    addWidget(navigation_screen);
    addWidget(search_screen);
    addWidget(results_screen);

    QObject::connect(&(navigation_screen->search_button_widget), &QPushButton::clicked,
                     this, &MapScreen::show_search);

    QObject::connect(search_screen, &SearchScreen::search,
                     this, &MapScreen::perform_search_slot);

    QObject::connect(search_manager, static_cast<void (SearchRunnerManager::*)(const QVector<GeoDataPlacemark *> &)>(&SearchRunnerManager::searchResultChanged),
                     this, &MapScreen::search_result_changed);

    QObject::connect(results_screen, &ListScreen::itemClicked,
                     this, &MapScreen::start_navigation);
}

void
MapScreen::perform_search(QString term, qreal distance)
{
    GeoDataLatLonBox box;
    GeoDataCoordinates location;

    setCurrentIndex(indexOf(results_screen));

    last_search_term = term;
    last_search_distance = distance;

    if (distance > MAX_DISTANCE) {
        search_manager->findPlacemarks(term);
        return;
    }

    location = navigation_screen->map_widget->focusPoint();
    box.setNorth(location.latitude() - km_to_rad(distance));
    box.setSouth(location.latitude() + km_to_rad(distance));
    box.setEast(location.longitude() - km_to_rad(distance));
    box.setWest(location.longitude() + km_to_rad(distance));

    qDebug() << "Box north:" << box.north(GeoDataCoordinates::Degree);
    qDebug() << "Box south:" << box.south(GeoDataCoordinates::Degree);
    qDebug() << "Box west:" << box.west(GeoDataCoordinates::Degree);
    qDebug() << "Box east:" << box.east(GeoDataCoordinates::Degree);

    search_manager->findPlacemarks(term, box);
}

void
MapScreen::perform_search_slot(QString term)
{
    perform_search(term, 10.0);
}

void
MapScreen::search_result_changed(const QVector<GeoDataPlacemark *> &result)
{
    QString name;

    results_screen->clear();
    results_screen->coordinates.clear();

    qDebug() << "Start search results";

    foreach (GeoDataPlacemark * m, result) {
        //if (!mark->hasOsmData() || ((name = mark->osmData().tagValue("name")) == "")) {
            name = m->coordinate().toString(Marble::GeoDataCoordinates::Decimal).trimmed();
        //}
        results_screen->addItem(name);
        results_screen->coordinates.append(m->coordinate());
        qDebug() << "Search result:" << name;
    }
    qDebug() << "End search results";

    if (result.size() == 0) {
        if (last_search_distance > MAX_DISTANCE) {
            qDebug() << "Giving up because max distance" << MAX_DISTANCE << "has been exceeded.";
            hide_search();
            return;
        }
        qDebug() << "Retrying search with distance" << last_search_distance * DISTANCE_INCREASE_FACTOR;
        perform_search(last_search_term, last_search_distance * DISTANCE_INCREASE_FACTOR);
    }
}

void
MapScreen::start_navigation(QListWidgetItem * item)
{
    GeoDataCoordinates coordinates;
    RoutingManager * rm;
    RouteRequest * rq;

    setCurrentIndex(indexOf(navigation_screen));

    coordinates = results_screen->coordinates[results_screen->row(item)];

    rm = navigation_screen->map_widget->model()->routingManager();
    rq = rm->routeRequest();

    rq->clear();
    rq->setRoutingProfile(rm->defaultProfile(Marble::RoutingProfile::Motorcar));
    rq->append(navigation_screen->last_position);
    rq->append(coordinates);
    rm->setShowGuidanceModeStartupWarning(false);
    rm->setGuidanceModeEnabled(true);
    rm->retrieveRoute();
}

void
MapScreen::show_search()
{
    setCurrentIndex(indexOf(search_screen));
}

void
MapScreen::hide_search()
{
    setCurrentIndex(indexOf(navigation_screen));
}

qreal
MapScreen::km_to_rad(qreal km)
{
    return km / 6371.0;
}
