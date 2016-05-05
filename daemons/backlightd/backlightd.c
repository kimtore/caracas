/**
 * Hardware event daemon for the CARACAS project.
 *
 * Monitors the Raspberry Pi's GPIO pins and uses ØMQ to emit events
 * when something happens.
 *
 * Requires the wiringPi library.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <wiringPi.h>
#include <mcp3004.h>
#include <zmq.h>
#include <syslog.h>

/**
 * Type definitions
 */
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;

/**
 * Pulse width modulation output pin for Adafruit display backlight regulation.
 */
#define PIN_BACKLIGHT_PWM   29  /* BCM_GPIO pin 21, physical pin 40 */

/**
 * Set the range of the PWM pin to 0-100. That means steps in increments of
 * 10Hz are possible, from 0-1kHz. The screen is initialized with full intensity.
 */
#define PWM_INITIAL_VALUE   100
#define PWM_RANGE           100

/**
 * The string that starts a backlight adjustment command.
 */
#define BACKLIGHT_CMD       "BACKLIGHT "

/**
 * Exit codes
 */
#define EXIT_ZMQ            1
#define EXIT_WIRING         2
#define EXIT_SPI            3

/**
 * Program options
 */
struct opts_t {
    int running;
};

struct opts_t opts;

/**
 * ZeroMQ globals
 */
void *zmq_context;
void *zmq_publisher;
void *zmq_sock;

/**
 * Initialize PWM output pin.
 */
static void init_pin()
{
    softPwmCreate(PIN_BACKLIGHT_PWM, PWM_INITIAL_VALUE, PWM_RANGE);
}

/**
 * Convert a intensity percentage to a PWM value.
 */
static uint16_t percentage_to_value(uint16_t percentage)
{
    return (percentage / 100.0) * PWM_RANGE;
}

/**
 * Returns a percentage value from a ZeroMQ message, or -1 if the message is invalid.
 */
static int16_t percentage_from_message(const char * msg)
{
    msg += strlen(BACKLIGHT_CMD);
    return atoi(msg);
}

/**
 * Send an event using ZeroMQ, and log the output.
 */
static int log_zmq_send(void *socket, const char *str)
{
    syslog(LOG_INFO, "Publishing ZeroMQ event: %s", str);
    return zmq_send(socket, str, strlen(str), 0);
}

/**
 * Send a normalized ZeroMQ event
 */
void simple_zmq_send(const char *source, const char *state)
{
    char msg[32];

    sprintf(msg, "%s %s", source, state);

    if (log_zmq_send(zmq_publisher, msg) == -1) {
        perror("Error when publishing ZeroMQ event");
    }
}

/**
 * Initialize options.
 */
static void clear_opts(struct opts_t *o)
{
    o->running = 1;
}

/**
 * Catch signals from operating system.
 */
void signal_handler(int s)
{
    syslog(LOG_NOTICE, "Caught signal %d", s);
    opts.running = 0;
}

/**
 * Main program.
 */
int main(int argc, char **argv)
{
    struct sigaction act;
    int err;
    char buf[128];
    int16_t percentage;
    uint16_t value;

    openlog("backlightd", LOG_PID, LOG_DAEMON);

    syslog(LOG_INFO, "Starting backlight daemon.");

    /* Create ZeroMQ context */
    zmq_context = zmq_ctx_new();
    if (!zmq_context) {
        syslog(LOG_EMERG, "Failed to create ZeroMQ context: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }

    /* Create ZeroMQ socket */
    zmq_sock = zmq_socket(zmq_context, ZMQ_SUB);
    if (!zmq_sock) {
        syslog(LOG_EMERG, "Failed to create ZeroMQ socket: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }

    /* Subscribe to everything */
    if (zmq_setsockopt(zmq_sock, ZMQ_SUBSCRIBE, BACKLIGHT_CMD, strlen(BACKLIGHT_CMD)) == -1) {
        syslog(LOG_EMERG, "Failed to set ZeroMQ socket options: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }

    /* Connect to ZeroMQ publisher */
    if (zmq_connect(zmq_sock, "tcp://localhost:9090") == -1) {
        syslog(LOG_EMERG, "Failed to connect to ZeroMQ publisher: %s", zmq_strerror(errno));
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "Connected to ZeroMQ publisher.");

    if (wiringPiSetup()) {
        perror("Fatal error: could not initialize wiringPi");
        return EXIT_WIRING;
    }

    init_pin();

    act.sa_handler = signal_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    clear_opts(&opts);

    syslog(LOG_NOTICE, "Backlight daemon started.");

    while (opts.running) {
        err = zmq_recv(zmq_sock, buf, 128, 0);
        if (err == -1) {
            syslog(LOG_ERR, "Failed to receive data from ZeroMQ publisher: %s", zmq_strerror(errno));
            break;
        }
        buf[err] = '\0';
        syslog(LOG_DEBUG, "Received ZeroMQ message: %s", buf);
        percentage = percentage_from_message(buf);
        if (percentage < 0 || percentage > 100) {
            continue;
        }
        value = percentage_to_value(percentage);
        syslog(LOG_INFO, "Setting backlight to %d%% (value %d)", percentage, value);
        softPwmWrite(PIN_BACKLIGHT_PWM, value);
    }

    syslog(LOG_NOTICE, "Received shutdown signal, exiting.");

    zmq_close(zmq_sock);
    zmq_term(zmq_context);

    return 0;
}
