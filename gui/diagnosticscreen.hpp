#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QTimer>
#include <QSvgRenderer>
#include <QPixmap>
#include <QPainter>


#ifndef _GUI_DIAGNOSTICSCREEN_H_
#define _GUI_DIAGNOSTICSCREEN_H_


class DiagnosticScreen : public QWidget
{
    Q_OBJECT

public:
    DiagnosticScreen();

public slots:
    void read_and_set_uptime();

private:

    double get_uptime();
    void format_uptime(char * buf, double up);

    QTimer * uptime_timer;

    QVBoxLayout * layout;
    QLabel * title;
    QLabel * uptime;
    QSvgRenderer * renderer;
    QPixmap * logo;
    QPainter * painter;
};


#endif /* _GUI_DIAGNOSTICSCREEN_H_ */
