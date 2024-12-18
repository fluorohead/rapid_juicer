#ifndef FORMATS_H
#define FORMATS_H

#include <QtMath>
#include <QtTypes>
#include <QString>
#include <QVector>
#include <QSet>
#include <QMap>

using u64i = quint64;
using s64i = qint64;
using u32i = quint32;
using s32i = qint32;
using u16i = quint16;
using u8i  = quint8;

#define RECOGNIZE_FUNC_HEADER (Engine *e)
#define RECOGNIZE_FUNC_DECL_RETURN static u64i
#define RECOGNIZE_FUNC_RETURN u64i

#define MAX_SIGNATURE_SIZE 4  // максимальный размер сигнатуры
#define MIN_RESOURCE_SIZE 32  // должно быть кратно MAX_SIGNATURE_SIZE

// Все возможные категории ресурсов
extern const u64i CAT_NONE;

// base category (1st place only)
extern const u64i CAT_IMAGE;
extern const u64i CAT_VIDEO;
extern const u64i CAT_AUDIO;
extern const u64i CAT_3D;
extern const u64i CAT_MUSIC;
extern const u64i CAT_OTHER;

// base category (2nd or 3rd place)
extern const u64i CAT_DOS;
extern const u64i CAT_WIN;
extern const u64i CAT_MAC;
extern const u64i CAT_AMIGA;
extern const u64i CAT_ATARI;
extern const u64i CAT_NIX;

// base category (2nd or 3rd place)
extern const u64i CAT_PERFRISK;
extern const u64i CAT_OUTDATED;
extern const u64i CAT_WEB;

// additional categories
extern const u64i CAT_RASTER;
extern const u64i CAT_VECTOR;
extern const u64i CAT_ANIM;
extern const u64i CAT_MIDI;
extern const u64i CAT_MOD;
extern const u64i CAT_FONT;
extern const u64i CAT_ICON;
extern const u64i CAT_MZEXE;
extern const u64i CAT_PE32;
extern const u64i CAT_ELF;
extern const u64i CAT_DLL;
extern const u64i CAT_SO;
extern const u64i CAT_MACHO;
extern const u64i CAT_STREAM;
extern const u64i CAT_MOTOROLA;
extern const u64i CAT_INTELX86;
extern const u64i CAT_LOSLESS;
extern const u64i CAT_LOSSY;
extern const u64i CAT_OBJECT;
extern const u64i CAT_SCENE;
extern const u64i CAT_USER;

struct FileFormat
{
    QString        description;
    QString        commentary; // для отображения более мелким шрифтом
    QString        extension;
    u64i           base_categories[3];
    u64i           additional_categories; // задаётся через битовое "|"
    QList<QString> signature_ids;  // список сигнатур для формата (может быть >= 1)
    u32i           index;
    QString        cat_str; // для отображения в tooltip'ах
};

extern QMap <QString, FileFormat> fformats;

// для структуры Message.msg_type
#define MSG_NONE          0x0
#define MSG_RESFOUND      1
#define MSG_OPEN_ERROR    2
#define MSG_UNKNOWN_ERROR 0xFFFFFFFFFFFFFFFF

struct Message
{
    u32i    msg_type;
    QString file_name;
    QString format_name;
    s64i    offset;
    u64i    length;
    QString info;
};

void indexFilesFormats();

#endif // FORMATS_H
