#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QProgressBar>
#include <QLabel>
#include <QStringList>
#include <QString>
#include <QTabWidget>
#include <QListWidgetItem>

#include "mpdclient.hpp"
#include "playerscreen.hpp"
#include "listscreen.hpp"
#include "albumartwidget.hpp"


#ifndef _GUI_MUSICSCREEN_H_
#define _GUI_MUSICSCREEN_H_


class MusicScreen : public QTabWidget
{
    Q_OBJECT

public:
    MusicScreen();

signals:
    void get_artist_list(QStringList * list);
    void get_album_list(QStringList * list, QString artist);
    void play_album(QString album);

public slots:
    void get_artist_list_slot();
    void get_album_list_slot(QListWidgetItem * item);
    void play_album_slot(QListWidgetItem * item);

private:
    PlayerScreen * player_screen;
    ListScreen * artist_screen;
    ListScreen * album_screen;

    MPDClient * mpd_client;
};


#endif /* _GUI_MUSICSCREEN_H_ */
