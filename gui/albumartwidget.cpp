#include <QPainter>
#include <QPoint>

#include "albumartwidget.hpp"


AlbumArtWidget::AlbumArtWidget()
{
    QPainter * albumart_painter;
    QPoint * albumart_center;

    albumart_blank = new QPixmap(QSize(240, 240));
    albumart_painter = new QPainter(albumart_blank);
    albumart_center = new QPoint(120, 120);

    albumart_blank->fill(Qt::black);
    albumart_painter->setRenderHint(QPainter::Antialiasing, true);

    albumart_painter->setPen(QPen(QColor(128, 128, 128, 255), 20));
    albumart_painter->drawEllipse(*albumart_center, 90, 90);
    albumart_painter->setPen(QPen(QColor(128, 128, 128, 200), 15));
    albumart_painter->drawEllipse(*albumart_center, 60, 60);
    albumart_painter->setPen(QPen(QColor(128, 128, 128, 150), 10));
    albumart_painter->drawEllipse(*albumart_center, 35, 35);

    delete albumart_painter;
    delete albumart_center;
}

void
AlbumArtWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() & Qt::LeftButton) {
        emit clicked();
    }
}

void
AlbumArtWidget::reset()
{
    setPixmap(*albumart_blank);
}
