#include <QStackedWidget>

#include <marble/GeoDataPlacemark.h>
#include <marble/SearchRunnerManager.h>

#include "navigationscreen.hpp"
#include "searchscreen.hpp"
#include "listscreen.hpp"


#ifndef _GUI_MAPSCREEN_H_
#define _GUI_MAPSCREEN_H_


class MapScreen : public QStackedWidget
{
    Q_OBJECT

public:
    MapScreen();

    void perform_search(QString term, qreal distance);

    NavigationScreen * navigation_screen;
    SearchScreen * search_screen;
    ListScreen * results_screen;
    SearchRunnerManager * search_manager;

    QString last_search_term;
    qreal last_search_distance;

public slots:

    void search_result_changed(const QVector<GeoDataPlacemark *> &result);
    void perform_search_slot(QString term);
    void start_navigation(QListWidgetItem * item);
    void show_search();
    void hide_search();
    qreal km_to_rad(qreal km);
};


#endif /* _GUI_MAPSCREEN_H_ */
