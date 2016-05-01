#include <QFile>

#include <stdio.h>

#include "diagnosticscreen.hpp"

void
DiagnosticScreen::read_and_set_uptime()
{
    char buf[512];
    double up;

    up = get_uptime();
    format_uptime(buf, up);
    uptime->setText(QString("Uptime: ") + QString(buf));
}

double
DiagnosticScreen::get_uptime()
{
    char buf[64];
    double up = 0;
    QFile file("/proc/uptime");
    QByteArray line;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        line = file.readLine();
        strncpy(buf, line.constData(), 63);
        sscanf(buf, "%lf", &up);
        file.close();
    }

    return up;
}

void
DiagnosticScreen::format_uptime(char * buf, double up)
{
    int days;
    int hours;
    int minutes;
    int seconds = up;

    days = seconds / 86400;
    seconds %= 86400;

    hours = seconds / 3600;
    seconds %= 3600;

    minutes = seconds / 60;
    seconds %= 60;

    sprintf(buf, "%d days  %d hours  %d minutes  %d seconds", days, hours, minutes, seconds);
}

DiagnosticScreen::DiagnosticScreen()
{
    layout = new QVBoxLayout(this);

    title = new QLabel;
    title->setText("<font size=18><i>Caracas</i></font>");
    title->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    layout->addWidget(title);

    uptime = new QLabel;
    uptime->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    layout->addWidget(uptime);
    read_and_set_uptime();

    uptime_timer = new QTimer(this);
    QObject::connect(uptime_timer, SIGNAL(timeout()), this, SLOT(read_and_set_uptime()));
    uptime_timer->start(1000);
}
