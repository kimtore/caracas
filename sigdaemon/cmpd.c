#include <mpd/client.h>
#include <zmq.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#define CMD_NUM_ERROR -1
#define CMD_NUM_NONE 0
#define CMD_NUM_VOLUME_STEP 1
#define CMD_NUM_NEXT 2
#define CMD_NUM_PREV 3
#define CMD_NUM_PLAY_PAUSE 4
#define CMD_NUM_PAUSE 5
#define CMD_NUM_UNPAUSE 6
#define CMD_NUM_NEXT_ARTIST 7
#define CMD_NUM_PREV_ARTIST 8
#define CMD_NUM_NEXT_ALBUM 9
#define CMD_NUM_PREV_ALBUM 10

#define CMD_VOLUME_STEP "VOLUME STEP"
#define CMD_NEXT "NEXT"
#define CMD_PREV "PREV"
#define CMD_PLAY_PAUSE "PLAY OR PAUSE"
#define CMD_PAUSE "PAUSE"
#define CMD_UNPAUSE "UNPAUSE"
#define CMD_NEXT_ARTIST "NEXT ARTIST"
#define CMD_PREV_ARTIST "PREV ARTIST"
#define CMD_NEXT_ALBUM "NEXT ALBUM"
#define CMD_PREV_ALBUM "PREV ALBUM"

#define PARAM_POS(X, Y)  (char *)X + strlen(Y) + 1

int cmd_from_msg(char *msg, void **params)
{
    *params = NULL;

    if (!strncmp(msg, CMD_VOLUME_STEP, strlen(CMD_VOLUME_STEP))) {
        *params = PARAM_POS(msg, CMD_VOLUME_STEP);
        return CMD_NUM_VOLUME_STEP;
    } else if (!strncmp(msg, CMD_NEXT_ARTIST, strlen(CMD_NEXT_ARTIST))) {
        return CMD_NUM_NEXT_ARTIST;
    } else if (!strncmp(msg, CMD_PREV_ARTIST, strlen(CMD_PREV_ARTIST))) {
        return CMD_NUM_PREV_ARTIST;
    } else if (!strncmp(msg, CMD_NEXT_ALBUM, strlen(CMD_NEXT_ALBUM))) {
        return CMD_NUM_NEXT_ALBUM;
    } else if (!strncmp(msg, CMD_PREV_ALBUM, strlen(CMD_PREV_ALBUM))) {
        return CMD_NUM_PREV_ALBUM;
    } else if (!strncmp(msg, CMD_NEXT, strlen(CMD_NEXT))) {
        return CMD_NUM_NEXT;
    } else if (!strncmp(msg, CMD_PREV, strlen(CMD_PREV))) {
        return CMD_NUM_PREV;
    } else if (!strncmp(msg, CMD_PLAY_PAUSE, strlen(CMD_PLAY_PAUSE))) {
        return CMD_NUM_PLAY_PAUSE;
    } else if (!strncmp(msg, CMD_PAUSE, strlen(CMD_PAUSE))) {
        return CMD_NUM_PAUSE;
    } else if (!strncmp(msg, CMD_UNPAUSE, strlen(CMD_UNPAUSE))) {
        return CMD_NUM_UNPAUSE;
    }

    return CMD_NUM_ERROR;
}

struct mpd_connection * get_mpd_connection()
{
    struct mpd_connection * connection;
    int err;

    /* Open a connection object */
    connection = mpd_connection_new(NULL, 0, 0);
    if (!connection) {
        syslog(LOG_EMERG, "Could not initialize mpd object: out of memory");
        exit(EXIT_FAILURE);
    }

    /* Check if connected */
    err = mpd_connection_get_error(connection);
    if (err != MPD_ERROR_SUCCESS) {
        syslog(LOG_WARNING, "Failed to connect to mpd: %s", mpd_connection_get_error_message(connection));
    } else {
        syslog(LOG_INFO, "Connected to mpd server.");
    }

    return connection;
}

int comp(const void * a, const void * b)
{
    const char **ac = (const char **) a;
    const char **bc = (const char **) b;
    return strcmp(*ac, *bc);
}

int get_sorted_tags(struct mpd_connection * connection, char ** array, enum mpd_tag_type tag, struct mpd_song * song)
{
    struct mpd_pair * pair;
    char ** ptr = array;
    const char * tag_content = NULL;
    int count;
    int total = 0;

    if (!mpd_search_db_tags(connection, tag)) {
        return -1;
    }

    if (tag == MPD_TAG_ALBUM && song) {
        tag_content = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
        if (tag_content && !mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT, MPD_TAG_ARTIST, tag_content)) {
            return -1;
        }
    }

    if (!mpd_search_commit(connection)) {
        return -1;
    }

    memset(array, 0, sizeof(array) * sizeof(char *));

    while ((pair = mpd_recv_pair_tag(connection, tag)) != NULL) {
        *ptr = malloc(100);
        strncpy(*ptr, pair->value, 99);
        ++ptr;
        ++total;
        mpd_return_pair(connection, pair);
    }

    ptr = array;
    qsort(ptr, total, sizeof(char *), comp);

    return total;
}

void free_sorted_tags(char ** array, int total)
{
    while (total-- > 0) {
        free(*array);
        array++;
    }
}

int get_next_in_line(struct mpd_connection * connection, int delta, enum mpd_tag_type tag, char *dest)
{
    struct mpd_song * song;
    char * array[4096];
    char ** ptr = array;
    char buf[128];
    char artist[128];
    const char * tag_content = NULL;
    int pos = 0;
    int total;

    song = mpd_run_current_song(connection);
    if (song) {
        tag_content = mpd_song_get_tag(song, tag, 0);
        if (tag_content) {
            strncpy(buf, tag_content, 127);
        }
    }

    if (!tag_content) {
        buf[0] = '\0';
    }

    total = get_sorted_tags(connection, array, tag, song);

    if (song) {
        mpd_song_free(song);
    }

    if (total == -1) {
        syslog(LOG_WARNING, "Could not retrieve a list of tags from the mpd server!");
        return -1;
    }

    while (*ptr != NULL) {
        if (!strcmp(buf, *ptr)) {
            pos += delta;
            while (pos >= total) {
                pos -= total;
            }
            while (pos < 0) {
                pos = total + pos;
            }
            ptr = array + pos;
            strncpy(dest, *ptr, 127);
            break;
        }
        ++ptr;
        ++pos;
    }

    free_sorted_tags(array, total);

    if (*ptr == NULL) {
        dest[0] = '\0';
        return -1;
    }

    return 0;
}

/* Jump to the next or previous song group, based on tag */
int jump_to(struct mpd_connection * connection, enum mpd_tag_type tag, int delta)
{
    struct mpd_pair * pair;
    char buf[128];

    if (!get_next_in_line(connection, delta, tag, buf) == -1) {
        syslog(LOG_WARNING, "Failed to get next tag in line!");
        return -1;
    }

    if (!mpd_search_add_db_songs(connection, true)) {
        syslog(LOG_WARNING, "Unable to initialize mpd search!");
        return -1;
    }

    if (!mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT, tag, buf)) {
        syslog(LOG_WARNING, "Unable to specify mpd search!");
        return -1;
    }

    if (!mpd_run_clear(connection)) {
        syslog(LOG_WARNING, "Unable to clear queue!");
        return -1;
    }

    if (!mpd_search_commit(connection)) {
        syslog(LOG_WARNING, "Unable to execute mpd search!");
        return -1;
    }

    while ((pair = mpd_recv_pair_tag(connection, tag)) != NULL) {
        mpd_return_pair(connection, pair);
    }

    if (!mpd_run_play(connection)) {
        syslog(LOG_WARNING, "Unable to start playback!");
        return -1;
    }

    return 0;
}

/* Command dispatcher */
void process_cmd(struct mpd_connection * connection, int cmd, const char *params)
{
    int err;
    int volume_delta;

    switch(cmd) {
        case CMD_NUM_VOLUME_STEP:
            volume_delta = atoi(params);
            syslog(LOG_INFO, "Running mpd command: change volume by delta %d", volume_delta);
            err = mpd_run_change_volume(connection, volume_delta);
            break;
        case CMD_NUM_PREV:
            syslog(LOG_INFO, "Running mpd command: change to previous song");
            err = mpd_run_previous(connection);
            break;
        case CMD_NUM_NEXT:
            syslog(LOG_INFO, "Running mpd command: change to next song");
            err = mpd_run_next(connection);
            break;
        case CMD_NUM_PLAY_PAUSE:
            syslog(LOG_INFO, "Running mpd command: toggle play/pause status");
            err = mpd_run_toggle_pause(connection);
            break;
        case CMD_NUM_PAUSE:
            syslog(LOG_INFO, "Running mpd command: pause music");
            err = mpd_run_pause(connection, true);
            break;
        case CMD_NUM_UNPAUSE:
            syslog(LOG_INFO, "Running mpd command: unpause music");
            err = mpd_run_pause(connection, false);
            break;
        case CMD_NUM_PREV_ARTIST:
            syslog(LOG_INFO, "Running mpd command: change to previous artist");
            err = jump_to(connection, MPD_TAG_ARTIST, -1);
            break;
        case CMD_NUM_NEXT_ARTIST:
            syslog(LOG_INFO, "Running mpd command: change to next artist");
            err = jump_to(connection, MPD_TAG_ARTIST, 1);
            break;
        case CMD_NUM_PREV_ALBUM:
            syslog(LOG_INFO, "Running mpd command: change to previous album");
            err = jump_to(connection, MPD_TAG_ALBUM, -1);
            break;
        case CMD_NUM_NEXT_ALBUM:
            syslog(LOG_INFO, "Running mpd command: change to next album");
            err = jump_to(connection, MPD_TAG_ALBUM, 1);
            break;
        default:
            syslog(LOG_WARNING, "Unable to run unhandled mpd command %d with params %s", cmd, params);
            break;
    }
}

int main(int argc, char *argv[])
{
    int err;
    int cmd = CMD_NUM_NONE;
    int errors = 0;
    struct mpd_connection * connection;
    void * context;
    void * socket;
    char buf[128];
    char * params = NULL;

    /* Syslog initialization */
    openlog("cmpd", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "cmpd initializing.");

    /* Create ZeroMQ context */
    context = zmq_ctx_new();
    if (!context) {
        syslog(LOG_EMERG, "Failed to create ZeroMQ context: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }

    /* Create ZeroMQ socket */
    socket = zmq_socket(context, ZMQ_SUB);
    if (!socket) {
        syslog(LOG_EMERG, "Failed to create ZeroMQ socket: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }

    /* Subscribe to everything */
    if (zmq_setsockopt(socket, ZMQ_SUBSCRIBE, "MPD ", 4) == -1) {
        syslog(LOG_EMERG, "Failed to set ZeroMQ socket options: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }

    /* Connect to ZeroMQ publisher */
    if (zmq_connect(socket, "tcp://localhost:9090") == -1) {
        syslog(LOG_EMERG, "Failed to connect to ZeroMQ publisher: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }
    syslog(LOG_INFO, "Connected to ZeroMQ publisher.");

    /* Connect to MPD */
    connection = get_mpd_connection();

    /* Main loop */
    while (1) {

        /* Check if MPD returned any errors */
        err = mpd_connection_get_error(connection);
        if (err != MPD_ERROR_SUCCESS && !mpd_connection_clear_error(connection)) {
            syslog(LOG_WARNING, "Could not recover from mpd error, reconnecting.");
            mpd_connection_free(connection);
            if (errors++ > 0) {
                sleep(1);
            }
            connection = get_mpd_connection();
            continue;
        }

        /* Reset mpd error count */
        errors = 0;

        /* If command failed due to MPD error, try re-running it */
        if (cmd == CMD_NUM_NONE) {

            /* Receive ZeroMQ message */
            err = zmq_recv(socket, buf, 128, 0);
            if (err == -1) {
                syslog(LOG_ERR, "Failed to receive data from ZeroMQ publisher: %s", zmq_strerror(errno));
                break;
            }
            buf[err] = '\0';
            syslog(LOG_DEBUG, "Received ZeroMQ message: %s", buf);

            /* Deduce if this is a valid command, and locate parameters */
            cmd = cmd_from_msg(buf + 4, (void**) &params);
            if (cmd == CMD_NUM_ERROR) {
                syslog(LOG_NOTICE, "Discarding invalid message");
                cmd = CMD_NUM_NONE;
                continue;
            }
        }

        /* Run the command */
        process_cmd(connection, cmd, params);
        err = mpd_connection_get_error(connection);
        if (err != MPD_ERROR_SUCCESS) {
            syslog(LOG_WARNING, "mpd command failed: %s", mpd_connection_get_error_message(connection));
            continue;
        }

        /* Reset command queue */
        cmd = CMD_NUM_NONE;
    }

    syslog(LOG_INFO, "cmpd shutting down.");
    zmq_ctx_destroy(context);
    mpd_connection_free(connection);

    return 0;
}
