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

#define DEBUG 0

/*
 * Type definitions
 */
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;

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
#define PIN_POWER_STATE     5   /* BCM_GPIO pin 24, physical pin 18 */

/**
 * Max size of wiringPi pin numbering scheme
 */
#define PIN_MAX             101

/**
 * SPI parameters
 */
#define SPI_BASE            100
#define SPI_CHAN            0
#define SPI_PIN_MAX         2   /* How many pins to read, ranging from 1-8 */

/**
 * Microseconds interval between each read of the SPI controller
 */
#define SPI_INTERVAL        50000

/**
 * ADC steering wheel values
 */
#define ANALOG_MODE         1
#define ANALOG_UP           (1 << 1)
#define ANALOG_DOWN         (1 << 2)
#define ANALOG_VOL_UP       (1 << 3)
#define ANALOG_VOL_DOWN     (1 << 4)

/**
 * Exit codes
 */
#define EXIT_ZMQ            1
#define EXIT_WIRING         2
#define EXIT_SPI            3

/**
 * How long to run debouncing on the rotary click button
 */
#define DEBOUNCE_DURATION   0.05

/**
 * Button events
 */
#define EVENT_NONE          0
#define EVENT_PRESS         1
#define EVENT_DEPRESS       2
#define EVENT_LEFT          3
#define EVENT_RIGHT         4

/**
 * ADC voltage lookup table
 */
uint8_t analog_table[SPI_PIN_MAX][1024];

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

/**
 * Current state of pins
 */
uint8_t pin_state[PIN_MAX+1];

/**
 * Forward declarations
 */
void callback_rotary_binary();
void callback_rotary_click();
void callback_power_state();

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
 * Convert active state to a string.
 */
const char *active_name(uint8_t active)
{
    return active ? "ON" : "OFF";
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
        case PIN_POWER_STATE:
            return (get_pin_state(pin) == LOW);
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
    wiringPiISR(PIN_ROTARY_CLICK, INT_EDGE_BOTH, callback_rotary_click);
    set_pin_state(PIN_ROTARY_CLICK, HIGH);
}

/**
 * Pin initialization for rotary left switch.
 */
static void init_pin_rotary_left()
{
    pinMode(PIN_ROTARY_LEFT, INPUT);
    pullUpDnControl(PIN_ROTARY_LEFT, PUD_UP);
    wiringPiISR(PIN_ROTARY_LEFT, INT_EDGE_BOTH, callback_rotary_binary);
    set_pin_state(PIN_ROTARY_LEFT, HIGH);
}

/**
 * Pin initialization for rotary right switch.
 */
static void init_pin_rotary_right()
{
    pinMode(PIN_ROTARY_RIGHT, INPUT);
    pullUpDnControl(PIN_ROTARY_RIGHT, PUD_UP);
    wiringPiISR(PIN_ROTARY_RIGHT, INT_EDGE_BOTH, callback_rotary_binary);
    set_pin_state(PIN_ROTARY_RIGHT, HIGH);
}

/**
 * Pin initialization for power state input.
 */
static void init_pin_power_state()
{
    pinMode(PIN_POWER_STATE, INPUT);
    pullUpDnControl(PIN_POWER_STATE, PUD_UP);
    wiringPiISR(PIN_POWER_STATE, INT_EDGE_BOTH, callback_power_state);
    set_pin_state(PIN_POWER_STATE, HIGH);
}

/**
 * Initialize ADC communication with MPC3008.
 */
static void init_adc()
{
    /* 3004 and 3008 are the same, with 4/8 channels */
    if (mcp3004Setup(SPI_BASE, SPI_CHAN) != 0) {
        perror("Error initializing SPI using MCP3008 chip");
        exit(EXIT_SPI);
    }
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
 * Convert a state into an event
 */
static uint8_t event_from_state(uint8_t state)
{
    return (state ? EVENT_DEPRESS : EVENT_PRESS);
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

    return event_from_state(state);
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
 * One iteration of the debounce code
 */
uint8_t debounce(uint16_t *state, uint8_t pin)
{
    *state = (*state << 1) | digitalRead(pin);
#if DEBUG
    syslog(LOG_DEBUG, "debounce %d %x", pin, *state);
#endif
    return (*state == 0xffff || *state == 0x0000);
}

/**
 * Make sure that 16 concurrent pin reads all read the same value
 */
int8_t read_debounced(uint8_t pin)
{
    uint16_t state = 0x1010;
    struct timeval tend;
    struct timeval tstart;
    double diff = 0;

    gettimeofday(&tstart, NULL);

    while (diff < DEBOUNCE_DURATION) {

        if (debounce(&state, pin)) {
            return digitalRead(pin);
        }

        usleep(DEBOUNCE_DURATION * 1.0e+3 / 50.0);  // 50 samples at most
        gettimeofday(&tend, NULL);
        diff = ((double)tend.tv_sec + 1.0e-6*tend.tv_usec) - 
               ((double)tstart.tv_sec + 1.0e-6*tstart.tv_usec);
    }

    return -1;
}

/**
 * Inner main loop for rotary binary switch.
 */
void callback_rotary_binary()
{
    char msg[32];
    uint8_t event;

    event = get_rotary_event(PIN_ROTARY_LEFT, PIN_ROTARY_RIGHT);

    if (event == EVENT_NONE) {
        return;
    }

    simple_zmq_send("ROTARY", event_name(event));
}

/**
 * Detect rotary click events
 */
void callback_rotary_click()
{
    uint8_t event;
    int8_t result;
    
    result = read_debounced(PIN_ROTARY_CLICK);
    if (result == -1) {
        syslog(LOG_DEBUG, "Encountered some noise on rotary click pin");
        return;
    }

    event = get_pin_event(PIN_ROTARY_CLICK);
    if (event == EVENT_NONE) {
        syslog(LOG_DEBUG, "Wrong event on rotary click pin, unexpected %s", event_name(event));
        return;
    }

    simple_zmq_send("ROTARY", event_name(event));
}

/**
 * Callback function for power state interrupt
 */
void callback_power_state()
{
    uint8_t event;
    uint8_t active;
    int8_t result;
    const char *name;
    
    result = read_debounced(PIN_POWER_STATE);
    if (result == -1) {
        syslog(LOG_DEBUG, "Encountered some noise on power pin");
        return;
    }

    event = get_pin_event(PIN_POWER_STATE);
    if (event == EVENT_NONE) {
        syslog(LOG_DEBUG, "Wrong event on power pin, unexpected %s", event_name(event));
        return;
    }

    active = pin_active(PIN_POWER_STATE);
    name = active_name(active);

    simple_zmq_send("POWER", name);
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
 * Initialize ADC lookup table
 *
 * Resistance between pins as follows:
 *
 * Function    Pin1    Pin2    Resistance      ADC value ref=3.33V divider=1k
 * --------------------------------------------------------------------------
 *             6       7       100.000 ohm     9    - 10
 *             6       8       100.000 ohm     9    - 10
 * Mode        6       8       1 ohm           1014 - 1023 (0 ohm)
 * Up          6       7       1 ohm           1014 - 1023 (0 ohm)
 * Down        6       7       330 ohm         764  - 769
 * Vol+        6       7       1.000 ohm       503  - 511
 * Vol-        6       7       3.100 ohm       243  - 250
 *
 */
void init_analog_table()
{
    uint16_t i;

    memset(&analog_table, 0, sizeof(analog_table));

    for (i = 1014; i <= 1023; i++) {
        analog_table[1][i] = ANALOG_MODE;
    }

    for (i = 1014; i <= 1023; i++) {
        analog_table[0][i] = ANALOG_UP;
    }

    for (i = 764; i <= 769; i++) {
        analog_table[0][i] = ANALOG_DOWN;
    }

    for (i = 503; i <= 511; i++) {
        analog_table[0][i] = ANALOG_VOL_UP;
    }

    for (i = 243; i <= 250; i++) {
        analog_table[0][i] = ANALOG_VOL_DOWN;
    }
}

/**
 * Process any changed events
 */
void handle_adc_events(uint8_t events, uint8_t last_events)
{
    uint8_t changed = events ^ last_events;
    uint8_t event;
    uint8_t state;

#if DEBUG
    syslog(LOG_DEBUG, "handle_adc_events: events=%d, last_events=%d, changed=%d", events, last_events, changed);
#endif

    if (changed & ANALOG_MODE) {
        event = event_from_state(!(events & ANALOG_MODE));
        simple_zmq_send("MODE", event_name(event));
    }
    if (changed & ANALOG_VOL_UP) {
        event = event_from_state(!(events & ANALOG_VOL_UP));
        simple_zmq_send("VOLUME UP", event_name(event));
    }
    if (changed & ANALOG_VOL_DOWN) {
        event = event_from_state(!(events & ANALOG_VOL_DOWN));
        simple_zmq_send("VOLUME DOWN", event_name(event));
    }
    if (changed & ANALOG_UP) {
        event = event_from_state(!(events & ANALOG_UP));
        simple_zmq_send("ARROW UP", event_name(event));
    }
    if (changed & ANALOG_DOWN) {
        event = event_from_state(!(events & ANALOG_DOWN));
        simple_zmq_send("ARROW DOWN", event_name(event));
    }
}

/**
 * Read from MCP3008.
 */
static uint8_t get_adc_event()
{
    static uint8_t last_events = 0;
    uint8_t events = 0;
    uint8_t pin;
    uint16_t voltage;

    for (pin = 0; pin < SPI_PIN_MAX; pin++) {
        voltage = analogRead(SPI_BASE + SPI_CHAN + pin);
#if DEBUG
        syslog(LOG_DEBUG, "Voltage value from ADC channel %d pin %d: voltage=%d event=%d", SPI_CHAN, pin, voltage, analog_table[pin][voltage]);
#endif
        events |= analog_table[pin][voltage];
    }

    if (events != last_events) {
        handle_adc_events(events, last_events);
    }

    last_events = events;
}

/**
 * Main program.
 */
int main(int argc, char **argv)
{
    zmq_context = zmq_init(1);
    zmq_publisher = zmq_socket(zmq_context, ZMQ_PUB);
    struct sigaction act;

    if ((zmq_connect(zmq_publisher, "tcp://localhost:9080")) != 0) {
        perror("Fatal error: could not connect to ZMQ socket tcp://localhost:9080");
        return EXIT_ZMQ;
    }

    if (wiringPiSetup()) {
        perror("Fatal error: could not initialize wiringPi");
        return EXIT_WIRING;
    }

    openlog("sigd", LOG_PID, LOG_DAEMON);

    init_pins();
    init_adc();
    init_analog_table();

    act.sa_handler = signal_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    clear_opts(&opts);

    syslog(LOG_NOTICE, "Caracas daemon started.");

    while (opts.running) {
        get_adc_event();
        usleep(SPI_INTERVAL);
    }

    syslog(LOG_NOTICE, "Received shutdown signal, exiting.");

    zmq_close(zmq_publisher);
    zmq_term(zmq_context);

    return 0;
}
