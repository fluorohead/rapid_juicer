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

QMap <QString, FileFormat> fformats {  // АВТОМАТИЧЕСКОЕ УПОРЯДОЧИВАНИЕ ПО КЛЮЧУ ЗА СЧЁТ КОНТЕЙНЕРА QMap
    // <ключ,     FileFormat>
    // |          |
    // V          V
    { "bmp",      { "Microsoft Windows Bitmap", "", "BMP", {CAT_IMAGE, CAT_WIN, CAT_NONE}, CAT_RASTER, {"bmp"}, 0, "" } },
    { "pcx",      { "Zsoft PC Paintbrush", "only v3.0", "PCX", { CAT_IMAGE, CAT_DOS, CAT_NONE }, CAT_RASTER, { "pcx_05" }, 0, "" } },
    { "png",      { "Portable Network Graphics", "", "PNG", { CAT_IMAGE, CAT_WEB, CAT_NONE }, CAT_RASTER, { "png" }, 0, "" } },
    { "avi",      { "Audio-Video Interleaved", "RIFF container", "AVI", { CAT_VIDEO, CAT_NONE, CAT_NONE }, CAT_NONE, { "riff" }, 0, "" } },
    { "wav",      { "Wave Audio Data", "RIFF container", "WAV", { CAT_AUDIO, CAT_NONE, CAT_NONE }, CAT_NONE, { "riff" }, 0, "" } },
    { "rmi",      { "MIDI music","RIFF container", "RMI", { CAT_MUSIC, CAT_WIN, CAT_NONE }, CAT_MIDI, { "riff" }, 0, "" } },
    { "mid",      { "Standard MIDI music", "", "MID", { CAT_MUSIC, CAT_NONE, CAT_NONE }, CAT_MIDI, { "mid" }, 0, "" } },
    { "xmi",      { "eXtended MIDI music (MSS)", "IFF container", "XMI", { CAT_MUSIC, CAT_DOS, CAT_OUTDATED }, CAT_MIDI, { "iff" }, 0, "" } },
    { "lbm",      { "Interleaved Bitmap", "IFF container", "LBM", { CAT_IMAGE, CAT_AMIGA, CAT_OUTDATED }, CAT_RASTER | CAT_DOS, { "iff" }, 0, "" } },
    { "gif",      { "Graphics Interchange Format", "", "GIF", { CAT_IMAGE, CAT_WEB, CAT_NONE }, CAT_RASTER, { "gif" }, 0, "" } },
    { "tif_ii",   { "Tag Image File Format", "Intel byte order", "TIF", { CAT_IMAGE, CAT_NONE, CAT_NONE }, CAT_RASTER | CAT_INTELX86, { "tiff_ii" }, 0, "" } },
    { "tif_mm",   { "Tag Image File Format", "Motorola byte order", "TIF", { CAT_IMAGE, CAT_MAC, CAT_OUTDATED }, CAT_RASTER | CAT_MOTOROLA, { "tiff_mm" }, 0, "" } },
    { "tga_tc32", { "Targa Graphics Adapter Image", "only true-color w/o RLE", "TGA", { CAT_IMAGE, CAT_OUTDATED, CAT_NONE }, CAT_RASTER, { "tga_tc32" }, 0, "" } },
    { "jpg",      { "JPEG File Interchange Format", "", "JPG", { CAT_IMAGE, CAT_NONE, CAT_NONE }, CAT_RASTER, { "jpg" }, 0, "" } },
    { "acon",     { "Windows Animated Cursor", "RIFF container", "ANI", { CAT_IMAGE, CAT_WIN, CAT_NONE }, CAT_RASTER, { "riff" }, 0, "" } },
    { "aif",      { "Audio Interchange Format", "IFF container", "AIF", { CAT_AUDIO, CAT_NONE, CAT_NONE }, CAT_NONE, { "iff" }, 0, "" } },
    { "mod_m.k.", { "Tracker Module", "most popular 'M.K.'", "MOD", { CAT_AUDIO, CAT_MUSIC, CAT_OUTDATED }, CAT_NONE, { "mod_m.k." }, 0, "" } },
    { "xm",       { "FastTracker II Module" , "", "XM", { CAT_AUDIO, CAT_MUSIC, CAT_OUTDATED }, CAT_DOS, { "xm" }, 0, "" } },
    { "s3m",      { "ScreamTracker 3 Module", "", "S3M", { CAT_AUDIO, CAT_MUSIC, CAT_OUTDATED }, CAT_DOS, { "s3m" }, 0, "" } },
    { "it",       { "ImpulseTracker Module", "", "IT", { CAT_AUDIO, CAT_MUSIC, CAT_OUTDATED }, CAT_DOS, { "it" }, 0, "" } },
    { "bik",      { "Bink Video", "", "BIK", { CAT_VIDEO, CAT_OUTDATED, CAT_NONE }, CAT_NONE, { "bink1" }, 0, "" } },
    { "bk2",      { "Bink2 Video", "", "BK2", { CAT_VIDEO, CAT_NONE, CAT_NONE }, CAT_NONE, { "bink2" }, 0, "" } },
    { "smk",      { "Smacker Video", "", "SMK", { CAT_VIDEO, CAT_DOS, CAT_OUTDATED }, CAT_NONE, { "smk2", "smk4" }, 0, "" } },
    { "flc",      { "FLIC Animation", "FLI/FLC/FLX", "FLC", { CAT_VIDEO, CAT_OUTDATED, CAT_NONE }, CAT_ANIM, { "fli_af11", "flc_af12", "flx_af44" }, 0, "" } },
    { "669",      { "Composer 669 Module", "", "669", { CAT_AUDIO, CAT_MUSIC, CAT_PERFRISK }, CAT_DOS, { "669_if", "669_jn" }, 0, "" } }
    //{ "mp3",      { "MPEG-1 Layer-3 Audio", "", "mp3", {CAT_AUDIO, CAT_NONE, CAT_NONE}, CAT_LOSSY, {"mp3"}, 0, /*true, */"" } }
};

void indexFilesFormats() {
    u32i cnt = 0;
    for (auto it = fformats.begin(); it != fformats.end(); it++)
    {
        it->index = cnt; // индексируем каждый формат
        // след. два for'а формируют св-во FileForamat.cat_str для отображения в tooltip'ах
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
