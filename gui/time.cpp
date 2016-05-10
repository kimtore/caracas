#include "time.hpp"


void
Time::set_total_ms(unsigned long milliseconds)
{
    unsigned long hours;
    unsigned long minutes;
    unsigned long seconds;

    hours = milliseconds / 3600000;
    milliseconds -= (hours * 3600000);

    minutes = milliseconds / 60000;
    milliseconds -= (minutes * 60000);

    seconds = milliseconds / 1000;
    milliseconds -= (seconds * 1000);

    this->setHMS(hours, minutes, seconds, milliseconds);
}

unsigned long
Time::get_total_ms()
{
    return (get_total_seconds() * 1000) + msec();
}

unsigned long
Time::get_total_seconds()
{
    return (hour() * 3600) + (minute() * 60) + second();
}

QString
Time::get_short_string()
{
    if (hour() == 0) {
        return toString("mm:ss");
    }
    return toString("HH:mm:ss");
}

void
Time::increment_ms(unsigned long ms)
{
    ms += get_total_ms();
    set_total_ms(ms);
}
