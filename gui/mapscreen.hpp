#include <QApplication>
#include <QVBoxLayout>

#include <marble/MarbleGlobal.h>
#include <marble/MarbleWidget.h>

using namespace Marble;


class MapScreen : public QWidget
{
    Q_OBJECT

public:
    MapScreen();

private:
    MarbleWidget *map_widget;
    QVBoxLayout *layout;
};
