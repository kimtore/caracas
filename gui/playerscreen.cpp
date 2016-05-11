#include "playerscreen.hpp"
#include "tagfile.hpp"


PlayerScreen::PlayerScreen()
{
    setObjectName("player_screen");

    /* Set up title widget */
    title = new QLabel;
    title->setObjectName("music_title");

    /* Set up artist widget */
    artist = new QLabel;
    artist->setObjectName("music_artist");

    /* Set up album widget */
    album = new QLabel;
    album->setObjectName("music_album");

    /* Set up album widget */
    track = new QLabel;
    track->setObjectName("music_track");

    /* Set up year widget */
    year = new QLabel;
    year->setObjectName("music_year");

    /* Set up elapsed widget */
    elapsed = new QLabel;
    elapsed->setObjectName("music_elapsed");

    /* Set up progressbar widget */
    progress = new QProgressBar;
    progress->setTextVisible(false);

    /* Set up volume widget */
    volume = new QProgressBar;
    volume->setTextVisible(true);
    volume->setMinimum(0);
    volume->setMaximum(100);

    /* Set up albumart widget */
    albumart_widget = new AlbumArtWidget;
    albumart_widget->setObjectName("music_album_art");

    /* Draw blank albumart */
    albumart_widget->reset();

    /* Set up layouts */
    layout = new QVBoxLayout(this);
    info_layout = new QHBoxLayout();
    song_layout = new QVBoxLayout();

    layout->addWidget(title);
    layout->addSpacing(10);
    layout->addLayout(info_layout);
    layout->addSpacing(20);
    layout->addWidget(volume);
    layout->setAlignment(title, Qt::AlignLeft | Qt::AlignTop);
    layout->setAlignment(volume, Qt::AlignBottom);

    info_layout->addWidget(albumart_widget);
    info_layout->setAlignment(Qt::AlignTop);
    info_layout->addSpacing(30);
    info_layout->addLayout(song_layout);
    info_layout->setAlignment(albumart_widget, Qt::AlignLeft | Qt::AlignTop);
    info_layout->setAlignment(song_layout, Qt::AlignLeft | Qt::AlignTop);

    song_layout->addWidget(artist);
    song_layout->addSpacing(20);
    song_layout->addWidget(album);
    song_layout->addWidget(track);
    song_layout->addWidget(year);
    song_layout->addSpacing(20);
    song_layout->addWidget(elapsed);
    song_layout->addWidget(progress);
}

void
PlayerScreen::info_changed(MPDPlayerInfo * info)
{
    title->setText(info->title);
    artist->setText(info->artist);
    album->setText(info->album);
    year->setText(info->year.left(4));
    if (info->track.size() == 0) {
        track->setText(info->track);
    } else {
        track->setText("Track " + info->track);
    }
    volume->setValue(info->volume);
}

void
PlayerScreen::elapsed_changed(MPDPlayerInfo * info)
{
    QString state_text;

    if (info->state == MPD_STATE_PAUSE) {
        state_text = "‖";
    } else if (info->state == MPD_STATE_PLAY) {
        state_text = "▶";
    } else if (info->state == MPD_STATE_STOP) {
        state_text = "■";
    } else {
        state_text = "?";
    }

    if (info->state == MPD_STATE_PLAY || info->state == MPD_STATE_PAUSE) {
        elapsed->setText(state_text + " " + info->elapsed.get_short_string() + " / " + info->duration.get_short_string());
        progress->setMaximum(info->duration.get_total_seconds());
    } else {
        elapsed->setText(state_text + " Stopped");
        progress->setMaximum(1);
    }
    progress->setMinimum(0);
    progress->setValue(info->elapsed.get_total_seconds());
}

void
PlayerScreen::uri_changed(MPDPlayerInfo * info)
{
    QPixmap source;
    TagFile f("/var/lib/mpd/music/" + info->uri);

    if (!f.get_album_art(&source)) {
        albumart_widget->reset();
        return;
    }

    albumart = source.scaled(240, 240, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    albumart_widget->setPixmap(albumart);
}
