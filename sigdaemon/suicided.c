/**
 * Suicide event daemon for the CARACAS project.
 *
 * Monitors the Raspberry Pi's GPIO pins for a HIGH input signal on the ACC
 * switch input pin. Triggers a shutdown when the power is turned off.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wiringPi.h>

/*
 * Type definitions
 */
typedef unsigned char       uint8_t;

/**
 * Power state, input.
 * This pin is HIGH when the ignition key is switched on.
 */
#define PIN_POWER_STATE     4   /* BCM_GPIO pin 23, physical pin 16 */

/**
 * Exit codes
 */
#define EXIT_SIGNAL         1
#define EXIT_WIRING         2

/**
 * Callback function for interrupt
 */
void power_state_rising_callback()
{
    printf("Power has disappeared, switching off system!\n");
    system("/sbin/init 0");
    exit(0);
}

/**
 * Pin initialization for power state input.
 */
static void init_pin_power_state()
{
    pinMode(PIN_POWER_STATE, INPUT);
    pullUpDnControl(PIN_POWER_STATE, PUD_DOWN);
    wiringPiISR(PIN_POWER_STATE, INT_EDGE_RISING, power_state_rising_callback);
}

/**
 * Catch signals from operating system.
 */
void signal_handler(int s)
{
    printf("Caught signal %d\n", s);
}

/**
 * Main program.
 */
int main(int argc, char **argv)
{
    struct sigaction act;

    if (wiringPiSetup()) {
        perror("Fatal error: could not initialize wiringPi");
        return EXIT_WIRING;
    }

    init_pin_power_state();

    act.sa_handler = signal_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    printf("Suicide daemon running.\n");

    sigsuspend(&act.sa_mask);

    printf("Received signal, exiting.\n");

    return 0;
}
