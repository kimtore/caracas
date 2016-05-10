#include <QLabel>
#include <QMouseEvent>


#ifndef _GUI_ALBUMARTWIDGET_H_
#define _GUI_ALBUMARTWIDGET_H_


class AlbumArtWidget : public QLabel
{
    Q_OBJECT

public:
    AlbumArtWidget();

    void reset();
    void mousePressEvent(QMouseEvent *event);

signals:
    void clicked();

private:
    QPixmap * albumart_blank;
};


#endif /* _GUI_ALBUMARTWIDGET_H_ */
