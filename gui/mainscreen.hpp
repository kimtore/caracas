#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QTimer>

#include "diagnosticscreen.hpp"
#include "mapscreen.hpp"
#include "musicscreen.hpp"
#include "mpdclient.hpp"

#define ICON_PATH "/usr/local/lib/caracas/icons/"

class MainScreen : public QTabWidget
{
    Q_OBJECT

public:
    MainScreen();

private:

    DiagnosticScreen * diagnostic_screen;
    MusicScreen * music_screen;
    MapScreen * map_screen;

    QString icon_path;

    QIcon * music_icon;
    QIcon * map_icon;
    QIcon * diagnostic_icon;
};
