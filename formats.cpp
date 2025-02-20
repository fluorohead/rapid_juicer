#include "formats.h"
#include "qdebug.h"
#include <QString>

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

QMap <QString, FileFormat> fformats // QMap -> АВТОМАТИЧЕСКОЕ УПОРЯДОЧИВАНИЕ ПО КЛЮЧУ
{
    // <ключ,     FileFormat>
    // |          |
    // V          V
    { "bmp",      { "MS Windows Bitmap", "", "BMP", {CAT_IMAGE, CAT_WIN, CAT_NONE}, CAT_RASTER, {"bmp"}, 0, "" } },
    { "pcx",      { "Zsoft PC Paintbrush", "only v3.0", "PCX", { CAT_IMAGE, CAT_DOS, CAT_NONE }, CAT_RASTER, { "pcx_05" }, 0, "" } },
    { "png",      { "Portable Network Graphics", "", "PNG", { CAT_IMAGE, CAT_WEB, CAT_NONE }, CAT_RASTER | CAT_MOTOROLA, { "png" }, 0, "" } },
    { "avi",      { "Audio-Video Interleaved", "RIFF container", "AVI", { CAT_VIDEO, CAT_NONE, CAT_NONE }, CAT_NONE, { "riff" }, 0, "" } },
    { "wav",      { "Wave Audio Data", "RIFF container", "WAV", { CAT_AUDIO, CAT_NONE, CAT_NONE }, CAT_NONE, { "riff" }, 0, "" } },
    { "rmi",      { "MIDI music","RIFF container", "RMI", { CAT_MUSIC, CAT_WIN, CAT_NONE }, CAT_MIDI | CAT_MOTOROLA, { "riff" }, 0, "" } },
    { "mid",      { "Standard MIDI music", "", "MID", { CAT_MUSIC, CAT_NONE, CAT_NONE }, CAT_MIDI | CAT_MOTOROLA, { "mid" }, 0, "" } },
    { "xmi",      { "eXtended MIDI music (MSS)", "IFF container", "XMI", { CAT_MUSIC, CAT_DOS, CAT_OUTDATED }, CAT_MIDI | CAT_MOTOROLA, { "iff" }, 0, "" } },
    { "lbm",      { "Interleaved Bitmap", "ACBM/DEEP/ILBM/PBM/RGB8/...", "LBM", { CAT_IMAGE, CAT_AMIGA, CAT_OUTDATED }, CAT_RASTER | CAT_MOTOROLA, { "iff" }, 0, "" } },
    { "gif",      { "Graphics Interchange Format", "", "GIF", { CAT_IMAGE, CAT_WEB, CAT_NONE }, CAT_RASTER, { "gif" }, 0, "" } },
    { "tif_ii",   { "Tag Image File Format", "Intel byte order", "TIF", { CAT_IMAGE, CAT_NONE, CAT_NONE }, CAT_RASTER | CAT_INTELX86, { "tiff_ii" }, 0, "" } },
    { "tif_mm",   { "Tag Image File Format", "Motorola byte order", "TIF", { CAT_IMAGE, CAT_MAC, CAT_OUTDATED }, CAT_RASTER | CAT_MOTOROLA, { "tiff_mm" }, 0, "" } },
    { "tga_tc",   { "Targa Graphics Adapter Image", "only true-color", "TGA", { CAT_IMAGE, CAT_OUTDATED, CAT_PERFRISK }, CAT_RASTER, { "tga_tp2", "tga_tp10" }, 0, "" } },
    { "jpg",      { "JPEG File Interchange Format", "", "JPG", { CAT_IMAGE, CAT_NONE, CAT_NONE }, CAT_RASTER, { "jpg" }, 0, "" } },
    { "ani_riff", { "MS Windows Animated Cursor", "RIFF container", "ANI", { CAT_IMAGE, CAT_WIN, CAT_NONE }, CAT_RASTER, { "riff" }, 0, "" } },
    { "aif",      { "Audio Interchange Format", "AIFF/AIFC/8SVX/16SV", "AIF", { CAT_AUDIO, CAT_NONE, CAT_NONE }, CAT_NONE, { "iff" }, 0, "" } },
    { "mod",      { "Tracker Module", "M.K./xCHN/xxCH", "MOD", { CAT_MUSIC, CAT_OUTDATED, CAT_NONE }, CAT_MOTOROLA, { "mod_m.k.", "mod_ch" }, 0, "" } },
    { "xm",       { "FastTracker II Module" , "", "XM", { CAT_MUSIC, CAT_DOS, CAT_OUTDATED }, CAT_NONE, { "xm" }, 0, "" } },
    { "s3m",      { "ScreamTracker 3 Module", "", "S3M", { CAT_MUSIC, CAT_DOS, CAT_OUTDATED }, CAT_NONE, { "s3m" }, 0, "" } },
    { "it",       { "ImpulseTracker Module", "", "IT", { CAT_MUSIC, CAT_DOS, CAT_OUTDATED }, CAT_NONE, { "it" }, 0, "" } },
    { "bik",      { "Bink Video", "", "BIK", { CAT_VIDEO, CAT_OUTDATED, CAT_NONE }, CAT_NONE, { "bink1" }, 0, "" } },
    { "bk2",      { "Bink2 Video", "", "BK2", { CAT_VIDEO, CAT_NONE, CAT_NONE }, CAT_NONE, { "bink2" }, 0, "" } },
    { "smk",      { "Smacker Video", "", "SMK", { CAT_VIDEO, CAT_DOS, CAT_OUTDATED }, CAT_NONE, { "smk2", "smk4" }, 0, "" } },
    { "flc",      { "FLIC Animation", "FLI/FLC/FLX", "FLC", { CAT_VIDEO, CAT_OUTDATED, CAT_NONE }, CAT_ANIM, { "fli_af11", "flc_af12", "flx_af44" }, 0, "" } },
    { "669",      { "Composer 669 Module", "", "669", { CAT_MUSIC, CAT_OUTDATED, CAT_PERFRISK }, CAT_DOS, { "669_if", "669_jn" }, 0, "" } },
    { "au",       { "Sun/NeXT/Java AU Sound", "", "AU", { CAT_AUDIO, CAT_NONE, CAT_NONE }, CAT_MOTOROLA, { "au" }, 0, "" } },
    { "voc",      { "Creative Labs Voice", "", "VOC", { CAT_AUDIO, CAT_DOS, CAT_OUTDATED }, CAT_NONE, { "voc" }, 0, "" } },
    { "mov_qt",   { "QuickTime Movie", "", "MOV", { CAT_VIDEO, CAT_NONE, CAT_NONE }, CAT_MAC | CAT_MOTOROLA, { "qt_mdat", "qt_moov" }, 0, "" } },
    { "mp4_qt",   { "MPEG-4 Video", "ISO/IEC 14496-1/-14", "MP4", { CAT_VIDEO, CAT_NONE, CAT_NONE }, CAT_MOTOROLA, { "qt_mdat", "qt_moov" }, 0, "" } },
    { "ico_win",  { "MS Windows Icon", "", "ICO", { CAT_IMAGE, CAT_WIN, CAT_PERFRISK }, CAT_NONE, { "ico_win" }, 0, "" } },
    { "cur_win",  { "MS Windows Cursor", "", "CUR", { CAT_IMAGE, CAT_WIN, CAT_PERFRISK }, CAT_NONE, { "cur_win" }, 0, "" } },
    { "mp3",      { "MPEG-1 Layer III Audio", "", "MP3", { CAT_AUDIO, CAT_PERFRISK, CAT_NONE }, CAT_LOSSY, { "mp3_fffa", "mp3_fffb", "mp3_id3v2" }, 0, "" } },
    { "ogg",      { "Ogg Audio/Video", "", "OGG", { CAT_AUDIO, CAT_VIDEO, CAT_NONE }, CAT_NONE, { "ogg" }, 0, "" } },
    { "med",      { "OctaMED Module", "MMD0/MMD1/MMD2/MMD03", "MED", { CAT_MUSIC, CAT_AMIGA, CAT_OUTDATED }, CAT_MOTOROLA, { "mmd0", "mmd1", "mmd2", "mmd3" }, 0, "" } },
    { "dbm0",     { "DigiBooster Pro Module", "", "DBM", { CAT_MUSIC, CAT_AMIGA, CAT_OUTDATED }, CAT_MOTOROLA, { "dbm0" }, 0, "" } },
    { "ttf",      { "TrueType/OpenType Font", "TTF/OTF", "TTF", { CAT_OTHER, CAT_NONE, CAT_NONE }, CAT_VECTOR, { "ttf", "otf" }, 0, "" } }
    /// Сейчас 36 форматов.
    /// ВНИМАНИЕ! При добавлении нового формата, нужно удостовериться, что хватит места для тайла в таблице результатов
    /// и при необходимости откорректировать макрос RESULTS_TABLE_ROWS в session.cpp ( RESULTS_TABLE_ROWS * RESULTS_TABLE_COLUMNS всегда должно быть >= кол-ва форматов).
};

void index_file_formats() {
    u32i cnt = 0;
    for (auto it = fformats.begin(); it != fformats.end(); ++it)
    {
        it->index = cnt; // индексируем каждый формат
        // след. два for'а формируют св-во FileForamat.tooltip_str для отображения в tooltip'ах
        for (u32i idx = 0; idx < 3; ++idx) // base-категории
        {
            if (it.value().base_categories[idx] != CAT_NONE)
            {
                it->tooltip_str.append(categories[it.value().base_categories[idx]]);
                it->tooltip_str.append(", ");
            }
        }
        for (u32i rshift = 0; rshift < 64; ++rshift) // дополнительные категории
        {
            if ((it->additional_categories >> rshift) & 1)
            {
                it->tooltip_str.append(categories[qPow(2, rshift)]);
                it->tooltip_str.append(", ");
            }
        }
        it->tooltip_str.removeLast();
        it->tooltip_str.removeLast();
        ++cnt;
    }
}
