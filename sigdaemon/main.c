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
#include <stdio.h>
#include <signal.h>
#include <wiringPi.h>
#include <zmq.h>

/*
 * Type definitions
 */
typedef unsigned char       uint8_t;

/*
 * Rotary button input detect pins.
 * These pins are pulled LOW when the button is active.
 */
#define PIN_ROTARY_CLICK    15  /* BCM_GPIO pin 14, physical pin  8 */
#define PIN_ROTARY_LEFT     16  /* BCM_GPIO pin 15, physical pin 10 */
#define PIN_ROTARY_RIGHT    1   /* BCM_GPIO pin 18, physical pin 12 */

/**
 * Suicide pin, output.
 * Set HIGH to request power cut-off from the power board.
 */
#define PIN_SUICIDE         0   /* BCM_GPIO pin 17, physical pin 11 */

/**
 * Power state, input.
 * This pin is HIGH when the ignition key is switched on.
 */
#define PIN_POWER_STATE     4   /* BCM_GPIO pin 23, physical pin 16 */

/**
 * Max size of wiringPi pin numbering scheme
 */
#define PIN_MAX             16

/**
 * Exit codes
 */
#define EXIT_ZMQ 1
#define EXIT_WIRING 2

/**
 * Button events
 */
#define EVENT_NONE          0
#define EVENT_PRESS         1
#define EVENT_DEPRESS       2

struct opts_t {
    int running;
};

struct opts_t opts;

/**
 * Current state of pins
 */
uint8_t pin_state[PIN_MAX+1];

static void set_pin_state(uint8_t pin, uint8_t state)
{
    pin_state[pin] = state;
}

static uint8_t get_pin_state(uint8_t pin)
{
    return pin_state[pin];
}

const char *pin_name(uint8_t pin)
{
    switch(pin) {
        case PIN_ROTARY_CLICK:
            return "ROTARY_CLICK";
        case PIN_ROTARY_LEFT:
            return "ROTARY_LEFT";
        case PIN_ROTARY_RIGHT:
            return "ROTARY_RIGHT";
        case PIN_POWER_STATE:
            return "POWER_STATE";
        default:
            assert(0);
    }
}

static uint8_t pin_active(uint8_t pin)
{
    switch(pin) {
        case PIN_ROTARY_CLICK:
        case PIN_ROTARY_LEFT:
        case PIN_ROTARY_RIGHT:
            return (get_pin_state(pin) == LOW);
        case PIN_POWER_STATE:
            return (get_pin_state(pin) == HIGH);
        default:
            assert(0);
    }
}

static void init_pin_rotary_click()
{
    pinMode(PIN_ROTARY_CLICK, INPUT);
    pullUpDnControl(PIN_ROTARY_CLICK, PUD_UP);
    set_pin_state(PIN_ROTARY_CLICK, HIGH);
}

static void init_pin_rotary_left()
{
}

static void init_pin_rotary_right()
{
}

static void init_pin_power_state()
{
}

static void init_pin_suicide()
{
}

static void init_pins()
{
    memset(&pin_state, 0, sizeof(pin_state));

    init_pin_rotary_click();
    init_pin_rotary_left();
    init_pin_rotary_right();
    init_pin_power_state();
    init_pin_suicide();
}

static uint8_t get_pin_event(uint8_t pin)
{
    uint8_t state = digitalRead(pin);
    uint8_t old_state = get_pin_state(pin);

    set_pin_state(pin, state);

    if (state == old_state) {
        return EVENT_NONE;
    } else if (pin_active(pin)) {
        return EVENT_PRESS;
    } else {
        return EVENT_DEPRESS;
    }
}

static int log_zmq_send(void *socket, const char *str)
{
    int len = strlen(str);
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, len);
    strncpy(zmq_msg_data(&msg), str, len);
    printf("PUB: %s\n", str);
    return zmq_send(socket, &msg, 0);
}

static int run_loop(void *context, void *publisher)
{
    char msg[32];
    const char *name;
    uint8_t event;

    if ((event = get_pin_event(PIN_ROTARY_CLICK)) != EVENT_NONE) {
        name = pin_name(PIN_ROTARY_CLICK);
        sprintf(msg, "%s %d", name, event);
        if (log_zmq_send(publisher, msg) != 0) {
            return 1;
        }
    }

    return 0;
}

static void clear_opts(struct opts_t *o)
{
    o->running = 1;
}

void signal_handler(int s)
{
    printf("Caught signal %d\n", s);
    opts.running = 0;
}

int main(int argc, char **argv)
{
    void *context = zmq_init(1);
    void *publisher = zmq_socket(context, ZMQ_PUB);
    struct sigaction act;

    if ((zmq_bind(publisher, "tcp://*:5555")) != 0) {
        perror("Fatal error: could not bind ZMQ socket tcp://*:5555");
        return EXIT_ZMQ;
    }

    if (wiringPiSetup()) {
        perror("Fatal error: could not initialize wiringPi");
        return EXIT_WIRING;
    }

    init_pins();

    act.sa_handler = signal_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    clear_opts(&opts);

    printf("Caracas daemon started.\n");

    while (opts.running) {
        run_loop(context, publisher);
        delay(25);
    }

    printf("Received shutdown signal, exiting.\n");

    zmq_close(publisher);
    zmq_term(context);

    return 0;
}
