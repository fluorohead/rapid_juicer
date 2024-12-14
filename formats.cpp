#include "formats.h"
#include <QMap>
#include <QHash>
#include <QString>
#include <QtMath>
#include <QDebug>

// Все возможные категории ресурсов
const u64i CAT_NONE       = 0x0;

// base category (1st place only)
const u64i CAT_IMAGE      = qPow(2, 0);
const u64i CAT_VIDEO      = qPow(2, 1);
const u64i CAT_AUDIO      = qPow(2, 2);
const u64i CAT_3D         = qPow(2, 3);
const u64i CAT_MUSIC      = qPow(2, 4);
const u64i CAT_OTHER      = qPow(2, 5);

// base category (2nd or 3rd place)
const u64i CAT_DOS        = qPow(2, 10);
const u64i CAT_WIN        = qPow(2, 11);
const u64i CAT_MAC        = qPow(2, 12);
const u64i CAT_AMIGA      = qPow(2, 13);
const u64i CAT_ATARI      = qPow(2, 14);
const u64i CAT_NIX        = qPow(2, 15);

// base category (2nd or 3rd place)
const u64i CAT_PERFRISK   = qPow(2, 21); // high risc of performance degradation
const u64i CAT_OUTDATED   = qPow(2, 22); // outdated formats
const u64i CAT_WEB        = qPow(2, 23); // web-used formats

// additional categories
const u64i CAT_RASTER     = qPow(2, 28);
const u64i CAT_VECTOR     = qPow(2, 29);
const u64i CAT_ANIM       = qPow(2, 30);
const u64i CAT_MIDI       = qPow(2, 31);
const u64i CAT_MOD        = qPow(2, 32);
const u64i CAT_FONT       = qPow(2, 33);
const u64i CAT_ICON       = qPow(2, 34);
const u64i CAT_MZEXE      = qPow(2, 35);
const u64i CAT_PE32       = qPow(2, 36);
const u64i CAT_ELF        = qPow(2, 37);
const u64i CAT_DLL        = qPow(2, 38);
const u64i CAT_SO         = qPow(2, 39);
const u64i CAT_MACHO      = qPow(2, 40);
const u64i CAT_STREAM     = qPow(2, 41);
const u64i CAT_MOTOROLA   = qPow(2, 42);
const u64i CAT_INTELX86   = qPow(2, 43);
const u64i CAT_LOSLESS    = qPow(2, 44);
const u64i CAT_LOSSY      = qPow(2, 45);
const u64i CAT_OBJECT     = qPow(2, 46);
const u64i CAT_SCENE      = qPow(2, 47);
const u64i CAT_USER       = 0xFFFFFFFFFFFFFFFF;

QHash <u64i, QString> categories {
    { CAT_NONE,       "no category" },
    { CAT_IMAGE,      "image" },
    { CAT_VIDEO,      "video" },
    { CAT_AUDIO,      "audio" },
    { CAT_RASTER,     "raster" },
    { CAT_VECTOR,     "vector" },
    { CAT_ANIM,       "animation" },
    { CAT_3D,         "3D" },
    { CAT_DOS,        "DOS platform" },
    { CAT_MZEXE,      "DOS executable" },
    { CAT_WIN,        "Windows platform" },
    { CAT_NIX,        "Unix/Linux platform" },
    { CAT_MAC,        "Mac platform" },
    { CAT_AMIGA,      "Amiga platform" },
    { CAT_ATARI,      "Atari platform" },
    { CAT_OUTDATED,   "outdated" },
    { CAT_MOD,        "tracker" },
    { CAT_MUSIC,      "music" },
    { CAT_PE32,       "Windows executable" },
    { CAT_ELF,        "Linux executable" },
    { CAT_DLL,        "Windows DLL" },
    { CAT_SO,         "Linux Shared Object" },
    { CAT_MIDI,       "midi" },
    { CAT_STREAM,     "stream" },
    { CAT_FONT,       "font" },
    { CAT_ICON,       "icon" },
    { CAT_MACHO,      "Mac library/executable" },
    { CAT_MOTOROLA,   "motorola byte order" },
    { CAT_INTELX86,   "x86 byte order" },
    { CAT_PERFRISK,   "performance risk" },
    { CAT_LOSLESS ,   "losless compression" },
    { CAT_LOSSY,      "lossy compression" },
    { CAT_WEB,        "web" },
    { CAT_OTHER,      "custom", },
};

// from : formats.h
// struct FileFormat {
//     QString description;
//     QString commentary; // для отображения более мелким шрифтом
//     QString extension;
//     u64i    base_categories[3];
//     u64i    additional_categories; // задаётся через битовое "|"
//     QList<QString> signature_ids;  // список сигнатур для формата (может быть >= 1)
//     u32i    index;
//     bool    selected;
//     QString cat_str; // для отображения в tooltip'ах
// };

QMap <QString, FileFormat> fformats {
    // <ключ,     FileFormat>
    // |          |
    // V          V
    { "bmp",      { "Microsoft Windows Bitmap", "", "bmp", {CAT_IMAGE, CAT_WIN, CAT_PERFRISK}, CAT_RASTER, {"bmp"}, 0, true, "" } },
    { "pcx",      { "PC Paintbrush", "only v4 and v5", "pcx", {CAT_IMAGE, CAT_DOS, CAT_OUTDATED}, CAT_RASTER, {"pcx4_8, pcx4_24, pcx5_8, pcx5_24"}, 0, true, "" } },
    { "png",      { "Portable Network Graphics", "", "png", {CAT_IMAGE, CAT_WEB, CAT_NONE}, CAT_RASTER, {"png"}, 0, true, "" } },
    { "avi",      { "Audio-Video Interleaved", "RIFF container", "avi", {CAT_VIDEO, CAT_NONE, CAT_NONE}, CAT_NONE, {"riff"}, 0, true, "" } },
    { "wav",      { "Wave Audio Data", "RIFF container", "wav", {CAT_AUDIO, CAT_NONE, CAT_NONE}, CAT_NONE, {"riff"}, 0, true, "" } },
    { "rmi",      { "MIDI music","RIFF container", "rmi", {CAT_MUSIC, CAT_WIN, CAT_NONE}, CAT_MIDI, {"riff"}, 0, true, "" } },
    { "lbm",      { "EA Interleaved Bitmap", "IFF container", "lbm", {CAT_IMAGE, CAT_AMIGA, CAT_OUTDATED}, CAT_RASTER | CAT_DOS, {"iff"}, 0, true, "" } },
    { "gif",      { "Graphics Interchange Format", "", "gif", {CAT_IMAGE, CAT_WEB, CAT_NONE}, CAT_RASTER, {"gif"}, 0, true, "" } },
    { "tif_ii",   { "Tag Image File Format", "Intel byte order", "tif", {CAT_IMAGE, CAT_PERFRISK, CAT_NONE}, CAT_RASTER | CAT_INTELX86, {"tiff_ii"}, 0, true, "" } },
    { "tif_mm",   { "Tag Image File Format", "Motorola byte order", "tif", {CAT_IMAGE, CAT_OUTDATED, CAT_PERFRISK}, CAT_RASTER | CAT_MOTOROLA, {"tiff_mm"}, 0, true, "" } },
    { "tga_tc32", { "Targa Graphics Adapter Image", "only true-color w/o RLE", "tga", {CAT_IMAGE, CAT_OUTDATED, CAT_NONE}, CAT_RASTER, {"tga_tc32"}, 0, true, "" } },
    { "jpg",      { "JPEG File Interchange Format", "", "jpg", {CAT_IMAGE, CAT_NONE, CAT_NONE}, CAT_RASTER, {"jfif_soi", "jfif_eoi"}, 0, true, "" } },
    { "mp3",      { "MPEG-1 Layer-3 Audio", "", "mp3", {CAT_AUDIO, CAT_NONE, CAT_NONE}, CAT_LOSSY, {"mp3"}, 0, true, "" } }
};

void indexFilesFormats() {
    u32i cnt = 0;
    for (auto it = fformats.begin(); it != fformats.end(); it++) {
        it->index = cnt; // индексируем каждый формат
        // след. два for'а формируют св-во FileForamat.cat_str для отображения в tooltip'ах
        for (u32i idx = 0; idx < 3; idx++) { // base-категории
            if (it.value().base_categories[idx] != CAT_NONE) {
                it->cat_str.append(categories[it.value().base_categories[idx]]);
                it->cat_str.append(", ");
            }
        }
        for (u32i rshift = 0; rshift < 64; rshift++) { // дополнительные категории
            if ((it->additional_categories >> rshift) & 1) {
                it->cat_str.append(categories[qPow(2, rshift)]);
                it->cat_str.append(", ");
                }
        }
        it->cat_str.removeLast();
        it->cat_str.removeLast();
        cnt++;
    }
}
