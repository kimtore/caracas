#include <QObject>
#include <QTime>


#ifndef _GUI_TIME_H_
#define _GUI_TIME_H_


class Time : public QTime
{
public:
    void set_total_ms(unsigned long seconds);
    unsigned long get_total_ms();
    unsigned long get_total_seconds();
    QString get_short_string();
    void increment_ms(unsigned long ms);
};


#endif /* _GUI_TIME_H_ */
