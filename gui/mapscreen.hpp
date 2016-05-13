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

public slots:
    void position_changed(GeoDataCoordinates position);
    void zoom_in();
    void zoom_out();

private:
    QVBoxLayout * layout;
    QHBoxLayout * main_layout;
    QVBoxLayout * button_layout;
    QHBoxLayout * info_layout;

    MarbleWidget * map_widget;
    QLabel * speed_widget;
    QLabel * direction_widget;
    QLabel * lat_widget;
    QLabel * lon_widget;

    QPushButton auto_homing_button;
    QPushButton zoom_in_widget;
    QPushButton zoom_out_widget;
    QPushButton search_button_widget;

    QStringList directions;

    PositionProviderPlugin * gpsd_provider_plugin;

    void setup_directions();
};
