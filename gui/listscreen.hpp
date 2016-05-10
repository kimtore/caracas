#include <QString>
#include <QListWidget>

#include "mpdclient.hpp"
#include "albumartwidget.hpp"


#ifndef _GUI_LISTSCREEN_H_
#define _GUI_LISTSCREEN_H_


class ListScreen : public QListWidget
{
    Q_OBJECT

public:
    ListScreen();

signals:
    void selected(QString text);

private:
};


#endif /* _GUI_LISTSCREEN_H_ */
