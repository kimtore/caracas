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
#include <mcp3004.h>
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
 * Power state, input.
 * This pin is HIGH when the ignition key is switched on.
 */
#define PIN_POWER_STATE     4   /* BCM_GPIO pin 23, physical pin 16 */

/**
 * Max size of wiringPi pin numbering scheme
 */
#define PIN_MAX             101

/**
 * SPI parameters
 */
#define SPI_BASE            100
#define SPI_CHAN            0

/**
 * Exit codes
 */
#define EXIT_ZMQ            1
#define EXIT_WIRING         2

/**
 * How long to sleep between each event
 */
#define DELAY_MICROSECONDS  2

/**
 * Button events
 */
#define EVENT_NONE          0
#define EVENT_PRESS         1
#define EVENT_DEPRESS       2
#define EVENT_LEFT          3
#define EVENT_RIGHT         4

/**
 * Program options
 */
struct opts_t {
    int running;
};

struct opts_t opts;

/**
 * Current state of pins
 */
uint8_t pin_state[PIN_MAX+1];


/**
 * Save pin state in the pin_state struct.
 */
static void set_pin_state(uint8_t pin, uint8_t state)
{
    pin_state[pin] = state;
}

/**
 * Read pin state from the pin_state struct.
 */
static uint8_t get_pin_state(uint8_t pin)
{
    return pin_state[pin];
}

/**
 * Convert an event integer to a string.
 */
const char *event_name(uint8_t event)
{
    switch(event) {
        case EVENT_NONE:
            return "NONE";
        case EVENT_PRESS:
            return "PRESS";
        case EVENT_DEPRESS:
            return "DEPRESS";
        case EVENT_LEFT:
            return "LEFT";
        case EVENT_RIGHT:
            return "RIGHT";
        default:
            assert(0);
    }
}

/**
 * Return true if a pin is considered active or pressed,
 * or false otherwise.
 */
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

/**
 * Pin initialization for rotary click button.
 */
static void init_pin_rotary_click()
{
    pinMode(PIN_ROTARY_CLICK, INPUT);
    pullUpDnControl(PIN_ROTARY_CLICK, PUD_UP);
    set_pin_state(PIN_ROTARY_CLICK, HIGH);
}

/**
 * Pin initialization for rotary left switch.
 */
static void init_pin_rotary_left()
{
    pinMode(PIN_ROTARY_LEFT, INPUT);
    pullUpDnControl(PIN_ROTARY_LEFT, PUD_UP);
    set_pin_state(PIN_ROTARY_LEFT, HIGH);
}

/**
 * Pin initialization for rotary right switch.
 */
static void init_pin_rotary_right()
{
    pinMode(PIN_ROTARY_RIGHT, INPUT);
    pullUpDnControl(PIN_ROTARY_RIGHT, PUD_UP);
    set_pin_state(PIN_ROTARY_RIGHT, HIGH);
}

/**
 * Pin initialization for power state input.
 */
static void init_pin_power_state()
{
    pinMode(PIN_POWER_STATE, INPUT);
    pullUpDnControl(PIN_POWER_STATE, PUD_DOWN);
    set_pin_state(PIN_POWER_STATE, LOW);
}

/**
 * Initialize ADC communication with MPC3008.
 */
static void init_adc()
{
    mcp3004Setup(SPI_BASE, SPI_CHAN);
}

/**
 * Initialize all input pins.
 */
static void init_pins()
{
    memset(&pin_state, 0, sizeof(pin_state));

    init_pin_rotary_click();
    init_pin_rotary_left();
    init_pin_rotary_right();
    init_pin_power_state();
}

/**
 * Check whether a pin has new events.
 */
static uint8_t get_pin_event(uint8_t pin)
{
    uint8_t state = digitalRead(pin);
    uint8_t old_state = get_pin_state(pin);

    set_pin_state(pin, state);

    if (state == old_state) {
        return EVENT_NONE;
    }

    if (pin_active(pin)) {
        return EVENT_PRESS;
    } else {
        return EVENT_DEPRESS;
    }
}

/**
 * Check whether the rotary switch has events.
 */
static uint8_t get_rotary_event(uint8_t pin_left, uint8_t pin_right)
{
    static uint8_t state_left_old = 0;
    uint8_t state_left;
    uint8_t state_right;
    uint8_t event = EVENT_NONE;

    get_pin_event(pin_left);
    get_pin_event(pin_right);

    state_left = pin_active(pin_left);
    state_right = pin_active(pin_right);

    if (!state_left_old && state_left) {
        if (state_right) {
            event = EVENT_LEFT;
        } else {
            event = EVENT_RIGHT;
        }
    }
    state_left_old = state_left;

    return event;
}

/**
 * Read from MCP3008.
 * FIXME
 */
static uint8_t get_adc_event()
{
    analogRead(SPI_BASE + SPI_CHAN);
}

/**
 * Send an event using ZeroMQ, and log the output.
 */
static int log_zmq_send(void *socket, const char *str)
{
    int len = strlen(str);
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, len);
    strncpy(zmq_msg_data(&msg), str, len);
    printf("PUB: %s\n", str);
    return zmq_send(socket, &msg, 0);
}

/**
 * Inner main loop.
 */
static int run_loop(void *context, void *publisher)
{
    char msg[32];
    const char *name;
    uint8_t event;

    event = get_pin_event(PIN_ROTARY_CLICK);
    if (event == EVENT_NONE) {
        event = get_rotary_event(PIN_ROTARY_LEFT, PIN_ROTARY_RIGHT);
    }

    if (event != EVENT_NONE) {
        name = event_name(event);
        sprintf(msg, "ROTARY %s", name);
        if (log_zmq_send(publisher, msg) != 0) {
            return 1;
        }
    }

    get_adc_event();

    return 0;
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
    printf("Caught signal %d\n", s);
    opts.running = 0;
}

/**
 * Main program.
 */
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
    init_adc();

    act.sa_handler = signal_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    clear_opts(&opts);

    printf("Caracas daemon started.\n");

    while (opts.running) {
        run_loop(context, publisher);
        delay(DELAY_MICROSECONDS);
    }

    printf("Received shutdown signal, exiting.\n");

    zmq_close(publisher);
    zmq_term(context);

    return 0;
}
