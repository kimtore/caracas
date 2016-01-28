#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QTimer>

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
};
