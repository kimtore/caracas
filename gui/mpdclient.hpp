#include <QObject>
#include <QThread>
#include <QtDebug>
#include <QMutex>
#include <QStringList>

#include <mpd/client.h>
#include <sys/select.h>
#include <errno.h>

#include "time.hpp"


#ifndef _GUI_MPDCLIENT_H_
#define _GUI_MPDCLIENT_H_


class MPDPlayerInfo : public QObject
{
    Q_OBJECT

public:
    MPDPlayerInfo();

    bool connected;
    QString uri;
    QString title;
    QString artist;
    QString album;
    QString year;
    QString track;
    mpd_state state;
    int volume;
    Time elapsed;
    Time duration;
};


class MPDClient : public QThread
{
    Q_OBJECT

public:
    MPDClient();

    /**
     * Connect to the MPD server.
     */
    bool connect();

    /**
     * Disconnect from the MPD server.
     */
    void disconnect();

    /**
     * Returns true if there is a valid MPD connection.
     */
    bool connected_and_ok();

    /**
     * Handle any MPD errors.
     */
    void handle_errors();

    /**
     * Poll stdin and the MPD socket for events.
     *
     * Returns true if any events occurred, false if not.
     */
    bool has_idle_events(long timeout_ms);

    /**
     * Receive IDLE updates from MPD.
     */
    mpd_idle receive_idle_updates();

    /**
     * Update status information from MPD.
     */
    bool update_player_info();

    /**
     * Tell MPD that we will chill out and wait for events.
     */
    bool idle();

    /**
     * Exit IDLE mode.
     */
    bool noidle();

    /**
     * Increment the elapsed timer.
     */
    bool progress_time();

    /**
     * Main loop.
     */
    void run();

signals:
    void info_changed(MPDPlayerInfo * info);
    void elapsed_changed(MPDPlayerInfo * info);
    void uri_changed(MPDPlayerInfo * info);

public slots:
    void get_artist_list(QStringList * list);
    void get_album_list(QStringList * list, QString artist);
    void play_album(QString album);

private:
    struct mpd_connection * connection;

    MPDPlayerInfo * info;

    Time timer;

    QString last_uri;

    QMutex mutex;

    bool is_idle;
    bool needs_update;

};

#endif /* _GUI_MPDCLIENT_H_ */
