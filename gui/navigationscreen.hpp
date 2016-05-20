#include <QApplication>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <marble/AbstractFloatItem.h>
#include <marble/GeoDataDocument.h>
#include <marble/GeoDataPlacemark.h>
#include <marble/MarbleGlobal.h>
#include <marble/MarbleModel.h>
#include <marble/MarbleWidget.h>
#include <marble/MarbleWidgetInputHandler.h>
#include <marble/PluginManager.h>
#include <marble/PositionProviderPlugin.h>
#include <marble/PositionTracking.h>
#include <marble/RouteRequest.h>
#include <marble/RoutingManager.h>
#include <marble/RoutingModel.h>


#ifndef _GUI_NAVIGATIONSCREEN_H_
#define _GUI_NAVIGATIONSCREEN_H_


using namespace Marble;


class NavigationScreen : public QWidget
{
    Q_OBJECT

public:
    NavigationScreen();

    QString direction_from_heading(qreal heading);
    QString latlon_to_string(qreal n);
    QString position_to_string(const GeoDataCoordinates & position);

    qreal mid_speed;

    QVBoxLayout * layout;
    QHBoxLayout * main_layout;
    QVBoxLayout * button_layout;
    QHBoxLayout * info_layout;

    MarbleWidget * map_widget;
    QLabel * speed_widget;
    QLabel * direction_widget;
    QLabel * coords_widget;

    QPushButton auto_homing_button;
    QPushButton zoom_in_widget;
    QPushButton zoom_out_widget;
    QPushButton search_button_widget;

    QStringList directions;

    PositionProviderPlugin * gpsd_provider_plugin;
    GeoDataCoordinates last_position;

    void setup_directions();
    void register_speed(qreal speed);
    void zoom_for_speed(qreal speed);
    void center_and_zoom();
    void route_state_changed(RoutingManager::State state);

    /* TEMPORARY */
    void navigate_to();

public slots:
    void position_changed(GeoDataCoordinates position);
    void zoom_in();
    void zoom_out();
    void track_toggled(bool checked);

private:
};


#endif /* _GUI_NAVIGATIONSCREEN_H_ */
