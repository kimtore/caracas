#include <wiringPi.h>
#include <mcp3004.h>
#include <stdio.h>
#include <stdlib.h>

#define BASE 100
#define SPI_CHAN 0
#define NEWLINE "\n"

unsigned char spinner()
{
    static const char *spin = "-\\|/";
    static const char *it = &spin;

    it += 1;
    if (it[0] == '\0') {
        it = spin;
    }
    return it[0];
}

int main()
{
    int chan;
    float x;

    wiringPiSetup();

    // 3004 and 3008 are the same 4/8 channels
    if (mcp3004Setup(BASE, SPI_CHAN) != 0) {
        perror("Error setting up MCP3008");
        exit(1);
    }

    printf("CHANNEL: %6d %6d %6d %6d %6d %6d %6d %6d\n", 1, 2, 3, 4, 5, 6, 7, 8);
    system("setterm -cursor off");

    while (1) {
        printf("SPI_CH%d: ", SPI_CHAN);
        for (chan = 0; chan < 8; ++chan) {
            x = analogRead(BASE + chan);
            printf("%.4f ", chan, x);
        }
        printf("  %c   ", spinner());
        printf("%s", NEWLINE);
        usleep(250000);
    }
}
