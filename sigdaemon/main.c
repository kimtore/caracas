/**
 * Hardware event daemon for the CARACAS project.
 *
 * Monitors the Raspberry Pi's GPIO pins and uses ØMQ to emit events
 * when something happens.
 *
 * Requires the bcm2835 library found at http://www.airspayce.com/mikem/bcm2835/
 */

#include <stdio.h>
#include <signal.h>
#include <bcm2835.h>
#include <zmq.h>

/*
 * Rotary button input detect pins.
 * These pins are HIGH when the button is active.
 */
#define PIN_ROTARY_CLICK RPI_BPLUS_GPIO_J8_08
#define PIN_ROTARY_LEFT RPI_BPLUS_GPIO_J8_10
#define PIN_ROTARY_RIGHT RPI_BPLUS_GPIO_J8_12

/**
 * Suicide pin.
 * Set HIGH to request power cut-off from the power board.
 */
#define PIN_SUICIDE RPI_BPLUS_GPIO_J8_11

/**
 * Power state input.
 * This pin is HIGH when the ignition key is switched on.
 */
#define PIN_POWER_STATE RPI_BPLUS_GPIO_J8_16

/**
 * Exit codes
 */
#define EXIT_ZMQ 1
#define EXIT_BCM 2

struct opts_t {
    int running;
};

struct opts_t opts;

static void init_pins()
{
    bcm2835_gpio_fsel(PIN_ROTARY_CLICK, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(PIN_ROTARY_LEFT, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(PIN_ROTARY_RIGHT, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(PIN_POWER_STATE, BCM2835_GPIO_FSEL_INPT);

    bcm2835_gpio_fsel(PIN_SUICIDE, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(PIN_SUICIDE, LOW);
}

static int run_loop(void *context, void *publisher)
{
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

    /* If you call this, it will not actually access the GPIO */
    //bcm2835_set_debug(1);

    if (!bcm2835_init()) {
        return EXIT_BCM;
    }

    act.sa_handler = signal_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    clear_opts(&opts);

    printf("Caracas daemon started.\n");

    while (opts.running) {
        run_loop(context, publisher);
        bcm2835_delay(50);
    }

    printf("Received shutdown signal, exiting.\n");

    zmq_close(publisher);
    zmq_term(context);

    return 0;
}
