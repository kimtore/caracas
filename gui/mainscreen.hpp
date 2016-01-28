#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QTimer>

#include "diagnosticscreen.hpp"
#include "mapscreen.hpp"

#define ICON_PATH "/usr/share/caracas/icons/"

class MainScreen : public QTabWidget
{
    Q_OBJECT

public:
    MainScreen();

private:

    DiagnosticScreen * diagnostic_screen;
    QWidget * music_screen;
    MapScreen * map_screen;

    QString icon_path;
    QIcon * map_icon;
};
