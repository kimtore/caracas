#include "mainscreen.hpp"
#include "tagfile.hpp"


MusicScreen::MusicScreen()
{
    setObjectName("music_screen");
    setUsesScrollButtons(false);

    /* Set up MPD client */
    mpd_client = new MPDClient();

    /* Set up player screen */
    player_screen = new PlayerScreen;
    artist_screen = new ListScreen;
    album_screen = new ListScreen;
    setTabPosition(QTabWidget::South);
    addTab(player_screen, "Now playing");

    /* Connect MPD info change to GUI draw */
    QObject::connect(mpd_client, &MPDClient::info_changed,
                     player_screen, &PlayerScreen::info_changed);
    QObject::connect(mpd_client, &MPDClient::elapsed_changed,
                     player_screen, &PlayerScreen::elapsed_changed);
    QObject::connect(mpd_client, &MPDClient::uri_changed,
                     player_screen, &PlayerScreen::uri_changed);

    /* Album and artist list interaction */
    QObject::connect(player_screen->albumart_widget, &AlbumArtWidget::clicked,
                     this, &MusicScreen::get_artist_list_slot);
    QObject::connect(artist_screen, &ListScreen::itemClicked,
                     this, &MusicScreen::get_album_list_slot);
    QObject::connect(album_screen, &ListScreen::itemClicked,
                     this, &MusicScreen::play_album_slot);

    /* Remote calls to MPD thread */
    QObject::connect(this, &MusicScreen::get_artist_list,
                     mpd_client, &MPDClient::get_artist_list);
    QObject::connect(this, &MusicScreen::get_album_list,
                     mpd_client, &MPDClient::get_album_list);
    QObject::connect(this, &MusicScreen::play_album,
                     mpd_client, &MPDClient::play_album);

    /* Start MPD client late to avoid updates before GUI is ready */
    mpd_client->start();
}

void
MusicScreen::get_artist_list_slot()
{
    QStringList artist_list;

    emit get_artist_list(&artist_list);

    artist_screen->clear();
    artist_screen->addItems(artist_list);

    if (indexOf(artist_screen) == -1) {
        addTab(artist_screen, "Artists");
    }
    setCurrentIndex(1);
}

void
MusicScreen::get_album_list_slot(QListWidgetItem * item)
{
    QStringList album_list;
    QString artist;

    artist = item->text();

    emit get_album_list(&album_list, artist);

    album_screen->clear();
    album_screen->addItems(album_list);

    if (indexOf(album_screen) == -1) {
        addTab(album_screen, "Albums");
    }
    setTabText(2, "Albums by " + artist);
    setCurrentIndex(2);
}

void
MusicScreen::play_album_slot(QListWidgetItem * item)
{
    QString album;

    album = item->text();

    emit play_album(album);

    setCurrentIndex(0);
}
