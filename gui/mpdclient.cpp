#include "mpdclient.hpp"

#include <unistd.h>


MPDPlayerInfo::MPDPlayerInfo()
{
    connected = false;
    uri = "";
    title = "";
    artist = "";
    album = "";
    year = "";
    track = "";
    volume = 0;
    elapsed.setHMS(0, 0, 0);
    duration.setHMS(0, 0, 0);
}


MPDClient::MPDClient()
{
    last_uri = "";
    connection = NULL;
    is_idle = false;
    needs_update = true;
    info = new MPDPlayerInfo();
}

void
MPDClient::handle_errors()
{
    enum mpd_error err;

    if (!connection) {
        return;
    }

    mutex.lock();

    err = mpd_connection_get_error(connection);

    switch(err) {
        case MPD_ERROR_SUCCESS:
            break;
        case MPD_ERROR_SERVER:
            qDebug() << "MPD returned an error: " << mpd_connection_get_error_message(connection);
            if (!mpd_connection_clear_error(connection)) {
                qDebug() << "Disconnecting from MPD due to error: " << mpd_connection_get_error_message(connection);
                disconnect();
            }
            break;
        default:
            qDebug() << "Disconnecting from MPD due to error: " << mpd_connection_get_error_message(connection);
            disconnect();
            break;
    }

    mutex.unlock();
}

bool
MPDClient::connect()
{
    qDebug() << "Connecting to MPD server.";
    mutex.lock();
    connection = mpd_connection_new(NULL, 0, 0);
    if (connected_and_ok()) {
        qDebug() << "Connected.";
        is_idle = false;
        needs_update = true;
        mutex.unlock();
        return true;
    }
    mutex.unlock();
    return false;
}

void
MPDClient::disconnect()
{
    qDebug() << "Disconnecting from MPD server.";
    if (!connection) {
        return;
    }
    mutex.lock();
    mpd_connection_free(connection);
    mutex.unlock();
}

bool
MPDClient::connected_and_ok()
{
    bool rc;
    rc = connection && mpd_connection_get_error(connection) == MPD_ERROR_SUCCESS;
    return rc;
}

bool
MPDClient::has_idle_events(long timeout_ms)
{
    fd_set pollfd;
    struct timeval timeout;
    int mpd_fd;
    int nfds;
    int rc;

    mutex.lock();

    FD_ZERO(&pollfd);

    if ((mpd_fd = mpd_connection_get_fd(connection)) != -1) {
        FD_SET(mpd_fd, &pollfd);
    } else {
        mutex.unlock();
        return false;
    }

    nfds = mpd_fd;

    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms * 1000) - (timeout.tv_sec * 1000000);

    rc = select(++nfds, &pollfd, NULL, NULL, &timeout);

    if (rc == EINVAL || rc == -1) {
        mutex.unlock();
        return false;
    }

    mutex.unlock();

    return (rc > 0) && FD_ISSET(mpd_fd, &pollfd);
}

bool
MPDClient::update_player_info()
{
    struct mpd_song * song;
    struct mpd_status * status;

    qDebug() << "Retrieving status and current song from MPD server.";

    mutex.lock();

    song = mpd_run_current_song(connection);
    if (!connected_and_ok()) {
        mutex.unlock();
        return false;
    }

    if (song) {
        info->uri = mpd_song_get_uri(song);
        info->title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
        info->artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
        info->album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
        info->year = mpd_song_get_tag(song, MPD_TAG_DATE, 0);
        info->track = mpd_song_get_tag(song, MPD_TAG_TRACK, 0);
        mpd_song_free(song);
    } else {
        info->title.clear();
        info->artist.clear();
        info->album.clear();
        info->year.clear();
        info->track.clear();
    }

    if (!(status = mpd_run_status(connection))) {
        mutex.unlock();
        return false;
    }

    info->duration.set_total_ms(mpd_status_get_total_time(status) * 1000);
    info->elapsed.set_total_ms(mpd_status_get_elapsed_ms(status));
    info->volume = mpd_status_get_volume(status);
    info->state = mpd_status_get_state(status);

    mpd_status_free(status);

    /* Start elapsed timer if playing */
    if (info->state == MPD_STATE_PLAY) {
        timer.start();
    }

    mutex.unlock();
    return true;
}

bool
MPDClient::progress_time()
{
    if (info->state != MPD_STATE_PLAY) {
        return false;
    }

    info->elapsed.increment_ms(timer.restart());
    return true;
}

void
MPDClient::run()
{
    mpd_idle events;

    while (1)
    {
        handle_errors();

        usleep(10000);

        mutex.lock();
        info->connected = connected_and_ok();
        mutex.unlock();

        if (!info->connected) {
            if (!connect()) {
                emit info_changed(info);
                emit elapsed_changed(info);
                sleep(1);
                continue;
            }
        }

        if (needs_update) {
            if (!update_player_info()) {
                continue;
            }
            needs_update = false;
            emit info_changed(info);
            emit elapsed_changed(info);
            if (info->uri != last_uri) {
                emit uri_changed(info);
                last_uri = info->uri;
            }
        }

        if (!is_idle) {
            idle();
            continue;
        }

        if (has_idle_events(200)) {
            qDebug() << "Received data on MPD connection.";
            events = receive_idle_updates();
            if (events == 0) {
                continue;
            }
            if (events & (MPD_IDLE_PLAYER | MPD_IDLE_MIXER)) {
                needs_update = true;
            }
        }

        if (progress_time()) {
            emit elapsed_changed(info);
        }
    }
}

bool
MPDClient::idle()
{
    qDebug() << "Putting MPD client in IDLE mode...";
    if (!mpd_send_idle(connection)) {
        return false;
    }
    qDebug() << "IDLE mode activated.";
    is_idle = true;
    return true;
}

mpd_idle
MPDClient::receive_idle_updates()
{
    mpd_idle rc;
    qDebug() << "Exiting IDLE mode...";
    rc = mpd_recv_idle(connection, true);
    if (connected_and_ok()) {
        qDebug() << "Exited IDLE mode with flags" << rc;
        is_idle = false;
    }
    return rc;
}

bool
MPDClient::noidle()
{
    if (is_idle) {
        if (!mpd_send_noidle(connection)) {
            return false;
        }
        receive_idle_updates();
    }

    return true;
}

void
MPDClient::get_artist_list(QStringList * list)
{
    mpd_pair * pair;

    qDebug() << "Fetching a list of artists from MPD server.";

    mutex.lock();

    list->clear();

    if (!noidle()) {
        qDebug() << "Unable to exit IDLE mode, aborting.";
        goto end;
    }
    if (!mpd_search_db_tags(connection, MPD_TAG_ARTIST)) {
        qDebug() << "Unable to initialize an MPD search, aborting.";
        goto end;
    }
    if (!mpd_search_commit(connection)) {
        qDebug() << "Unable to run an MPD search, aborting.";
        goto end;
    }

    qDebug() << "Search completed.";

    while ((pair = mpd_recv_pair_tag(connection, MPD_TAG_ARTIST)) != NULL) {
        list->push_back(pair->value);
        mpd_return_pair(connection, pair);
    }

end:
    mutex.unlock();
}

void
MPDClient::get_album_list(QStringList * list, QString artist)
{
    mpd_pair * pair;

    list->clear();

    qDebug() << "Fetching a list of albums by" << artist << "from MPD server.";

    mutex.lock();

    list->clear();

    if (!noidle()) {
        qDebug() << "Unable to exit IDLE mode, aborting.";
        goto end;
    }
    if (!mpd_search_db_tags(connection, MPD_TAG_ALBUM)) {
        qDebug() << "Unable to initialize an MPD search, aborting.";
        goto end;
    }
    if (!mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT, MPD_TAG_ARTIST, artist.toUtf8().data())) {
        qDebug() << "Unable to add tag constraint on MPD search, aborting.";
        goto end;
    }
    if (!mpd_search_commit(connection)) {
        qDebug() << "Unable to run an MPD search, aborting.";
        goto end;
    }

    qDebug() << "Search completed.";

    while ((pair = mpd_recv_pair_tag(connection, MPD_TAG_ALBUM)) != NULL) {
        list->push_back(pair->value);
        mpd_return_pair(connection, pair);
    }

end:
    mutex.unlock();
}

void
MPDClient::play_album(QString album)
{
    mpd_pair * pair;

    qDebug() << "Playing album" << album;

    mutex.lock();

    if (!noidle()) {
        qDebug() << "Unable to exit IDLE mode, aborting.";
        goto end;
    }
    if (!mpd_run_clear(connection)) {
        qDebug() << "Unable to clear playlist, aborting.";
        goto end;
    }
    if (!mpd_search_add_db_songs(connection, true)) {
        qDebug() << "Unable to initialize an MPD search, aborting.";
        goto end;
    }
    if (!mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, album.toUtf8().data())) {
        qDebug() << "Unable to add tag constraint on MPD search, aborting.";
        goto end;
    }
    if (!mpd_search_commit(connection)) {
        qDebug() << "Unable to run an MPD search, aborting.";
        goto end;
    }

    while ((pair = mpd_recv_pair_tag(connection, MPD_TAG_ALBUM)) != NULL) {
        mpd_return_pair(connection, pair);
    }

    if (!mpd_run_play(connection)) {
        qDebug() << "Unable to start playing, aborting.";
        goto end;
    }

    qDebug() << "Album added successfully, now playing.";

end:
    mutex.unlock();
}
