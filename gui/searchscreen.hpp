#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>

#include <marble/SearchRunnerManager.h>


#ifndef _GUI_SEARCHSCREEN_H_
#define _GUI_SEARCHSCREEN_H_


class SearchScreen : public QWidget
{
    Q_OBJECT

public:
    SearchScreen();

    QVBoxLayout * layout;
    QHBoxLayout rows[6];
    QLabel term;
    QPushButton b_keys[39];
    QPushButton b_backspace;
    QPushButton b_enter;
    QPushButton b_cancel;
    QPushButton b_spacebar;

signals:

    void search(QString term);
    void cancel();

public slots:

    void key_pressed();
    void backspace_pressed();
    void enter_pressed();
    void cancel_pressed();
    void spacebar_pressed();

private:

    void setup_layout();
};


#endif /* _GUI_SEARCHSCREEN_H_ */
