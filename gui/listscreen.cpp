#include <QScrollBar>

#include "listscreen.hpp"


ListScreen::ListScreen()
{
    verticalScrollBar()->setSingleStep(3);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}
