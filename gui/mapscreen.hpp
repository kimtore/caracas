#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <marble/AbstractFloatItem.h>
#include <marble/MarbleModel.h>
#include <marble/PositionProviderPlugin.h>
#include <marble/PluginManager.h>
#include <marble/MarbleGlobal.h>
#include <marble/MarbleWidget.h>
#include <marble/MarbleWidgetInputHandler.h>
#include <marble/PositionTracking.h>

using namespace Marble;


class MapScreen : public QWidget
{
    Q_OBJECT

public:
    MapScreen();

    QString direction_from_heading(qreal heading);
    QString latlon_to_string(qreal n);

    qreal mid_speed;

public slots:
    void position_changed(GeoDataCoordinates position);
    void zoom_in();
    void zoom_out();
    void track_toggled(bool checked);

private:
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
};
