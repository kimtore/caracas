#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QProgressBar>
#include <QLabel>
#include <QList>
#include <QString>
#include <QTabWidget>

#include "mpdclient.hpp"
#include "albumartwidget.hpp"


#ifndef _GUI_PLAYERSCREEN_H_
#define _GUI_PLAYERSCREEN_H_


class PlayerScreen : public QTabWidget
{
    Q_OBJECT

public:
    PlayerScreen();

    AlbumArtWidget * albumart_widget;

public slots:
    void info_changed(MPDPlayerInfo * info);
    void elapsed_changed(MPDPlayerInfo * info);
    void uri_changed(MPDPlayerInfo * info);

private:
    QVBoxLayout * layout;
    QHBoxLayout * info_layout;
    QVBoxLayout * song_layout;

    QLabel * title;
    QLabel * artist;
    QLabel * album;
    QLabel * track;
    QLabel * year;
    QLabel * elapsed;
    QProgressBar * progress;
    QProgressBar * volume;
    QPixmap albumart;
};


#endif /* _GUI_PLAYERSCREEN_H_ */
