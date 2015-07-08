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

#define CMD_VOLUME_STEP "VOLUME STEP"
#define CMD_NEXT "NEXT"
#define CMD_PREV "PREV"
#define CMD_PLAY_PAUSE "PLAY OR PAUSE"
#define CMD_PAUSE "PAUSE"

#define PARAM_POS(X, Y)  (char *)X + strlen(Y) + 1

int cmd_from_msg(char *msg, void **params)
{
    *params = NULL;

    if (!strncmp(msg, CMD_VOLUME_STEP, strlen(CMD_VOLUME_STEP))) {
        *params = PARAM_POS(msg, CMD_VOLUME_STEP);
        return CMD_NUM_VOLUME_STEP;
    } else if (!strncmp(msg, CMD_NEXT, strlen(CMD_NEXT))) {
        return CMD_NUM_NEXT;
    } else if (!strncmp(msg, CMD_PREV, strlen(CMD_PREV))) {
        return CMD_NUM_PREV;
    } else if (!strncmp(msg, CMD_PLAY_PAUSE, strlen(CMD_PLAY_PAUSE))) {
        return CMD_NUM_PLAY_PAUSE;
    } else if (!strncmp(msg, CMD_PAUSE, strlen(CMD_PAUSE))) {
        return CMD_NUM_PAUSE;
    }

    return CMD_NUM_ERROR;
}

void process_cmd(struct mpd_connection * connection, int cmd, const char *params)
{
    int err;
    int volume_delta;

    switch(cmd) {
        case CMD_NUM_VOLUME_STEP:
            volume_delta = atoi(params);
            syslog(LOG_INFO, "Changing volume by delta %d", volume_delta);
            err = mpd_run_change_volume(connection, volume_delta);
            break;
        case CMD_NUM_PREV:
            syslog(LOG_INFO, "Changing to previous song");
            err = mpd_run_previous(connection);
            break;
        case CMD_NUM_NEXT:
            syslog(LOG_INFO, "Changing to next song");
            err = mpd_run_next(connection);
            break;
        case CMD_NUM_PLAY_PAUSE:
            syslog(LOG_INFO, "Toggling play/pause status");
            err = mpd_run_toggle_pause(connection);
            break;
        case CMD_NUM_PAUSE:
            syslog(LOG_INFO, "Pausing music");
            err = mpd_run_pause(connection, true);
            break;
        default:
            syslog(LOG_WARNING, "Unhandled command %d", cmd);
            break;
    }
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

int main(int argc, char *argv[])
{
    int err;
    int cmd = CMD_NUM_NONE;
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
            sleep(1);
            connection = get_mpd_connection();
            continue;
        }

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
