#include "searchscreen.hpp"


SearchScreen::SearchScreen()
{
    int i;

    setup_layout();

    /* Connect 0-9 and A-Å */
    for (i = 0; i < 39; i++) {
        QObject::connect(&b_keys[i], &QPushButton::clicked,
                         this, &SearchScreen::key_pressed);
    }

    QObject::connect(&b_backspace, &QPushButton::clicked,
                     this, &SearchScreen::backspace_pressed);

    QObject::connect(&b_enter, &QPushButton::clicked,
                     this, &SearchScreen::enter_pressed);

    QObject::connect(&b_cancel, &QPushButton::clicked,
                     this, &SearchScreen::cancel_pressed);

    QObject::connect(&b_spacebar, &QPushButton::clicked,
                     this, &SearchScreen::spacebar_pressed);
}

void
SearchScreen::key_pressed()
{
    QPushButton * sender;
    sender = dynamic_cast<QPushButton *>(QObject::sender());
    term.setText(term.text() + sender->text());
}

void
SearchScreen::backspace_pressed()
{
    term.setText(term.text().left(term.text().length() - 1));
}

void
SearchScreen::enter_pressed()
{
    emit search(term.text());
}

void
SearchScreen::cancel_pressed()
{
    emit cancel();
}

void
SearchScreen::spacebar_pressed()
{
    term.setText(term.text() + " ");
}

void
SearchScreen::setup_layout()
{
    int i;

    setObjectName("search_screen");
    term.setObjectName("search_term");

    layout = new QVBoxLayout(this);

    for (i = 0; i < 6; i++) {
        layout->addLayout(&rows[i]);
    }

    rows[0].setAlignment(Qt::AlignTop);
    rows[0].addWidget(&term);

    for (i = 0; i < 10; i++) {
        b_keys[i].setText(QString::number(i));
    }

    for (i = 1; i < 10; i++) {
        rows[1].addWidget(&b_keys[i]);
    }
    rows[1].addWidget(&b_keys[0]);

    for (i = 65; i < 91; i++) {
        b_keys[i - 55].setText(QChar(i));
    }

    b_keys[36].setText("Æ");
    b_keys[37].setText("Ø");
    b_keys[38].setText("Å");

    b_backspace.setText("⌫");
    b_backspace.setAutoRepeat(true);
    b_backspace.setAutoRepeatDelay(500);
    b_backspace.setAutoRepeatInterval(100);
    b_enter.setText("↵");
    b_cancel.setText("Esc");
    b_spacebar.setText("␣");

    rows[2].addWidget(&b_keys[26]);
    rows[2].addWidget(&b_keys[32]);
    rows[2].addWidget(&b_keys[14]);
    rows[2].addWidget(&b_keys[27]);
    rows[2].addWidget(&b_keys[29]);
    rows[2].addWidget(&b_keys[34]);
    rows[2].addWidget(&b_keys[30]);
    rows[2].addWidget(&b_keys[18]);
    rows[2].addWidget(&b_keys[24]);
    rows[2].addWidget(&b_keys[25]);

    rows[3].addWidget(&b_keys[10]);
    rows[3].addWidget(&b_keys[28]);
    rows[3].addWidget(&b_keys[13]);
    rows[3].addWidget(&b_keys[15]);
    rows[3].addWidget(&b_keys[16]);
    rows[3].addWidget(&b_keys[17]);
    rows[3].addWidget(&b_keys[19]);
    rows[3].addWidget(&b_keys[20]);
    rows[3].addWidget(&b_keys[21]);

    rows[4].addWidget(&b_keys[35]);
    rows[4].addWidget(&b_keys[33]);
    rows[4].addWidget(&b_keys[12]);
    rows[4].addWidget(&b_keys[31]);
    rows[4].addWidget(&b_keys[11]);
    rows[4].addWidget(&b_keys[23]);
    rows[4].addWidget(&b_keys[22]);
    rows[4].addWidget(&b_keys[36]);
    rows[4].addWidget(&b_keys[37]);
    rows[4].addWidget(&b_keys[38]);

    rows[5].addWidget(&b_cancel);
    rows[5].addWidget(&b_spacebar);
    rows[5].addWidget(&b_backspace);
    rows[5].addWidget(&b_enter);
    rows[5].setStretch(1, 100);
}
