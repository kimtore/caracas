#include <QObject>
#include <QPixmap>


#ifndef _GUI_TAGFILE_H_
#define _GUI_TAGFILE_H_


/**
 * Represents a music file with ID3 or FLAC tags.
 */
class TagFile : public QObject
{
    Q_OBJECT

public:
    TagFile(const QString & path_);

    bool get_id3_album_art(QPixmap * image);
    bool get_flac_album_art(QPixmap * image);
    bool get_album_art(QPixmap * image);

private:
    QString path;
};


#endif /* _GUI_TAGFILE_H_ */
