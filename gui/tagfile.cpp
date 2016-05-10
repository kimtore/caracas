#include "tagfile.hpp"

#include <id3v2tag.h>
#include <mpegfile.h>
#include <id3v2frame.h>
#include <id3v2header.h>
#include <attachedpictureframe.h>

#include <tlist.h>
#include <flacfile.h>
#include <flacpicture.h>
#include <tbytevector.h>

bool
TagFile::get_id3_album_art(QPixmap * image)
{
    TagLib::MPEG::File file(path.toUtf8().data());
    TagLib::ID3v2::Tag *tag;
    TagLib::ID3v2::FrameList frame;
    TagLib::ID3v2::AttachedPictureFrame *picture_frame;
    TagLib::ID3v2::FrameList::ConstIterator it;
    unsigned long size;

    /* Open file and read tags */
    tag = file.ID3v2Tag();

    if (!tag) {
        return false;
    }

    /* List picture frames */
    frame = tag->frameListMap()["APIC"];
    if (frame.isEmpty()) {
        return false;
    }

    /* Discover front cover picture frame */
    for (it = frame.begin(); it != frame.end(); ++it) {
        picture_frame = (TagLib::ID3v2::AttachedPictureFrame *)(*it);

        if (picture_frame->type() != TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
            continue;
        }

        size = picture_frame->picture().size();
        if (!size) {
            continue;
        }

        image->loadFromData((const unsigned char *)picture_frame->picture().data(), size);
        return true;
    }

    return false;
}

bool
TagFile::get_flac_album_art(QPixmap * image)
{
    TagLib::FLAC::File file(path.toUtf8().data());
    const TagLib::List<TagLib::FLAC::Picture *> & list = file.pictureList();
    TagLib::List<TagLib::FLAC::Picture *>::ConstIterator it;
    TagLib::FLAC::Picture * picture;
    unsigned long size;

    if (!list.size()) {
        return false;
    }

    for (it = list.begin(); it != list.end(); ++it) {

        picture = *it;

        if (picture->type() != TagLib::FLAC::Picture::FrontCover) {
            continue;
        }

        size = picture->data().size();
        if (!size) {
            continue;
        }

        image->loadFromData((const unsigned char *)picture->data().data(), size);
        return true;
    }

    return false;
}

bool
TagFile::get_album_art(QPixmap * image)
{
    if (get_id3_album_art(image)) {
        return true;
    } else if (get_flac_album_art(image)) {
        return true;
    }
    return false;
}

TagFile::TagFile(const QString & path_)
{
    path = path_;
}
